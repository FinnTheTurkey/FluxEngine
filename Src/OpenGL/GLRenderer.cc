#include "Flux/OpenGL/GLRenderer.hh"
#include "Flux/ECS.hh"
#include "Flux/Log.hh"
#include "Flux/Renderer.hh"

#include "Flux/Resources.hh"
#include "GLFW/glfw3.h"
#include <bits/stdint-uintn.h>
#include <map>
#include <string>
#include <sstream>

#include "glm/gtc/type_ptr.hpp"
#include "glm/trigonometric.hpp"


using namespace Flux::GLRenderer;

// We need this for callbacks
static GLCtx* current_window = nullptr;

static Flux::ComponentTypeID MeshComponentID;
static Flux::ComponentTypeID TransformComponentID;
static Flux::ComponentTypeID GLMeshComponentID;
static Flux::ComponentTypeID GLEntityComponentID;
static Flux::ComponentTypeID TempShaderComponentID;
static Flux::ComponentTypeID GLShaderComponentID;

// GLFW callbacks
void onFramebufferSizeChanged(GLFWwindow* w, int width, int height)
{
    current_window->height = height;
    current_window->width = width;

    glViewport(0, 0, width, height);
}

// Destructors
void destructorGLMesh(Flux::ECSCtx* ctx, Flux::EntityID entity)
{
    GLMeshCom* com = (GLMeshCom*)Flux::getComponent(ctx, entity, GLMeshComponentID);

    // glDeleteProgram(com->shader_program);
    glDeleteBuffers(1, &com->VAO);
    glDeleteBuffers(1, &com->IBO);
    glDeleteVertexArrays(1, &com->VAO);
}

void destructorGLShader(Flux::ECSCtx* ctx, Flux::EntityID entity)
{
    GLShaderCom* com = (GLShaderCom*)Flux::getComponent(ctx, entity, GLShaderComponentID);

    glDeleteProgram(com->shader_program);
    // glDeleteBuffers(1, &com->VAO);
    // glDeleteBuffers(1, &com->IBO);
    // glDeleteVertexArrays(1, &com->VAO);
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
    gctx->window = glfwCreateWindow(width, height, title.c_str(), NULL, NULL);

    if (gctx->window == NULL)
    {
        LOG_ERROR("Could not initialize GLFW window");
        return;
    }

    glfwMakeContextCurrent(gctx->window);

    // Initialize OpenGL
    if (!gladLoadGLES2Loader((GLADloadproc)glfwGetProcAddress))
    {
        LOG_ERROR("Could not initialize OpenGL");
        return;
    }

    LOG_SUCCESS(std::string("OpenGL Version ") + (char*)glGetString(GL_VERSION) + " on " + (char*)glGetString(GL_VENDOR) + " " + (char*)glGetString(GL_RENDERER));

    // Create viewport
    glViewport(0, 0, gctx->width, gctx->height);

    // Setup callbacks
    glfwSetFramebufferSizeCallback(gctx->window, onFramebufferSizeChanged);

    // Disable V-Sync
    // TODO: Make this an option
    glfwSwapInterval(0); // 1 for v-sync

    // Load IDs for components
    MeshComponentID = Flux::getComponentType("mesh");
    TransformComponentID = Flux::getComponentType("transform");
    GLMeshComponentID = Flux::getComponentType("gl_mesh");
    GLEntityComponentID = Flux::getComponentType("gl_entity");
    TempShaderComponentID = Flux::getComponentType("temp_shader");
    GLShaderComponentID = Flux::getComponentType("gl_shader");

    // Setup destructors
    Flux::setComponentDestructor(GLMeshComponentID, destructorGLMesh);
    Flux::setComponentDestructor(GLShaderComponentID, destructorGLShader);
}


double Flux::GLRenderer::getTime()
{
    return glfwGetTime();
}

