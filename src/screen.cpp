#include "screen.h"

#include "globals.h"
#include "job.h"
#include "path_visualization.h"
#include "photon_renderer.h"
#include "rasterizer.h"
#include "visualization.h"

#include <nanogui/checkbox.h>
#include <nanogui/button.h>
#include <nanogui/entypo.h>
#include <nanogui/glcanvas.h>
#include <nanogui/layout.h>
#include <nanogui/opengl.h>

#include <iostream>
#include <sstream>

using namespace nanogui;
using namespace std;

static const int MaxSavedSamples = 16;

class GLWidget : public nanogui::Widget {
public:
    GLWidget(Widget *parent, Scene &scene, int width, int height)
        : nanogui::Widget(parent)
    {
        using namespace nanogui;

        setSize({width, height});

        m_rasterizer = new Rasterizer(this, scene, width, height);
        m_rasterizer->setSize({width, height});
        m_rasterizer->init();
        m_rasterizer->setBackgroundColor({100, 100, 100, 255});
    }

    void setShowVisualization(bool showVisualization) {
        m_rasterizer->setShowVisualization(showVisualization);
    }

    void setVisualization(std::unique_ptr<gl::Visualization> renderer) {
        m_rasterizer->setVisualization(std::move(renderer));
    }

    virtual bool keyboardEvent(int key, int scancode, int action, int modifiers) {
        if (key == GLFW_KEY_W) {
            m_rasterizer->move(Direction::Forward);
        } else if (key == GLFW_KEY_S) {
            m_rasterizer->move(Direction::Backward);
        } else if (key == GLFW_KEY_A) {
            m_rasterizer->move(Direction::Left);
        } else if (key == GLFW_KEY_D) {
            m_rasterizer->move(Direction::Right);
        } else if (key == GLFW_KEY_M && action == GLFW_PRESS) {
            m_rasterizer->updateDebugMode();
        }

        return false;
    }

private:
    Rasterizer *m_rasterizer;
};


class SampleWidget : public nanogui::Widget {
public:
    SampleWidget(
        Widget *parent,
        const SampleWidgetProps &props,
        std::function<void(int)> callback
    )
        : nanogui::Widget(parent),
          m_props(props),
          m_callback(callback)
    {
        using namespace nanogui;

        setLayout(new GroupLayout());

        auto container = new Widget(this);
        auto containerLayout = new BoxLayout(Orientation::Horizontal);
        containerLayout->setSpacing(6);
        container->setLayout(containerLayout);

        m_backButton = new Button(container, "", ENTYPO_ICON_ARROW_LEFT);
        m_backButton->setFixedWidth(40);
        m_backButton->setCallback([this] { m_callback(-1); });

        m_sampleLabel = new Label(container, "");

        m_forwardButton = new Button(container, "", ENTYPO_ICON_ARROW_RIGHT);
        m_forwardButton->setFixedWidth(40);
        m_forwardButton->setCallback([this] { m_callback(1); });

        updateButtonStates();
        updateCaption();
    }

    void update(const SampleWidgetProps &props) {
        m_props = props;

        updateButtonStates();
        updateCaption();
    }

private:
    void updateButtonStates() {
        m_backButton->setEnabled(m_props.currentSample > 0);
        m_forwardButton->setEnabled(m_props.currentSample + 1 < m_props.sampleCount);

        std::cout << "enabled? " << m_backButton->enabled() << std::endl;
    }

    void updateCaption() {
        std::string sampleCaption = "Sample: " + std::to_string(m_props.currentSample + 1);
        m_sampleLabel->setCaption(sampleCaption);
    }

    SampleWidgetProps m_props;
    std::function<void(int)> m_callback;

    nanogui::ref<Label> m_sampleLabel;
    nanogui::ref<Button> m_backButton;
    nanogui::ref<Button> m_forwardButton;
};

RenderWidget::RenderWidget(
    Widget *parent,
    Image &image,
    std::function<void(int x, int y)> clickCallback,
    int width,
    int height
)
    : nanogui::Widget(parent),
      m_clickCallback(clickCallback)
{
    using namespace nanogui;

    setSize({width, height});

    m_canvas = new Canvas(this, image, width, height);
    m_canvas->setSize({width, height});
    m_canvas->init();
    m_canvas->setBackgroundColor({100, 100, 100, 255});
}

bool RenderWidget::keyboardEvent(int key, int scancode, int action, int modifiers)
{
    if (Widget::keyboardEvent(key, scancode, action, modifiers)) {
        return true;
    }

    if (key == GLFW_KEY_S && action == GLFW_PRESS) {
        m_canvas->save("output");
    }

    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        setVisible(false);
        return true;
    }

    return false;
}

