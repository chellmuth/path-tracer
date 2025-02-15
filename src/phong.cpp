#include "phong.h"

#include "monte_carlo.h"
#include "util.h"

#include <math.h>

Phong::Phong(Color kd, Color ks, float n, Color emit)
    : Material(emit), m_kd(kd), m_ks(ks), m_n(n)
{}

Color Phong::f(const Intersection &intersection, const Vector3 &wiWorld, float *pdf) const
{
    const Vector3 &wo = intersection.woWorld;
    const Vector3 &normal = intersection.normal;

    Color diffuse = m_kd / M_PI;

    Vector3 reflected = wo.reflect(normal);
    float cosAlpha = fmaxf(0.f, wiWorld.dot(reflected));
    Color specular = m_ks * (m_n + 2) / M_TWO_PI * powf(cosAlpha, m_n);

    *pdf = UniformHemispherePdf(wiWorld);

    return diffuse + specular;
}

BSDFSample Phong::sample(
    const Intersection &intersection,
    RandomGenerator &random
) const
{
    BSDFSample bs = {
        .wiWorld = Vector3(0.f, 0.f, 0.f),
        .pdf = 0.f,
        .throughput = Color(0.f),
        .material = this
    };
    return bs;
}
