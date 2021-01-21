#include "Flux/ECS.hh"
#include "Flux/Flux.hh"
#include "Flux/Log.hh"

// TODO: Implement threading
#include "Flux/Threads.hh"

// STL
#include <algorithm>
#include <iostream>
#include <map>
#include <string>

using namespace Flux;

Flux::ECSCtx::ECSCtx()
{

    // Clean out entities
    for (int i = 0; i < FLUX_MAX_ENTITIES; i++)
    {
        entities[i] = nullptr;
    }

    // Initialise systems
    system_order = std::vector<SystemID>();
    system_count = 0;

    // We don't need to clean out systems
    for (int i = 0; i < FLUX_MAX_SYSTEMS; i++)
    {
        systems[i] = SystemContainer {nullptr, -1, false};
    }

    for (int i = 0; i < FLUX_MAX_DESTRUCTION_QUEUE; i++)
    {
        destruction_queue[i] = 0;
    }

    for (int i = 0; i < FLUX_MAX_SYSTEM_QUEUE; i++)
    {
        system_queue[i] = 0;
    }

    // Initialize reuse queue
    reuse = std::queue<int>();
    system_reuse = std::queue<SystemID>();

    // Start ID count
    current_id = 0;
    id_count = 0;

    // Make sure queues don't segfault
    destruction_count = 0;
    system_queue_count = 0;
}

void Flux::ECSCtx::destroyAllEntities()
{
    // Free everything
    
    // Free all Entities
    for (int i = 0; i < FLUX_MAX_ENTITIES; i++)
    {
        if (entities[i] != nullptr)
        {
            destroyEntity(EntityRef(this, i));
        }
    }
}

EntityRef Flux::ECSCtx::createEntity()
{
    // Allocate
    Entity* entity = new Entity;

    // Clean out components
    for (int i = 0; i < FLUX_MAX_COMPONENTS; i++)
    {
        entity->components[i] = nullptr;
    }

    // Add to ctx
    // Check ID queue
    int entity_id;

    if (reuse.size() > 0)
    {
        // Take first used id
        entity_id = reuse.front();
        reuse.pop();
    }
    else
    {
        if (current_id < FLUX_MAX_ENTITIES)
        {
            entity_id = current_id;
            current_id ++;
        }
        else
        {
            // Out of IDs
            LOG_ERROR("Out of Entity IDs. Returning -1. If you require more entities, increase FLUX_MAX_ENTITIES");
            return EntityRef(this, -1);
        }
    }

    // Now add reference to ctx
    entities[entity_id] = entity;

    return EntityRef(this, entity_id);
}

EntityRef Flux::ECSCtx::createNamedEntity(const std::string& name)
{
    auto entity = createEntity();
    auto nc = new NameCom;
    nc->name = name;
    entity.addComponent(nc);

    return entity;
}

Entity* Flux::ECSCtx::getEntity(EntityRef entity)
{
    return entities[entity.getEntityID()];
}

EntityRef Flux::ECSCtx::getNamedEntity(const std::string &name)
{
    for (int i = 0; i < FLUX_MAX_ENTITIES; i++)
    {
        auto er = EntityRef(this, i);
        if (er.hasComponent<NameCom>())
        {
            if (er.getComponent<NameCom>()->name == name)
            {
                return er;
            }
        }
    }
    
    LOG_WARN("Could not find entity with name " + name);
    return EntityRef();
}

bool Flux::ECSCtx::destroyEntity(EntityRef entity)
{
    if (entity.getEntityID() == -1)
    {
        LOG_WARN("Warning: Attempting to remove inexistant entity");
        return false;
    }

    auto en = getEntity(entity);

    // Remove components
    for (int i = 0; i < FLUX_MAX_COMPONENTS; i++)
    {
        if (_hasComponent(entity.getEntityID(), i))
        {
            _removeComponent(entity.getEntityID(), i);
        }
    }

    // Free entity
    delete en;

    // Remove from ctx
    entities[entity.getEntityID()] = nullptr;

    // Add to reuse pile
    reuse.push(entity.getEntityID());

    return true;
}

bool Flux::ECSCtx::queueDestroyEntity(EntityRef entity)
{
    if (destruction_count+1 > FLUX_MAX_SYSTEM_QUEUE)
    {
        LOG_ERROR("System Queue overloaded. Increase FLUX_MAX_SYSTEM_QUEUE to remove this error");
        return false;
    }

    destruction_count++;
    destruction_queue[destruction_count-1] = entity.getEntityID();

    return true;
}

bool Flux::ECSCtx::destroyQueuedEntities()
{
    for (int i = destruction_count-1; i >= 0; i--)
    {
        destroyEntity(EntityRef(this, destruction_queue[i]));
        destruction_count -= 1;
    }

    return true;
}

void Flux::_setComponentDestructor(ComponentTypeID component, void (*function)(EntityRef))
{
    // component_destructors[component] = function;
}

