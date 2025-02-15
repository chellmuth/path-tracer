#pragma once

#include "color.h"
#include "intersection.h"
#include "material.h"
#include "vector.h"

class PerfectTransmission : public Material {
public:
    PerfectTransmission();

    Color f(
        const Intersection &intersection,
        const Vector3 &wiWorld,
        float *pdf
    ) const override;

    BSDFSample sample(
        const Intersection &intersection,
        RandomGenerator &random
    ) const override;

    bool isDelta() const { return true; }

private:
};
