#include "Flux/Debug.hh"
#include "Flux/ECS.hh"

// TODO: Common renderer interface
#include "Flux/OpenGL/GLRenderer.hh"
#include "Flux/Renderer.hh"
#include <cmath>
#include <cstring>
#include <math.h>

static bool enabled = false;
static Flux::ECSCtx* ctx = nullptr;
static Flux::EntityRef solids[9];
static bool solids_changed[9];
static Flux::EntityRef wireframes[9];
static bool wireframes_changed[9];

void Flux::Debug::enableDebugDraw(ECSCtx* ct)
{
    ctx = ct;
    enabled = true;
    Flux::GLRenderer::addGLRenderer(ctx);
    Flux::Transform::addTransformSystems(ctx);

    // Add one entity per color, per solidity
    glm::vec3 colors[9] = {glm::vec3(1, 0, 0), glm::vec3(0, 1, 0), glm::vec3(0, 0, 1),
                    glm::vec3(1, 0.5, 3), glm::vec3(1, 0.75, 0.79), glm::vec3(0.5, 0, 0.5),
                    glm::vec3(0, 0, 0), glm::vec3(1, 1, 1), glm::vec3(0.58, 0.29, 0)};

    // TODO: Not static paths
    auto shaders = Flux::Renderer::createShaderResource("shaders/basic.vert", "shaders/basic.frag");
    
    for (int i = 0; i < 9; i++)
    {
        // Create material
        auto mat = Flux::Renderer::createMaterialResource(shaders);
        Renderer::setUniform(mat, "color", colors[i]);

        // Create solid entity
        auto solid = ctx->createEntity();
        auto wireframe = ctx->createEntity();

        auto solid_meshres = Flux::Resources::createResource(new Renderer::MeshRes);
        solid_meshres->draw_mode = Renderer::DrawMode::Triangles;
        solid_meshres->indices = nullptr;
        solid_meshres->vertices = nullptr;
        solid_meshres->indices_length = 0;
        solid_meshres->vertices_length = 0;
        
        Flux::Renderer::addMesh(solid, solid_meshres, mat);

        auto wireframe_meshres = Flux::Resources::createResource(new Renderer::MeshRes);
        wireframe_meshres->draw_mode = Renderer::DrawMode::Lines;
        wireframe_meshres->indices = nullptr;
        wireframe_meshres->vertices = nullptr;
        wireframe_meshres->indices_length = 0;
        wireframe_meshres->vertices_length = 0;

        Flux::Renderer::addMesh(wireframe, wireframe_meshres, mat);

        solids[i] = solid;
        wireframes[i] = wireframe;

        solids_changed[i] = true;
        wireframes_changed[i] = true;
    }
}

void Flux::Debug::run(float delta)
{
    if (!enabled) return;

    // ctx->runSystems(delta);

    // Delete all the mesh data
    for (int i = 0; i < 9; i++)
    {
        if (solids_changed[i])
        {
            delete[] solids[i].getComponent<Renderer::MeshCom>()->mesh_resource->vertices;
            delete[] solids[i].getComponent<Renderer::MeshCom>()->mesh_resource->indices;
            solids[i].getComponent<Renderer::MeshCom>()->mesh_resource->vertices_length = 0;
            solids[i].getComponent<Renderer::MeshCom>()->mesh_resource->vertices = nullptr;
            solids[i].getComponent<Renderer::MeshCom>()->mesh_resource->indices_length = 0;
            solids[i].getComponent<Renderer::MeshCom>()->mesh_resource->indices = nullptr;
            solids_changed[i] = false;
        }

        if (wireframes_changed[i])
        {
            delete[] wireframes[i].getComponent<Renderer::MeshCom>()->mesh_resource->vertices;
            delete[] wireframes[i].getComponent<Renderer::MeshCom>()->mesh_resource->indices;
            wireframes[i].getComponent<Renderer::MeshCom>()->mesh_resource->vertices_length = 0;
            wireframes[i].getComponent<Renderer::MeshCom>()->mesh_resource->vertices = nullptr;
            wireframes[i].getComponent<Renderer::MeshCom>()->mesh_resource->indices_length = 0;
            wireframes[i].getComponent<Renderer::MeshCom>()->mesh_resource->indices = nullptr;
            wireframes_changed[i] = false;
        }
    }
}

