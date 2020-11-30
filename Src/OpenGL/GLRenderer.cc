#include "Flux/OpenGL/GLRenderer.hh"
#include "Flux/ECS.hh"
#include "Flux/Log.hh"
#include "Flux/Renderer.hh"
#include "Flux/Input.hh"

#include "Flux/Resources.hh"
// #include "GLFW/glfw3.h"
// #include <bits/stdint-uintn.h>
#include <cstdlib>
#include <cstring>
#include <map>
#include <string>
#include <sstream>

#include "glm/gtc/type_ptr.hpp"
#include "glm/trigonometric.hpp"


using namespace Flux::GLRenderer;

// We need this for callbacks
// static GLCtx* current_window = nullptr;

static Flux::ComponentTypeID MeshComponentID;
static Flux::ComponentTypeID TransformComponentID;
static Flux::ComponentTypeID GLMeshComponentID;
static Flux::ComponentTypeID GLEntityComponentID;
static Flux::ComponentTypeID TempShaderComponentID;
static Flux::ComponentTypeID GLShaderComponentID;
static Flux::ComponentTypeID GLUniformComponentID;


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

void Flux::GLRenderer::_startGL()
{
    LOG_SUCCESS(std::string("OpenGL Version ") + (char*)glGetString(GL_VERSION) + " on " + (char*)glGetString(GL_VENDOR) + " " + (char*)glGetString(GL_RENDERER));

    // Create viewport
    glViewport(0, 0, current_window->width, current_window->height);

    // Load IDs for components
    MeshComponentID = Flux::getComponentType("mesh");
    TransformComponentID = Flux::getComponentType("transform");
    GLMeshComponentID = Flux::getComponentType("gl_mesh");
    GLEntityComponentID = Flux::getComponentType("gl_entity");
    TempShaderComponentID = Flux::getComponentType("temp_shader");
    GLShaderComponentID = Flux::getComponentType("gl_shader");
    GLUniformComponentID = Flux::getComponentType("gl_uniform");

    // Setup destructors
    Flux::setComponentDestructor(GLMeshComponentID, destructorGLMesh);
    Flux::setComponentDestructor(GLShaderComponentID, destructorGLShader);
}

