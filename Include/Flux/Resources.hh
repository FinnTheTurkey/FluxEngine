#ifndef FLUX_RESOURCES_HH
#define FLUX_RESOURCES_HH
#include <filesystem>
#include <fstream>
#include <string>

// #ifndef FLUX_MAX_RESOURCES
// #define FLUX_MAX_RESOURCES 4096
// #endif

// Flux includes
#include "Flux/ECS.hh"
#include "Flux/Log.hh"
#include "FluxArc/FluxArc.hh"

// STL includes
#include <queue>
#include <typeinfo>
#include <utility>
#include <typeindex>

#define FLUX_RESOURCE(type, name) static type* _flux_res_create()\
{\
    return new type;\
}\
\
std::string _flux_res_get_name() override { return #name;}\
static inline bool _flux_res_registered = \
Flux::Resources::registerResource(#name, (Flux::Resources::Resource*(*)())&type::_flux_res_create)

namespace Flux { namespace Resources {

    typedef int ResourceID;

    class Deserializer;

    // Component for keeping track of where entities came from
    struct SerializeCom: public Flux::Component
    {
        FLUX_COMPONENT(SerializeCom, SerializeCom);

        // Ironically, we're not actually serializing this component
        // It's data will be stored somewhere where it's easier to access

        // TODO: Maybe make this a number/id rather than a string
        std::string inherited_file;

        // So we don't have to go through a map each time a ResourceRef is created,
        // Here's a pointer to the parent file
        Deserializer* parent_file;

        /** This entity's inheritance ID */
        uint32_t inheritance_id;
    };

    /**
    Little secret: The resource system is just an ECS.
    That being said, usually there would only be one component, the resource, per Entity.
    There would be other components, but those are usually added by other systems such as the renderer
    */
    struct Resource: Component
    {
        FLUX_COMPONENT(Resource, resource);
        virtual std::string _flux_res_get_name() {return "resource";}

    };

    /**
    Component that reference-counts resources
    */
    struct ResourceCountCom: public Component
    {
        FLUX_COMPONENT(ResourceCountCom, ResourceCountCom);

        uint32_t references;
    };

    /**
    The ECS that powers the resource system
    */
    extern ECSCtx* rctx;

    // Why must I do this...

    /**
    Object that contains a resource.
    ResourceRef is a smart pointer:
        If it wasn't loaded from a file, it will free itself when there are no longer references to it
        If it was loaded from a file, once there are no references to any of that file's resources,
            it will be freed
    */
    template <typename T>
    class ResourceRef;

    void removeResource(EntityRef res);

    // ===================================================
    // Serialization and deserialization
    // ===================================================

    /*
    The "Inheritance ID" system
    Each scene has seperate inheritance ids
    By default, the inheritance id equals the entity id
    But it can also be set manually
    If an entity in a scene has a parent, it will have the parent's entity ID
    Otherwise, it will just have 0
    Some smart importer, like the Assimp importer, might assign manual ids to make inheritance work
    Its ids would come from names, so if the asset was re-imported, it would remain linked
    */

    /** Class to aid serialization. You put in Entities, it puts them into a file */
    class Serializer
    {
    public:
        Serializer(const std::string& filename);

        /**
        Add an Entity to the file
        Returns an int that can be put in a BinaryFile to get that Resource
        Optional "inheritance id" for entities and resources were order is not guarenteed
        */
        uint32_t addEntity(EntityRef entity, uint32_t ihid=0);

        /**
        Adds a resource to the file as a dependency.
        Returns an int that can be put in a BinaryFile to get that Resource
        Optional "inheritance id" for entities and resources were order is not guarenteed
        */
        uint32_t addResource(ResourceRef<Resource> res, uint32_t ihid=0);

        std::string getRelativePath(std::string path)
        {
            std::filesystem::path p(path);
            return std::filesystem::relative(path, std::filesystem::path(fname).parent_path());
        };

        /** Saves the Entities and Resources to a file */
        void save(FluxArc::Archive& arc, bool release);

    private:
        std::vector<EntityRef> entities;
        std::vector<ResourceRef<Resource>> resources;
        std::vector<uint32_t> resource_ihids;

        std::map<std::string, int> name_count;

        std::string fname;
    };

    struct EntityData
    {
        int id;
        std::map<std::string, FluxArc::BinaryFile*> component_data;
    };

    /** Loads a saved scene into memory.  */
    class Deserializer
    {
    public:
        Deserializer(const std::string& filename, bool unlinked_res);
        ~Deserializer();

        /** Add all of the Entities from the file to an ECSCtx. Returns a vector if EntityRefs */
        std::vector<EntityRef> addToECS(ECSCtx* ctx);

        /** Get a resource from the deserializer */
        ResourceRef<Resource> getResource(uint32_t id);

        /** Get a resource from the deserializer, using it's Inheritance ID */
        ResourceRef<Resource> getResourceByIHID(uint32_t ihid);

        /** Gets a singular entity from the Deserializer. Warning: This should only be called from the "deserialize" function */
        EntityRef getEntity(int id);

        /** Returns the folder in which the archive is located */
        std::filesystem::path getDirectory() const
        {
            return dir;
        }

        /** Destroyes all the resources loaded in by this Deserializer */
        void destroyResources();

        // Functions for ResourceRefs
        void addRef();
        void subRef();
    
    private:
        FluxArc::Archive arc;
        std::vector<EntityData> entities;
        std::vector<bool> resource_done;
        std::vector<ResourceRef<Resource>> resources;
        std::map<uint32_t, uint32_t> ihid_table;

        std::vector<bool> entity_done;
        std::vector<EntityRef> entitys;
        ECSCtx* current_ctx;

        std::filesystem::path dir;
        std::filesystem::path fname;

        bool unlinked;

        /** Tracks the total references of all this file's resources */
        uint64_t total_references;

        /** Bool to make sure it doesn't free itself before it's even fully loaded */
        bool created;
    };

    /**
    Link an entity to another scene, and load that other scene.
    If the entity has TransformCom, the other scene will be loaded
    as it's child.
    */
    void createSceneLink(EntityRef entity, const std::string& scene);

    /**
    Instanciate a scene link
    */
    void instanciateSceneLink(EntityRef entity);

    /**
    Component that instanciates another scene.
    Scenes will only be instanciated at load time, or if instanciateScene is called
    */
    struct SceneLinkCom: public Component
    {
        FLUX_COMPONENT(SceneLinkCom, SceneLinkCom);
        std::string filename;

        bool serialize(Resources::Serializer* ser, FluxArc::BinaryFile *output) override
        {
            output->set(ser->getRelativePath(filename));

            return true;
        }

        void deserialize(Resources::Deserializer *deserializer, FluxArc::BinaryFile *file) override
        {
            auto fname = file->get();
            filename = deserializer->getDirectory() / fname;
        }
    };

    template <typename T>
    class ResourceRef
    {
        private:
            EntityRef entity;
        
        public:
            /** Bool that teels you if this resource originated from a file */
            bool from_file;

            ResourceRef(): from_file(false) {};
            ResourceRef(EntityRef entity)
            {
                this->entity = entity;
                entity.getComponent<ResourceCountCom>()->references ++;

                if (entity.hasComponent<SerializeCom>())
                {
                    from_file = true;
                    entity.getComponent<SerializeCom>()->parent_file->addRef();
                }
                else
                {
                    from_file = false;
                }
            }

            ResourceRef(const ResourceRef<T>& res)
            {
                entity = res.getBaseEntity();
                from_file = res.from_file;
                entity.getComponent<ResourceCountCom>()->references ++;

                if (from_file)
                {
                    entity.getComponent<SerializeCom>()->parent_file->addRef();
                }
            }

            template<typename X>
            ResourceRef(const ResourceRef<X>& res)
            {
                entity = res.getBaseEntity();
                from_file = res.from_file;
                entity.getComponent<ResourceCountCom>()->references ++;

                if (from_file)
                {
                    entity.getComponent<SerializeCom>()->parent_file->addRef();
                }
            }

            // ResourceRef(const ResourceRef<Resource>& res)
            // {
            //     entity = res.getBaseEntity();
            //     entity.getComponent<ResourceCountCom>()->references ++;
            // }

            // ResourceRef(const ResourceRef<T>& res)
            // {
            //     entity = res.getBaseEntity();
            //     entity.getComponent<ResourceCountCom>()->references ++;
            // }

            ~ResourceRef()
            {
                if (entity.getEntityID() == -1)
                {
                    // We were uninitialized.
                    return;
                }

                entity.getComponent<ResourceCountCom>()->references --;
                auto refs = entity.getComponent<ResourceCountCom>()->references;

                if (from_file)
                {
                    entity.getComponent<SerializeCom>()->parent_file->subRef();
                }

                if (refs < 1)
                {
                    // Even for resources that are in files,
                    // The file keeps a copy of it
                    // So it will only drop to 0 when the file is freed
                    // So we must free it
                    removeResource(entity);
                    // LOG_SUCCESS("Freed resource");
                }
            }

            ResourceRef<T>& operator = (const ResourceRef<T>& that)
            {
                if (this != &that)
                {
                    if (entity.getEntityID() != -1)
                    {
                        // Not sure if I need this...
                        entity.getComponent<ResourceCountCom>()->references --;
                        if (from_file)
                        {
                            entity.getComponent<SerializeCom>()->parent_file->subRef();
                        }
                        
                        if (entity.getComponent<ResourceCountCom>()->references < 1)
                        {
                            removeResource(entity);
                            // LOG_SUCCESS("Freed resource");
                        }
                    }

                    // Copy data
                    if (that.entity.getEntityID() == -1)
                    {
                        // Make myself null entity
                        this->entity = EntityRef();
                        from_file = false;
                        return *this;
                    }
                    this->entity = that.entity;
                    from_file = that.from_file;
                    entity.getComponent<ResourceCountCom>()->references ++;
                    if (from_file)
                    {
                        entity.getComponent<SerializeCom>()->parent_file->addRef();
                    }
                }

                return *this;
            }

            // ResourceRef(ResourceRef<T>&& that): entity(that.entity)
            // {
            //     entity.getComponent<ResourceCountCom>()->references ++;
            // }

            // ResourceRef<T>& operator = (ResourceRef<T>&& that)
            // {
            //     if (this != &that)
            //     {
            //         // Apparently this doesn't work, hopefully it's not important
            //         // TODO: Find out if it's important
            //         // if (entity.getEntityID() != -1)
            //         // {
            //         //     if (entity.getComponent<ResourceCountCom>()->references == 0)
            //         //     {
            //         //         removeResource(*this);
            //         //     }
            //         // }

            //         // Copy data
            //         this->entity = that.entity;
            //     }
            //     return *this;
            // }

            T* operator -> ()
            {
                return (T*)entity.getComponent<Resource>();
            }

            friend bool operator == (const ResourceRef<T>& a, const ResourceRef<T>& b)
            {
                return a.entity == b.entity;
            }

            EntityRef getBaseEntity() const
            {
                return entity;
            }

            /**
            Get the pointer to the resource. This should be used when
            changing a resource every frame, because using the -> operator,
            and resource ref's gc is quite slow.
            If you're storing a reference to the resource, use a resource ref
            so that it still gets GCed.
            */
            T* getPtr()
            {
                return (T*)entity.getComponent<Resource>();
            }
    };

    /**
    Setup the resource system
    */
    void initialiseResources();

    /**
    Destroy the resource system and free all allocated memory
    */
    void destroyResources();

    /**
    Add a resource to the resource system. This function needs a pointer to a Resource.
    If you want to add a Resource from a file, use `loadResourceFromFile`
    */
    template <typename T>
    ResourceRef<T> createResource(T* res)
    {
        EntityRef res_id = rctx->createEntity();
        res_id.addComponent((Resource *)res);
        res_id.addComponent(new ResourceCountCom);

        return (ResourceRef<T>)ResourceRef<Resource>(res_id);
    }

    /**
    Deletes the given resource. Make sure nothing is using it, as it will be freed
    */
    // template <typename T>
    // void removeResource(ResourceRef<T> res)
    // {
    //     rctx->destroyEntity(res.getBaseEntity());
    // }

    /**
    Deletes the given resource. Make sure nothing is using it, as it will be freed
    This overload is for gerbage collection purposes, so that it doesn't
    create a new ResourceRef from the destructor of another ResourceRef, causing
    many problems
    */
    inline void removeResource(EntityRef res)
    {
        rctx->destroyEntity(res);
    }

    /**
    Returns the actual data from the given resource.
    */
    // Resource* getResource(ResourceID res);

    /// Base function for loadResourceFromFile
    ResourceRef<Resource> _loadResourceFromFile(const std::string& filename);

    /**
    Loads a file and returns the ResourceID.
    File is relative. We recommend setting your working directory to the root of your project
    */
    template <typename T>
    ResourceRef<T> loadResourceFromFile(const std::string& filename)
    {
        // TODO: Better utilise templates
        return (ResourceRef<T>)_loadResourceFromFile(filename);
    }

    /**
    Registers file extension with the resource system.
    When a file with the given extension is loaded, the given function will be called. Extension does not include "."
    */
    void addFileLoader(const std::string& ext, Flux::Resources::Resource*(*function)(std::ifstream*));

    // inline std::map<std::string, std::type_index > resource_types_map;
    inline std::map<std::string, Resource*(*)() > resource_factory_map;

    /**
    Registers a resource's existance
    */
    inline bool registerResource(const std::string& name, Resource*(*function)())
    {
        // resource_types_map[name] = type_info;
        resource_factory_map[name] = function;

        return true;
    }

    // ==================================
    // Text resource
    // ==================================
    struct TextResource: public Resource
    {
        FLUX_RESOURCE(TextResource, text-resource);
        std::string content;
    };

    /**
    Creates a text resource from the given text
    */
    ResourceRef<TextResource> createTextResource(const std::string& text);

    /**
    Creates a text resource from a file. Default resource generator
    */
    Resource* ext_createTextResource(std::ifstream* file);

    // Other bits of serialization and deserialization

    inline std::map<std::string, Deserializer*> loaded_files;
    Deserializer* deserialize(const std::string& filename, bool unlinked_res = false);

    /**
    Remove a Deserializer currently in memory. 
    WARNING: This will also remove all resources loaded from that file
    It is advised to just let the automatic garbage collection system
    handle freeing files
    */
    void freeFile(const std::string& filename);

}}

// Components
// FLUX_DEFINE_COMPONENT(Flux::Resources::Resource, resource);

#endif