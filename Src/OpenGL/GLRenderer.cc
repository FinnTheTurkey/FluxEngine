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
GLMeshCom::~GLMeshCom()
{
    // glDeleteProgram(com->shader_program);
    glDeleteBuffers(1, &VAO);
    glDeleteBuffers(1, &IBO);
    glDeleteVertexArrays(1, &VAO);
}

GLShaderCom::~GLShaderCom()
{

    glDeleteProgram(shader_program);
    // glDeleteBuffers(1, &com->VAO);
    // glDeleteBuffers(1, &com->IBO);
    // glDeleteVertexArrays(1, &com->VAO);
}

void Flux::GLRenderer::_startGL()
{
    LOG_SUCCESS(std::string("OpenGL Version ") + (char*)glGetString(GL_VERSION) + " on " + (char*)glGetString(GL_VENDOR) + " " + (char*)glGetString(GL_RENDERER));

    // Create viewport
    glViewport(0, 0, current_window->width, current_window->height);
    glEnable(GL_DEPTH_TEST);

    // Load IDs for components
    // MeshComponentID = Flux::getComponentType("mesh");
    // TransformComponentID = Flux::getComponentType("transform");
    // GLMeshComponentID = Flux::getComponentType("gl_mesh");
    // GLEntityComponentID = Flux::getComponentType("gl_entity");
    // TempShaderComponentID = Flux::getComponentType("temp_shader");
    // GLShaderComponentID = Flux::getComponentType("gl_shader");
    // GLUniformComponentID = Flux::getComponentType("gl_uniform");

    // Setup destructors
    // TODO: Replace this with struct destructors
    // Flux::setComponentDestructor<GLMeshCom>(destructorGLMesh);
    // Flux::setComponentDestructor<GLShaderCom>(destructorGLShader);
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

void processTexture(Flux::Resources::ResourceRef<Flux::Renderer::TextureRes> texture)
{
    if (!texture.getBaseEntity().hasComponent<GLTextureCom>())
    {
        // Create GL texture
        auto txcom = new GLTextureCom;

        glGenTextures(1, &txcom->handle);
        glBindTexture(GL_TEXTURE_2D, txcom->handle);

        // Set the texture wrapping/filtering options (on the currently bound texture object)
        // Also, thanks learnopengl.com
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texture->width, texture->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, texture->image_data);
        glGenerateMipmap(GL_TEXTURE_2D);

        // Free the texture
        texture->destroy();

        texture.getBaseEntity().addComponent(txcom);
    }
}

