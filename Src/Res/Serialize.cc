#include "Flux/ECS.hh"
#include "Flux/Log.hh"
#include "Flux/Resources.hh"
#include "FluxArc/FluxArc.hh"
#include <algorithm>
#include <filesystem>
#include <queue>
#include <string>

using namespace Flux::Resources;

Serializer::Serializer(const std::string& filename)
: entities(), resources(), name_count(), fname(filename)
{

}

uint32_t Serializer::addEntity(EntityRef entity, uint32_t ihid)
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

uint32_t Serializer::addResource(ResourceRef<Resource> res, uint32_t ihid)
{
    if (std::find(resources.begin(), resources.end(), res) != resources.end())
    {
        // Return index
        auto r = std::find(resources.begin(), resources.end(), res);
        return r - resources.begin();
    }

    // Add to vector
    resources.push_back(res);

    if (ihid == 0)
    {
        // Automatically assign Inheritance ID
        ihid = resources.size();
    }
    resource_ihids.push_back(ihid);

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

        if (resources[j].getBaseEntity().hasComponent<SerializeCom>())
        {
            // It came from a file, so just link back to that one
            en.set(true);
            auto sc = resources[j].getBaseEntity().getComponent<SerializeCom>();

            // Make sure the path is relative to the current file
            std::string relative_fname = std::filesystem::relative(sc->inherited_file, std::filesystem::path(fname).parent_path());
            en.set(relative_fname);
            en.set(sc->inheritance_id);
        }
        else
        {
            en.set(false);
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
        }

        arc.setFile("Resource-" + std::to_string(j), en, true, release);

        j++;
        it2++;
    }

    // Serialize Inheritance ID Table
    FluxArc::BinaryFile ihid_table;

    // Pre-allocate to save time
    ihid_table.allocate(sizeof(uint32_t) * 2 * resources.size());

    for (uint32_t i = 0; i < resources.size(); i++)
    {
        ihid_table.set(i);
        ihid_table.set(resource_ihids[i]);
    }

    arc.setFile("--ihid-table--", ihid_table);
    // TODO: Deal with link resources

    FluxArc::BinaryFile properties;

    // Set # of entities and # of resources
    properties.set((uint32_t)entities.size());
    properties.set((uint32_t)resources.size());

    arc.setFile("--scene-properties--", properties);
}

Deserializer* Flux::Resources::deserialize(const std::string& filename, bool reload)
{
    if (loaded_files.find(filename) != loaded_files.end())
    {
        // We have already loaded this file
        return loaded_files[filename];
    }

    auto ds = new Deserializer(filename, reload);
    loaded_files[filename] = ds;

    return ds;
}

Deserializer::Deserializer(const std::string& filename, bool unlinked_res)
:arc(filename), total_references(0), created(false), unlinked(unlinked_res)
{
    dir = std::filesystem::path(filename).parent_path();
    fname = std::filesystem::path(filename);

    LOG_ASSERT_MESSAGE_FATAL(!arc.hasFile("--scene-properties--"), "Cannot Deserialize: Invalid File");
    auto pbf = arc.getBinaryFile("--scene-properties--");

    uint32_t entity_count, resource_count;
    pbf.get<uint32_t>(&entity_count);
    pbf.get<uint32_t>(&resource_count);

    // Load IHID Table
    auto ihid_bf = arc.getBinaryFile("--ihid-table--");

    for (int i = 0; i < resource_count; i++)
    {
        uint32_t resid, ihid;
        ihid_bf.get(&resid);
        ihid_bf.get(&ihid);

        ihid_table[ihid] = resid;
    }

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

    arc = FluxArc::Archive();
    LOG_INFO("Deserialized file " + filename);

    created = true;
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

    LOG_INFO("Destroyed file " + fname.string());
}

void Deserializer::destroyResources()
{
    auto file = std::filesystem::absolute(fname);
    for (auto i : resources)
    {
        if (i.getBaseEntity().getComponent<SerializeCom>()->inherited_file == file)
        {
            // Removing a resource basically causes a double free
            // removeResource(i);
        }
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

    // Check if it's linked
    bool lk;
    bf.get(&lk);

    if (lk)
    {
        // It's a resource from another file
        std::string filename = bf.get();
        auto real_fname = dir / filename;

        uint32_t ihid;
        bf.get(&ihid);

        auto ser = deserialize(real_fname);
        auto res = ser->getResourceByIHID(ihid);

        resource_done[id] = true;
        resources[id] = res;

        return res;
    }
    else
    {
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
        auto res = Resources::resource_factory_map[name]();
        auto data = new char[size];
        bf.get(data, size);
        auto en = FluxArc::BinaryFile(data, size);
        res->deserialize(this, &en);

        // Find ihid
        // TODO: Maybe do this a more efficient way?
        uint32_t ihid = 0;
        for (auto i : ihid_table)
        {
            if (i.second == id)
            {
                ihid = i.first;
            }
        }
        
        if (!unlinked)
        {
            // Add identifier tag
            SerializeCom* s = new SerializeCom;
            s->inheritance_id = ihid;
            s->inherited_file = std::filesystem::absolute(fname);
            s->parent_file = this;

            auto real_res = Resources::createResource(res);
            real_res.getBaseEntity().addComponent(s);

            // Hack to get this to actually work:
            // TODO: Go this in a less hacky way
            real_res.from_file = true;
            addRef();

            resource_done[id] = true;
            resources[id] = real_res;
            return real_res;
        }
        else
        {
            auto real_res = Resources::createResource(res);
            
            resource_done[id] = true;
            resources[id] = real_res;
            return real_res;
        }

        
    }
}

ResourceRef<Resource> Deserializer::getResourceByIHID(uint32_t ihid)
{
    return getResource(ihid_table[ihid]);
}

std::vector<Flux::EntityRef> Deserializer::addToECS(ECSCtx *ctx)
{
    entity_done = std::vector<bool>(entities.size(), false);
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

    // Initialise scene links
    if (entity.hasComponent<SceneLinkCom>())
    {
        instanciateSceneLink(entity);
    }

    entitys[id] = entity;
    entity_done[id] = true;
    
    return entity;
}

void Deserializer::addRef()
{
    total_references ++;
}

void Deserializer::subRef()
{
    total_references --;

    if (!created)
    {
        // Don't destroy ourselves because all our resources haven't been loaded yet
        return;
    }

    if (total_references <= resources.size())
    {
        // The only references to all our resources is us
        // Which means time to free :(
        freeFile(fname); // wrong remove!
    }
}

void Flux::Resources::freeFile(const std::string &filename)
{
    // Resources should be freed automatically
    delete[] loaded_files[filename];

    // And delete the map entry
    loaded_files.erase(filename);
}