bool Flux::GLRenderer::startFrame()
{
    auto sc = _windowStartFrame();

    glClearColor(0.0, 0.74, 1.0, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    return !sc;
}

void Flux::GLRenderer::endFrame()
{
    _windowEndFrame();
}

static glm::mat4 projection;


void dealWithUniforms(Flux::Renderer::MeshCom* mesh, Flux::Renderer::MaterialRes* mat_res, GLShaderCom* shader_res)
{
    bool create = false;
    if (!Flux::hasComponent(Flux::Resources::rctx, mesh->mat_resource, GLUniformComponentID))
    {
        // Create the uniform buffer
        create = true;
    }

    if (create || mat_res->changed)
    {
        // We have to re-upload the uniform data to the gpu
        GLUniformCom* uni;
        if (create)
        {
            uni = new GLUniformCom;

            // Get nessesary data
            std::vector<const char*> names;
            int count = 0;
            for (auto item : mat_res->uniforms)
            {
                names.push_back(item.first.c_str());
                count ++;
            }

            // Request block size from opengl
            uint32_t block_index = glGetUniformBlockIndex(shader_res->shader_program, "Material");
            // LOG_ASSERT_MESSAGE(block_index == GL_INVALID_OPERATION, "OpenGL Error: Invalid Operation: Could not find shader program");
            glGetActiveUniformBlockiv(shader_res->shader_program, block_index, GL_UNIFORM_BLOCK_DATA_SIZE, &uni->block_size);
            LOG_ASSERT_MESSAGE(glGetError() == GL_INVALID_VALUE, "OpenGL Error: GL_INVALID_VALUE: Could not find uniform block; make sure your shader has a uniform called Material");

            // Request indices from OpenGL
            uni->indices.resize(count);
            glGetUniformIndices(shader_res->shader_program, count, &names[0], &uni->indices[0]);
            LOG_ASSERT_MESSAGE(glGetError() == GL_INVALID_VALUE, "OpenGL Error: GL_INVALID_VALUE: Could not find uniform block; make sure your shader has a uniform called Material");

            // Request offset from OpenGL
            // TODO: Use this function to get list of types for error checking
            uni->offset.resize(count);
            glGetActiveUniformsiv(shader_res->shader_program, count, &uni->indices[0], GL_UNIFORM_OFFSET, &uni->offset[0]);

            Flux::addComponent(Flux::Resources::rctx, mesh->mat_resource, GLUniformComponentID, uni);
        }
        else
        {
            uni = (GLUniformCom*)Flux::getComponent(Flux::Resources::rctx, mesh->mat_resource, GLUniformComponentID);
        }

        // Create buffer for data
        unsigned char* block_buffer;
        block_buffer = (unsigned char*) std::malloc(uni->block_size);

        // Chuck data in the buffer
        int index = 0;
        for (auto item: mat_res->uniforms)
        {
            switch (item.second.type)
            {
                case (Flux::Renderer::UniformType::Int):
                {
                    memcpy(block_buffer + uni->offset[index], item.second.value, sizeof(int));
                    break;
                }
                case (Flux::Renderer::UniformType::Float):
                {
                    memcpy(block_buffer + uni->offset[index], item.second.value, sizeof(float));
                    break;
                }
                case (Flux::Renderer::UniformType::Bool):
                {
                    int res = ((bool*)item.second.value ? 1: 0);
                    memcpy(block_buffer + uni->offset[index], &res, sizeof(int));
                    break;
                }
                case (Flux::Renderer::UniformType::Vector2):
                {
                    // I'm hoping glm's vectors are just a struct of floats...
                    memcpy(block_buffer + uni->offset[index], item.second.value, sizeof(float) * 2);
                    break;
                }
                case (Flux::Renderer::UniformType::Vector3):
                {
                    // I'm hoping glm's vectors are just a struct of floats...
                    memcpy(block_buffer + uni->offset[index], item.second.value, sizeof(float) * 3);
                    break;
                }
                case (Flux::Renderer::UniformType::Vector4):
                {
                    // I'm hoping glm's vectors are just a struct of floats...
                    memcpy(block_buffer + uni->offset[index], item.second.value, sizeof(float) * 4);
                    break;
                }
                case (Flux::Renderer::UniformType::Mat4):
                {
                    memcpy(block_buffer + uni->offset[index], glm::value_ptr(*(glm::mat4*)item.second.value), sizeof(float) * 16);
                    break;
                }
            }
            index ++;
        }

        // Put data in buffer
        if (create)
        {
            glGenBuffers(1, &uni->handle);
        }

        glBindBuffer(GL_UNIFORM_BUFFER, uni->handle);
        glBufferData(GL_UNIFORM_BUFFER, uni->block_size, block_buffer, GL_DYNAMIC_DRAW);
        glBindBufferBase(GL_UNIFORM_BUFFER, 0, uni->handle);

        delete block_buffer;
    }

}

void initGLMaterial(Flux::Renderer::MeshCom* mesh)
{
    // Step 1: Shaders
    // On the material

    // This large block of code adds shaders to the shader resource referenced
    // by the material resource if it doesn't already have them
    auto mat_res = (Flux::Renderer::MaterialRes*)Flux::Resources::getResource(mesh->mat_resource);
    if (!Flux::hasComponent(Flux::Resources::rctx, mat_res->shaders, GLShaderComponentID))
    {
        // Find the resources
        Flux::Renderer::ShaderRes* shader_res = (Flux::Renderer::ShaderRes*)Flux::Resources::getResource(mat_res->shaders);

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

        shader_com->mvp_location = glGetUniformLocation(shader_com->shader_program, "model_view");

        // Cleanup
        glDeleteShader(vertex_shader);
        glDeleteShader(fragment_shader);

        // Add to resource entity
        Flux::addComponent(Flux::Resources::rctx, mat_res->shaders, GLShaderComponentID, shader_com);
    }

    // Custom Uniforms
    // This creates a Uniform Buffer if one doesn't already exist
    if (!Flux::hasComponent(Flux::Resources::rctx, mesh->mat_resource, GLShaderComponentID))
    {
        // Make shaders active
        GLShaderCom* shader_com = (GLShaderCom*)Flux::getComponent(Flux::Resources::rctx, mat_res->shaders, GLShaderComponentID);
        glUseProgram(shader_com->shader_program);

        dealWithUniforms(mesh, mat_res, shader_com);
    }
}

void sysGlRenderer(Flux::ECSCtx* ctx, Flux::EntityID entity, float delta)
{
    if (entity == 0)
    {
        // First entity
        // Calcualte projection matrix
        // TODO: Customisable field of view
        projection = glm::perspective(1.570796f, (float)current_window->width/current_window->height, 0.01f, 100.0f);
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
        if (!Flux::hasComponent(Flux::Resources::rctx, mesh->mesh_resource, GLMeshComponentID))
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
            Flux::addComponent(Flux::Resources::rctx, mesh->mesh_resource, GLMeshComponentID, mesh_com);
        }

        initGLMaterial(mesh);

        Flux::addComponent(ctx, entity, GLEntityComponentID, new GLEntityCom);
    }

    auto mat_res = (Flux::Renderer::MaterialRes*)Flux::Resources::getResource(mesh->mat_resource);

    // Actually render
    GLMeshCom* mesh_com = (GLMeshCom*)Flux::getComponent(Flux::Resources::rctx, mesh->mesh_resource, GLMeshComponentID);
    GLShaderCom* shader_com = (GLShaderCom*)Flux::getComponent(Flux::Resources::rctx, mat_res->shaders, GLShaderComponentID);
    Flux::Transform::TransformCom* trans_com = (Flux::Transform::TransformCom*)Flux::getComponent(ctx, entity, TransformComponentID);

    // TODO: Probably put this in a better place
    // And do it in a better way

    glUseProgram(shader_com->shader_program);

    // int loc = glGetUniformLocation(shader_com->shader_program, "model_view");
    auto mvp = projection * trans_com->model_view;
    glUniformMatrix4fv(shader_com->mvp_location, 1, GL_FALSE, glm::value_ptr(mvp));

    dealWithUniforms(mesh, mat_res, shader_com);

    glBindVertexArray(mesh_com->VAO);
    glDrawElements(GL_TRIANGLES, mesh_com->num_indices, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

}

int Flux::GLRenderer::addGLRenderer(ECSCtx *ctx)
{
    // LOG_INFO("Adding GL Renderer system");
    return Flux::addSystemBack(ctx, sysGlRenderer, false);
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

// void Flux::GLRenderer::setUniform(const std::string& name, const glm::vec2& v)
// {
//     GLint loc = getUniformLocation(name);
//     glUniform2f(loc, v.x, v.y);
// }
// void Flux::GLRenderer::setUniform(const std::string& name, const glm::vec3& v)
// {
//     GLint loc = getUniformLocation(name);
//     glUniform3f(loc, v.x, v.y, v.z);
// }
// void Flux::GLRenderer::setUniform(const std::string& name, const glm::vec4& v)
// {
//     GLint loc = getUniformLocation(name);
//     glUniform4f(loc, v.x, v.y, v.z, v.w);
// }
// void Flux::GLRenderer::setUniform(const std::string& name, const glm::mat4& v)
// {
//     GLint loc = getUniformLocation(name);
//     glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(v));
// }

// void Flux::GLRenderer::setUniform(const std::string& name, const int& v)
// {
//     GLint loc = getUniformLocation(name);
//     glUniform1i(loc, v);
// }

// void Flux::GLRenderer::setUniform(const std::string& name, const float& v)
// {
//     GLint loc = getUniformLocation(name);
//     glUniform1f(loc, v);
// }

// void Flux::GLRenderer::setUniform(const std::string& name, const bool& v)
// {
//     GLint loc = getUniformLocation(name);
//     glUniform1i(loc, v);
// }