void GLRendererSystem::dealWithUniforms(Flux::Renderer::MeshCom* mesh, Flux::Resources::ResourceRef<Flux::Renderer::MaterialRes> mat_res, GLShaderCom* shader_res)
{
    bool create = false;
    if (!mesh->mat_resource.getBaseEntity().hasComponent<GLUniformCom>())
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
            // std::vector<const char*> names;
            // int count = 0;

            // for (auto it = mat_res->uniforms.begin(); it != mat_res->uniforms.end(); ++it)
            // {
            //     names.push_back(const_cast<char*>(it->first.c_str()));
            //     if (it->second.type != Renderer::UniformType::Texture)
            //     {
            //         count ++;
            //     }
            // }

            // Request block size from opengl
            // uint32_t block_index = glGetUniformBlockIndex(shader_res->shader_program, "Material");
            // LOG_ASSERT_MESSAGE(glGetError() == GL_INVALID_OPERATION, "OpenGL Error: Invalid Operation: Could not find shader program");
            // LOG_ASSERT_MESSAGE(block_index == GL_INVALID_INDEX, "OpenGL Error: Invalid Index: Could not find uniform block");
            // glGetActiveUniformBlockiv(shader_res->shader_program, block_index, GL_UNIFORM_BLOCK_DATA_SIZE, &uni->block_size);
            // LOG_ASSERT_MESSAGE(glGetError() == GL_INVALID_VALUE, "OpenGL Error: GL_INVALID_VALUE: Could not find uniform block; make sure your shader has a uniform called Material");

            // // Request indices from OpenGL
            // uni->indices.resize(count);
            // glGetUniformIndices(shader_res->shader_program, count, &names[0], &uni->indices[0]);
            // LOG_ASSERT_MESSAGE(glGetError() == GL_INVALID_OPERATION, "OpenGL Error: GL_INVALID_OPERATION: Shader program is not valid. This is most likely an engine problem");

            // int num = 0;
            // for (auto i : uni->indices)
            // {
            //     if (i == GL_INVALID_INDEX)
            //     {
            //         LOG_ERROR("Matching index for uniform " + std::string(names[num]) + " could not be found");
            //     }
            //     num++;
            // }

            // Request offset from OpenGL
            // TODO: Use this function to get list of types for error checking
            // uni->offset.resize(count);
            // glGetActiveUniformsiv(shader_res->shader_program, count, &uni->indices[0], GL_UNIFORM_OFFSET, &uni->offset[0]);

            // TODO: The above function always seems to generate a GL_INVALID_VALUE
            // I have no clue why
            // But everything still works
            // I'm sure this will come back to bite me later
            
            // LOG_ASSERT_MESSAGE(glGetError() == GL_INVALID_VALUE, "Error: Too many uniforms");

            // Flux::addComponent(Flux::Resources::rctx, mesh->mat_resource, GLUniformComponentID, uni);

            uni->block_size = glGetUniformBlockIndex(shader_res->shader_program, "Material");
            glUniformBlockBinding(shader_res->shader_program, uni->block_size, 0);
            mesh->mat_resource.getBaseEntity().addComponent(uni);

            // Put data in buffer
            glGenBuffers(1, &uni->handle);

            // Calculate size of uniform buffer
            int total_size = 0;
            for (auto item: mat_res->uniforms)
            {
                switch (item.second.type)
                {
                    case (Flux::Renderer::UniformType::Int): total_size += sizeof(int); break;
                    case (Flux::Renderer::UniformType::Float): total_size += sizeof(float); break;
                    case (Flux::Renderer::UniformType::Bool): total_size += sizeof(bool); break;
                    case (Flux::Renderer::UniformType::Vector2): total_size += sizeof(glm::vec2); break;
                    case (Flux::Renderer::UniformType::Vector3): total_size += sizeof(glm::vec3); break;
                    case (Flux::Renderer::UniformType::Vector4): total_size += sizeof(glm::vec4); break;
                    case (Flux::Renderer::UniformType::Mat4): total_size += sizeof(glm::mat4); break;
                    case (Flux::Renderer::UniformType::Texture): break;
                }
            }

            auto mc = mesh->mesh_resource.getBaseEntity().getComponent<GLMeshCom>();

            // glBindVertexArray(mc->VAO);
            glBindBuffer(GL_UNIFORM_BUFFER, uni->handle);
            glBufferData(GL_UNIFORM_BUFFER, total_size, NULL, GL_DYNAMIC_DRAW);
            glBindBufferBase(GL_UNIFORM_BUFFER, 0, uni->handle);
            // glBindVertexArray(0);

        }
        else
        {
            // uni = (GLUniformCom*)Flux::getComponent(Flux::Resources::rctx, mesh->mat_resource, GLUniformComponentID);
            uni = mesh->mat_resource.getBaseEntity().getComponent<GLUniformCom>();
        }

        // Create buffer for data
        // unsigned char* block_buffer;
        // block_buffer = (unsigned char*) std::malloc(uni->block_size);
        glBindBuffer(GL_UNIFORM_BUFFER, uni->handle);

        // Chuck data in the buffer
        int index = 0;
        int tex_id = 0;
        uni->textures = std::vector<GLTextureStore>();
        for (auto item: mat_res->uniforms)
        {
            switch (item.second.type)
            {
                case (Flux::Renderer::UniformType::Int):
                {
                    glBufferSubData(GL_UNIFORM_BUFFER, index, sizeof(int), item.second.value);
                    index += sizeof(int);
                    // memcpy(block_buffer + uni->offset[index], item.second.value, sizeof(int));
                    break;
                }
                case (Flux::Renderer::UniformType::Float):
                {
                    glBufferSubData(GL_UNIFORM_BUFFER, index, sizeof(float), item.second.value);
                    index += sizeof(float);
                    // memcpy(block_buffer + uni->offset[index], item.second.value, sizeof(float));
                    break;
                }
                case (Flux::Renderer::UniformType::Bool):
                {
                    int res = ((bool*)item.second.value ? 1: 0);
                    glBufferSubData(GL_UNIFORM_BUFFER, index, sizeof(float), &res);
                    index += sizeof(int);
                    // memcpy(block_buffer + uni->offset[index], &res, sizeof(int));
                    break;
                }
                case (Flux::Renderer::UniformType::Vector2):
                {
                    // I'm hoping glm's vectors are just a struct of floats...
                    // memcpy(block_buffer + uni->offset[index], item.second.value, sizeof(float) * 2);
                    glBufferSubData(GL_UNIFORM_BUFFER, index, sizeof(float) * 2, item.second.value);
                    index += sizeof(float)*2;
                    break;
                }
                case (Flux::Renderer::UniformType::Vector3):
                {
                    // I'm hoping glm's vectors are just a struct of floats...
                    // memcpy(block_buffer + uni->offset[index], item.second.value, sizeof(float) * 3);
                    glBufferSubData(GL_UNIFORM_BUFFER, index, sizeof(float) * 3, item.second.value);
                    index += sizeof(float)*3;
                    break;
                }
                case (Flux::Renderer::UniformType::Vector4):
                {
                    // I'm hoping glm's vectors are just a struct of floats...
                    // memcpy(block_buffer + uni->offset[index], item.second.value, sizeof(float) * 4);
                    glBufferSubData(GL_UNIFORM_BUFFER, index, sizeof(float) * 4, item.second.value);
                    index += sizeof(float)*4;
                    break;
                }
                case (Flux::Renderer::UniformType::Mat4):
                {
                    // memcpy(block_buffer + uni->offset[index], glm::value_ptr(*(glm::mat4*)item.second.value), sizeof(float) * 16);
                    glBufferSubData(GL_UNIFORM_BUFFER, index, sizeof(float) * 16, item.second.value);
                    index += sizeof(float)*16;
                    break;
                }
                case (Flux::Renderer::UniformType::Texture):
                {
                    // For some very stupid reason, textures can't be in uniform buffers
                    auto res = (Resources::ResourceRef<Renderer::TextureRes>*)item.second.value;
                    
                    if (!res->getBaseEntity().hasComponent<GLTextureCom>())
                    {
                        processTexture(Resources::ResourceRef<Renderer::TextureRes>(res->getBaseEntity()));
                    }

                    // memcpy(block_buffer + uni->offset[index], &tex_id, sizeof(int32_t));
                    tex_id ++;
                    uni->textures.push_back(GLTextureStore {glGetUniformLocation(shader_res->shader_program, item.first.c_str()), res->getBaseEntity()});

                    break;
                }
            }
            // index ++;
        }

        glBindBuffer(GL_UNIFORM_BUFFER, 0);
        // glBufferData(GL_UNIFORM_BUFFER, uni->block_size, block_buffer, GL_DYNAMIC_DRAW);
        // glBindBufferBase(GL_UNIFORM_BUFFER, 0, uni->handle);

        // delete block_buffer;

        mat_res->changed = false;

        // Textures
        // if (mat_res->has_texture)
        // {
        //     processTexture(mat_res->diffuse_texture);
        // }
    }
    else
    {
        // Hasn't changed, but we still have to bind it
        auto uni = mesh->mat_resource.getBaseEntity().getComponent<GLUniformCom>();

        // Textures
        for (int i = 0; i < uni->textures.size(); i++)
        {
            // Why must I do this
            int tx;
            switch (i)
            {
                case (0): tx = GL_TEXTURE0; break;
                case (1): tx = GL_TEXTURE1; break;
                case (2): tx = GL_TEXTURE2; break;
                case (3): tx = GL_TEXTURE3; break;
                case (4): tx = GL_TEXTURE4; break;
                case (5): tx = GL_TEXTURE5; break;
                case (6): tx = GL_TEXTURE6; break;
                case (7): tx = GL_TEXTURE7; break;
                case (8): tx = GL_TEXTURE8; break;
                default: LOG_WARN("Too many textures!"); break;
            }

            glActiveTexture(tx);
            glBindTexture(GL_TEXTURE_2D, uni->textures[i].resource.getBaseEntity().getComponent<GLTextureCom>()->handle);
            glUniform1i(uni->textures[i].location, i);
        }
        
        glBindBufferBase(GL_UNIFORM_BUFFER, 0, uni->handle);
        // glBindBuffer(GL_UNIFORM_BUFFER, uni->handle);
        glBindBuffer(GL_UNIFORM_BUFFER, uni->handle);
    }

}

