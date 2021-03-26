#ifndef FLUX_RENDERER_HH
#define FLUX_RENDERER_HH

#include "Flux/ECS.hh"
#include "Flux/Log.hh"
#include "Flux/Resources.hh"

// STL
// #include <bits/stdint-uintn.h>
#include <map>

// GLM
#include <glm/glm.hpp>
#include <set>
#include <vector>
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

        float tanx;
        float tany;
        float tanz;

        float btanx;
        float btany;
        float btanz;
    };

    enum DrawMode
    {
        Triangles, Lines
    };

    /**
    Renderer-independant mesh component which stores all the data nessesary to render the defined mesh
    TODO: Make it deallocate memory after the mesh is on the gpu
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

        /** How to draw the mesh */
        DrawMode draw_mode;

        // Functions
        ~MeshRes()
        {
            delete[] vertices;
            delete[] indices;
        }

        bool serialize(Resources::Serializer* serializer, FluxArc::BinaryFile* output) override
        {
            output->set(vertices_length);

            // Resize the binary file to limit copies
            output->allocate(sizeof(float) * 14 * vertices_length);
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

                output->set(vertices[i].tanx);
                output->set(vertices[i].tany);
                output->set(vertices[i].tanz);

                output->set(vertices[i].btanx);
                output->set(vertices[i].btany);
                output->set(vertices[i].btanz);
            }

            output->set(indices_length);
            output->allocate(sizeof(uint32_t) * indices_length);
            for (int i = 0; i < indices_length; i++)
            {
                output->set(indices[i]);
            }

            output->set((int)draw_mode);

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

                file->get(&vertices[i].tanx);
                file->get(&vertices[i].tany);
                file->get(&vertices[i].tanz);

                file->get(&vertices[i].btanx);
                file->get(&vertices[i].btany);
                file->get(&vertices[i].btanz);
            }

            file->get(&indices_length);
            indices = new uint32_t[indices_length];

            for (int i = 0; i < indices_length; i++)
            {
                file->get(&indices[i]);
            }

            int i;
            file->get(&i);
            draw_mode = (DrawMode)i;
        };
    };

    enum UniformType
    {
        Int, Float, Vector2, Vector3, Vector4, Mat4, Bool, Texture
    };

    struct Uniform
    {
        int location;
        UniformType type;
        void* value;
    };

    struct ShaderRes;

    /**
    Creates a shader resource from 2 text files.
    TODO: Use proper file loading system
    */
    Resources::ResourceRef<ShaderRes> createShaderResource(const std::string& vert, const std::string& frag);

    struct ShaderRes: public Resources::Resource
    {
        FLUX_RESOURCE(ShaderRes, shader);

        std::string vert_src;
        std::string frag_src;

        // Source files
        std::string vert_fname;
        std::string frag_fname;

        bool serialize(Resources::Serializer* serializer, FluxArc::BinaryFile* output) override
        {
            output->set(vert_fname);
            output->set(frag_fname);

            return true;
        };

        void deserialize(Resources::Deserializer* deserializer, FluxArc::BinaryFile* file) override
        {
            vert_fname = file->get();
            frag_fname = file->get();

            // Yes, this is basically the worst way possible to do this
            // _but I don't care_
            auto res = createShaderResource(vert_fname, frag_fname);
            vert_src = res->vert_src;
            frag_src = res->frag_src;

            // Resources::removeResource(res);
        };
    };

    /**
    Texture Resource. Can either reference an internal file, or an external one
    */
    struct TextureRes: public Resources::Resource
    {
        FLUX_RESOURCE(TextureRes, texture);

        std::string filename;
        bool internal = false;

        uint32_t image_data_size;
        unsigned char* image_data;

        uint32_t width;
        uint32_t height;

        /** Once the renderer has processed the texture, the data will be freed */
        bool processed = false;

        bool serialize(Resources::Serializer* serializer, FluxArc::BinaryFile* output) override
        {
            if (processed && internal && image_data == nullptr)
            {
                if (width == 1 && height == 1)
                {
                    // It's a dummy texture
                    output->set(internal);
                    output->set(image_data_size);
                    unsigned char x[4] { 1, 0, 0, 1};
                    output->set((char*)x, image_data_size);

                    output->set(width);
                    output->set(height);
                    return true;
                }

                // TODO: Don't free textures in editor
                LOG_WARN("Texture has already been procesed, and data has been freed. Image will not be serialized");
                return false;
            }

            output->set(internal);

            if (!internal)
            {
                output->set(filename);
            }
            else
            {
                output->set(image_data_size);
                output->set((char*)image_data, image_data_size);
            }

            output->set(width);
            output->set(height);

            return true;
        }

        void deserialize(Resources::Deserializer* deserializer, FluxArc::BinaryFile* file) override
        {
            file->get(&internal);

            if (!internal)
            {
                filename = deserializer->getDirectory() / file->get();

                loadImage(filename);
            }
            else
            {
                file->get(&image_data_size);
                image_data = new unsigned char[image_data_size];
                file->get((char*)image_data, image_data_size);
            }

            file->get(&width);
            file->get(&height);
        }

        void loadImage(const std::string& filename);

        void destroy();

        ~TextureRes()
        {
            destroy();
        }
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

        bool has_texture = false;
        Resources::ResourceRef<TextureRes> diffuse_texture;

        ~MaterialRes()
        {
            // Deallocate uniforms
            for (auto i : uniforms)
            {
                switch (i.second.type)
                {
                    case (UniformType::Bool):
                        delete (bool*)i.second.value;
                        break;
                    
                    case (UniformType::Float):
                        delete (float*)i.second.value;
                        break;

                    case (UniformType::Int):
                        delete (int*)i.second.value;
                        break;

                    case (UniformType::Mat4):
                        delete (glm::mat4*)i.second.value;
                        break;
                    
                    case (UniformType::Vector2):
                        delete (glm::vec2*)i.second.value;
                        break;
                    
                    case (UniformType::Vector3):
                        delete (glm::vec3*)i.second.value;
                        break;

                    case (UniformType::Vector4):
                        delete (glm::vec4*)i.second.value;
                        break;
                    
                    case (UniformType::Texture):
                        // No need
                        break;
                }
            }
        }

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
                
                case (Texture):
                    output->set(serializer->addResource(((Resources::ResourceRef<TextureRes>*)i.second.value)->getBaseEntity()));
                    break;
                }
            }

            // Textures
            // output->set(has_texture);

            // if (has_texture)
            // {
            //     output->set(serializer->addResource(diffuse_texture.getBaseEntity()));
            // }

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

                case (Texture):
                    uint32_t tvalue;
                    file->get(&tvalue);
                    auto tres = new Resources::ResourceRef<TextureRes>(deserializer->getResource(tvalue));
                    uniforms[name] = Uniform {0, type, (void*)tres};
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
    void setUniform(Resources::ResourceRef<MaterialRes> res, const std::string& name, Resources::ResourceRef<TextureRes> v);

    /**
    Links a mesh to an entity. Takes a MeshRes, a ShaderRes, and a MaterialRes.
    Also adds a transformation component
    */
    void addMesh(EntityRef entity, Resources::ResourceRef<MeshRes> mesh, Resources::ResourceRef<MaterialRes> material);

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

    enum LightType
    {
        Point = 0, Directional = 1, Spot = 2
    };

    /**
    Component that defines a light
    */
    struct LightCom: public Component
    {
        FLUX_COMPONENT(LightCom, LightCom);

        LightType type;
        float radius;

        /* In Radians */
        float cutoff;

        glm::vec3 color;
        glm::vec3 direction;

        bool inducted = false;

        bool serialize(Resources::Serializer *serializer, FluxArc::BinaryFile *output) override
        {
            output->set(type);
            output->set(radius);
            output->set(cutoff);

            output->set(color.x);
            output->set(color.y);
            output->set(color.z);

            output->set(direction.x);
            output->set(direction.y);
            output->set(direction.z);

            return true;
        }

        void deserialize(Resources::Deserializer *deserializer, FluxArc::BinaryFile *file) override
        {
            file->get(&type);
            file->get(&radius);
            file->get(&cutoff);

            file->get(&color.x);
            file->get(&color.y);
            file->get(&color.z);

            file->get(&direction.x);
            file->get(&direction.y);
            file->get(&direction.z);

            inducted = false;
        }
    };

    /**
    Runtime component that tells each object which lights are interacting with it
    */
    struct LightInfoCom: public Component
    {
        FLUX_COMPONENT(LightInfoCom, LightInfoCom);

        int effected_lights[8];
    };

    // extern EntityRef lights[128];
    // extern std::vector<int> lights_that_changed;

    class LightSystem: public System
    {
    public:
        EntityRef lights[128];
        std::vector<int> lights_that_changed;
        std::vector<EntityRef> new_lights;

        LightSystem();
        void onSystemStart() override;
        void runSystem(EntityRef entity, float delta) override;
    private:
    };

    /** Turns the given entity into a point light. The entity must have a transformation */
    void addPointLight(EntityRef entity, float radius, const glm::vec3& color);

    /** Turns the given entity into a directional light. The entity must have a transformation */
    void addDirectionalLight(EntityRef entity, const glm::vec3& direction, float radius, glm::vec3 color);

    /** Turns the given entity into a spot light. The entity must have a transformation */
    void addSpotLight(EntityRef entity, float cutoff_radians, float radius, glm::vec3 color);

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
        glm::mat4 model;

        bool has_parent;
        EntityRef parent;

        bool visible;

        bool global_visibility;

        bool has_changed = true;

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

            // Parent
            output->set(has_parent);
            if (has_parent)
            {
                output->set(serializer->addEntity(parent));
            }

            output->set(visible ? 1 : 0);

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
            model_view = glm::mat4();
            model = glm::mat4();

            // Parent
            output->get(&has_parent);
            if (has_parent)
            {
                uint32_t pnum;
                output->get(&pnum);

                parent = derializer->getEntity(pnum);
            }

            int n;
            output->get(&n);

            if (n == 0)
            {
                visible = false;
            }
            else
            {
                visible = true;
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

    /** Get the global transformation matrix for an entity */
    glm::mat4 getParentTransform(EntityRef entity);

    glm::mat4 _getParentTransform(EntityRef entity, bool* has_changed, bool* visibility);

    void setVisible(EntityRef entity, bool vis);

    /**
    Sets whether an entity can be seen.
    If an entity is invisible, all it's descendants are also invisible.
    */
    bool getVisibility(EntityRef entity);

    /**
    Adds a transform component to the given entity, with a pos of 0, 0, 0
    */
    void giveTransform(EntityRef entity);

    /**
    Rotates an entity with the transform component
    */
    void rotate(EntityRef entity, const glm::vec3& axis, const float& angle_rad);

    /**
    Rotates an entity on a global axis
    */
    void rotateGlobalAxis(EntityRef entity, const glm::vec3& axis, const float& angle_rad);

    /**
    Move (or translates) an entity by offset
    */
    void translate(EntityRef entity, const glm::vec3& offset);

    /**
    Move (or translates) an entity by an offset in global space
    */
    void globalTranslate(EntityRef entity, const glm::vec3& offset);

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
    Get the current translation in global space
    */
    glm::vec3 getGlobalTranslation(EntityRef entity);

    /**
    Set the current translation (in local space if parented)
    */
    void setTranslation(EntityRef entity, const glm::vec3& translation);

    /**
    Set the current translation in global space
    */
    void setGlobalTranslation(EntityRef entity, const glm::vec3& translation);

    /**
    Get the current rotation relative to its parent
    */
    glm::vec3 getRotation(EntityRef entity);

    /**
    Get the current rotation in global space
    */
    glm::vec3 getGlobalRotation(EntityRef entity);

    /**
    Sets the current rotation relative to its parent
    */
    void setRotation(EntityRef entity, const glm::vec3& rotation);

    /**
    Sets the current rotation in global space
    */
    void setGlobalRotation(EntityRef entity, const glm::vec3& rotation);

    /**
    Get the current scale relative to its parent
    */
    glm::vec3 getScale(EntityRef entity);

    /**
    Set the current scale relative to its parent
    */
    void setScale(EntityRef entity, const glm::vec3& scale);

    // Systems Transformations
    // ========================================

    /**
    Add the camera and transformation systems to the given ECS
    */
    void addTransformSystems(ECSCtx* ctx);

    class TransformationSystem: public System
    {
        void runSystem(EntityRef entity, float delta) override;
    };

    /** Helper variable for the renderer that says the global position of the camera */
    extern glm::vec3 camera_position;

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

    class EndFrameSystem: public Flux::System
    {
        void runSystem(EntityRef entity, float delta) override
        {
            if (entity.hasComponent<Flux::Transform::TransformCom>())
            {
                entity.getComponent<Flux::Transform::TransformCom>()->has_changed = false;
            }
        }
    };

}
}

// FLUX_DEFINE_COMPONENT(Flux::Transform::TransformCom, transform);
// FLUX_DEFINE_COMPONENT(Flux::Transform::CameraCom, camera);
// FLUX_DEFINE_COMPONENT(Flux::Renderer::MeshCom, mesh);

#endif