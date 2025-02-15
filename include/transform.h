#pragma once

class Point3;
class Ray;
class Vector3;

class Transform {
public:
    Transform();
    Transform(const float matrix[4][4]);
    Transform(
        const float matrix[4][4],
        const float inverse[4][4]
    );

    Transform transposed() const;
    Transform inversed() const;

    Point3 apply(const Point3 &point) const;
    Point3 applyInverse(const Point3 &point) const;

    Vector3 apply(const Vector3 &vector) const;
    Transform apply(const Transform &transform) const;
    Ray apply(const Ray &ray) const;

    void debug() const;

private:
    float m_matrix[4][4];

    bool m_hasInverse;
    float m_inverse[4][4];
};

Transform lookAt(
    const Point3 &source,
    const Point3 &target,
    const Vector3 &up,
    bool flipHandedness
);
Transform lookAtInverse(const Point3 &source, const Point3 &target, const Vector3 &up);
Transform normalToWorldSpace(const Vector3 &normal, const Vector3 &rayDirection);
Transform normalToWorldSpace(const Vector3 &normal);
Transform worldSpaceToNormal(const Vector3 &normal);
Transform worldSpaceToNormal(const Vector3 &normal, const Vector3 &rayDirection);
