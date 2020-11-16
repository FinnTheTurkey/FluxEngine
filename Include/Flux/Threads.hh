#ifndef FLUX_THREADS_HH
#define FLUX_THREADS_HH

#include "Flux/ECS.hh"

namespace Flux { namespace Threads
{
    /**
    The context for Flux's threading. This struct should **only* be modified by the main thread.
    The worker threads should **never** change the ThreadCtx. Except for the done array
    */
    struct ThreadCtx
    {
        /** Tells the thread when it should stop */
        bool alive;

        /** Tells the threads if they should be executing */
        bool executing;

        /** Which systems need to be run */
        System* system;

        /** ECS Context to operate on */
        ECSCtx* ctx;

        /** Delta, because systems require it */
        float delta;

        /** Tells each thread how many entities to skip. 
        For example, with 4 threads, the delemiter would be 4, meaning that thread one would do entity 0, then 4, then 8, etc...
        */
        int delimiter;

        /** Number of worker threads */
        int threads;

        /** Dynamically allocated array. This is how threads tell the main thread that they're done. */
        bool* done;
    };

    /** Creates a threading context, as well as an optimal number of worker threads */
    ThreadCtx* startThreads();

    /** Frees the Threading context, and releases the threads */
    bool destroyThreads(ThreadCtx* tctx);

    /** Makes the worker threads run the given system */
    void runThreads(ThreadCtx* tctx, ECSCtx* ctx, System* system, float delta);

    /** Waits until the worker threads are done processing the given system */
    void waitForThreads(ThreadCtx* tctx);

} }

#endif