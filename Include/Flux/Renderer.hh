#ifndef FLUX_RENDERER_HH
#define FLUX_RENDERER_HH

#include "Flux/ECS.hh"
#include "Flux/Resources.hh"

// STL
// #include <bits/stdint-uintn.h>
#include <map>

// GLM
#include <glm/glm.hpp>
#include "glm/fwd.hpp"


#ifndef FLUX_MAX_CHILDREN
#define FLUX_MAX_CHILDREN 32
#endif


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

    enum UniformType
    {
        Int, Float, Vector2, Vector3, Vector4, Mat4, Bool
    };

    struct Uniform
    {
        int location;
        UniformType type;
        void* value;
    };

    struct ShaderRes: Resources::Resource
    {
        std::string vert_src;
        std::string frag_src;
    };

    /**
    Material Resource
    */
    struct MaterialRes: Resources::Resource
    {
        Resources::ResourceRef<ShaderRes> shaders;
        std::map<std::string, Uniform> uniforms;
        bool changed;
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
        Resources::ResourceRef<MeshRes> mesh_resource;

        /**
        ID of the shader resource.
        **NOTE:** If this is _not_ a shader resource, the program will crash horrible
        */
        // Resources::ResourceID shader_resource;

        /**
        ID of the material resource.
        **NOTE:** If this is _not_ a material resource, the program will crash horrible
        */
        Resources::ResourceRef<MaterialRes> mat_resource;
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
    Set a uniform in a uniform Resource
    */
    void setUniform(Resources::ResourceRef<MaterialRes> res, const std::string& name, const glm::vec2& v);
    void setUniform(Resources::ResourceRef<MaterialRes> res, const std::string& name, const glm::vec3& v);
    void setUniform(Resources::ResourceRef<MaterialRes> res, const std::string& name, const glm::vec4& v);
    void setUniform(Resources::ResourceRef<MaterialRes> res, const std::string& name, const glm::mat4& v);
    void setUniform(Resources::ResourceRef<MaterialRes> res, const std::string& name, const int& v);
    void setUniform(Resources::ResourceRef<MaterialRes> res, const std::string& name, const float& v);
    void setUniform(Resources::ResourceRef<MaterialRes> res, const std::string& name, const bool& v);

    /**
    Links a mesh to an entity. Takes a MeshRes, a ShaderRes, and a MaterialRes.
    Also adds a transformation component
    */
    void addMesh(EntityRef entity, Resources::ResourceRef<MeshRes> mesh, Resources::ResourceRef<MaterialRes> material);

    /**
    Creates a shader resource from 2 text files.
    TODO: Use proper file loading system
    */
    Resources::ResourceRef<ShaderRes> createShaderResource(const std::string& vert, const std::string& frag);

    /**
    Adds a material resource. This takes shaders
    */
    Resources::ResourceRef<MaterialRes> createMaterialResource(Resources::ResourceRef<ShaderRes> shader_resource);

    /**
    Temporary function for adding the temporary shader component
    */
    // void temp_addShaders(ECSCtx* ctx, EntityID entity, const std::string& vert_fname, const std::string& frag_fname);

    /**
    Returns the time since the window was created, in seconds
    */
    double getTime();

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

        bool has_parent;
        EntityRef parent;
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
    void setCamera(EntityRef entity);

    /**
    Rotates an entity with the transform component
    */
    void rotate(EntityRef entity, const glm::vec3& axis, const float& angle_rad);

    /**
    Move (or translates) an entity by offset
    */
    void translate(EntityRef entity, const glm::vec3& offset);

    /**
    Scales an entity by the given scalar
    */
    void scale(EntityRef entity, const glm::vec3& scalar);

    // Euler Transformations
    // ========================================

    /**
    Get the current translation (in local space if parented)
    */
    glm::vec3 getTranslation(EntityRef entity);

    /**
    Set the current translation (in local space if parented)
    */
    void setTranslation(EntityRef entity, const glm::vec3& translation);

    // TODO: Setters for translation and scale

    /**
    Add the camera and transformation systems to the given ECS
    */
    void addTransformSystems(ECSCtx* ctx);

    class TransformationSystem: public System
    {
        void runSystem(EntityRef entity, float delta) override;
    };

    class CameraSystem: public System
    {
        void runSystem(EntityRef entity, float delta) override;
    };

    /**
    Add a parent entity to the given entity.
    This means that the given entity's transformation is not in it's parent's local space.
    **WARNING: ** If the parent Entity is deleted, this Entity will **not know**, and will probably segfault
    */
    void setParent(EntityRef entity, EntityRef parent);

    /**
    If the given Entity has a parent, remove it
    */
    void removeParent(EntityRef entity);

}
}

FLUX_DEFINE_COMPONENT(Flux::Transform::TransformCom, transform);
FLUX_DEFINE_COMPONENT(Flux::Transform::CameraCom, camera);
FLUX_DEFINE_COMPONENT(Flux::Renderer::MeshCom, mesh);

#endif