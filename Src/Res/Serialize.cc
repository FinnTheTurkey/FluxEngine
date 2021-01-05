#include "Flux/ECS.hh"
#include "Flux/Log.hh"
#include "Flux/Resources.hh"
#include "FluxArc/FluxArc.hh"
#include <algorithm>
#include <queue>
#include <string>

using namespace Flux::Resources;

Serializer::Serializer()
: entities(), resources()
{

}

uint32_t Serializer::addEntity(EntityRef entity)
{
    if (std::find(entities.begin(), entities.end(), entity) != entities.end())
    {
        // Return index
        auto r = std::find(entities.begin(), entities.end(), entity);
        return r - entities.begin();
    }

    // Add to vector
    entities.push_back(entity);
    return entities.size()-1;
}

uint32_t Serializer::addResource(ResourceRef<Resource> res)
{
    if (std::find(resources.begin(), resources.end(), res) != resources.end())
    {
        // Return index
        auto r = std::find(resources.begin(), resources.end(), res);
        return r - resources.begin();
    }

    // Add to vector
    resources.push_back(res);
    return resources.size()-1;
}

void Serializer::save(FluxArc::Archive& arc, bool release = false)
{
    // Serialize all the entities
    auto it = entities.begin();
    int j = 0;
    while (it != entities.end())
    {
        auto e = entities[j];
        FluxArc::BinaryFile en;

        // Placeholder value
        uint32_t t = 10;
        en.set(t);

        // Write all components
        auto real_entity = e.getCtx()->getEntity(e);

        uint32_t component_count = 0;
        for (int i = 0; i < FLUX_MAX_COMPONENTS; i++)
        {
            if (real_entity->components[i] != nullptr)
            {
                // Find it's name, and add it to the file
                // We have to use the name, because component types
                // Are not guarenteed to be the same each run
                std::string name = getComponentType(i);

                // Serialize
                FluxArc::BinaryFile bf;
                bool out = real_entity->components[i]->serialize(this, &bf);

                if (out)
                {
                    // They want it, so save it in a file
                    en.set(name);
                    en.set(bf.getSize());
                    en.set(bf.getDataPtr(), bf.getSize());
                    component_count++;
                }
            }
        }

        // Set component count
        en.setCursor(0);
        en.set(component_count);

        // Add to archive
        arc.setFile("Entity-" + std::to_string(j), en, true, release);
        // arc.setFile("Entity-" + std::to_string(j), en, false, release);

        it++;
        j++;
    }

    // Serialize all Resources
    auto it2 = resources.begin();
    j = 0;
    while (it2 != resources.end())
    {
        if (j >= resources.size())
        {
            // This happens sometimes
            // idk why
            // Hopefully this will "fix" it
            break;
        }

        // Serialize
        FluxArc::BinaryFile bf, en;
        bool out = resources[j]->serialize(this, &bf);

        if (out)
        {
            // They want it, so save it in a file
            en.set(resources[j]->_flux_res_get_name());
            en.set(bf.getSize());
            en.set(bf.getDataPtr(), bf.getSize());
        }
        else
        {
            en.set("...");
            en.set(-1);
        }

        arc.setFile("Resource-" + std::to_string(j), en, true, release);
        // arc.setFile("Resource-" + std::to_string(j), en, false, release);

        j++;
        it2++;
    }

    FluxArc::BinaryFile properties;

    // Set # of entities and # of resources
    properties.set((uint32_t)entities.size());
    properties.set((uint32_t)resources.size());

    arc.setFile("--scene-properties--", properties);
}

Deserializer::Deserializer(std::string filename)
{
    arc = FluxArc::Archive(filename);
    dir = std::filesystem::path(filename).parent_path();

    LOG_ASSERT_MESSAGE_FATAL(!arc.hasFile("--scene-properties--"), "Cannot Deserialize: Invalid File");
    auto pbf = arc.getBinaryFile("--scene-properties--");

    uint32_t entity_count, resource_count;
    pbf.get<uint32_t>(&entity_count);
    pbf.get<uint32_t>(&resource_count);

    // Entities will be created on the fly
    // Which is why heavy data should _always_ be in a resource
    // But, we are going to load their data
    entity_done = std::vector<bool>(entity_count, false);
    entitys.resize(entity_count);
    for (int i = 0; i < entity_count; i++)
    {
        LOG_ASSERT_MESSAGE_FATAL(!arc.hasFile("Entity-" + std::to_string(i)), "Cannot Deserialize: Invalid File");

        auto bf = arc.getBinaryFile("Entity-" + std::to_string(i));
        uint32_t num_components;
        bf.get(&num_components);

        EntityData e;
        e.component_data = std::map<std::string, FluxArc::BinaryFile*>();
        e.id = i;

        // Format: Name, Size, char* Data
        for (int j = 0; j < num_components; j++)
        {
            std::string name = bf.get();
            uint32_t size = 0;
            bf.get(&size);
            char* data = new char[size];
            bf.get(data, size);

            auto bnf = new FluxArc::BinaryFile(data, size);
            e.component_data[name] = bnf;
        }

        entities.push_back(e);
    }

    // Create Resources
    resource_done = std::vector<bool>(resource_count, 0);
    resources.resize(resource_count);

    for (int i = 0; i < resource_count; i++)
    {
        getResource(i);
    }

}

Deserializer::~Deserializer()
{
    for (auto i : entities)
    {
        for (auto j : i.component_data)
        {
            delete j.second;
        }
    }
}

void Deserializer::destroyResources()
{
    for (auto i : resources)
    {
        removeResource(i);
    }
}

ResourceRef<Resource> Deserializer::getResource(uint32_t id)
{
    if (resource_done[id])
    {
        return resources[id];
    }

    // Create a new resource
    auto bf = arc.getBinaryFile("Resource-" + std::to_string(id));

    std::string name = bf.get();
    uint32_t size;
    bf.get(&size);

    if (size == -1)
    {
        // Resource didn't want to be serialized?
        LOG_WARN("Warning: Resource was not properly serialized");
        return ResourceRef<Resource>();
    }

    // Initialize
    auto res = Resources::createResource(Resources::resource_factory_map[name]());
    auto data = new char[size];
    bf.get(data, size);
    auto en = FluxArc::BinaryFile(data, size);
    res->deserialize(this, &en);

    resource_done[id] = true;
    resources[id] = res;
    return res;
}

std::vector<Flux::EntityRef> Deserializer::addToECS(ECSCtx *ctx)
{
    entity_done = {};
    entitys = {};
    
    current_ctx = ctx;
    std::vector<EntityRef> output = {};
    for (auto i : entities)
    {
        output.push_back(getEntity(i.id));
    }

    return output;
}

Flux::EntityRef Deserializer::getEntity(int id)
{
    if (entity_done[id])
    {
        return entitys[id];
    }

    auto components = entities[id];
    auto entity = current_ctx->createEntity();

    for (auto i : components.component_data)
    {
        auto new_com = Flux::component_factory[i.first]();
        new_com->deserialize(this, i.second);

        // Make sure cursor is at the start
        i.second->setCursor(0);

        // We have to use the old method because we can't use the templated function
        // If we don't know the freaking type!
        current_ctx->_addComponent(entity.getEntityID(), Flux::getComponentType(i.first), new_com);
    }

    entitys[id] = entity;
    entity_done[id] = true;
    
    return entity;
}