#include <cstdlib>
#include <cstring>
#include "Flux/Log.hh"
#include "Flux/Renderer.hh"
#include "Flux/ECS.hh"
#include "Flux/Resources.hh"
#include "glm/fwd.hpp"

#include <glm/glm.hpp>

// STL includes
#include <fstream>

// STB
#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

using namespace Flux;


std::string temp_readFile(const std::string& filename)
{
    std::string line;
    std::string output = "";
    std::ifstream myfile (filename);
    if (myfile.is_open())
    {
        while ( getline (myfile,line) )
        {
            output += line + '\n';
        }
        myfile.close();
    }

    return output;
}

// void Flux::Renderer::temp_addShaders(ECSCtx *ctx, EntityID entity, const std::string &vert_fname, const std::string &frag_fname)
// {
//     auto sc = new temp_ShaderCom;

//     sc->frag_src = temp_readFile(frag_fname);
//     sc->vert_src = temp_readFile(vert_fname);

//     // This is extremely bad practice
//     // If this were not a temp function, I would not do this
//     Flux::addComponent(ctx, entity, getComponentType("temp_shader"), sc);
// }

Resources::ResourceRef<Flux::Renderer::ShaderRes> Flux::Renderer::createShaderResource(const std::string &vert, const std::string &frag)
{
    // TODO: Change later
    auto sr = new ShaderRes;

    sr->frag_src = temp_readFile(frag);
    sr->vert_src = temp_readFile(vert);

    sr->vert_fname = vert;
    sr->frag_fname = frag;

    return Resources::createResource(sr);
}

Resources::ResourceRef<Flux::Renderer::MaterialRes> Renderer::createMaterialResource(Resources::ResourceRef<ShaderRes> shader_resource)
{
    auto mr = new MaterialRes;
    mr->shaders = shader_resource;
    mr->uniforms = std::map<std::string, Uniform>();
    mr->changed = true;

    return Resources::createResource(mr);
}

void Flux::Renderer::addMesh(EntityRef entity, Resources::ResourceRef<MeshRes> mesh, Resources::ResourceRef<MaterialRes> mat)
{
    auto mc = new MeshCom;
    mc->mesh_resource = mesh;
    // mc->shader_resource = shaders;
    mc->mat_resource = mat;

    // Flux::addComponent(ctx, entity, MeshComponentID, mc);
    // Flux::addComponent(ctx, entity, TransformComponentID, tc);
    entity.addComponent(mc);
    // entity.addComponent(tc);

    Transform::giveTransform(entity);
}

void Flux::Renderer::setUniform(Resources::ResourceRef<MaterialRes> res, const std::string& name, const glm::vec2& v)
{
    // auto mr = (MaterialRes*)Resources::getResource(res);

    Uniform us;
    us.location = -1;
    us.type = UniformType::Vector2;

    // Allocate enw memory, and copy value into it
    // glm::vec2* value = (glm::vec2*)std::malloc(sizeof(v));
    // std::memcpy(value, &v, sizeof(v));
    glm::vec2* value = new glm::vec2(v);

    us.value = value;

    res->uniforms[name] = us;
    res->changed = true;
}

void Flux::Renderer::setUniform(Resources::ResourceRef<MaterialRes> res, const std::string& name, const glm::vec3& v)
{
    // auto mr = (MaterialRes*)Resources::getResource(res);

    Uniform us;
    us.location = -1;
    us.type = UniformType::Vector3;

    // Allocate enw memory, and copy value into it
    // glm::vec3* value = (glm::vec3*)std::malloc(sizeof(v));
    // std::memcpy(value, &v, sizeof(v));
    glm::vec3* value = new glm::vec3(v);

    us.value = value;

    res->uniforms[name] = us;
    res->changed = true;
}

void Flux::Renderer::setUniform(Resources::ResourceRef<MaterialRes> res, const std::string& name, const glm::vec4& v)
{
    // auto mr = (MaterialRes*)Resources::getResource(res);

    Uniform us;
    us.location = -1;
    us.type = UniformType::Vector4;

    // Allocate enw memory, and copy value into it
    // glm::vec4* value = (glm::vec4*)std::malloc(sizeof(v));
    // std::memcpy(value, &v, sizeof(v));
    glm::vec4* value = new glm::vec4(v);

    us.value = value;

    res->uniforms[name] = us;
    res->changed = true;
}

