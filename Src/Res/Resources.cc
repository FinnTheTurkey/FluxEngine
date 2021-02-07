#include "Flux/Resources.hh"
#include "Flux/ECS.hh"
#include "Flux/Log.hh"
#include "Flux/Renderer.hh"
#include <cstring>
#include <fstream>
#include <map>
#include <string>

Flux::ECSCtx* Flux::Resources::rctx = nullptr;

void Flux::Resources::initialiseResources()
{
    if (rctx != nullptr)
    {
        LOG_WARN("Tried to initialize Resource system twice!");
    }
    rctx = new ECSCtx();
}

void Flux::Resources::destroyResources()
{
    rctx->destroyAllEntities();
}

void Flux::Resources::createSceneLink(EntityRef entity, const std::string& scene)
{
    auto slc = new SceneLinkCom;
    slc->filename = scene;
    entity.addComponent(slc);

    instanciateSceneLink(entity);
}

void Flux::Resources::instanciateSceneLink(EntityRef entity)
{
    auto fname = entity.getComponent<SceneLinkCom>()->filename;
    
    auto scene = deserialize(fname);
    auto output = scene->addToECS(entity.getCtx());

    if (entity.hasComponent<Flux::Transform::TransformCom>())
    {
        // Make it the parent of all unparented entities
        for (auto i : output)
        {
            if (i.hasComponent<Flux::Transform::TransformCom>())
            {
                auto tc = i.getComponent<Flux::Transform::TransformCom>();
                if (tc->has_parent == false)
                {
                    Flux::Transform::setParent(i, entity);
                }
            }
        }
    }
}

// Flux::Resources::ResourceRef Flux::Resources::createResource(Resource *res)
// {
//     ResourceID res_id = Flux::createEntity(rctx);
//     Flux::addComponent(rctx, res_id, ResourceComponentType, res);

//     return res_id;
// }

// void Flux::Resources::removeResource(ResourceID res)
// {
//     Flux::destroyEntity(rctx, res);
// }

// Flux::Resources::Resource* Flux::Resources::getResource(ResourceID res)
// {
//     return (Resource*)Flux::getComponent(rctx, res, ResourceComponentType);
// }

// https://stackoverflow.com/a/26822961/7179625
size_t find_ext_idx(const std::string& filename)
{
    const char* fileName = filename.c_str();
    size_t len = strlen(fileName);
    size_t idx = len-1;
    for(size_t i = 0; *(fileName+i); i++) {
        if (*(fileName+i) == '.') {
            idx = i;
        } else if (*(fileName + i) == '/' || *(fileName + i) == '\\') {
            idx = len - 1;
        }
    }
    return idx+1;
}

std::string get_file_ext(const std::string& fileName)
{
    return fileName.substr(find_ext_idx(fileName));
}

std::map<std::string, Flux::Resources::Resource*(*)(std::ifstream*)> extension_db;

void Flux::Resources::addFileLoader(const std::string &ext, Flux::Resources::Resource *(*function)(std::ifstream*))
{
    extension_db[ext] = function;
}

Flux::Resources::ResourceRef<Flux::Resources::Resource> Flux::Resources::_loadResourceFromFile(const std::string &filename)
{
    std::string extension = get_file_ext(filename);

    if (extension_db.find(extension) != extension_db.end())
    {
        // Use function from extensiondb
        std::ifstream* myfile = new std::ifstream(filename);
        ResourceRef<Resource> id = createResource<Resource>(extension_db[extension](myfile));

        myfile->close();
        delete myfile;
        return id;
    }
    else
    {
        // Use text resource
        std::ifstream* myfile = new std::ifstream(filename);
        ResourceRef<Resource> id = createResource<Resource>(ext_createTextResource(myfile));

        myfile->close();
        delete myfile;
        return id;
    }
}

Flux::Resources::ResourceRef<Flux::Resources::TextResource> Flux::Resources::createTextResource(const std::string& text)
{
    auto tr = new TextResource;
    tr->content = text;
    return createResource(tr);
}

Flux::Resources::Resource* Flux::Resources::ext_createTextResource(std::ifstream *file)
{
    std::string line;
    std::string output = "";
    if (file->is_open())
    {
        while ( getline (*file,line) )
        {
            output += line + '\n';
        }
        file->close();
    }

    auto tr = new TextResource;
    tr->content = output;
    return tr;
}