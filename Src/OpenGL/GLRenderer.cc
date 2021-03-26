#include "Flux/OpenGL/GLRenderer.hh"
#include "Flux/ECS.hh"
#include "Flux/Log.hh"
#include "Flux/Physics.hh"
#include "Flux/Renderer.hh"
#include "Flux/Input.hh"

#include "Flux/Resources.hh"
// #include "GLFW/glfw3.h"
// #include <bits/stdint-uintn.h>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <map>
#include <string>
#include <sstream>

#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glm/trigonometric.hpp"


using namespace Flux::GLRenderer;

// We need this for callbacks
// static GLCtx* current_window = nullptr;

// Destructors
GLMeshCom::~GLMeshCom()
{
    // TODO: Figure out why this line was breaking everything
    // glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &IBO);
}

GLShaderCom::~GLShaderCom()
{

    glDeleteProgram(shader_program);
}

void Flux::GLRenderer::_startGL()
{
    LOG_SUCCESS(std::string("OpenGL Version ") + (char*)glGetString(GL_VERSION) + " on " + (char*)glGetString(GL_VENDOR) + " " + (char*)glGetString(GL_RENDERER));

    // Create viewport
    glViewport(0, 0, current_window->width, current_window->height);
    glEnable(GL_DEPTH_TEST);
}

bool Flux::GLRenderer::startFrame()
{
    auto sc = _windowStartFrame();

    glClearColor(0.0, 0.74, 1.0, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    return !sc;
}

void Flux::GLRenderer::endFrame()
{
    _windowEndFrame();
}

// static glm::mat4 projection;

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
        LOG_ASSERT_MESSAGE(glGetError() == GL_INVALID_VALUE, "Texture "+ texture->filename + " failed");
        glGenerateMipmap(GL_TEXTURE_2D);

        // Free the texture
        texture->destroy();

        texture.getBaseEntity().addComponent(txcom);
    }
}

GLRendererSystem::GLRendererSystem():
lights(new Renderer::LightSystem)
{
    
}

void GLRendererSystem::onSystemAdded(ECSCtx *ctx)
{
    ctx->addSystemFront(lights);
    ctx->addSystemFront(new Flux::Physics::BroadPhaseSystem);
    ctx->addSystemBack(new Flux::Transform::EndFrameSystem);
}

void GLRendererSystem::dealWithLights()
{
    if (!setup_lighting)
    {
        // Create initial light data
        glGenBuffers(1, &light_buffer);

        // This is all in layout std140
        // Which means:
        //  - Vec3s and Vec4s are _both_ 4 floats long
        //  - Arrays are mostly normal, except all arrays must be a multiple
        //    of the size of a vec4
        constexpr int vec4_size = sizeof(float) * 4;
        // 128 floats is already a multiple of vec4_size, so we don't need to worry about that
        // constexpr int total_size = (128 * vec4_size * 2) + (128 * sizeof(float));
        constexpr int total_size = (128 * vec4_size * 4); // + (128 * sizeof(float));

        glBindBuffer(GL_UNIFORM_BUFFER, light_buffer);
        glBufferData(GL_UNIFORM_BUFFER, total_size, NULL, GL_DYNAMIC_DRAW);
        glBindBufferRange(GL_UNIFORM_BUFFER, 1, light_buffer, 0, total_size);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);

        // Fill with zeros for now
        // Buffer of zeros:
        char zeros[total_size];
        for (int i = 0; i < total_size; i++)
        {
            zeros[i] = 0;
        }

        glBindBuffer(GL_UNIFORM_BUFFER, light_buffer);
        glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(zeros), &zeros);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
        setup_lighting = true;

        const GLenum err = glGetError();
        if (GL_NO_ERROR != err)
        {
            LOG_ERROR("OpenGL Error");
        }
    }

    // Add all the changed lights to the buffer
    glBindBuffer(GL_UNIFORM_BUFFER, light_buffer);
    for (auto i : lights->lights_that_changed)
    {
        auto light = lights->lights[i];
        auto tc = light.getComponent<Transform::TransformCom>();
        auto pos = glm::vec3(tc->model * glm::vec4(0,0,0,1));

        auto lc = light.getComponent<Renderer::LightCom>();

        if (lc->type == Renderer::LightType::Spot)
        {
            // Calculate direction from transform
            lc->direction = -glm::vec3(tc->model * glm::vec4(0, 0, 1, 0));
        }

        // Add to buffer
        glBufferSubData(GL_UNIFORM_BUFFER, sizeof(glm::vec4) * i, sizeof(glm::vec3), &pos);
        glBufferSubData(GL_UNIFORM_BUFFER, sizeof(glm::vec4) * 128 + (sizeof(glm::vec4) * i), sizeof(glm::vec3), &lc->direction);
        glBufferSubData(GL_UNIFORM_BUFFER, sizeof(glm::vec4) * 128 * 2 + (sizeof(glm::vec4) * i), sizeof(glm::vec3), &lc->color);
        auto infos = glm::vec3((float)lc->type, lc->radius, glm::cos(lc->cutoff));
        glBufferSubData(GL_UNIFORM_BUFFER, sizeof(glm::vec4) * 128 * 3 + (sizeof(glm::vec4) * i), sizeof(glm::vec3), &infos);
    }
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    const GLenum err = glGetError();
    if (GL_NO_ERROR != err)
    {
        LOG_ERROR("OpenGL Error");
    }
}