void Flux::Renderer::setUniform(Resources::ResourceRef<MaterialRes> res, const std::string& name, const float& v)
{
    // auto mr = (MaterialRes*)Resources::getResource(res);

    Uniform us;
    us.location = -1;
    us.type = UniformType::Float;

    // Allocate enw memory, and copy value into it
    // float* value = (float*)std::malloc(sizeof(v));
    // std::memcpy(value, &v, sizeof(v));
    float* value = new float(v);

    us.value = value;

    res->uniforms[name] = us;
    res->changed = true;
}

void Flux::Renderer::setUniform(Resources::ResourceRef<MaterialRes> res, const std::string& name, const int& v)
{
    // auto mr = (MaterialRes*)Resources::getResource(res);

    Uniform us;
    us.location = -1;
    us.type = UniformType::Int;

    // Allocate enw memory, and copy value into it
    // int* value = (int*)std::malloc(sizeof(v));
    int* value = new int(v);
    // std::memcpy(value, &v, sizeof(v));

    us.value = value;

    res->uniforms[name] = us;
    res->changed = true;
}

void Flux::Renderer::setUniform(Resources::ResourceRef<MaterialRes> res, const std::string& name, const bool& v)
{
    // auto mr = (MaterialRes*)Resources::getResource(res);

    Uniform us;
    us.location = -1;
    us.type = UniformType::Int;

    // Allocate enw memory, and copy value into it
    int32_t* value = new int32_t(v);
    // int realv = v ? 1 : 0;
    // value = realv;

    us.value = value;

    res->uniforms[name] = us;
    res->changed = true;
}

void Flux::Renderer::setUniform(Resources::ResourceRef<MaterialRes> res, const std::string& name, const glm::mat4& v)
{
    // auto mr = (MaterialRes*)Resources::getResource(res);

    Uniform us;
    us.location = -1;
    us.type = UniformType::Mat4;

    // Allocate enw memory, and copy value into it
    glm::mat4* value = new glm::mat4(v);
    // std::memcpy(value, &v, sizeof(v));

    us.value = value;

    res->uniforms[name] = us;
    res->changed = true;
}

void Flux::Renderer::setUniform(Resources::ResourceRef<MaterialRes> res, const std::string& name, Resources::ResourceRef<TextureRes> v)
{
    Uniform us;
    us.location = -1;
    us.type = UniformType::Texture;

    // Create pointer version of ResourceRef
    auto value = new Resources::ResourceRef<TextureRes>(v.getBaseEntity());

    us.value = value;

    res->uniforms[name] = us;
    res->changed = true;
}

// =========================================================
// Lighting
// =========================================================

// EntityRef Flux::Renderer::lights[128];
// static bool setup = false;
// std::vector<int> Flux::Renderer::lights_that_changed;

void Renderer::addLight(EntityRef entity, float radius, glm::vec3 color)
{
    if (!entity.hasComponent<Transform::TransformCom>())
    {
        LOG_WARN("A TransformCom is required to create a light");
        return;
    }

    auto lc = new LightCom;
    lc->color = color;
    lc->radius = radius;
    lc->inducted = false;
    entity.addComponent(lc);
}

Renderer::LightSystem::LightSystem()
{
    // Null all the lights
    for (int i = 0; i < 128; i++)
    {
        lights[i] = EntityRef();
    }

    return;
}

void Renderer::LightSystem::onSystemStart()
{
    // Check all of our lights to see if they've changed
    lights_that_changed.clear();

    // Add it to the lighting setup if it hasn't already been
    for (auto entity : new_lights)
    {
        for (int i = 0; i < 128; i++)
        {
            if (lights[i].getEntityID() == -1)
            {
                lights[i] = entity;

                // Force it to be on the lights_that_changed list
                entity.getComponent<Transform::TransformCom>()->has_changed = true;
                break;
            }
        }

        entity.getComponent<LightCom>()->inducted = true;
    }

    new_lights.clear();

    int c = 0;
    for (auto i : lights)
    {
        if (i.getEntityID() != -1)
        {
            if (i.getComponent<Transform::TransformCom>()->has_changed)
            {
                // Oh well, I guess we're recalculating that light
                lights_that_changed.push_back(c);
            }
        }

        c++;
    }
}

