// #include "Flux/Threads.hh"
// #include "Flux/ECS.hh"
// #include "Flux/Log.hh"

// #include <thread>

// using namespace Flux::Threads;

// /** Store the threads so we can properly dispose of them */
// static std::vector<std::thread> worker_threads;


// // Helper functions
// void workerThread(ThreadCtx* tctx, int id);
// void resetDoneArray(ThreadCtx* tctx);

// ThreadCtx* Flux::Threads::startThreads()
// {
//     ThreadCtx* tctx = new ThreadCtx;

//     // Create worker threads
//     unsigned long num_threads = std::thread::hardware_concurrency();
//     if (num_threads == 0)
//     {
//         // Default to 4 threads
//         num_threads = 4;
//     }

//     tctx->threads = num_threads;
//     tctx->delimiter = num_threads;

//     // Create done array
//     tctx->done = new bool[num_threads];
//     resetDoneArray(tctx);

//     // Setup other variables
//     tctx->executing = false;
//     tctx->system = nullptr;
//     tctx->alive = true;

//     worker_threads = std::vector<std::thread>();

//     // Create threads
//     for (int i = 0; i < num_threads; i++)
//     {
//         worker_threads.push_back(std::thread(workerThread, tctx, i));
//     }

//     return tctx;
// }

// bool Flux::Threads::destroyThreads(ThreadCtx *tctx)
// {
//     resetDoneArray(tctx);

//     // Tell the workers to stop
//     tctx->alive = false;

//     // Wait for them to stop
//     waitForThreads(tctx);

//     // Merge them all back
//     // for (int i = 0; i < worker_threads.size(); i ++)
//     // {
//     //     if (worker_threads[i].joinable())
//     //     {
//     //         worker_threads[i].join();
//     //     }
//     // }

//     // Free done list
//     delete[] tctx->done;

//     // Free tctx
//     delete tctx;

//     return true;
// }

// void Flux::Threads::runThreads(ThreadCtx *tctx, Flux::ECSCtx *ctx, Flux::System *system, float delta)
// {
//     // Set the states
//     tctx->ctx = ctx;
//     tctx->system = system;
//     tctx->delta = delta;

//     // Reset the done array
//     resetDoneArray(tctx);

//     // And let them loose
//     tctx->executing = true;
// }

// void Flux::Threads::waitForThreads(ThreadCtx *tctx)
// {
//     int trues;
//     while (true)
//     {
//         trues = 0;
//         for (int i = 0; i < tctx->threads; i++)
//         {
//             if (tctx->done[i]  == true)
//             {
//                 trues++;
//             }
//         }

//         if (trues == tctx->threads)
//         {
//             break;
//         }
//     }
//     tctx->executing = false;
// }

// void workerThread(ThreadCtx* tctx, int id)
// {
//     while (tctx->alive == true)
//     {
//         // Wait until called
//         while ((tctx->executing == false || tctx->done[id] == true) && tctx->alive == true) {

//         }

//         if (tctx->alive == false)
//         {
//             break;
//         }

//         // Called!
//         // Execute system on entities
//         for (int i = id; i < FLUX_MAX_ENTITIES; i+=tctx->delimiter)
//         {
//             if (tctx->ctx->entities[i] != nullptr)
//             {
//                 tctx->system->function(tctx->ctx, i, tctx->delta);
//             }
//         }

//         // Tell the main thread we're done
//         tctx->done[id] = true;

//         // Wait for everything to be done
//         // while (tctx->executing == true && tctx->alive == true) {}
//     }

//     // Tell the main thread we're done shutting down
//     tctx->done[id] = true;
// }

// void resetDoneArray(ThreadCtx* tctx)
// {
//     for (int i = 0; i < tctx->threads; i++)
//     {
//         tctx->done[i] = false;
//     }
// }