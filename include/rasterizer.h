#pragma once

#include <nanogui/opengl.h>
#include <nanogui/glcanvas.h>

#include "gl_scene.h"
#include "scene.h"
#include "shader.h"

class Rasterizer : public nanogui::GLCanvas {
public:
    Rasterizer(Widget *parent, Scene &scene, int width, int height);

    void init();
    virtual void drawGL() override;

private:
    Scene &mScene;
    gl::Scene mGLScene;

    int mWidth, mHeight;
    Shader mShader;
};