// This must run AFTER transform system
void Renderer::LightSystem::runSystem(EntityRef entity, float delta)
{
    if (entity.hasComponent<LightCom>())
    {
        auto lc = entity.getComponent<LightCom>();
        if (!lc->inducted)
        {
            new_lights.push_back(entity);
        }
    }

    if (!entity.hasComponent<MeshCom>() || !entity.hasComponent<Transform::TransformCom>())
    {
        return;
    }

    auto tc = entity.getComponent<Transform::TransformCom>();
    glm::vec3 position = glm::vec3(tc->model * glm::vec4(0,0,0,1));

    if (!entity.hasComponent<LightInfoCom>())
    {

        // Calculate initial lighting
        int light_count = 0;
        auto lightinfo = new LightInfoCom;
        for (int i = 0; i < 128; i++)
        {
            if (lights[i].getEntityID() != -1)
            {
                auto light_pos = glm::vec3(lights[i].getComponent<Transform::TransformCom>()->model * glm::vec4(0,0,0,1));
                if (glm::distance(light_pos, position) <= lights[i].getComponent<LightCom>()->radius)
                {
                    lightinfo->effected_lights[light_count] = i;
                    light_count ++;
                    if (light_count == 8)
                    {
                        // We've reached max lights
                        break;
                    }
                }
            }
        }

        // Full up the rest of the light com
        for (int i = light_count; i < 8; i++)
        {
            lightinfo->effected_lights[i] = -1;
        }
        
        entity.addComponent(lightinfo);
        // No need to recalculate anything else
        return;
    }

    auto lightinfo = entity.getComponent<LightInfoCom>();

    if (tc->has_changed == true)
    {
        // We have to do a full recalculation, anyways
        // Calculate initial lighting
        int light_count = 0;
        for (int i = 0; i < 128; i++)
        {
            if (lights[i].getEntityID() != -1)
            {
                auto light_pos = glm::vec3(lights[i].getComponent<Transform::TransformCom>()->model * glm::vec4(0,0,0,1));
                if (glm::distance(light_pos, position) <= lights[i].getComponent<LightCom>()->radius)
                {
                    lightinfo->effected_lights[light_count] = i;
                    light_count ++;
                    if (light_count == 8)
                    {
                        // We've reached max lights
                        break;
                    }
                }
            }
        }

        // Full up the rest of the light com
        for (int i = light_count; i < 8; i++)
        {
            lightinfo->effected_lights[i] = -1;
        }
        
        // No need to recalculate anything else
        return;
    }
    else
    {
        // Check if this entity is effected by any of the moved lights
        for (auto l : lights_that_changed)
        {
            bool is_current = false;
            for (int i = 0; i < 8; i ++)
            {
                if (l == lightinfo->effected_lights[i])
                {
                    // Re-calculate that light
                    auto light_pos = glm::vec3(lights[l].getComponent<Transform::TransformCom>()->model * glm::vec4(0,0,0,1));
                    if (glm::distance(light_pos, position) > lights[l].getComponent<LightCom>()->radius)
                    {
                        // Remove that light from the list
                        // Move every light after it forward
                        for (int li = i; i < 7; i++)
                        {
                            lightinfo->effected_lights[li] = lightinfo->effected_lights[li + 1];
                        }
                        lightinfo->effected_lights[7] = -1;
                        is_current = true;
                    }
                }
            }

            if (!is_current)
            {
                auto light_pos = glm::vec3(lights[l].getComponent<Transform::TransformCom>()->model * glm::vec4(0,0,0,1));
                if (glm::distance(light_pos, position) <= lights[l].getComponent<LightCom>()->radius)
                {
                    // Add this light
                    for (int i = 0; i < 8; i++)
                    {
                        if (lightinfo->effected_lights[i] == -1)
                        {
                            // Put it here
                            lightinfo->effected_lights[i] = l;
                            break;
                        }
                    }
                }
            }
        }
    }
}

// =========================================================
// Special functions in resources
// =========================================================
void Flux::Renderer::TextureRes::loadImage(const std::string& filename)
{
    int t_width, t_height, nr_channels;
    stbi_set_flip_vertically_on_load(true);
    image_data = (unsigned char*)stbi_load(filename.c_str(), &t_width, &t_height, &nr_channels, 4);

    if (!image_data)
    {
        LOG_ERROR("Error: Could not load image at " + filename);
    }

    width = t_width;
    height = t_height;
    this->filename = filename;
}

void Flux::Renderer::TextureRes::destroy()
{
    if (!processed)
    {
        if (internal)
        {
            delete[] image_data;
        }
        else
        {
            stbi_image_free(image_data);
        }

        processed = true;
    }
}