bool Flux::GLRenderer::startFrame()
{
    bool sc = glfwWindowShouldClose(current_window->window);
    glfwPollEvents();

    glClearColor(0.0, 0.74, 1.0, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    return !sc;
}

void Flux::GLRenderer::endFrame()
{
    glfwSwapBuffers(current_window->window);
}

void Flux::GLRenderer::destroyWindow()
{
    glfwTerminate();
    delete current_window;
    current_window = nullptr;
}
static glm::mat4 projection;
void showFPS(GLFWwindow* window);

void sysGlRenderer(Flux::ECSCtx* ctx, Flux::EntityID entity, float delta)
{
    if (entity == 0)
    {
        // First entity
        // Calcualte projection matrix
        // TODO: Customisable field of view
        projection = glm::perspective(1.570796f, (float)current_window->width/current_window->height, 0.01f, 100.0f);

        showFPS(current_window->window);
    }

    if (!Flux::hasComponent(ctx, entity, MeshComponentID))
    {
        // Doesn't have a mesh - we don't care
        return;
    }

    // Get the mesh
    Flux::Renderer::MeshCom* mesh = (Flux::Renderer::MeshCom*)Flux::getComponent(ctx, entity, MeshComponentID);

    if (!Flux::hasComponent(ctx, entity, GLEntityComponentID))
    {
        // It hasn't been initialized yet
        // Make sure they don't already exist
        if (!Flux::hasComponent(&Flux::Resources::rctx, mesh->mesh_resource, GLMeshComponentID))
        {
            
            // Get the resource
            Flux::Renderer::MeshRes* mesh_res = (Flux::Renderer::MeshRes*)Flux::Resources::getResource(mesh->mesh_resource);

            // Create new component
            GLMeshCom* mesh_com = new GLMeshCom;

            // Create buffer
            glGenBuffers(1, &mesh_com->VBO);
            glGenBuffers(1, &mesh_com->IBO);

            // Create Vertex Array
            glGenVertexArrays(1, &mesh_com->VAO);

            // Bind vertex array
            glBindVertexArray(mesh_com->VAO);

            // Fill up buffer
            glBindBuffer(GL_ARRAY_BUFFER, mesh_com->VBO);
            glBufferData(GL_ARRAY_BUFFER, sizeof(Flux::Renderer::Vertex) * mesh_res->vertices_length, mesh_res->vertices, GL_STATIC_DRAW);

            // Fill up buffer for indices
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh_com->IBO);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint32_t) * mesh_res->indices_length, mesh_res->indices, GL_STATIC_DRAW);

            // Tell OpenGL what our data means
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 3, (void*)0);
            glEnableVertexAttribArray(0);

            mesh_com->num_vertices = mesh_res->vertices_length;
            mesh_com->num_indices = mesh_res->indices_length;

            // Add to resource entity
            Flux::addComponent(&Flux::Resources::rctx, mesh->mesh_resource, GLMeshComponentID, mesh_com);
        }

        // Make sure they don't already exist
        if (!Flux::hasComponent(&Flux::Resources::rctx, mesh->mesh_resource, GLShaderComponentID))
        {

            // Find the resources
            Flux::Renderer::ShaderRes* shader_res = (Flux::Renderer::ShaderRes*)Flux::Resources::getResource(mesh->shader_resource);

            // Create new component
            auto shader_com = new GLShaderCom;

            // Create shaders
            uint32_t vertex_shader, fragment_shader;
            vertex_shader = glCreateShader(GL_VERTEX_SHADER);
            fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);

            // std::cout << shaders_src->vert_src << "\n" << shaders_src->frag_src << "\n";

            const char* vsrc = shader_res->vert_src.c_str();
            const char* fsrc = shader_res->frag_src.c_str();

            // std::cout << vsrc << "\n" << fsrc << "\n";

            // Compile shaders
            glShaderSource(vertex_shader, 1, &vsrc, NULL);
            glShaderSource(fragment_shader, 1, &fsrc, NULL);

            glCompileShader(vertex_shader);
            glCompileShader(fragment_shader);

            // Check for errors
            int success;
            char infoLog[512];
            glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &success);
            if(!success)
            {
                glGetShaderInfoLog(vertex_shader, 512, NULL, infoLog);
                std::cout << "Vertex shader compilation failed:\n" << infoLog << std::endl;
            }

            success = 0;
            char infoLog2[512];
            glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &success);
            if(!success)
            {
                glGetShaderInfoLog(fragment_shader, 512, NULL, infoLog2);
                std::cout << "Fragment shader compilation failed:\n" << infoLog2 << std::endl;
            }

            // Link shaders
            shader_com->shader_program = glCreateProgram();

            glAttachShader(shader_com->shader_program, vertex_shader);
            glAttachShader(shader_com->shader_program, fragment_shader);
            glLinkProgram(shader_com->shader_program);

            // Check for linking errors
            success = 0;
            char infoLog3[512];
            glGetProgramiv(shader_com->shader_program, GL_LINK_STATUS, &success);
            if(!success)
            {
                glGetProgramInfoLog(shader_com->shader_program, 512, NULL, infoLog3);
                std::cout << "Shader linking failed:\n" << infoLog3 << std::endl;
            }

            // Cleanup
            glDeleteShader(vertex_shader);
            glDeleteShader(fragment_shader);

            // Add to resource entity
            Flux::addComponent(&Flux::Resources::rctx, mesh->shader_resource, GLShaderComponentID, shader_com);
        }

        Flux::addComponent(ctx, entity, GLEntityComponentID, new GLEntityCom);
    }

    // Actually render
    GLMeshCom* mesh_com = (GLMeshCom*)Flux::getComponent(&Flux::Resources::rctx, mesh->mesh_resource, GLMeshComponentID);
    GLShaderCom* shader_com = (GLShaderCom*)Flux::getComponent(&Flux::Resources::rctx, mesh->shader_resource, GLShaderComponentID);
    Flux::Transform::TransformCom* trans_com = (Flux::Transform::TransformCom*)Flux::getComponent(ctx, entity, TransformComponentID);

    // TODO: Probably put this in a better place
    // And do it in a better way

    glUseProgram(shader_com->shader_program);

    int loc = glGetUniformLocation(shader_com->shader_program, "model_view");
    auto mvp = projection * trans_com->model_view;
    glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(mvp));

    glBindVertexArray(mesh_com->VAO);
    glDrawElements(GL_TRIANGLES, mesh_com->num_indices, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

}

