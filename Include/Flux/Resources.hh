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
    The ECS that powers the resource system
    */
    extern ECSCtx* rctx;

    /**
    Object that contains a resource. Can be used like a smart pointer
    */
    template <typename T>
    class ResourceRef
    {
        private:
            EntityRef entity;
        
        public:

            ResourceRef() {};
            ResourceRef(EntityRef entity)
            {
                this->entity = entity;
            }
            ResourceRef(const ResourceRef<Resource>& res)
            {
                entity = res.getBaseEntity();
            }


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

        return (ResourceRef<T>)ResourceRef<Resource>(res_id);
    }

    /**
    Deletes the given resource. Make sure nothing is using it, as it will be freed
    */
    template <typename T>
    void removeResource(ResourceRef<T> res)
    {
        rctx->destroyEntity(res.getBaseEntity());
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

    // ===================================
    // Serialization and deserialization
    // ===================================
    
    /** Class to aid serialization. You put in Entities, it puts them into a file */
    class Serializer
    {
    public:
        Serializer();

        /**
        Add an Entity to the file
        Returns an int that can be put in a BinaryFile to get that Resource
        */
        uint32_t addEntity(EntityRef entity);

        /**
        Adds a resource to the file as a dependency.
        Returns an int that can be put in a BinaryFile to get that Resource
        */
        uint32_t addResource(ResourceRef<Resource> res);

        /** Saves the Entities and Resources to a file */
        void save(FluxArc::Archive& arc, bool release);

    private:
        std::vector<EntityRef> entities;
        std::vector<ResourceRef<Resource>> resources;
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
        Deserializer(std::string filename);
        ~Deserializer();

        /** Add all of the Entities from the file to an ECSCtx. Returns a vector if EntityRefs */
        std::vector<EntityRef> addToECS(ECSCtx* ctx);

        /** Get a resource from the deserializer */
        ResourceRef<Resource> getResource(uint32_t id);

        /** Gets a singular entity from the Deserializer. Warning: This should only be called from the "deserialize" function */
        EntityRef getEntity(int id);

        /** Returns the folder in which the archive is located */
        std::filesystem::path getDirectory() const
        {
            return dir;
        }

        /** Destroyes all the resources loaded in by this Deserializer */
        void destroyResources();
    
    private:
        FluxArc::Archive arc;
        std::vector<EntityData> entities;
        std::vector<bool> resource_done;
        std::vector<ResourceRef<Resource>> resources;

        std::vector<bool> entity_done;
        std::vector<EntityRef> entitys;
        ECSCtx* current_ctx;

        std::filesystem::path dir;
    };

}}

// Components
// FLUX_DEFINE_COMPONENT(Flux::Resources::Resource, resource);

#endif