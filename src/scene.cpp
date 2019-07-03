#include "scene.h"

#include "camera.h"
#include "color.h"
#include "intersection.h"
#include "ray.h"
#include "util.h"

#include <limits>

Scene::Scene(
    std::vector<std::shared_ptr<Primitive>> primitives,
    std::vector<std::shared_ptr<Surface>> surfaces,
    std::vector<std::shared_ptr<Light>> lights,
    std::shared_ptr<Camera> camera
)
    : m_surfaces(surfaces), m_lights(lights), m_camera(camera), m_bvh(new BVH())
{
    printf("BAKING...\n");
    m_bvh->bake(primitives);
    printf("BAKED...\n");
}

std::vector<std::shared_ptr<Surface>> Scene::getSurfaces()
{
    return m_surfaces;
}

Intersection Scene::testIntersect(const Ray &ray) const
{
    // Intersection result = IntersectionHelper::miss;

    // for (std::shared_ptr<Surface> surfacePtr : m_surfaces) {
    //     Intersection intersection = surfacePtr->testIntersect(ray);
    //     if (intersection.hit && intersection.t < result.t) {
    //         result = intersection;
    //     }
    // }

    // return result;

    return m_bvh->testIntersect(ray);
}

LightSample Scene::sampleLights(RandomGenerator &random) const
{
    int lightCount = m_lights.size();
    int lightIndex = (int)floorf(random.next() * lightCount);

    std::shared_ptr<Light> light = m_lights[lightIndex];
    SurfaceSample surfaceSample = light->sample(random);
    LightSample lightSample(
        light,
        surfaceSample.point,
        surfaceSample.normal,
        surfaceSample.invPDF
    );
    return lightSample;
}
