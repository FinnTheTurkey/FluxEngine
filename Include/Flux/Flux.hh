#ifndef FLUX_HH
#define FLUX_HH

#include "Flux/ECS.hh"
#include "Flux/Resources.hh"
#include "Flux/Renderer.hh"

// #include "Flux/Threads.hh"

/**
 * Basic file which includes all the important components of flux
 * It also defined the entry point
 */

namespace Flux
{
    // extern Threads::ThreadCtx* threading_context;

    void setMainLoopFunction(void (*fun)());

    void runMainloop();
}

void init(int argc, char *argv[]);

void loop(float delta);

void end();

#endif