void RenderWidget::draw(NVGcontext *ctx)
{
    m_canvas->syncTextureBuffer();

    Widget::draw(ctx);
}

bool RenderWidget::mouseButtonEvent(const Eigen::Vector2i &p, int button, bool down, int modifiers)
{
    if (button == GLFW_MOUSE_BUTTON_1 && down) {
        m_clickCallback(p.x(), p.y());
        return true;
    }

    return false;
}

PathedScreen::PathedScreen(
    Image &image,
    Scene &scene,
    int width,
    int height
)
    : nanogui::Screen(Eigen::Vector2i(width + 300, height), "Pathed: " + g_job->outputDirectory(), false),
      m_width(width),
      m_height(height),
      m_sampleProps({ .sampleCount = 0, .currentSample = 0, .renderX = 0, .renderY = 0 })
{
    setLayout(new BoxLayout(Orientation::Horizontal));

    Widget *leftPanel = new Widget(this);
    leftPanel->setLayout(new GroupLayout());
    leftPanel->setFixedSize(Eigen::Vector2i(300, height));

    Widget *rightPanel = new Widget(this);
    rightPanel->setSize(Eigen::Vector2i(width, height));

    auto sampleCallback = [this](int sampleOffset) {
        m_sampleProps.currentSample += sampleOffset;
        m_sampleWidget->update(m_sampleProps);

        setPathVisualization();
    };
    m_sampleWidget = new SampleWidget(leftPanel, m_sampleProps, sampleCallback);

    m_glWidget = new GLWidget(rightPanel, scene, width, height);
    auto clickCallback = [=](int x, int y) {
        std::cout << "clicked x: " << x << " y: " << y << std::endl;
        int renderY = height - y - 1;

        m_sampleProps.currentSample = 0;
        m_sampleProps.renderY = renderY;
        m_sampleProps.renderX = x;

        m_sampleWidget->update(m_sampleProps);

        setPathVisualization();
    };
    m_renderWidget = new RenderWidget(rightPanel, image, clickCallback, width, height);

    m_renderWidget->setVisible(true);
    m_glWidget->setVisible(false);

    auto visualizationToggle = new CheckBox(leftPanel, "Debug Mode");
    visualizationToggle->setCallback([this](bool isChecked) {
        if (isChecked) {
            m_renderWidget->setVisible(false);
            m_glWidget->setVisible(true);
        } else {
            m_renderWidget->setVisible(true);
            m_glWidget->setVisible(false);
        }
    });

    m_sampleLabel = new Label(leftPanel, "Sample: 0");

    auto reloadButton = new Button(leftPanel, "Reload json");
    reloadButton->setCallback([this]() {
        reloadRadioButtons();
    });

    m_buttonsGroup = new Widget(leftPanel);
    m_buttonsGroup->setLayout(new GroupLayout());

    reloadRadioButtons();

    performLayout();
}

void PathedScreen::setPathVisualization()
{
    auto pathVisualization = std::make_unique<gl::PathVisualization>();

    const auto &sampleLookup = *m_sampleLookups[m_sampleProps.currentSample];
    const Sample &sample = sampleLookup[m_sampleProps.renderY * m_width + m_sampleProps.renderX];
    pathVisualization->init(sample);

    m_glWidget->setVisualization(std::move(pathVisualization));
}

void PathedScreen::reloadRadioButtons()
{
    std::vector<std::string> files = visualization::files();

    int childCount = m_buttonsGroup->childCount();
    for (int i = childCount - 1; i >= 0; i--) {
        m_buttonsGroup->removeChild(i);
    }

    for (auto &file : files) {
        Button *fileButton = new Button(m_buttonsGroup, file);
        fileButton->setFlags(Button::RadioButton);
        fileButton->setCallback([this, file](void) {
            auto renderer = std::make_unique<gl::PhotonRenderer>();
            renderer->init(g_job->visualizationPath(file));
            m_glWidget->setVisualization(std::move(renderer));
            // m_glWidget->setShowVisualization(true);
        });
    }

    performLayout();
}

void PathedScreen::updateRenderStatus(const RenderStatus &renderStatus)
{
    std::ostringstream sampleStream;
    sampleStream << "Sample: " << renderStatus.sample();

    m_sampleLabel->setCaption(sampleStream.str());
    if (renderStatus.sample() <= MaxSavedSamples) {
        m_sampleLookups.push_back(renderStatus.sampleLookup());

        m_sampleProps.sampleCount = m_sampleLookups.size();
        m_sampleWidget->update(m_sampleProps);
    }
}

bool PathedScreen::keyboardEvent(int key, int scancode, int action, int modifiers)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        setVisible(false);
        return true;
    }

    return m_glWidget->keyboardEvent(key, scancode, action, modifiers);
}
