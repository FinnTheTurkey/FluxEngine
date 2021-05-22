/**
This file is where all the windowing related stuff will be stored for the GLFW backend
*/

#include "Flux/Log.hh"
#include "Flux/OpenGL/GLRenderer.hh"
#include "Flux/Input.hh"
#include "Flux/Flux.hh"

// #include <glad/glad.h>
#include "Flux/Renderer.hh"
#include "GLFW/glfw3.h"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/html5.h>
#endif

// STL
#include <sstream>

#include <glm/glm.hpp>

using namespace Flux::GLRenderer;

struct WindowContainer
{
    GLFWwindow* window;
};

static WindowContainer* w;
GLCtx* Flux::GLRenderer::current_window = nullptr;

// GLFW callbacks
void onFramebufferSizeChanged(GLFWwindow* w, int width, int height)
{
    current_window->height = height;
    current_window->width = width;

    glViewport(0, 0, width, height);
}

static float scroll_offset = 0;

void onScroll(GLFWwindow* window, double xoffset, double yoffset)
{
    scroll_offset += yoffset;

    // Call nuklear callbacks
    // nk_gflw3_scroll_callback(window, xoffset, yoffset);
}

void handleMouseMove(GLFWwindow* win, double x, double y)
{
    current_window->mouse_pos.x = x;
    current_window->mouse_pos.y = y;
}

void (*func)();
bool run_loop = false;

void Flux::setMainLoopFunction(void (*fun)())
{
    func = fun;
}

void emRunMainloopFunc()
{
    if (run_loop)
    {
        func();
    }
    // return 1;
}

void Flux::GLRenderer::createWindow(const int &width, const int &height, const std::string &title)
{
    if (current_window != nullptr)
    {
        LOG_ERROR("There can only be one window open at once!");
        return;
    }

    GLCtx* gctx = new GLCtx;
    current_window = gctx;

    gctx->width = width;
    gctx->height = height;
    gctx->title = title;
    gctx->mouse_mode = Input::MouseMode::Free;
    gctx->offset = glm::vec2();
    gctx->mouse_pos = glm::vec2();

#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop(emRunMainloopFunc, -1000, 0);
#endif

    // Create GLFW window
    glfwInit();

    // Tell it the OpenGL version
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifndef __APPLE__
    // This is required on mac
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // Create window
    // TODO: Fullscreen support
    w = new WindowContainer;
    w->window = glfwCreateWindow(width, height, title.c_str(), NULL, NULL);

    if (w->window == NULL)
    {
        LOG_ERROR("Could not initialize GLFW window");
        return;
    }

    glfwMakeContextCurrent(w->window);

    // Initialize OpenGL
    if (!gladLoadGLES2Loader((GLADloadproc)glfwGetProcAddress))
    {
        LOG_ERROR("Could not initialize OpenGL");
        return;
    }

    current_window = gctx;

    _startGL();

    glfwMakeContextCurrent(w->window);

    // Setup callbacks
    glfwSetFramebufferSizeCallback(w->window, onFramebufferSizeChanged);
    glfwSetScrollCallback(w->window, onScroll);
    glfwSetCursorPosCallback(w->window, handleMouseMove);

    // Disable V-Sync
    // TODO: Make this an option
    // glfwSwapInterval(0); // 1 for v-sync
    glfwSwapInterval(1);

#ifdef EMSCRIPTEN
    glfwSwapInterval(1);
#endif
}

static glm::vec2 old_position = glm::vec2(0);