void Flux::Debug::drawMesh(std::vector<Renderer::Vertex> vertices, std::vector<uint32_t> indices, Colors color, bool wireframe)
{
    if (wireframe)
    {
        // We're about to shake up all the indices
        std::vector<uint32_t> new_indices;
        new_indices.resize(indices.size() * 2);

        for (int i = 0, ni = 0; i < indices.size(); i += 3, ni += 6)
        {
            new_indices[ni] = indices[i];
            new_indices[ni+1] = indices[i+1];
            new_indices[ni+2] = indices[i+1];
            new_indices[ni+3] = indices[i+2];
            new_indices[ni+4] = indices[i+2];
            new_indices[ni+5] = indices[i+3];
        }

        indices = new_indices;
    }
    
    Flux::Renderer::MeshRes* mesh_res;

    if (wireframe)
    {
        mesh_res = wireframes[color].getComponent<Renderer::MeshCom>()->mesh_resource.getPtr();
    }
    else
    {
        mesh_res = solids[color].getComponent<Renderer::MeshCom>()->mesh_resource.getPtr();
    }

    // Update the indices' values
    auto old_vlength = mesh_res->vertices_length;
    auto old_ilength = mesh_res->indices_length;

    for (auto i : indices)
    {
        i += old_vlength;
    }

    // Add to the arrays
    mesh_res->vertices_length += vertices.size();
    auto new_vertices = new Renderer::Vertex[mesh_res->vertices_length];

    mesh_res->indices_length += indices.size();
    auto new_indices = new uint32_t[mesh_res->indices_length];

    // Copy over the memories
    std::memcpy(new_vertices, mesh_res->vertices, sizeof(Renderer::Vertex) * old_vlength);
    std::memcpy(new_vertices + old_vlength, &vertices[0], sizeof(Renderer::Vertex) * vertices.size());

    std::memcpy(new_indices, mesh_res->indices, sizeof(uint32_t) * old_ilength);
    std::memcpy(new_indices + old_ilength, &indices[0], sizeof(uint32_t) * indices.size());

    // Free the old one
    delete[] mesh_res->vertices;
    delete[] mesh_res->indices;

    // Replace it
    mesh_res->vertices = new_vertices;
    mesh_res->indices = new_indices;

    // TODO: Make a better way of doing this
    if (wireframe)
    {
        wireframes[color].removeComponent<GLRenderer::GLEntityCom>();
        wireframes[color].getComponent<Renderer::MeshCom>()->mesh_resource.getBaseEntity().removeComponent<GLRenderer::GLMeshCom>();
        wireframes_changed[color] = true;
    }
    else
    {
        solids[color].removeComponent<GLRenderer::GLEntityCom>();
        solids[color].getComponent<Renderer::MeshCom>()->mesh_resource.getBaseEntity().removeComponent<GLRenderer::GLMeshCom>();
        solids_changed[color] = true;
    }
}

void Flux::Debug::drawLine(glm::vec3 start, glm::vec3 end, Colors color)
{
    std::vector<Renderer::Vertex> v {Renderer::Vertex {start.x, start.y, start.z}, Renderer::Vertex {end.x, end.y, end.z, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                                     Renderer::Vertex {end.x, end.y, end.z, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}};

    std::vector<uint32_t> i {0, 1, 2};

    drawMesh(v, i, color, true);
}

void Flux::Debug::drawPoint(const glm::vec3 &pos, float radius, Colors color, bool wireframe)
{
    std::vector<Renderer::Vertex> v;
    v.resize(12);

    // Copied from https://andrewnoske.com/wiki/Generating_a_sphere_as_a_3D_mesh
    v[0].x = 0.000;    v[0].y = 1.000;    v[0].z = 0.000;    // Top-most point.
    v[1].x = 0.894;    v[1].y =  0.447;   v[1].z = 0.000;
    v[2].x = 0.276;    v[2].y =  0.447;  v[2].z = 0.851;
    v[3].x = -0.724;  v[3].y =  0.447;  v[3].z = 0.526;
    v[4].x = -0.724;  v[4].y =  0.447;  v[4].z = -0.526;
    v[5].x = 0.276;    v[5].y =  0.447;  v[5].z = -0.851;
    v[6].x = 0.724;    v[6].y = -0.447;  v[6].z = 0.526;
    v[7].x = -0.276;  v[7].y = -0.447;  v[7].z = 0.851;
    v[8].x = -0.894;  v[8].y = -0.447;  v[8].z = 0.000;
    v[9].x = -0.276;  v[9].y = -0.447;  v[9].z = -0.851;
    v[10].x= 0.724;    v[10].y= -0.447;  v[10].z= -0.526;
    v[11].x= 0.000;    v[11].y= -1.000;  v[11].z= 0.000;    // Bottom-most point.

    for (int i = 0; i < v.size(); i++)
    {
        v[i].x = v[i].x * radius + pos.x;
        v[i].y = v[i].y * radius + pos.y;
        v[i].z = v[i].z * radius + pos.z;
    }

    // Indices
    std::vector<uint32_t> indices;
    indices.resize(20 * 3);

    indices[0] = 11;
    indices[1] = 9;
    indices[2] = 10;
    indices[3] = 11;
    indices[4] = 8;
    indices[5] = 9;
    indices[6] = 11;
    indices[7] = 7;
    indices[8] = 8;
    indices[9] = 11;
    indices[10] = 6;
    indices[11] = 7;
    indices[12] = 11;
    indices[13] = 10;
    indices[14] = 6;
    indices[15] = 0;
    indices[16] = 5;
    indices[17] = 4;
    indices[18] = 0;
    indices[19] = 4;
    indices[20] = 3;
    indices[21] = 0;
    indices[22] = 3;
    indices[23] = 2;
    indices[24] = 0;
    indices[25] = 2;
    indices[26] = 1;
    indices[27] = 0;
    indices[28] = 1;
    indices[29] = 5;
    indices[30] = 10;
    indices[31] = 9;
    indices[32] = 5;
    indices[33] = 9;
    indices[34] = 8;
    indices[35] = 4;
    indices[36] = 8;
    indices[37] = 7;
    indices[38] = 3;
    indices[39] = 7;
    indices[40] = 6;
    indices[41] = 2;
    indices[42] = 6;
    indices[43] = 10;
    indices[44] = 1;
    indices[45] = 5;
    indices[46] = 9;
    indices[47] = 4;
    indices[48] = 4;
    indices[49] = 8;
    indices[50] = 3;
    indices[51] = 3;
    indices[52] = 7;
    indices[53] = 2;
    indices[54] = 2;
    indices[55] = 6;
    indices[56] = 1;
    indices[57] = 1;
    indices[58] = 10;
    indices[59] = 5;

    drawMesh(v, indices, color, wireframe);
}