void GLRendererSystem::initGLMaterial(Flux::Renderer::MeshCom* mesh)
{
    // Step 1: Shaders
    // On the material

    // This large block of code adds shaders to the shader resource referenced
    // by the material resource if it doesn't already have them
    auto mat_res = mesh->mat_resource;
    if (!mat_res->shaders.getBaseEntity().hasComponent<GLShaderCom>())
    {
        // Find the resources
        auto shader_res = mat_res->shaders;

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
        // Flux::addComponent(Flux::Resources::rctx, mat_res->shaders, GLShaderComponentID, shader_com);
        mat_res->shaders.getBaseEntity().addComponent(shader_com);
    }

    // Custom Uniforms
    // This creates a Uniform Buffer if one doesn't already exist
    // Don't know if we need this if statement
    if (!mat_res.getBaseEntity().hasComponent<GLUniformCom>())
    {
        // Make shaders active
        GLShaderCom* shader_com = mat_res->shaders.getBaseEntity().getComponent<GLShaderCom>();
        glUseProgram(shader_com->shader_program);

        dealWithUniforms(mesh, mat_res, shader_com);
    }
}

void GLRendererSystem::onSystemStart()
{
    // TODO: Customisable FOV
    projection = glm::perspective(1.570796f, (float)current_window->width/current_window->height, 0.01f, 100.0f);
    
}

