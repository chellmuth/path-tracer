#pragma once

#include "intersection.h"
#include "material.h"
#include "point.h"
#include "shape.h"
#include "transform.h"

#include <memory>

class Ray;

class Sphere : public Shape {
public:
    Sphere(Point3 center, float radius);

    SurfaceSample sample(RandomGenerator &random) const;
    float pdf(const Point3 &point) const override;
    Intersection testIntersect(const Ray &ray);

    Point3 centroid() const override { return m_center; }

    std::shared_ptr<Shape> transform(const Transform &transform) const override;

    float area() const override;

    void create(
        const Transform &transform,
        std::shared_ptr<Material> material
    );

    bool useBackwardsNormals() const override { return false; }

private:
    Point3 m_center;
    float m_radius;
};
