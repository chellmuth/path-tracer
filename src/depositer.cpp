#include "depositer.h"

#include "bounce_controller.h"
#include "color.h"
#include "globals.h"
#include "job.h"
#include "light.h"
#include "monte_carlo.h"
#include "path_tracer.h"
#include "photon_pdf.h"
#include "ray.h"
#include "transform.h"
#include "util.h"
#include "vector.h"

#include "json.hpp"
using json = nlohmann::json;

#include <assert.h>
#include <fstream>
#include <iostream>
#include <limits>
#include <math.h>
#include <utility>
#include <vector>

static int photonSamples = 1e6;
static int maxBounces = 2;

Depositer::Depositer(BounceController bounceController)
    : mBounceController(bounceController)
{
    mDataSource = std::make_shared<DataSource>();
}

void Depositer::preprocess(const Scene &scene, RandomGenerator &random)
{
    for (int i = 0; i < photonSamples; i++) {
        LightSample lightSample = scene.sampleLights(random);

        Vector3 hemisphereSample = UniformSampleHemisphere(random);
        Transform hemisphereToWorld = normalToWorldSpace(lightSample.normal);
        Vector3 bounceDirection = hemisphereToWorld.apply(hemisphereSample);
        Ray lightRay(lightSample.point, bounceDirection);

        Color throughput = lightSample.light->getMaterial()->emit();
        for (int bounce = 0; bounce < maxBounces; bounce++) {
            Intersection intersection = scene.testIntersect(lightRay);
            if (!intersection.hit) { break; }

            throughput *= fmaxf(0.f, intersection.wi.dot(intersection.normal * -1.f));
            if (throughput.isBlack()) { break; }

            if (bounce > 0) { // don't guide towards direct lights
                mDataSource->points.push_back({
                    intersection.point.x(),
                    intersection.point.y(),
                    intersection.point.z(),
                    lightRay.origin(),
                    throughput
                });
            }

            hemisphereSample = UniformSampleHemisphere(random);
            hemisphereToWorld = normalToWorldSpace(intersection.normal);
            bounceDirection = hemisphereToWorld.apply(hemisphereSample);
            lightRay = Ray(intersection.point, bounceDirection);

            throughput *= intersection.material->f(intersection, bounceDirection);
        }
    }

    mKDTree = new KDTree(3, *mDataSource, nanoflann::KDTreeSingleIndexAdaptorParams(10));
}

static Color average(const DataSource &dataSource, const size_t indices[], size_t size)
{
    if (size == 0) { return Color(0.f); }

    Color sum(0.f);
    for (int i = 0; i < size; i++) {
        sum += dataSource.points[indices[i]].throughput / size;
    }

    return sum;
}

static Color average(const DataSource &dataSource, const std::vector<std::pair<size_t, float> > &indicesDistances)
{
    size_t size = indicesDistances.size();

    if (size == 0) { return Color(0.f); }

    Color sum(0.f);
    for (int i = 0; i < size; i++) {
        size_t index = indicesDistances[i].first;
        sum += dataSource.points[index].throughput / size;
    }

    return sum;
}

static float max(const float values[], size_t size)
{
    if (size == 0) { return 0.f; }

    float max = std::numeric_limits<float>::lowest();
    for (int i = 0; i < size; i++) {
        max = fmaxf(max, values[i]);
    }

    return max;
}

