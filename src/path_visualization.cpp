#include "path_visualization.h"

#include "point.h"

#include "nanoflann.hpp"

#include <algorithm>

void gl::PathVisualization::init(
    const Sample& sample,
    const int bounce,
    const std::vector<DataSource::Point> &photons,
    const DataSource &dataSource,
    DebugMode debugMode
) {
    if (sample.eyePoints.size() <= 1) { return; }

    std::vector<gl::Line> lines;

    for (int i = 0; i + 1 < sample.eyePoints.size(); i++) {
        gl::Line line = {
            sample.eyePoints[i],
            sample.eyePoints[i + 1],
            Color(1.f, 0.f, 0.f)
        };

        lines.push_back(line);
    }

    for (auto &shadowTest : sample.shadowTests) {
        if (shadowTest.occluded) { continue; }

        gl::Line line = {
            shadowTest.shadingPoint,
            shadowTest.lightPoint,
            Color(1.f, 1.f, 0.f)
        };

        lines.push_back(line);
    }

    m_glLines.init(lines);

    assert(bounce + 1 < sample.eyePoints.size());
    Point3 queryPoint = sample.eyePoints[bounce + 1];
    float queryPointStruct[3] = {
        queryPoint.x(),
        queryPoint.y(),
        queryPoint.z()
    };
    KDTree *tree = new KDTree(3, dataSource, nanoflann::KDTreeSingleIndexAdaptorParams(10));
    tree->buildIndex();

    const size_t searchCount = std::min((size_t)100, dataSource.points.size());
    auto resultIndices = std::vector<size_t>(searchCount);
    std::vector<float> outDistanceSquared(searchCount);

    nanoflann::KNNResultSet<float> resultSet(searchCount);
    resultSet.init(resultIndices.data(), outDistanceSquared.data());

    tree->findNeighbors(resultSet, queryPointStruct, nanoflann::SearchParams());
    std::vector<DataSource::Point> resultPhotons;
    for (auto &index : resultIndices) {
        resultPhotons.push_back(dataSource.points[index]);
    }

    m_photonRenderer.init(queryPoint, resultPhotons, debugMode);
}

void gl::PathVisualization::draw(
    GLfloat (&model)[4][4],
    GLfloat (&view)[4][4],
    GLfloat (&projection)[4][4]
) {
    m_glLines.draw(model, view, projection);
    m_photonRenderer.draw(model, view, projection);
}