void GLRendererSystem::dealWithUniforms(Flux::Renderer::MeshCom* mesh, Flux::Renderer::MaterialRes* mat_res, GLShaderCom* shader_res)
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

            uni->block_size = glGetUniformBlockIndex(shader_res->shader_program, "Material");
            glUniformBlockBinding(shader_res->shader_program, uni->block_size, 0);
            mesh->mat_resource.getBaseEntity().addComponent(uni);

            int block_size;
            glGetActiveUniformBlockiv(shader_res->shader_program, uni->block_size, GL_UNIFORM_BLOCK_DATA_SIZE, &block_size);
            std::cout << "Block size: " << block_size << "\n";

            // Put data in buffer
            glGenBuffers(1, &uni->handle);
            glBindBuffer(GL_UNIFORM_BUFFER, uni->handle);

            int size = mat_res->uniforms.size();
            
            std::vector<const char*> uniformNames;

            // uniform_names_nice.resize(mat_res->uniforms.size());
            int count = 0;
            Renderer::UniformType last_type;
            for (auto item : mat_res->uniforms)
            {
                // TODO: "Material." + name
                if (item.second.type != Renderer::UniformType::Texture)
                {
                    char* x = new char[item.first.length()];
                    auto length = item.first.copy(x, item.first.length(), 0);
                    x[length] = '\0';
                    uniformNames.push_back(x);
                    last_type = item.second.type;
                }
                else
                {
                    // Make the size smaller so we don't tell OpenGL we're sending uniforms that don't exist
                    size--;
                }

                count++;
            }

            // Just a safety check
            size = uniformNames.size();

            // For MSVC
            const int array_size = size;

            GLuint uniformIndex[array_size];
            glGetUniformIndices(shader_res->shader_program, size, &uniformNames[0], uniformIndex);
            LOG_ASSERT_MESSAGE(glGetError() == GL_INVALID_OPERATION, "GL_INVALID_OPERATION");

            GLint offsets[array_size];
            glGetActiveUniformsiv(shader_res->shader_program, size, uniformIndex, GL_UNIFORM_OFFSET, offsets);
            LOG_ASSERT_MESSAGE(glGetError() == GL_INVALID_VALUE, " uniformCount is greater than or equal to the value of GL_ACTIVE_UNIFORMS for program.");

            int uniform_count;
            glGetProgramiv(shader_res->shader_program, GL_ACTIVE_UNIFORM_BLOCKS, &uniform_count);
            std::cout << "Uniform Count: " << uniform_count << "\n";

            int total_size;

            switch(last_type) {
                case (Flux::Renderer::UniformType::Int):
                    total_size = sizeof(int);
                    break;
                case (Flux::Renderer::UniformType::Float):
                    total_size = sizeof(float);
                    break;
                case (Flux::Renderer::UniformType::Bool):
                    total_size = sizeof(int);
                    break;
                case (Flux::Renderer::UniformType::Vector2):
                    total_size = sizeof(float) * 2;
                    break;
                case (Flux::Renderer::UniformType::Vector3):
                    total_size = sizeof(glm::vec3);
                    break;
                // case (Flux::Renderer::UniformType::Vector3): total_size += sizeof(float) * 4; break;
                case (Flux::Renderer::UniformType::Vector4):
                    total_size = sizeof(float) * 4;
                    break;
                case (Flux::Renderer::UniformType::Mat4):
                    total_size = sizeof(glm::mat4);
                    break;
                case (Flux::Renderer::UniformType::Texture): break;
            }

            // glBindVertexArray(mc->VAO);
            glBindBuffer(GL_UNIFORM_BUFFER, uni->handle);
            glBufferData(GL_UNIFORM_BUFFER, block_size, NULL, GL_DYNAMIC_DRAW);
            glBindBufferBase(GL_UNIFORM_BUFFER, 0, uni->handle);
            // glBindVertexArray(0);

            // Add offsets to vector
            uni->offset.resize(size);
            for (int i = 0; i < size; i++)
            {
                uni->offset[i] = offsets[i];
            }

            const GLenum err = glGetError();
            if (GL_NO_ERROR != err)
            {
                // LOG_ERROR("OpenGL Error");
                switch (err)
                {
                case GL_NO_ERROR:          LOG_ERROR("No error");
                case GL_INVALID_ENUM:      LOG_ERROR("Invalid enum");
                case GL_INVALID_VALUE:     LOG_ERROR("Invalid value");
                case GL_INVALID_OPERATION: LOG_ERROR("Invalid operation");
                // case GL_STACK_OVERFLOW:    return "Stack overflow";
                // case GL_STACK_UNDERFLOW:   return "Stack underflow";
                case GL_OUT_OF_MEMORY:     LOG_ERROR("Out of memory");
                default:                   LOG_ERROR("Unknown error");
                }
            }

            // Deallocate memory
            for (auto i : uniformNames)
            {
                delete[] i;
            }

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
        int count = 0;
        for (auto item: mat_res->uniforms)
        {
            switch (item.second.type)
            {
                case (Flux::Renderer::UniformType::Int):
                {
                    glBufferSubData(GL_UNIFORM_BUFFER, uni->offset[count-tex_id], sizeof(int), item.second.value);
                    index += sizeof(int);
                    LOG_ASSERT_MESSAGE(glGetError() == GL_INVALID_VALUE, "Something broke");
                    // memcpy(block_buffer + uni->offset[index], item.second.value, sizeof(int));
                    break;
                }
                case (Flux::Renderer::UniformType::Float):
                {
                    glBufferSubData(GL_UNIFORM_BUFFER, uni->offset[count-tex_id], sizeof(float), item.second.value);
                    index += sizeof(float);
                    LOG_ASSERT_MESSAGE_FATAL(glGetError() == GL_INVALID_VALUE, "Something broke");
                    // memcpy(block_buffer + uni->offset[index], item.second.value, sizeof(float));
                    break;
                }
                case (Flux::Renderer::UniformType::Bool):
                {
                    int res = ((bool*)item.second.value ? 1: 0);
                    glBufferSubData(GL_UNIFORM_BUFFER, uni->offset[count-tex_id], sizeof(int), &res);
                    index += sizeof(int);
                    LOG_ASSERT_MESSAGE(glGetError() == GL_INVALID_VALUE, "Something broke");
                    // memcpy(block_buffer + uni->offset[index], &res, sizeof(int));
                    break;
                }
                case (Flux::Renderer::UniformType::Vector2):
                {
                    // I'm hoping glm's vectors are just a struct of floats...
                    // memcpy(block_buffer + uni->offset[index], item.second.value, sizeof(float) * 2);
                    glBufferSubData(GL_UNIFORM_BUFFER, uni->offset[count-tex_id], sizeof(float) * 2, glm::value_ptr(*(glm::vec2*)item.second.value));
                    index += sizeof(float)*2;
                    LOG_ASSERT_MESSAGE(glGetError() == GL_INVALID_VALUE, "Something broke");
                    break;
                }
                case (Flux::Renderer::UniformType::Vector3):
                {
                    // I'm hoping glm's vectors are just a struct of floats...
                    // memcpy(block_buffer + uni->offset[index], item.second.value, sizeof(float) * 3);
                    glBufferSubData(GL_UNIFORM_BUFFER, uni->offset[count-tex_id], sizeof(float) * 3, glm::value_ptr(*(glm::vec3*)item.second.value));
                    index += sizeof(float)*4;
                    // index += sizeof(float)*3;
                    LOG_ASSERT_MESSAGE(glGetError() == GL_INVALID_VALUE, "Something broke");
                    break;
                }
                case (Flux::Renderer::UniformType::Vector4):
                {
                    // I'm hoping glm's vectors are just a struct of floats...
                    // memcpy(block_buffer + uni->offset[index], item.second.value, sizeof(float) * 4);
                    glBufferSubData(GL_UNIFORM_BUFFER, uni->offset[count-tex_id], sizeof(float) * 4, glm::value_ptr(*(glm::vec4*)item.second.value));
                    index += sizeof(float)*4;
                    LOG_ASSERT_MESSAGE(glGetError() == GL_INVALID_VALUE, "Something broke");
                    break;
                }
                case (Flux::Renderer::UniformType::Mat4):
                {
                    // memcpy(block_buffer + uni->offset[index], glm::value_ptr(*(glm::mat4*)item.second.value), sizeof(float) * 16);
                    glBufferSubData(GL_UNIFORM_BUFFER, uni->offset[count-tex_id], sizeof(float) * 16, item.second.value);
                    index += sizeof(float)*16;
                    LOG_ASSERT_MESSAGE(glGetError() == GL_INVALID_VALUE, "Something broke");
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
                    LOG_ASSERT_MESSAGE(glGetError() == GL_INVALID_OPERATION, "Could not get texture's uniform location");

                    break;
                }
            }
            count ++;
        }

        glBindBuffer(GL_UNIFORM_BUFFER, 0);

        std::cout << uni->textures.size() << "\n";

        const GLenum err = glGetError();
        if (GL_NO_ERROR != err)
        {
            switch (err)
            {
            case GL_NO_ERROR:          LOG_ERROR("No error"); break;
            case GL_INVALID_ENUM:      LOG_ERROR("Invalid enum"); break;
            case GL_INVALID_VALUE:     LOG_ERROR("Invalid value"); break;
            case GL_INVALID_OPERATION: LOG_ERROR("Invalid operation"); break;
            // case GL_STACK_OVERFLOW:    return "Stack overflow";
            // case GL_STACK_UNDERFLOW:   return "Stack underflow";
            case GL_OUT_OF_MEMORY:     LOG_ERROR("Out of memory"); break;
            default:                   LOG_ERROR("Unknown error"); break;
            }
        }
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
                default: LOG_WARN("Too many textures!"); tx = 0; break;
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
    auto mat_res_en = mesh->mat_resource.getBaseEntity();
    auto mat_res = mesh->mat_resource.getPtr();
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

        shader_com->mvp_location = glGetUniformLocation(shader_com->shader_program, "model_view_projection");
        shader_com->mv_location = glGetUniformLocation(shader_com->shader_program, "model_view");
        shader_com->m_location = glGetUniformLocation(shader_com->shader_program, "model");
        shader_com->cam_pos_location = glGetUniformLocation(shader_com->shader_program, "cam_pos");
        shader_com->light_indexes_location = glGetUniformLocation(shader_com->shader_program, "light_indexes");

        // Cleanup
        glDeleteShader(vertex_shader);
        glDeleteShader(fragment_shader);

        // Link lights
        // It should stay linked, since I'm not re-binding that binding point
        auto bs = glGetUniformBlockIndex(shader_com->shader_program, "Lights");
        if (bs != GL_INVALID_INDEX)
        {
            glUniformBlockBinding(shader_com->shader_program, bs, 1);
        }

        // Add to resource entity
        // Flux::addComponent(Flux::Resources::rctx, mat_res->shaders, GLShaderComponentID, shader_com);
        mat_res->shaders.getBaseEntity().addComponent(shader_com);
    }

    // Custom Uniforms
    // This creates a Uniform Buffer if one doesn't already exist
    // Don't know if we need this if statement
    if (!mat_res_en.hasComponent<GLUniformCom>())
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

    // Make sure the lights are in the correct positions
    dealWithLights();
}

