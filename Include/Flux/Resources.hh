#ifndef FLUX_RESOURCES_HH
#define FLUX_RESOURCES_HH
#include <fstream>
#include <string>
#ifndef FLUX_MAX_RESOURCES
#define FLUX_MAX_RESOURCES 4096
#endif

// Flux includes
#include "Flux/ECS.hh"

// STL includes
#include <queue>

namespace Flux { namespace Resources {

    typedef int ResourceID;

    /**
    Little secret: The resource system is just an ECS.
    That being said, usually there would only be one component, the resource, per Entity.
    There would be other components, but those are usually added by other systems such as the renderer
    */
    struct Resource: Component
    {

    };

    /**
    The ECS that powers the resource system
    */
    extern ECSCtx rctx;

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
    ResourceID createResource(Resource* res);

    /**
    Deletes the given resource. Make sure nothing is using it, as it will be freed
    */
    void removeResource(ResourceID res);

    /**
    Returns the actual data from the given resource.
    */
    Resource* getResource(ResourceID res);

    /**
    Loads a file and returns the ResourceID.
    File is relative. We recommend setting your working directory to the root of your project
    */
    ResourceID loadResourceFromFile(const std::string& filename);

    /**
    Registers file extension with the resource system.
    When a file with the given extension is loaded, the given function will be called. Extension does not include "."
    */
    void addFileLoader(const std::string& ext, Flux::Resources::Resource*(*function)(std::ifstream*));

    // ==================================
    // Text resource
    // ==================================
    struct TextResource: Resource
    {
        std::string content;
    };

    /**
    Creates a text resource from the given text
    */
    ResourceID createTextResource(const std::string& text);

    /**
    Creates a text resource from a file. Default resource generator
    */
    Resource* ext_createTextResource(std::ifstream* file);

}}

#endif