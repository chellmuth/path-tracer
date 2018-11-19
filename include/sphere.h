#pragma once

#include "color.h"
#include "intersection.h"
#include "point.h"
#include "shape.h"

class Ray;

class Sphere : public Shape {
public:
    Sphere(Point3 center, float radius, Color color);

    SurfaceSample sample(RandomGenerator &random) const;
    Intersection testIntersect(const Ray &ray);

private:
    float area() const;

    Point3 m_center;
    float m_radius;
    Color m_color;
};