void GLRendererSystem::runSystem(Flux::EntityRef entity, float delta)
{
    // if (entity.hasComponent<Flux::Transform::TransformCom>())
    // {
    //     entity.getComponent<Flux::Transform::TransformCom>()->has_changed = false;
    // }

    if (!entity.hasComponent<Flux::Renderer::MeshCom>())
    {
        // Doesn't have a mesh - we don't care
        return;
    }

    Flux::Transform::TransformCom* trans_com = entity.getComponent<Flux::Transform::TransformCom>();

    if (!trans_com->global_visibility)
    {
        // Don't actually render
        // LOG_INFO("Not rendering");
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
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 14, (void*)0);
            glEnableVertexAttribArray(0);

            // Normal
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 14, (void*)(sizeof(float) * 3));
            glEnableVertexAttribArray(1);

            // Texture
            glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 14, (void*)(sizeof(float) * 6));
            glEnableVertexAttribArray(2);

            // Tangents
            glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 14, (void*)(sizeof(float) * 8));
            glEnableVertexAttribArray(3);

            // Bitangents
            glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 14, (void*)(sizeof(float) * 11));
            glEnableVertexAttribArray(4);

            mesh_com->num_vertices = mesh_res->vertices_length;
            mesh_com->num_indices = mesh_res->indices_length;

            // Set draw mode
            if (mesh_res->draw_mode == Renderer::DrawMode::Triangles)
            {
                mesh_com->draw_type = GL_TRIANGLES;
            }
            else
            {
                mesh_com->draw_type = GL_LINES;
            }

            // Add to resource entity
            // Flux::addComponent(Flux::Resources::rctx, mesh->mesh_resource, GLMeshComponentID, mesh_com);
            mesh->mesh_resource.getBaseEntity().addComponent(mesh_com);
        }

        initGLMaterial(mesh);

        entity.addComponent(new GLEntityCom);
    }

    auto mat_res = mesh->mat_resource.getPtr();

    // Actually render
    GLMeshCom* mesh_com = mesh->mesh_resource.getBaseEntity().getComponent<GLMeshCom>();
    GLShaderCom* shader_com = mat_res->shaders.getBaseEntity().getComponent<GLShaderCom>();

    glUseProgram(shader_com->shader_program);

    // int loc = glGetUniformLocation(shader_com->shader_program, "model_view");
    auto mvp = projection * trans_com->model_view;
    glUniformMatrix4fv(shader_com->mvp_location, 1, GL_FALSE, glm::value_ptr(mvp));
    glUniformMatrix4fv(shader_com->mv_location, 1, GL_FALSE, glm::value_ptr(trans_com->model_view));
    glUniformMatrix4fv(shader_com->m_location, 1, GL_FALSE, glm::value_ptr(trans_com->model));
    glUniform3f(shader_com->cam_pos_location, Transform::camera_position.x,
                                                Transform::camera_position.y,
                                                Transform::camera_position.z);

    dealWithUniforms(mesh, mat_res, shader_com);

    // Deal with lights
    if (entity.hasComponent<Renderer::LightInfoCom>())
    {
        auto lic = entity.getComponent<Renderer::LightInfoCom>();
        glUniform1iv(shader_com->light_indexes_location, 8, lic->effected_lights);
    }

    glEnable(GL_DEPTH_TEST);
    glBindVertexArray(mesh_com->VAO);
    glDrawElements(mesh_com->draw_type, mesh_com->num_indices, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

    // trans_com->has_changed = false;

}

int Flux::GLRenderer::addGLRenderer(ECSCtx* ctx)
{
    // LOG_INFO("Adding GL Renderer system");
    return ctx->addSystemBack(new GLRendererSystem, false);
}
