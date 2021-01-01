/**
 * This file is where the engine is initialised. 
 * The user should define the init function in Flux.hh
 */

#include "Flux/Flux.hh"
#include "Flux/Renderer.hh"
#include "Flux/Threads.hh"
#include "Flux/OpenGL/GLRenderer.hh"

#include "Flux/Resources.hh"

// #ifndef FLUX_NO_THREADING
// Flux::Threads::ThreadCtx* Flux::threading_context = Flux::Threads::startThreads();
// #endif

float last_time;

void mainloop()
{
    Flux::GLRenderer::startFrame();
    float delta = (Flux::Renderer::getTime() - last_time);
    last_time = Flux::Renderer::getTime();
    loop(delta);
    Flux::GLRenderer::endFrame();
}

int main(int argc, char *argv[], char *envp[])
{
    Flux::Resources::initialiseResources();
    Flux::GLRenderer::createWindow(800, 600, "Hello Flux");
    last_time = Flux::Renderer::getTime();
    Flux::setMainLoopFunction(mainloop);
    init(argc, argv);

    // Mainloop
    Flux::runMainloop();

    // It is the responsability of the window to call end();
    // end();

    // // Destroy resources before windows so opengl resource cleanup
    // // doesn't cause a segfault 'cause the window is gone
    // Flux::Resources::destroyResources();
    // Flux::GLRenderer::destroyWindow();

    // #ifndef FLUX_NO_THREADING
    // Flux::Threads::destroyThreads(Flux::threading_context);
    // #endif
}
