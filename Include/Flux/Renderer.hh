#ifndef FLUX_RENDERER_HH
#define FLUX_RENDERER_HH

#include "Flux/ECS.hh"
#include "Flux/Resources.hh"

// STL
#include <bits/stdint-uintn.h>

// GLM
#include <glm/glm.hpp>
#include "glm/fwd.hpp"

namespace Flux { namespace Renderer {

    /**
    Simple struct to represent a vertex in the MeshCom. 
    Why not just use a glm::vec3? Because I can just dump this class into opengl
    */
    struct Vertex
    {
        float x;
        float y;
        float z;
    };

    /**
    Renderer-independant mesh component which stores all the data nessesary to render the defined mesh
    TODO: Make this point to the same instance if there are multiple copies of a mesh
    */
    struct MeshRes: Resources::Resource
    {
        /**
        Array of vertices in the mesh. See vertices_len for length
        */
        Vertex* vertices;
        uint32_t vertices_length;

        /**
        Array of indices in the mesh. See vertices_len for length
        */
        uint32_t* indices;
        uint32_t indices_length;
    };

    /**
    Simple component that holds a MeshRes
    */
    struct MeshCom: Component
    {
        /**
        ID of the mesh resource.
        **NOTE:** If this is _not_ a mesh resource, the program will crash horrible
        */
        Resources::ResourceID mesh_resource;

        /**
        ID of the shader resource.
        **NOTE:** If this is _not_ a shader resource, the program will crash horrible
        */
        Resources::ResourceID shader_resource;
    };

    /**
    Temporary component for shaders until I write the resource system
    */
    struct temp_ShaderCom: Component
    {
        std::string vert_src;
        std::string frag_src;
    };

    /**
    Shader Resource
    */
    struct ShaderRes: Resources::Resource
    {
        std::string vert_src;
        std::string frag_src;
    };


    /**
    Links a mesh to an entity. Takes a MeshRes and a ShaderRes.
    Also adds a transformation component
    */
    void addMesh(ECSCtx* ctx, EntityID entity, Resources::ResourceID mesh, Resources::ResourceID shaders);

    /**
    Creates a shader resource from 2 text files.
    TODO: Use proper file loading system
    */
    Resources::ResourceID createShaderResource(const std::string& vert, const std::string& frag);

    /**
    Temporary function for adding the temporary shader component
    */
    void temp_addShaders(ECSCtx* ctx, EntityID entity, const std::string& vert_fname, const std::string& frag_fname);

}

namespace Transform
{
    /**
    Component that handles all the math involved in the transformation of 3d objects
    */
    struct TransformCom: Component
    {
        glm::mat4 transformation;
        glm::mat4 model_view;
    };

    /**
    Camera component
    */
    struct CameraCom: Component
    {
        glm::mat4 view_matrix;
    };

    /** 
    Sets an entity as a camera. Can be used on any entity with the Transfrom component
    */
    void setCamera(ECSCtx* ctx, EntityID entity);

    /**
    Rotates an entity with the transform component
    */
    void rotate(ECSCtx* ctx, EntityID entity, const glm::vec3& axis, const float& angle_rad);

    /**
    Move (or translates) an entity by offset
    */
    void translate(ECSCtx *ctx, EntityID entity, const glm::vec3& offset);

    /**
    Add the camera and transformation systems to the given ECS
    */
    void addTransformSystems(ECSCtx* ctx);

}
}

#endif