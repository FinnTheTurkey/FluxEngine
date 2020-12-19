#ifndef FLUX_ECS_HH
#define FLUX_ECS_HH

#include "Flux/Log.hh"
#include "FluxArc/FluxArc.hh"
#ifndef FLUX_MAX_ENTITIES
#define FLUX_MAX_ENTITIES 1024
#endif

#ifndef FLUX_MAX_COMPONENTS
#define FLUX_MAX_COMPONENTS 256
#endif 

#ifndef FLUX_MAX_SYSTEMS
#define FLUX_MAX_SYSTEMS 256
#endif 

#ifndef FLUX_MAX_DESTRUCTION_QUEUE
#define FLUX_MAX_DESTRUCTION_QUEUE 256
#endif 

#ifndef FLUX_MAX_SYSTEM_QUEUE
#define FLUX_MAX_SYSTEM_QUEUE 256
#endif 

// STL includes
#include <queue>
#include <string>
#include <vector>

// Defines
// Aka dark magic

// #define FLUX_DEFINE_COMPONENT(type, nameid) namespace FluxTypes { template <> struct TypeMap< FluxTypes::type_id<type> > {static constexpr const char* name = #nameid;}; }

#define FLUX_COMPONENT(type, name) static type* _flux_create()\
{\
    return new type;\
}\
\
static std::string _flux_get_name() { return #name;}\
static inline bool _flux_registered = \
Flux::registerComponent(#name, (Flux::Component*(*)())&type::_flux_create);


namespace FluxTypes
{
    template <void (*)()> struct TypeMap;

    template<typename T>
    void type_id(){}
}

namespace Flux
{
    // typedef int EntityID;
    typedef int ComponentTypeID;
    typedef int SystemID;

    
    // Pre-definitions
    struct ECSCtx;
    namespace Resources
    {
        class Serializer;
        class Deserializer;
    }

    /** 
    All components should inherit from this. 
    */
    struct Component
    {
        /**
        Function to override for serialization.
        Takes in a Serializer, which can be used to add any needed Resources
        Put serialized data in the given BinaryFile.

        If you do not wish this component to be serialized, return false.
        Otherwise, return true
        */
        virtual bool serialize(Resources::Serializer* serializer, FluxArc::BinaryFile* output) { return false; };
        virtual void deserialize(Resources::Deserializer* deserializer, FluxArc::BinaryFile* file) {};
    };

    /**
    Anything that needs to be controlled by Engine should be an Entity.
    Entities should only be accessed via their EntityID.
    Each Entity stores Components, which are containers that store the Entities data
    */
    struct Entity
    {
        Component *components[FLUX_MAX_COMPONENTS];
    };

    class EntityRef;

    /**
    Base class for systems. Has 3 virtual functions that can be overriden
    */
    class System
    {
    public:
        /** Called right before this system is run.  */
        virtual void onSystemStart() {}

        /** Called for every entity in the ECS */
        virtual void runSystem(EntityRef entity, float delta);

        /** Called after this system is run */
        virtual void onSystemEnd() {}

    };

    /**
     * System struct. This contains infomation on how to run a system
     */
    struct SystemContainer
    {
        System* sys;
        int id;
        bool threaded;
    };

    /**
    The context for the entire ECS.
    */
    class ECSCtx
    {
    private:
        /**
        Array of all entities. An EntityID is an index in this array
        */
        Entity *entities[FLUX_MAX_ENTITIES];

        /* ID to be used for the next Entity if the reuse queue is empty*/
        int current_id;

        /** Array where the actual systems are stored */
        SystemContainer systems[FLUX_MAX_SYSTEMS];

        /**
         * Vector of systems. Using a vector so systems can be added at the front and back.
         */
        std::vector<SystemID> system_order;
        int system_count;
        int id_count;

        // Queues
        // ===================

        /** Array of Entities to be destroyed */
        int destruction_queue[FLUX_MAX_DESTRUCTION_QUEUE];

        /** Number of Entities currently in the destruction queue */
        int destruction_count;

        /** Array of Systems to be run. They will be run after the current system is finished, or,
         if not called from a system, the next time a system is run */
        int system_queue[FLUX_MAX_SYSTEM_QUEUE];

        /** Number of Entities in the system queue */
        int system_queue_count;

        // Reuse section
        // ===================

        /** Queue of IDs waiting to be reused */
        std::queue<int> reuse;

        /** System reuse */
        std::queue<SystemID> system_reuse;

    public:
        // Functions
        // Entity Section
        // ===============================================

        // Constructor and destructor
        ECSCtx();
        ~ECSCtx() {};

        /** Destroy all the entities */
        void destroyAllEntities();

        /**
        Creates an Entity. Returns an identifier which represents the new Entity in the ECS, otherwise known as the EntityID.
        */
        EntityRef createEntity();

        /**
        Removes an Entity from the ECS. Once an Entity is removed, it is gone.
        Its EntityID will be reused, so be sure to remove all references to it
        */
        bool destroyEntity(EntityRef entity);

        /**
        Queues an Entity for destruction. It will be destroyed at the end of runAllSystems,
        or if destroyQueuedEntities() is called
        */
        bool queueDestroyEntity(EntityRef entity);

        /**
        Destroyes all the entities in the destruction queue
        */
        bool destroyQueuedEntities();


        /**
        Gets the entity from the context, and returns it to you as a pointer
        There are very few cases where this function should be used, most of the time you should be accessing components using the Entity's id
        Returns _nullptr_ if it cannot find entity
        */
        Entity* getEntity(EntityRef entity);


        // Component Section
        // ===============================================

        /**
        Adds a component to an Entity. 
        Since the component could be of any type, the component must be provided.
        Once a component is added to an Entity, it is managed by the ECS context. It will be freed when it is removed, the entity is destroyed, or the entire ECS is destroyed
        */
        void _addComponent(int entity, ComponentTypeID component_type, Component* component);

        /**
        Returns true if the given entity has a component of the given type
        */
        bool _hasComponent(int entity, ComponentTypeID component_type);

        /**
        Returns the given component on the given entity. Returns nullptr if the component doesn't exist
        */
        Component* _getComponent(int entity, ComponentTypeID component_type);

        /**
        Removes and frees the given component of the given entity
        Components are unrecoverable, and any pointers to them will become dangling
        */
        bool _removeComponent(int entity, ComponentTypeID component_type);


        // System Section
        // ===============================================

        /**
        Adds a system to the ECS. This system will not be run unless explicitly called.
        */
        int addSystem(System* sys, bool threaded=true);

        /**
        * Adds a system to the system queue.
        * This system is added to the front, and will be executed first
        * Returns the system's ID, which can be used to delete it.
        * Takes a function pointer. When the system is run, that function pointer will be called against every Entity
        * **WARNING:** This function is quite expensive, and _should never_ be run every frame
        */
        int addSystemFront(System* sys, bool threaded = true);

        /**
        * Adds a system to the system queue.
        * This system is added to the back, and will be executed last
        * Returns the system's ID, which can be used to delete it.
        * Takes a function pointer. When the system is run, that function pointer will be called against every Entity
        * **WARNING:** This function is quite expensive, and _should never_ be run every frame
        */
        int addSystemBack(System* sys, bool threaded = true);

        /**
        * Removes the given system.
        * **WARNING:** This function is very expensive
        */
        bool removeSystem(int system_id);

        /**
        * Runs through all the systems on all the Entities
        */
        void runSystems(float delta);

        /**
        Runs a single system.
        Disable run_queue if you do not want any queued systems to be run before this one
        */
        void runSystem(SystemID sys, float delt, bool run_queue = true);

        /**
        Adds a system to the run queue. It will be run after the current system is done, 
        or before a new system is run.
        */
        void queueRunSystem(SystemID system);

        /**
        Runs the queued systems. Primarily intended for internal use.
        */
        void runQueuedSystems(float delta);
    };

    inline std::map<std::string, ComponentTypeID> component_types;
    inline std::map<std::string, Component*(*)()> component_factory;
    inline ComponentTypeID on = 0;
    inline void (*component_destructors[FLUX_MAX_COMPONENTS])(EntityRef entity);

    /**
    Returns the ID of the given component type, or creates it if it doesn't exist
    Component types are the same across all ECS contexts
    **WARNING:** This function should **never** be run every frame due to it's use of a map
    */
    inline ComponentTypeID getComponentType(const std::string& name)
    {
        if (component_types.find(name) != component_types.end())
        {
            // It's in the map
            return component_types[name];
        }

        if (on >= FLUX_MAX_COMPONENTS)
        {
            // We're out of component ids
            LOG_ERROR("Out of ComponentTypeIDs. Returning -1. Increase FLUX_MAX_COMPONENTS if more component IDs are required");
            return -1;
        }

        // Create new id
        ComponentTypeID next = on;
        on++;
        component_types[name] = next;

        // Zero the destructor
        component_destructors[next] = nullptr;

        // LOG_INFO("Got component type for component " + name);

        return next;
    }
    
    inline std::string getComponentType(ComponentTypeID input)
    {
        for (auto c : component_types)
        {
            if (c.second == input)
            {
                return c.first;
            }
        }

        return "how did this happen";
    }

    /**
    Register's a component's existance
    */
    inline bool registerComponent(std::string name, Component*(*function)())
    {
        component_factory[name] = function;
        getComponentType(name);
        return true;
    }

    /**
    Base function for setComponentDestructor
    */
    void _setComponentDestructor(ComponentTypeID component, void (*function)(EntityRef));

    /**
    Adds a function that will be called when that type of component is destroyed
    */
    template <typename T>
    void setComponentDestructor(void (*function)(EntityRef entity))
    {
        // static int component_id = getComponentType(std::string(FluxTypes::TypeMap<FluxTypes::type_id<T>>().name));
        static int component_id = getComponentType(T::_flux_get_name());
        _setComponentDestructor(component_id, function);
    }


    // Context Section
    // ===============================================

    /** The function that starts everything. It returns an empty Entity Component System */
    // ECSCtx* createContext();

    /**
    Destroyes ECS and frees memory.
    Warning: This will deallocate ESSCtx
    */
    // bool destroyContext(ECSCtx* ctx);

    /**
    A class that points to a specific entity in a specific ECSCtx
    */
    class EntityRef
    {
    private:
        ECSCtx* ctx;
        int entity_id;

        bool checkInvalid()
        {
            if (entity_id != -1 && ctx != nullptr)
            {
                if (ctx->getEntity(*this) != nullptr)
                {
                    return false;
                }
            }

            return true;
        }
    
    public:
        EntityRef()
        {
            // For before it is filled up
            entity_id = -1;
            ctx = nullptr;
        }

        EntityRef(ECSCtx* ctx_a, int entity_id_a):
        entity_id(entity_id_a),
        ctx(ctx_a)
        {
        }

        EntityRef(const EntityRef& ref)
        {
            entity_id = ref.entity_id;
            ctx = ref.ctx;
        }
        ~EntityRef() {};

        // Getters

        int getEntityID() const
        {
            return entity_id;
        }

        ECSCtx* getCtx() const
        {
            return ctx;
        }

        // Utility functions

        /**
        Get a component in this entity.
        Components are identified by their type, so put the type of the component you wish
        into the template to retreive it.
        */
        template<typename T>
        T* getComponent()
        {
            LOG_ASSERT_MESSAGE_FATAL(checkInvalid(), "This is not a valid entity: Make sure it has been initialized, and has not be deleted");
            static int component_id = getComponentType(T::_flux_get_name());

            return (T*)ctx->_getComponent(entity_id, component_id);
        }

        /**
        Add a component to the Entity by type
        */
        template<typename T>
        void addComponent(T* comp)
        {
            LOG_ASSERT_MESSAGE_FATAL(checkInvalid(), "This is not a valid entity: Make sure it has been initialized, and has not be deleted");
            static int component_id = getComponentType(T::_flux_get_name());

            ctx->_addComponent(entity_id, component_id, (Component*)comp);
        }

        /**
        Check if a component of the given type exists
        */
        template<typename T>
        bool hasComponent()
        {
            LOG_ASSERT_MESSAGE_FATAL(checkInvalid(), "This is not a valid entity: Make sure it has been initialized, and has not be deleted");
            static int component_id = getComponentType(T::_flux_get_name());

            return ctx->_hasComponent(entity_id, component_id);
        }

        /**
        Remove a component of the given type
        */
        template<typename T>
        void removeComponent()
        {
            LOG_ASSERT_MESSAGE_FATAL(checkInvalid(), "This is not a valid entity: Make sure it has been initialized, and has not be deleted");
            static int component_id = getComponentType(T::_flux_get_name());

            ctx->_removeComponent(entity_id, component_id);
        }

        friend bool operator== (const EntityRef& a, const EntityRef& b)
        {
            return a.getEntityID() == b.getEntityID();
        }
    };

};


#endif