void Flux::ECSCtx::_addComponent(int entity, ComponentTypeID component_type, Component* component)
{
    auto en = getEntity(EntityRef(this, entity));
    
    // Check for existing component
    if (en->components[component_type] != nullptr)
    {
        #ifndef FLUX_NO_WARN_OVERRIDE_COMPONENT
        LOG_WARN("Component already exists - overwriting (disable this warning by defining FLUX_NO_WARN_OVERRIDE_COMPONENT)");
        #endif

        _removeComponent(entity, component_type);
    }

    en->components[component_type] = component;
}

bool Flux::ECSCtx::_hasComponent(int entity, ComponentTypeID component_type)
{
    return getEntity(EntityRef(this, entity))->components[component_type] != nullptr;
}

Component* Flux::ECSCtx::_getComponent(int entity, ComponentTypeID component_type)
{
    return getEntity(EntityRef(this, entity))->components[component_type];
}

bool Flux::ECSCtx::_removeComponent(int entity, ComponentTypeID component_type)
{
    // Run destructor
    auto er = EntityRef(this, entity);

    // TODO: Fully kill the old destructors system
    // if (component_destructors[component_type] != nullptr)
    // {
    //     component_destructors[component_type](er);
    // }

    // Remove it
    auto comp = getEntity(er)->components[component_type];
    delete comp;
    getEntity(er)->components[component_type] = nullptr;

    return true;
}

int Flux::ECSCtx::addSystem(System* sys, bool threaded)
{
    // Create System
    SystemContainer s;
    s.sys = sys;
    // s.id = id_count;
    #ifndef FLUX_NO_THREADING
    s.threaded = threaded;
    #else
    s.threaded = false;
    #endif

    // LOG_INFO("Created system");

    // id_count ++;

    if (!system_reuse.empty())
    {
        // Use recycled ID
        int id = system_reuse.front();
        system_reuse.pop();

        systems[id] = s;
        return id;
    }
    else
    {
        // Use new ID
        int id = id_count;
        // std::cout << id << std::endl;
        // LOG_INFO("About to add...");
        systems[id] = s;
        // LOG_INFO("Adding id...");
        id_count++;
        // LOG_INFO("Added to systems");
        return id;
    }
}

int Flux::ECSCtx::addSystemFront(System* sys, bool threaded)
{
    SystemID s = addSystem(sys, threaded);
    // Add to ctx
    system_order.insert(system_order.begin(), s);
    system_count++;

    return s;
}

int Flux::ECSCtx::addSystemBack(System* sys, bool threaded)
{
    SystemID s = addSystem(sys, threaded);
    // LOG_INFO("Added system");

    // Add to ctx
    system_order.push_back(s);
    system_count++;
    // LOG_INFO("Added to ctx");

    return s;
}

bool Flux::ECSCtx::removeSystem(int system_id)
{
    int idx = -1;
    for (int s = 0; s < system_count; s++)
    {
        if (system_order[s] == system_id)
        {
            idx = s;
            break;
        }
    }

    if (idx == -1)
    {
        LOG_WARN("Could not remove system with id " + std::to_string(system_id));
    }
    else
    {
        system_order.erase(system_order.begin() + idx);

        // Remove from array
        // We don't _actually_ remove it from the array,
        // But the next system added will overwrite it
        system_reuse.push(system_id);
        
        system_count--;
    }

    return true;
}

void Flux::ECSCtx::runSystem(SystemID sys, float delta, bool run_queue)
{
    // Run system queue
    if (run_queue)
    {
        runQueuedSystems(delta);
    }

    if (systems[sys].threaded)
    {
        #ifndef FLUX_NO_THREADING
        // Do threading stuff
        // Threads::runThreads(Flux::threading_context, ctx, systems[sys], delta);
        // Threads::waitForThreads(Flux::threading_context);
        #endif
    }
    else
    {
        systems[sys].sys->onSystemStart();
        for (int i = 0; i < FLUX_MAX_ENTITIES; i++)
        {
            if (entities[i] != nullptr)
            {
                systems[sys].sys->runSystem(EntityRef(this, i), delta);
            }
        }
        systems[sys].sys->onSystemEnd();
    }
}

void Flux::ECSCtx::runQueuedSystems(float delta)
{
    for (int i = system_queue_count-1; i >= 0; i--)
    {
        runSystem(system_queue[i], delta, false);
        system_queue_count -= 1;
    }
}

void Flux::ECSCtx::runSystems(float delta)
{
    for (int s = 0; s < system_count; s++)
    {
        runSystem(system_order[s], delta);
    }

    // Make sure we end with an empty queue
    runQueuedSystems(delta);

    // And destroy queued entities
    destroyQueuedEntities();
}

void Flux::ECSCtx::queueRunSystem(SystemID system)
{
    if (system_queue_count+1 > FLUX_MAX_SYSTEM_QUEUE)
    {
        LOG_ERROR("System Queue overloaded. Increase FLUX_MAX_SYSTEM_QUEUE to remove this error");
        return;
    }

    system_queue_count++;
    system_queue[system_queue_count-1] = system;
}

// Because C++ is stupid
void Flux::System::runSystem(EntityRef entity, float delta) {}