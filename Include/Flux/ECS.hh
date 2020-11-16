#ifndef FLUX_ECS_HH
#define FLUX_ECS_HH

#ifndef FLUX_MAX_ENTITIES
#define FLUX_MAX_ENTITIES 1024
#endif

#ifndef FLUX_MAX_COMPONENTS
#define FLUX_MAX_COMPONENTS 256
#endif 

#ifndef FLUX_MAX_SYSTEMS
#define FLUX_MAX_SYSTEMS 256
#endif 

// STL includes
#include <queue>
#include <string>
#include <vector>


namespace Flux 
{
    typedef int EntityID;
    typedef int ComponentTypeID;
    typedef int SystemID;

    struct ECSCtx;

    /** Empty struct. All components should inherit from this */
    struct Component
    {

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

    /**
     * System struct. This contains infomation on how to run a system
     */
    struct System
    {
        void (*function)(ECSCtx*, EntityID, float);
        int id;
        bool threaded;
    };

    /**
    The context for the entire ECS.
    */
    struct ECSCtx
    {
        /**
        Array of all entities. An EntityID is an index in this array
        */
        Entity *entities[FLUX_MAX_ENTITIES];

        /* ID to be used for the next Entity if the reuse queue is empty*/
        int current_id;

        /**
         * Vector of systems. Using a vector so systems can be added at the front and back.
         */
        std::vector<System> systems;
        int system_count;
        int id_count;

        // Reuse section
        // ===================

        /** Queue of IDs waiting to be reused */
        std::queue<EntityID> reuse;
    };


    // Context Section
    // ===============================================

    /** The function that starts everything. It returns an empty Entity Component System */
    ECSCtx* createContext();

    /**
    Destroyes ECS and frees memory.
    Warning: This will deallocate ESSCtx
    */
    bool destroyContext(ECSCtx* ctx);


    // Entity Section
    // ===============================================

    /**
    Creates an Entity. Returns an identifier which represents the new Entity in the ECS, otherwise known as the EntityID.
    */
    EntityID createEntity(ECSCtx* ctx);

    /**
    Removes an Entity from the ECS. Once an Entity is removed, it is gone.
    Its EntityID will be reused, so be sure to remove all references to it
    */
    bool destroyEntity(ECSCtx* ctx, EntityID entity);

    /**
    Gets the entity from the context, and returns it to you as a pointer
    There are very few cases where this function should be used, most of the time you should be accessing components using the Entity's id
    Returns _nullptr_ if it cannot find entity
    */
    Entity* getEntity(ECSCtx* ctx, EntityID entity);


    // Component Section
    // ===============================================

    /**
    Returns the ID of the given component type, or creates it if it doesn't exist
    Component types are the same across all ECS contexts
    **WARNING:** This function should **never** be run every frame due to it's use of a map
    */
    ComponentTypeID getComponentType(const std::string& name);

    /**
    Adds a component to an Entity. 
    Since the component could be of any type, the component must be provided.
    Once a component is added to an Entity, it is managed by the ECS context. It will be freed when it is removed, the entity is destroyed, or the entire ECS is destroyed
    */
    void addComponent(ECSCtx* ctx, EntityID entity, ComponentTypeID component_type, Component* component);

    /**
    Returns true if the given entity has a component of the given type
    */
    bool hasComponent(ECSCtx* ctx, EntityID entity, ComponentTypeID component_type);

    /**
    Returns the given component on the given entity. Returns nullptr if the component doesn't exist
    */
    Component* getComponent(ECSCtx* ctx, EntityID entity, ComponentTypeID component_type);

    /**
    Removes and frees the given component of the given entity
    Components are unrecoverable, and any pointers to them will become dangling
    */
    bool removeComponent(ECSCtx* ctx, EntityID entity, ComponentTypeID component_type);


    // System Section
    // ===============================================


    /**
     * Adds a system to the system queue.
     * This system is added to the front, and will be executed first
     * Returns the system's ID, which can be used to delete it.
     * Takes a function pointer. When the system is run, that function pointer will be called against every Entity
     * **WARNING:** This function is quite expensive, and _should never_ be run every frame
     */
    int addSystemFront(ECSCtx* ctx, void (*function)(ECSCtx*, EntityID, float), bool threaded = true);

    /**
     * Adds a system to the system queue.
     * This system is added to the back, and will be executed last
     * Returns the system's ID, which can be used to delete it.
     * Takes a function pointer. When the system is run, that function pointer will be called against every Entity
     * **WARNING:** This function is quite expensive, and _should never_ be run every frame
     */
    int addSystemBack(ECSCtx* ctx, void (*function)(ECSCtx*, EntityID, float), bool threaded = true);

    /**
     * Removes the given system.
     * **WARNING:** This function is very expensive
     */
    bool removeSystem(ECSCtx* ctx, int system_id);

    /**
     * Runs through all the systems on all the Entities
     */
    void runSystems(ECSCtx* ctx, float delta);

};


#endif