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

        float nx;
        float ny;
        float nz;

        float tx;
        float ty;
    };

    /**
    Renderer-independant mesh component which stores all the data nessesary to render the defined mesh
    TODO: Make this point to the same instance if there are multiple copies of a mesh
    */
    struct MeshRes: public Resources::Resource
    {
        FLUX_RESOURCE(MeshRes, mesh);

        uint32_t vertices_length;
        /**
        Array of vertices in the mesh. See vertices_len for length
        */
        Vertex* vertices;

        uint32_t indices_length;
        /**
        Array of indices in the mesh. See vertices_len for length
        */
        uint32_t* indices;

        // Functions
        bool serialize(Resources::Serializer* serializer, FluxArc::BinaryFile* output) override
        {
            output->set(vertices_length);
            for (int i = 0; i < vertices_length; i++)
            {
                output->set(vertices[i].x);
                output->set(vertices[i].y);
                output->set(vertices[i].z);

                output->set(vertices[i].nx);
                output->set(vertices[i].ny);
                output->set(vertices[i].nz);

                output->set(vertices[i].tx);
                output->set(vertices[i].ty);
            }

            output->set(indices_length);
            for (int i = 0; i < indices_length; i++)
            {
                output->set(indices[i]);
            }

            return true;
        };

        virtual void deserialize(Resources::Deserializer* deserializer, FluxArc::BinaryFile* file) override
        {
            file->get(&vertices_length);
            vertices = new Vertex[vertices_length];

            for (int i = 0; i < vertices_length; i++)
            {
                file->get(&vertices[i].x);
                file->get(&vertices[i].y);
                file->get(&vertices[i].z);

                file->get(&vertices[i].nx);
                file->get(&vertices[i].ny);
                file->get(&vertices[i].nz);

                file->get(&vertices[i].tx);
                file->get(&vertices[i].ty);
            }

            file->get(&indices_length);
            indices = new uint32_t[indices_length];

            for (int i = 0; i < indices_length; i++)
            {
                file->get(&indices[i]);
            }
        };
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

    struct ShaderRes: public Resources::Resource
    {
        FLUX_RESOURCE(ShaderRes, shader);

        std::string vert_src;
        std::string frag_src;

        // TODO: Load this a much, much, much better way
        bool serialize(Resources::Serializer* serializer, FluxArc::BinaryFile* output) override
        {
            output->set(vert_src);
            output->set(frag_src);

            return true;
        };

        virtual void deserialize(Resources::Deserializer* deserializer, FluxArc::BinaryFile* file) override
        {
            vert_src = file->get();
            frag_src = file->get();
        };
    };

    /**
    Material Resource
    */
    struct MaterialRes: public Resources::Resource
    {
        FLUX_RESOURCE(MaterialRes, material);

        Resources::ResourceRef<ShaderRes> shaders;
        std::map<std::string, Uniform> uniforms;
        bool changed;

        bool serialize(Resources::Serializer* serializer, FluxArc::BinaryFile* output) override
        {
            output->set(serializer->addResource(Resources::ResourceRef<Resources::Resource>(shaders.getBaseEntity())));
            output->set((uint32_t)uniforms.size());
            changed = true;

            for (auto i : uniforms)
            {
                output->set(i.first);
                output->set(i.second.type);
                switch (i.second.type)
                {
                case (Int):
                    output->set(*((int32_t*)i.second.value));
                    break;
                
                case (Float):
                    output->set(*((float*)i.second.value));
                    break;

                case (Bool):
                    output->set(*((bool*)i.second.value));
                    break;
                
                case (Vector2):
                    output->set(((glm::vec2*)i.second.value)->x);
                    output->set(((glm::vec2*)i.second.value)->y);
                    break;
                
                case (Vector3):
                    output->set(((glm::vec3*)i.second.value)->x);
                    output->set(((glm::vec3*)i.second.value)->y);
                    output->set(((glm::vec3*)i.second.value)->z);
                    break;

                case (Vector4):
                    output->set(((glm::vec4*)i.second.value)->x);
                    output->set(((glm::vec4*)i.second.value)->y);
                    output->set(((glm::vec4*)i.second.value)->z);
                    output->set(((glm::vec4*)i.second.value)->w);
                    break;

                case (Mat4):
                    // Oh Boy...
                    output->set((*((glm::mat4*)i.second.value))[0][0]);
                    output->set((*((glm::mat4*)i.second.value))[0][1]);
                    output->set((*((glm::mat4*)i.second.value))[0][2]);
                    output->set((*((glm::mat4*)i.second.value))[0][3]);

                    output->set((*((glm::mat4*)i.second.value))[1][0]);
                    output->set((*((glm::mat4*)i.second.value))[1][1]);
                    output->set((*((glm::mat4*)i.second.value))[1][2]);
                    output->set((*((glm::mat4*)i.second.value))[1][3]);

                    output->set((*((glm::mat4*)i.second.value))[2][0]);
                    output->set((*((glm::mat4*)i.second.value))[2][1]);
                    output->set((*((glm::mat4*)i.second.value))[2][2]);
                    output->set((*((glm::mat4*)i.second.value))[2][3]);

                    output->set((*((glm::mat4*)i.second.value))[3][0]);
                    output->set((*((glm::mat4*)i.second.value))[3][1]);
                    output->set((*((glm::mat4*)i.second.value))[3][2]);
                    output->set((*((glm::mat4*)i.second.value))[3][3]);
                    break;
                }
            }

            return true;
        };

        virtual void deserialize(Resources::Deserializer* deserializer, FluxArc::BinaryFile* file) override
        {
            uint32_t res;
            file->get(&res);
            shaders = deserializer->getResource(res);

            uint32_t size;
            file->get(&size);

            for (int i = 0; i < size; i++)
            {
                std::string name = file->get();
                UniformType type;
                file->get(&type);

                // Variables
                int32_t* ivalue;
                float* fvalue;
                bool* bvalue;
                glm::vec2* v2value;
                glm::vec3* v3value;
                glm::vec4* v4value;
                glm::mat4* mvalue;

                switch (type) {
                case (Int):
                    ivalue = new int32_t;
                    file->get(ivalue);
                    uniforms[name] = Uniform {0, type, ivalue};
                    break;
                
                case (Float):
                    fvalue = new float;
                    file->get(fvalue);
                    uniforms[name] = Uniform {0, type, fvalue};
                    break;

                case (Bool):
                    bvalue = new bool;
                    file->get(bvalue);
                    uniforms[name] = Uniform {0, type, bvalue};
                    break;
                
                case (Vector2):
                    v2value = new glm::vec2;
                    file->get(&v2value->x);
                    file->get(&v2value->y);
                    uniforms[name] = Uniform {0, type, v2value};
                    break;
                
                case (Vector3):
                    v3value = new glm::vec3;
                    file->get(&v3value->x);
                    file->get(&v3value->y);
                    file->get(&v3value->z);
                    uniforms[name] = Uniform {0, type, v3value};
                    break;

                case (Vector4):
                    v4value = new glm::vec4;
                    file->get(&v4value->x);
                    file->get(&v4value->y);
                    file->get(&v4value->z);
                    file->get(&v4value->w);
                    uniforms[name] = Uniform {0, type, v4value};
                    break;

                case (Mat4):
                    // Oh Boy...
                    mvalue = new glm::mat4;
                    file->get(&(*(mvalue))[0][0]);
                    file->get(&(*(mvalue))[0][1]);
                    file->get(&(*(mvalue))[0][2]);
                    file->get(&(*(mvalue))[0][3]);

                    file->get(&(*(mvalue))[1][0]);
                    file->get(&(*(mvalue))[1][1]);
                    file->get(&(*(mvalue))[1][2]);
                    file->get(&(*(mvalue))[1][3]);

                    file->get(&(*(mvalue))[2][0]);
                    file->get(&(*(mvalue))[2][1]);
                    file->get(&(*(mvalue))[2][2]);
                    file->get(&(*(mvalue))[2][3]);

                    file->get(&(*(mvalue))[3][0]);
                    file->get(&(*(mvalue))[3][1]);
                    file->get(&(*(mvalue))[3][2]);
                    file->get(&(*(mvalue))[3][3]);
                    uniforms[name] = Uniform {0, type, mvalue};
                    break;
                }
            }

            changed = true;
        };
    };

    /**
    Simple component that holds a MeshRes
    */
    struct MeshCom: Component
    {
        FLUX_COMPONENT(MeshCom, mesh);
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

        bool serialize(Resources::Serializer *serializer, FluxArc::BinaryFile *output) override
        {
            output->set(serializer->addResource(Resources::ResourceRef<Resources::Resource>(mesh_resource.getBaseEntity())));
            output->set(serializer->addResource(Resources::ResourceRef<Resources::Resource>(mat_resource.getBaseEntity())));

            return true;
        }

        void deserialize(Resources::Deserializer *deserializer, FluxArc::BinaryFile *file) override
        {
            uint32_t mesh_res, mat_res;
            file->get(&mesh_res);
            file->get(&mat_res);

            mesh_resource = deserializer->getResource(mesh_res);
            mat_resource = deserializer->getResource(mat_res);
        }
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
        FLUX_COMPONENT(TransformCom, transform);
        glm::mat4 transformation;
        glm::mat4 model_view;

        bool has_parent;
        EntityRef parent;

        bool serialize(Resources::Serializer *serializer, FluxArc::BinaryFile *output) override
        {
            // TODO: Do this a better way
            // Transformation
            output->set(transformation[0][0]);
            output->set(transformation[0][1]);
            output->set(transformation[0][2]);
            output->set(transformation[0][3]);

            output->set(transformation[1][0]);
            output->set(transformation[1][1]);
            output->set(transformation[1][2]);
            output->set(transformation[1][3]);

            output->set(transformation[2][0]);
            output->set(transformation[2][1]);
            output->set(transformation[2][2]);
            output->set(transformation[2][3]);

            output->set(transformation[3][0]);
            output->set(transformation[3][1]);
            output->set(transformation[3][2]);
            output->set(transformation[3][3]);

            // Model View
            output->set(model_view[0][0]);
            output->set(model_view[0][1]);
            output->set(model_view[0][2]);
            output->set(model_view[0][3]);

            output->set(model_view[1][0]);
            output->set(model_view[1][1]);
            output->set(model_view[1][2]);
            output->set(model_view[1][3]);

            output->set(model_view[2][0]);
            output->set(model_view[2][1]);
            output->set(model_view[2][2]);
            output->set(model_view[2][3]);

            output->set(model_view[3][0]);
            output->set(model_view[3][1]);
            output->set(model_view[3][2]);
            output->set(model_view[3][3]);

            // Parent
            output->set(has_parent);
            if (has_parent)
            {
                output->set(serializer->addEntity(parent));
            }

            return true;
        }

        void deserialize(Resources::Deserializer *derializer, FluxArc::BinaryFile *output) override
        {
            // TODO: Do this a better way
            // Transformation
            output->get(&transformation[0][0]);
            output->get(&transformation[0][1]);
            output->get(&transformation[0][2]);
            output->get(&transformation[0][3]);

            output->get(&transformation[1][0]);
            output->get(&transformation[1][1]);
            output->get(&transformation[1][2]);
            output->get(&transformation[1][3]);

            output->get(&transformation[2][0]);
            output->get(&transformation[2][1]);
            output->get(&transformation[2][2]);
            output->get(&transformation[2][3]);

            output->get(&transformation[3][0]);
            output->get(&transformation[3][1]);
            output->get(&transformation[3][2]);
            output->get(&transformation[3][3]);

            // Model view
            output->get(&model_view[0][0]);
            output->get(&model_view[0][1]);
            output->get(&model_view[0][2]);
            output->get(&model_view[0][3]);

            output->get(&model_view[1][0]);
            output->get(&model_view[1][1]);
            output->get(&model_view[1][2]);
            output->get(&model_view[1][3]);

            output->get(&model_view[2][0]);
            output->get(&model_view[2][1]);
            output->get(&model_view[2][2]);
            output->get(&model_view[2][3]);

            output->get(&model_view[3][0]);
            output->get(&model_view[3][1]);
            output->get(&model_view[3][2]);
            output->get(&model_view[3][3]);

            // Parent
            output->get(&has_parent);
            if (has_parent)
            {
                uint32_t pnum;
                output->get(&pnum);

                parent = derializer->getEntity(pnum);
            }
        }
    };

    /**
    Camera component
    */
    struct CameraCom: Component
    {
        FLUX_COMPONENT(CameraCom, camera);
        glm::mat4 view_matrix;

        bool serialize(Resources::Serializer *serializer, FluxArc::BinaryFile *output) override
        {
            output->set(view_matrix[0][0]);
            output->set(view_matrix[0][1]);
            output->set(view_matrix[0][2]);
            output->set(view_matrix[0][3]);

            output->set(view_matrix[1][0]);
            output->set(view_matrix[1][1]);
            output->set(view_matrix[1][2]);
            output->set(view_matrix[1][3]);

            output->set(view_matrix[2][0]);
            output->set(view_matrix[2][1]);
            output->set(view_matrix[2][2]);
            output->set(view_matrix[2][3]);

            output->set(view_matrix[3][0]);
            output->set(view_matrix[3][1]);
            output->set(view_matrix[3][2]);
            output->set(view_matrix[3][3]);

            return true;
        }

        void deserialize(Resources::Deserializer *deserializer, FluxArc::BinaryFile *file) override
        {
            file->get(&view_matrix[0][0]);
            file->get(&view_matrix[0][1]);
            file->get(&view_matrix[0][2]);
            file->get(&view_matrix[0][3]);

            file->get(&view_matrix[1][0]);
            file->get(&view_matrix[1][1]);
            file->get(&view_matrix[1][2]);
            file->get(&view_matrix[1][3]);

            file->get(&view_matrix[2][0]);
            file->get(&view_matrix[2][1]);
            file->get(&view_matrix[2][2]);
            file->get(&view_matrix[2][3]);

            file->get(&view_matrix[3][0]);
            file->get(&view_matrix[3][1]);
            file->get(&view_matrix[3][2]);
            file->get(&view_matrix[3][3]);
        }
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

// FLUX_DEFINE_COMPONENT(Flux::Transform::TransformCom, transform);
// FLUX_DEFINE_COMPONENT(Flux::Transform::CameraCom, camera);
// FLUX_DEFINE_COMPONENT(Flux::Renderer::MeshCom, mesh);

#endif