Color Depositer::L(
    const Intersection &intersection,
    const Scene &scene,
    RandomGenerator &random,
    Sample &sample
) const {
    sample.eyePoints.push_back(intersection.point);

    Color result(0.f);
    if (mBounceController.checkCounts(1)) {
        result = direct(intersection, scene, random, sample);
    }

    Color modulation = Color(1.f, 1.f, 1.f);
    Intersection lastIntersection = intersection;

    for (int bounce = 2; !mBounceController.checkDone(bounce); bounce++) {
        Transform hemisphereToWorld = normalToWorldSpace(lastIntersection.normal);

        Point3 intersectionPoint = lastIntersection.point;
        float queryPoint[3] = {
            intersectionPoint.x(),
            intersectionPoint.y(),
            intersectionPoint.z()
        };

        const int debugSearchCount = g_job->debugSearchCount();
        auto resultIndices = std::make_shared<std::vector<size_t>>(debugSearchCount);
        std::vector<float> outDistanceSquared(debugSearchCount);

        nanoflann::KNNResultSet<float> resultSet(debugSearchCount);
        resultSet.init(resultIndices->data(), outDistanceSquared.data());

        mKDTree->findNeighbors(resultSet, queryPoint, nanoflann::SearchParams());

        Transform worldToNormal = worldSpaceToNormal(lastIntersection.normal);

        RandomGenerator random;
        PhotonPDF photonPDF(lastIntersection.point, mDataSource, resultIndices);
        float pdf;
        Vector3 pdfSample = photonPDF.sample(random, worldToNormal, &pdf);

        Vector3 bounceDirection = hemisphereToWorld.apply(pdfSample);

        if (bounceDirection.dot(lastIntersection.normal) < 0.f) { 
            assert(false);
            break;
        }

        Ray bounceRay(
            lastIntersection.point,
            bounceDirection
        );

        Intersection bounceIntersection = scene.testIntersect(bounceRay);
        if (!bounceIntersection.hit) { break; }

        sample.eyePoints.push_back(bounceIntersection.point);

        Color f = lastIntersection.material->f(lastIntersection, bounceDirection);
        float invPDF = 1.f / pdf;

        modulation *= f
            * fmaxf(0.f, bounceDirection.dot(lastIntersection.normal))
            * invPDF;
        lastIntersection = bounceIntersection;

        result += direct(bounceIntersection, scene, random, sample) * modulation;
    }

    return result;
}

Color Depositer::direct(
    const Intersection &intersection,
    const Scene &scene,
    RandomGenerator &random,
    Sample &sample
) const {
    Color emit = intersection.material->emit();
    if (!emit.isBlack()) {
        // part of my old logic - if you hit an emitter, don't do direct lighting?
        // sample.shadowRays.push_back(intersection.point);
        return Color(0.f, 0.f, 0.f);
    }

    int lightCount = scene.lights().size();
    int lightIndex = (int)floorf(random.next() * lightCount);

    std::shared_ptr<Light> light = scene.lights()[lightIndex];
    SurfaceSample lightSample = light->sample(random);

    Vector3 lightDirection = (lightSample.point - intersection.point).toVector();
    Vector3 wo = lightDirection.normalized();

    sample.shadowPoints.push_back(lightSample.point);

    if (lightSample.normal.dot(wo) >= 0.f) {
        return Color(0.f, 0.f, 0.f);
    }

    Ray shadowRay = Ray(intersection.point, wo);
    Intersection shadowIntersection = scene.testIntersect(shadowRay);
    float lightDistance = lightDirection.length();

    if (shadowIntersection.hit && shadowIntersection.t + 0.0001f < lightDistance) {
        return Color(0.f, 0.f, 0.f);
    }

    float invPDF = lightSample.invPDF * lightCount;

    return light->biradiance(lightSample, intersection.point)
        * intersection.material->f(intersection, wo)
        * fmaxf(0.f, wo.dot(intersection.normal))
        * invPDF;
}

