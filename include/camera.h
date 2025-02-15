#pragma once

#include "point.h"
#include "transform.h"
#include "vector.h"

class Ray;

struct Pixel {
    int x;
    int y;
};

struct Resolution {
    int x;
    int y;
};

class Camera {
public:
    Camera(
        Point3 origin,
        Point3 target,
        Vector3 up,
        float verticalFOV,
        Resolution resolution,
        bool flipHandedness
    );

    float getVerticalFOV() const { return m_verticalFOV; }
    Point3 getOrigin() const { return m_origin; }
    Point3 getTarget() const { return m_target; }
    Resolution getResolution() const { return m_resolution; }

    Ray generateRay(float row, float col) const;
    Ray generateRay(int row, int col) const;

    void calculatePixel(const Point3 &point, Pixel *pixel) const;
private:
    Point3 m_origin;
    Point3 m_target;
    Vector3 m_up;
    Resolution m_resolution;

    float m_zNear;
    Transform m_cameraToWorld;
    Transform m_worldToCamera;
    float m_verticalFOV;
};
