#include "Flux/ECS.hh"
#include "Flux/Flux.hh"
#include "Flux/Log.hh"

// TODO: Implement threading
#include "Flux/Threads.hh"

// STL
#include <map>
#include <string>

using namespace Flux;

ECSCtx* Flux::createContext()
{
    ECSCtx* ctx = new ECSCtx;

    // Clean out entities
    for (int i = 0; i < FLUX_MAX_ENTITIES; i++)
    {
        ctx->entities[i] = nullptr;
    }

    // Initialise systems
    ctx->systems = std::vector<System>();
    ctx->system_count = 0;

    // Initialize reuse queue
    ctx->reuse = std::queue<int>();

    // Start ID count
    ctx->current_id = 0;

    return ctx;
}

bool Flux::destroyContext(ECSCtx* ctx)
{
    // Free everything
    
    // Free all Entities
    for (int i = 0; i < FLUX_MAX_ENTITIES; i++)
    {
        if (ctx->entities[i] != nullptr)
        {
            destroyEntity(ctx, i);
        }
    }

    // Free ctx
    delete ctx;

    return true;
}

EntityID Flux::createEntity(ECSCtx* ctx)
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
    EntityID entity_id;

    if (ctx->reuse.size() > 0)
    {
        // Take first used id
        entity_id = ctx->reuse.front();
        ctx->reuse.pop();
    }
    else
    {
        if (ctx->current_id < FLUX_MAX_ENTITIES)
        {
            entity_id = ctx->current_id;
            ctx->current_id ++;
        }
        else
        {
            // Out of IDs
            LOG_ERROR("Out of Entity IDs. Returning -1. If you require more entities, increase FLUX_MAX_ENTITIES");
            return -1;
        }
    }

    // Now add reference to ctx
    ctx->entities[entity_id] = entity;

    return entity_id;
}

Entity* Flux::getEntity(ECSCtx* ctx, EntityID entity)
{
    return ctx->entities[entity];
}

bool Flux::destroyEntity(ECSCtx* ctx, EntityID entity)
{
    auto en = getEntity(ctx, entity);

    // Remove components
    for (int i = 0; i < FLUX_MAX_COMPONENTS; i++)
    {
        if (hasComponent(ctx, entity, i))
        {
            removeComponent(ctx, entity, i);
        }
    }

    // Free entity
    delete en;

    // Remove from ctx
    ctx->entities[entity] = nullptr;

    // Add to reuse pile
    ctx->reuse.push(entity);

    return true;
}

// Component type map
static std::map<std::string, ComponentTypeID> component_types;
static ComponentTypeID on = 0;

ComponentTypeID Flux::getComponentType(const std::string& name)
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
    return next;
}

void Flux::addComponent(ECSCtx* ctx, EntityID entity, ComponentTypeID component_type, Component* component)
{
    auto en = getEntity(ctx, entity);
    
    // Check for existing component
    if (en->components[component_type] != nullptr)
    {
        #ifndef FLUX_NO_WARN_OVERRIDE_COMPONENT
        LOG_WARN("Component already exists - overwriting (disable this warning by defining FLUX_NO_WARN_OVERRIDE_COMPONENT)");
        #endif

        removeComponent(ctx, entity, component_type);
    }

    en->components[component_type] = component;
}

bool Flux::hasComponent(ECSCtx* ctx, EntityID entity, ComponentTypeID component_type)
{
    return getEntity(ctx, entity)->components[component_type] != nullptr;
}

Component* Flux::getComponent(ECSCtx* ctx, EntityID entity, ComponentTypeID component_type)
{
    return getEntity(ctx, entity)->components[component_type];
}

bool Flux::removeComponent(ECSCtx* ctx, EntityID entity, ComponentTypeID component_type)
{
    auto comp = getEntity(ctx, entity)->components[component_type];
    delete comp;
    getEntity(ctx, entity)->components[component_type] = nullptr;

    return true;
}

int Flux::addSystemFront(ECSCtx* ctx, void (*function)(ECSCtx*, EntityID, float), bool threaded)
{
    // Create System
    System s;
    s.function = function;
    s.id = ctx->id_count;
    #ifndef FLUX_NO_THREADING
    s.threaded = threaded;
    #else
    s.threaded = false;
    #endif
    ctx->id_count ++;

    // Add to ctx
    ctx->systems.insert(ctx->systems.begin(), s);
    ctx->system_count++;

    return s.id;
}

int Flux::addSystemBack(ECSCtx* ctx, void (*function)(ECSCtx*, EntityID, float), bool threaded)
{
    // Create System
    System s;
    s.function = function;
    s.id = ctx->id_count;
    #ifndef FLUX_NO_THREADING
    s.threaded = threaded;
    #else
    s.threaded = false;
    #endif
    ctx->id_count ++;

    // Add to ctx
    ctx->systems.push_back(s);
    ctx->system_count++;

    return s.id;
}

bool Flux::removeSystem(ECSCtx* ctx, int system_id)
{
    int idx = -1;
    for (int s = 0; s < ctx->system_count; s++)
    {
        if (ctx->systems[s].id == system_id)
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
        ctx->systems.erase(ctx->systems.begin() + idx);
    }

    return true;
}

void Flux::runSystems(ECSCtx* ctx, float delta)
{
    for (int s = 0; s < ctx->system_count; s++)
    {
        if (ctx->systems[s].threaded)
        {
            // Do threading stuff
            Threads::runThreads(Flux::threading_context, ctx, &ctx->systems[s], delta);
            Threads::waitForThreads(Flux::threading_context);
        }
        else
        {
            for (int i = 0; i < FLUX_MAX_ENTITIES; i++)
            {
                if (ctx->entities[i] != nullptr)
                {
                    ctx->systems[s].function(ctx, i, delta);
                }
            }
        }
        
    }
}