int Flux::GLRenderer::addGLRenderer(ECSCtx *ctx)
{
    return Flux::addSystemBack(ctx, sysGlRenderer, false);
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

        glfwSetWindowTitle(window, outs.str().c_str());

        // Reset frame count
        frameCount = 0;
    }

    frameCount ++;
}

// std::map<std::string, int> uniform_locations;

// GLint Flux::GLRenderer::getUniformLocation(const std::string name) 
// {
//     std::map<std::string, GLint>::iterator it = uniform_locations.find(name);
//     if (it == uniform_locations.end())
//     {
//         uniform_locations[name] = glGetUniformLocation(handle, name.c_str());
//     }

//     return uniform_locations[name];
//     // return glGetUniformLocation(handle, name.c_str());
// }

int getUniformLocation(const std::string& name)
{
    LOG_WARN("This is currently broken");
    return -1;
}

void Flux::GLRenderer::setUniform(const std::string& name, const glm::vec2& v)
{
    GLint loc = getUniformLocation(name);
    glUniform2f(loc, v.x, v.y);
}
void Flux::GLRenderer::setUniform(const std::string& name, const glm::vec3& v)
{
    GLint loc = getUniformLocation(name);
    glUniform3f(loc, v.x, v.y, v.z);
}
void Flux::GLRenderer::setUniform(const std::string& name, const glm::vec4& v)
{
    GLint loc = getUniformLocation(name);
    glUniform4f(loc, v.x, v.y, v.z, v.w);
}
void Flux::GLRenderer::setUniform(const std::string& name, const glm::mat4& v)
{
    GLint loc = getUniformLocation(name);
    glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(v));
}

void Flux::GLRenderer::setUniform(const std::string& name, const int& v)
{
    GLint loc = getUniformLocation(name);
    glUniform1i(loc, v);
}

void Flux::GLRenderer::setUniform(const std::string& name, const float& v)
{
    GLint loc = getUniformLocation(name);
    glUniform1f(loc, v);
}

void Flux::GLRenderer::setUniform(const std::string& name, const bool& v)
{
    GLint loc = getUniformLocation(name);
    glUniform1i(loc, v);
}