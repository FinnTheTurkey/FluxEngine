#include <cstdlib>
#include <cstring>
#include "Flux/Renderer.hh"
#include "Flux/ECS.hh"
#include "Flux/Resources.hh"
#include "glm/fwd.hpp"

#include <glm/glm.hpp>

// STL includes
#include <fstream>

using namespace Flux;

static Flux::ComponentTypeID MeshComponentID = -1;
static Flux::ComponentTypeID TransformComponentID = -1;

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

    if (MeshComponentID == -1)
    {
        // Setup components
        MeshComponentID = Flux::getComponentType("mesh");
        TransformComponentID = Flux::getComponentType("transform");
    }

    Flux::Transform::TransformCom* tc = new Flux::Transform::TransformCom;
    tc->transformation = glm::mat4();
    tc->has_parent = false;

    // Flux::addComponent(ctx, entity, MeshComponentID, mc);
    // Flux::addComponent(ctx, entity, TransformComponentID, tc);
    entity.addComponent(mc);
    entity.addComponent(tc);
}

void Flux::Renderer::setUniform(Resources::ResourceRef<MaterialRes> res, const std::string& name, const glm::vec2& v)
{
    // auto mr = (MaterialRes*)Resources::getResource(res);

    Uniform us;
    us.location = -1;
    us.type = UniformType::Vector2;

    // Allocate enw memory, and copy value into it
    glm::vec2* value = (glm::vec2*)std::malloc(sizeof(v));
    std::memcpy(value, &v, sizeof(v));

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
    glm::vec3* value = (glm::vec3*)std::malloc(sizeof(v));
    std::memcpy(value, &v, sizeof(v));

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
    glm::vec4* value = (glm::vec4*)std::malloc(sizeof(v));
    std::memcpy(value, &v, sizeof(v));

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
    float* value = (float*)std::malloc(sizeof(v));
    std::memcpy(value, &v, sizeof(v));

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
    int* value = (int*)std::malloc(sizeof(v));
    std::memcpy(value, &v, sizeof(v));

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
    bool* value = (bool*)std::malloc(sizeof(v));
    std::memcpy(value, &v, sizeof(v));

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
    glm::mat4* value = (glm::mat4*)std::malloc(sizeof(v));
    std::memcpy(value, &v, sizeof(v));

    us.value = value;

    res->uniforms[name] = us;
    res->changed = true;
}