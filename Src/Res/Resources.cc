#include "Flux/Resources.hh"
#include "Flux/ECS.hh"
#include "Flux/Log.hh"
#include <cstring>
#include <fstream>
#include <map>
#include <string>

Flux::ECSCtx* Flux::Resources::rctx = nullptr;

Flux::ComponentTypeID ResourceComponentType;

void Flux::Resources::initialiseResources()
{
    if (rctx != nullptr)
    {
        LOG_WARN("Tryied to initialize Resource system twice!");
    }
    rctx = Flux::createContext();
    
    ResourceComponentType = Flux::getComponentType("resource");
}

void Flux::Resources::destroyResources()
{
    Flux::destroyContext(rctx);
}

Flux::Resources::ResourceID Flux::Resources::createResource(Resource *res)
{
    ResourceID res_id = Flux::createEntity(rctx);
    Flux::addComponent(rctx, res_id, ResourceComponentType, res);

    return res_id;
}

void Flux::Resources::removeResource(ResourceID res)
{
    Flux::destroyEntity(rctx, res);
}

Flux::Resources::Resource* Flux::Resources::getResource(ResourceID res)
{
    return (Resource*)Flux::getComponent(rctx, res, ResourceComponentType);
}

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

Flux::Resources::ResourceID Flux::Resources::loadResourceFromFile(const std::string &filename)
{
    std::string extension = get_file_ext(filename);

    if (extension_db.find(extension) != extension_db.end())
    {
        // Use function from extensiondb
        std::ifstream* myfile = new std::ifstream(filename);
        ResourceID id = createResource(extension_db[extension](myfile));

        myfile->close();
        delete myfile;
        return id;
    }
    else
    {
        // Use text resource
        std::ifstream* myfile = new std::ifstream(filename);
        ResourceID id = createResource(ext_createTextResource(myfile));

        myfile->close();
        delete myfile;
        return id;
    }
}

Flux::Resources::ResourceID Flux::Resources::createTextResource(const std::string& text)
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