void Depositer::debug(const Intersection &intersection, const Scene &scene) const
{
    Point3 intersectionPoint = intersection.point;
    float queryPoint[3] = {
        intersectionPoint.x(),
        intersectionPoint.y(),
        intersectionPoint.z()
    };

    const int debugSearchCount = g_job->debugSearchCount();
    auto resultIndices = std::make_shared<std::vector<size_t>>(debugSearchCount);
    std::vector<float> outDistanceSquared(debugSearchCount);

    nanoflann::KNNResultSet<float> resultSet(debugSearchCount);
    resultSet.init(resultIndices->data(), outDistanceSquared.data());

    mKDTree->findNeighbors(resultSet, queryPoint, nanoflann::SearchParams());

    Transform worldToNormal = worldSpaceToNormal(intersection.normal);
    RandomGenerator random;
    PhotonPDF photonPDF(intersection.point, mDataSource, resultIndices);
    float pdf;
    Vector3 wi = photonPDF.sample(random, worldToNormal, &pdf, true);
    Vector3 bounceDirection = wi;
    bounceDirection.debug();

    json j;
    j["QueryPoint"] = { intersectionPoint.x(), intersectionPoint.y(), intersectionPoint.z() };
    j["Results"] = json::array();
    for (int i = 0; i < debugSearchCount; i++) {
        auto &point = mDataSource->points[resultIndices->at(i)];
        j["Results"].push_back({
            { "point", { point.x, point.y, point.z } },
            { "source", { point.source.x(), point.source.y(), point.source.z() } },
            { "throughput", { point.throughput.r(), point.throughput.g(), point.throughput.b() } },
        });
    }

    std::ofstream jsonFile("live-photons.json");
    jsonFile << j.dump(4) << std::endl;
    std::cout << "Wrote to live-photons.json" << std::endl;

    // std::cout << j.dump(4) << std::endl;

    debug2(intersection, scene);
}

void Depositer::debug2(const Intersection &intersection, const Scene &scene) const
{
    const int phiSteps = 200;
    const int thetaSteps = 200;
    const int spp = 256;

    BounceController bounceController = g_job->bounceController().copyAfterBounce();
    PathTracer integrator(bounceController);
    RandomGenerator random;

    Transform hemisphereToWorld = normalToWorldSpace(intersection.normal);

    json j;
    j["QueryPoint"] = {
        intersection.point.x(),
        intersection.point.y(),
        intersection.point.z()
    };
    j["Steps"] = { { "phi", phiSteps }, { "theta", thetaSteps } };
    j["Gt"] = json::array();

    for (int phiStep = 0; phiStep < phiSteps; phiStep++) {
        for (int thetaStep = 0; thetaStep < thetaSteps; thetaStep++) {
            float phi = M_TWO_PI * phiStep / phiSteps;
            float theta = M_PI * thetaStep / thetaSteps;

            float y = cosf(theta);
            float x = sinf(theta) * cosf(phi);
            float z = sinf(theta) * sinf(phi);

            // Vector3 wiHemisphere(x, y, z);
            // Vector3 wiWorld = hemisphereToWorld.apply(wiHemisphere);

            Vector3 wiWorld = Vector3(x, y, z).normalized();

            Ray ray = Ray(intersection.point, wiWorld);
            const Intersection fisheyeIntersection = scene.testIntersect(ray);
            Color sampleL(0.f);
            if (fisheyeIntersection.hit) {
                for (int i = 0; i < spp; i++) {
                    Sample sample;

                    sampleL += integrator.L(
                        fisheyeIntersection,
                        scene,
                        random,
                        sample
                    ) / spp;

                    if (bounceController.checkCounts(0)) {
                        Color emit = fisheyeIntersection.material->emit();
                        sampleL += emit / spp;
                    }
                }
            }

            // std::cout << "phi: " << phi << " theta: " << theta << std::endl;
            // std::cout << "x: " << x << " y: " << y << " z: " << z << std::endl;
            // std::cout << sampleL << std::endl;

            j["Gt"].push_back({
                { "wi", { x, y, z } },
                { "phiStep", phiStep },
                { "thetaStep", thetaStep },
                { "phi", phi },
                { "theta", theta },
                { "radiance", { sampleL.r(), sampleL.g(), sampleL.b() } },
                { "luminance", { sampleL.luminance() } }
            });
        }
    }

    std::ofstream jsonFile("live-gt.json");
    jsonFile << j.dump(4) << std::endl;
    std::cout << "Wrote to live-gt.json" << std::endl;
}