void GLRendererSystem::runSystem(Flux::EntityRef entity, float delta)
{
    // TODO: change (once systems are redone)
    // if (entity.getEntityID() == 0)
    // {
    //     // First entity
    //     // Calcualte projection matrix
    //     // TODO: Customisable field of view
    //     projection = glm::perspective(1.570796f, (float)current_window->width/current_window->height, 0.01f, 100.0f);
    // }

    if (!entity.hasComponent<Flux::Renderer::MeshCom>())
    {
        // Doesn't have a mesh - we don't care
        return;
    }

    // Get the mesh
    Flux::Renderer::MeshCom* mesh = entity.getComponent<Flux::Renderer::MeshCom>();

    if (!entity.hasComponent<GLEntityCom>())
    {
        // It hasn't been initialized yet
        // Make sure they don't already exist
        if (!mesh->mesh_resource.getBaseEntity().hasComponent<GLMeshCom>())
        {
            // Get the resource
            auto mesh_res = mesh->mesh_resource;

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
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 8, (void*)0);
            glEnableVertexAttribArray(0);

            // Normal
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 8, (void*)(sizeof(float) * 3));
            glEnableVertexAttribArray(1);

            // Texture
            glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 8, (void*)(sizeof(float) * 6));
            glEnableVertexAttribArray(2);

            mesh_com->num_vertices = mesh_res->vertices_length;
            mesh_com->num_indices = mesh_res->indices_length;

            // Add to resource entity
            // Flux::addComponent(Flux::Resources::rctx, mesh->mesh_resource, GLMeshComponentID, mesh_com);
            mesh->mesh_resource.getBaseEntity().addComponent(mesh_com);
        }

        initGLMaterial(mesh);

        entity.addComponent(new GLEntityCom);
    }

    auto mat_res = mesh->mat_resource;

    // Actually render
    GLMeshCom* mesh_com = mesh->mesh_resource.getBaseEntity().getComponent<GLMeshCom>();
    GLShaderCom* shader_com = mat_res->shaders.getBaseEntity().getComponent<GLShaderCom>();
    Flux::Transform::TransformCom* trans_com = entity.getComponent<Flux::Transform::TransformCom>();

    // TODO: Probably put this in a better place
    // And do it in a better way

    glUseProgram(shader_com->shader_program);

    // int loc = glGetUniformLocation(shader_com->shader_program, "model_view");
    auto mvp = projection * trans_com->model_view;
    glUniformMatrix4fv(shader_com->mvp_location, 1, GL_FALSE, glm::value_ptr(mvp));

    dealWithUniforms(mesh, mat_res, shader_com);

    glEnable(GL_DEPTH_TEST);
    glBindVertexArray(mesh_com->VAO);
    glDrawElements(GL_TRIANGLES, mesh_com->num_indices, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

}

int Flux::GLRenderer::addGLRenderer(ECSCtx *ctx)
{
    // LOG_INFO("Adding GL Renderer system");
    return ctx->addSystemBack(new GLRendererSystem, false);
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