void handleInputs()
{
    if (run_loop == false)
    {
        return;
    }
    
    double x, y;
    x = current_window->mouse_pos.x;
    y = current_window->mouse_pos.y;
    
    if (current_window->mouse_mode == Flux::Input::MouseMode::Captured)
    {
        // Always set the cursor to the center of the screen
        current_window->offset.x = x;
        current_window->offset.y = y;
        x = (float)current_window->width/2;
        y = (float)current_window->height/2;
        glfwSetCursorPos(w->window, x, y);
    }
    else if (current_window->mouse_mode == Flux::Input::MouseMode::Confined)
    {
        // Keep the cursor within the confines of the screen
        current_window->offset.x = old_position.x - x;
        current_window->offset.y = old_position.y - y;
        old_position.x = x;
        old_position.y = y;
        glm::vec2 new_position(x,y);

        if (old_position.x < 10)
        {
            new_position.x = current_window->width-11;
        }
        else if (old_position.x > current_window->width-10)
        {
            new_position.x = 11;
        }

        if (old_position.y < 10)
        {
            new_position.y = current_window->height-11;
        }
        else if (old_position.y > current_window->height-10)
        {
            new_position.y = 11;
        }

        if (new_position != old_position)
        {
            glfwSetCursorPos(w->window, new_position.x, new_position.y);
            old_position = new_position;
        }
    }
    else {
        // The cursor can do whatever it wants
        current_window->offset.x = old_position.x - x;
        current_window->offset.y = old_position.y - y;
    }
    old_position.x = x;
    old_position.y = y;
}

void Flux::runMainloop()
{
    run_loop = true;
#ifdef __EMSCRIPTEN__
    // emscripten_request_animation_frame_loop(emRunMainloopFunc, 0);
#else
    while (!glfwWindowShouldClose(w->window))
    {
        func();
    }

    end();

    // Destroy resources before windows so opengl resource cleanup
    // doesn't cause a segfault 'cause the window is gone
    // Flux::Resources::destroyResources();
    Flux::GLRenderer::destroyWindow();
#endif
}

bool Flux::GLRenderer::_windowStartFrame()
{
    bool sc = glfwWindowShouldClose(w->window);
    glfwPollEvents();

    handleInputs();

    return sc;
}

void showFPS(GLFWwindow* window)
{
    static double previousSeconds = 0.0;
    static int frameCount = 0;
    double elapsedSeconds;
    double currentSeconds = glfwGetTime(); // Time since start (s)

    elapsedSeconds = currentSeconds - previousSeconds;

    // Limit text update to 4/second
    if (elapsedSeconds > 0.25) {
        previousSeconds = currentSeconds;
        double fps = (double)frameCount / elapsedSeconds;
        double msPerFrame = 1000.0 / fps;

        std::ostringstream outs;
        outs.precision(3); // Set precision of numbers

        outs << std::fixed << "FluxTest" << " - " << "FPS: " << fps << " Frame time: " << msPerFrame << "ms";

        glfwSetWindowTitle(w->window, outs.str().c_str());

        // Reset frame count
        frameCount = 0;
    }

    frameCount ++;
}

void Flux::GLRenderer::_windowEndFrame()
{
#ifndef EMSCRIPTEN
    showFPS(w->window);
#endif
    glfwSwapBuffers(w->window);
}

void Flux::GLRenderer::destroyWindow()
{
    glfwTerminate();
    delete current_window;
    current_window = nullptr;
}

double Flux::Renderer::getTime()
{
    return glfwGetTime();
}

bool Flux::Input::isKeyPressed(int key)
{
    if (glfwGetKey(w->window, key) == GLFW_PRESS)
    {
        return true;
    }
    
    return false;
}

bool Flux::Input::isMouseButtonPressed(int button)
{
    if (glfwGetMouseButton(w->window, button) == GLFW_PRESS)
    {
        return true;
    }
    
    return false;
}

// Mouse stuff
void Flux::Input::setCursorMode(Input::CursorMode mode)
{
    if (mode == Input::CursorMode::Hidden)
    {
        glfwSetInputMode(w->window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
    }
    else
    {
        glfwSetInputMode(w->window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
}

void Flux::Input::setMouseMode(Input::MouseMode mode)
{
    current_window->mouse_mode = mode;
}

glm::vec2& Flux::Input::getMouseOffset()
{
    return current_window->offset;
}

glm::vec2& Flux::Input::getMousePosition()
{
    return current_window->mouse_pos;
}

float Flux::Input::getScrollWheelOffset()
{
    return scroll_offset;
}
