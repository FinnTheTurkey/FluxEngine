#include "Flux/ECS.hh"
#include "Flux/Flux.hh"
#include "Flux/Log.hh"

// TODO: Implement threading
#include "Flux/Threads.hh"

// STL
#include <algorithm>
#include <map>
#include <string>

using namespace Flux;

ECSCtx Flux::createContext()
{
    ECSCtx ctx;

    // Clean out entities
    for (int i = 0; i < FLUX_MAX_ENTITIES; i++)
    {
        ctx.entities[i] = nullptr;
    }

    // Initialise systems
    ctx.system_order = std::vector<SystemID>();
    ctx.system_count = 0;

    // We don't need to clean out systems

    // Initialize reuse queue
    ctx.reuse = std::queue<int>();
    ctx.system_reuse = std::queue<SystemID>();

    // Start ID count
    ctx.current_id = 0;

    return std::move(ctx);
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

// Component destructor array
void (*component_destructors[FLUX_MAX_COMPONENTS])(ECSCtx* ctx, EntityID entity);

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

    // Zero the destructor
    component_destructors[next] = nullptr;

    return next;
}

void Flux::setComponentDestructor(ComponentTypeID component, void (*function)(ECSCtx* ctx, EntityID entity))
{
    component_destructors[component] = function;
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
    // Run destructor
    if (component_destructors[component_type] != nullptr)
    {
        component_destructors[component_type](ctx, entity);
    }

    // Remove it
    auto comp = getEntity(ctx, entity)->components[component_type];
    delete comp;
    getEntity(ctx, entity)->components[component_type] = nullptr;

    return true;
}

// Private base function that adds system to the array
static int addSystem(ECSCtx* ctx, void (*function)(ECSCtx*, EntityID, float), bool threaded)
{
    // Create System
    System s;
    s.function = function;
    // s.id = ctx->id_count;
    #ifndef FLUX_NO_THREADING
    s.threaded = threaded;
    #else
    s.threaded = false;
    #endif

    // ctx->id_count ++;

    if (!ctx->system_reuse.empty())
    {
        // Use recycled ID
        int id = ctx->system_reuse.front();
        ctx->system_reuse.pop();

        ctx->systems[id] = s;
        return id;
    }
    else
    {
        // Use new ID
        int id = ctx->id_count;

        ctx->systems[id] = s;
        ctx->id_count++;
        return id;
    }
}

int Flux::addSystemFront(ECSCtx* ctx, void (*function)(ECSCtx*, EntityID, float), bool threaded)
{
    SystemID s = addSystem(ctx, function, threaded);
    // Add to ctx
    ctx->system_order.insert(ctx->system_order.begin(), s);
    ctx->system_count++;

    return s;
}

int Flux::addSystemBack(ECSCtx* ctx, void (*function)(ECSCtx*, EntityID, float), bool threaded)
{
    SystemID s = addSystem(ctx, function, threaded);
    // Add to ctx
    ctx->system_order.push_back(s);
    ctx->system_count++;

    return s;
}

bool Flux::removeSystem(ECSCtx* ctx, int system_id)
{
    int idx = -1;
    for (int s = 0; s < ctx->system_count; s++)
    {
        if (ctx->system_order[s] == system_id)
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
        ctx->system_order.erase(ctx->system_order.begin() + idx);

        // Remove from array
        // We don't _actually_ remove it from the array,
        // But the enxt system added will overwrite it
        ctx->system_reuse.push(system_id);
        
        ctx->system_count--;
    }

    return true;
}

void Flux::runSystem(ECSCtx *ctx, SystemID sys, float delta)
{
    if (ctx->systems[sys].threaded)
    {
        #ifndef FLUX_NO_THREADING
        // Do threading stuff
        Threads::runThreads(Flux::threading_context, ctx, &ctx->systems[sys], delta);
        Threads::waitForThreads(Flux::threading_context);
        #endif
    }
    else
    {
        for (int i = 0; i < FLUX_MAX_ENTITIES; i++)
        {
            if (ctx->entities[i] != nullptr)
            {
                ctx->systems[sys].function(ctx, i, delta);
            }
        }
    }
}

void Flux::runSystems(ECSCtx* ctx, float delta)
{
    for (int s = 0; s < ctx->system_count; s++)
    {
        runSystem(ctx, ctx->system_order[s], delta);
        
    }
}