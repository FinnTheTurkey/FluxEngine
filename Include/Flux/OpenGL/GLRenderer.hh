#ifndef FLUX_GL_RENDERER_HH
#define FLUX_GL_RENDERER_HH

// Flux includes
#include "Flux/ECS.hh"
#include "glm/fwd.hpp"

// GL includes
#include <bits/stdint-uintn.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

// STL includes
#include <string>

namespace Flux { namespace GLRenderer {

    /**
    Context struct with all the data used by the GL renderer
    */
    struct GLCtx
    {
        GLFWwindow* window;

        int width;
        int height;
        std::string title;
    };

    /**
    Component where GLRenderer stores all GL-related data
    */
    struct GLMeshCom: Component
    {
        uint32_t VBO;
        uint32_t IBO;
        uint32_t VAO;

        uint32_t num_vertices;
        uint32_t num_indices;
    };

    /**
    Component for shaders
    */
    struct GLShaderCom: Component
    {
        // Shader stuff
        uint32_t shader_program;
    };

    /**
    Little component that tells the renderer that GL has already been setup
    */
    struct GLEntityCom: Component
    {

    };

    /**
    Returns the time since the window was created, in seconds
    */
    double getTime();

    /**
    Creates the window, and the GL context.
    For the GLRenderer, there can only be one context, so it is stored globally
    */
    void createWindow(const int& width, const int& height, const std::string& title);

    /**
    Close the current window, and free all it's resources
    */
    void destroyWindow();

    /**
    Tells the renderer to start the process or rendering a frame. 
    Should be called before any systems.
    If this function returns true, then the frame can be drawn. If it returns false, the window has been destroyed
    */
    bool startFrame();
    
    /**
    Tells the renderer that the frame is over
    */
    void endFrame();

    /**
    Adds the rendering system to the given ECS.
    This allows for entities to have a visual component.
    Returns the system ID
    */
    int addGLRenderer(ECSCtx* ctx);

    /**
    Sets the value of a uniform
    */
    // TODO: Something with these
    void setUniform(const std::string& name, const glm::vec2& v);
    void setUniform(const std::string& name, const glm::vec3& v);
    void setUniform(const std::string& name, const glm::vec4& v);
    void setUniform(const std::string& name, const glm::mat4& v);
    void setUniform(const std::string& name, const int& v);
    void setUniform(const std::string& name, const float& v);
    void setUniform(const std::string& name, const bool& v);

}}

#endif