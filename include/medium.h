#pragma once

#include "color.h"
#include "point.h"

struct TransmittanceQueryResult {
    bool isValid;
    float distance;
};

class Medium {
public:
    virtual Color transmittance(const Point3 &pointA, const Point3 &pointB) const = 0;
    virtual TransmittanceQueryResult findTransmittance(
        const Point3 &entryPointWorld,
        const Point3 &exitPointWorld,
        float targetTransmittance
    ) const = 0;

};
