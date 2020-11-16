/**
 * This file is where the engine is initialised. 
 * The user should define the init function in Flux.hh
 */

#include "Flux/Flux.hh"
#include "Flux/Threads.hh"
// #include "Flux/Threads.hh"

#ifndef FLUX_NO_THREADING
Flux::Threads::ThreadCtx* Flux::threading_context = Flux::Threads::startThreads();
#endif

int main(int argc, char *argv[], char *envp[])
{
    init();

    Flux::Threads::destroyThreads(Flux::threading_context);
}

