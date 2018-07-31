#include <iostream>
#include <fstream>

#include "json.hpp"
using json = nlohmann::json;

#include "camera.h"
#include "color.h"
#include "image.h"
#include "intersection.h"
#include "material.h"
#include "ray.h"
#include "scene.h"
#include "obj_parser.h"
#include "vector.h"
#include "transform.h"
#include "window.h"

using namespace std;

int main() {
    printf("Hello, world!\n");

    const int width = 400;
    const int height = 300;

    Image image(width, height);

    // ifstream sceneFile("data/simple.obj");
    // ObjParser objParser(sceneFile);

    ifstream sceneFile("CornellBox-Original.obj");
    ObjParser objParser(sceneFile, Handedness::Left);
    Scene scene = objParser.parseScene();

    Transform cameraToWorld = lookAt(
        Point3(0.f, 1.3f, 4.f),
        Point3(0.f, 1.3f, 0.f),
        Vector3(0.f, 1.f, 0.f)
    );
    Camera camera(cameraToWorld, 45 / 180.f * M_PI);

    for (int row = 0; row < height; row++) {
        for (int col = 0; col < width; col++) {
            Ray ray = camera.generateRay(
                row, col,
                width, height
            );

            Intersection intersection = scene.testIntersect(ray);
            if (intersection.hit) {
                // Vector3 normal = intersection.normal;
                // image.set(
                //     row,
                //     col,
                //     0.5f * (normal.x() + 1.f),
                //     0.5f * (normal.y() + 1.f),
                //     0.5f * (normal.z() + 1.f)
                // );

                Color color = intersection.material->shade(intersection, scene);
                image.set(
                    row,
                    col,
                    color.r(),
                    color.g(),
                    color.b()
                );
            } else {
                image.set(row, col, 0.f, 0.f, 0.f);
            }
        }
    }

    // image.debug();
    // image.write("test.bmp");

    return loop(image.data());
}
