#include "Flux/Renderer.hh"
#include "Flux/ECS.hh"
#include "Flux/Resources.hh"

// STL includes
#include <fstream>

using namespace Flux;

static Flux::ComponentTypeID MeshComponentID = -1;

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

void Flux::Renderer::temp_addShaders(ECSCtx *ctx, EntityID entity, const std::string &vert_fname, const std::string &frag_fname)
{
    auto sc = new temp_ShaderCom;

    sc->frag_src = temp_readFile(frag_fname);
    sc->vert_src = temp_readFile(vert_fname);

    // This is extremely bad practice
    // If this were not a temp function, I would not do this
    Flux::addComponent(ctx, entity, getComponentType("temp_shader"), sc);
}

Resources::ResourceID Flux::Renderer::createShaderResource(const std::string &vert, const std::string &frag)
{
    // TODO: Change later
    auto sr = new ShaderRes;

    sr->frag_src = temp_readFile(frag);
    sr->vert_src = temp_readFile(vert);

    return Resources::createResource(sr);
}

void Flux::Renderer::addMesh(ECSCtx *ctx, EntityID entity, Resources::ResourceID mesh, Resources::ResourceID shaders)
{
    auto mc = new MeshCom;
    mc->mesh_resource = mesh;
    mc->shader_resource = shaders;

    if (MeshComponentID == -1)
    {
        MeshComponentID = Flux::getComponentType("mesh");
    }

    Flux::addComponent(ctx, entity, MeshComponentID, mc);
}