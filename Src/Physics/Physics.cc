#include "Flux/Physics.hh"
#include "Flux/Debug.hh"
#include "Flux/Log.hh"
#include "Flux/Renderer.hh"
#include "Flux/Resources.hh"

// TODO: Remove ASAP
#include "Flux/OpenGL/GLRenderer.hh"
#include "glm/detail/func_geometric.hpp"
#include "glm/detail/type_vec.hpp"
#include <algorithm>
#include <cfloat>

using namespace Flux::Physics;

/* ================================================== */
// Bounding boxes
/* ================================================== */

// ==================================================
// Segmented Interval List
// ==================================================

DS::SegmentedIntervalList::SegmentedIntervalList(const int index):
index(index)
{
    // Create starting chunk
    linked_list = new Chunk;
    linked_list->extrema_amount = 0;
    linked_list->next = nullptr;
    linked_list->index = 0;

    array.push_back(linked_list);
}

DS::SegmentedIntervalList::~SegmentedIntervalList()
{
    for (auto i : array)
    {
        delete i;
    }
}

int DS::SegmentedIntervalList::addExtrema(DS::ExtremaType type, float extrema, BoundingBox* box)
{
    std::vector<Chunk*>::iterator extrema_it;
    // if (type == Minima)
    // {
    extrema_it = std::lower_bound(array.begin(), array.end(), extrema,
                [](Chunk*& info, const float& value) {
        if (info->extrema_amount == 0)
        {
            // Empty chunk
            return false;
        }
        return value < info->extrema[0].pos;
    });
    // }
    // else
    // {
        // Why did I do this???
        // extrema_it = std::upper_bound(array.begin(), array.end(), extrema,
        //         [](int value, Chunk*& info) {
        //     if (info->extrema_amount == 0)
        //     {
        //         // Empty chunk
        //         return true;
        //     }
        //     return value < info->extrema[info->extrema_amount-1].pos;
        // });
    // }
    if (extrema_it == array.end())
    {
        // If no good chunk is found, then it's probably because
        // the value is smaller than the smallest chunk
        // so putting it in the _biggest_ chunk was a bad idea
        extrema_it = array.begin();
    }
    auto extrema_index = extrema_it - array.begin();

    // Insert the minima and maxima into their respective chunks
    if (array[extrema_index]->extrema_amount == FLUX_SIL_CHUNK_SIZE)
    {
        // Split the chunk
        auto chunk = array[extrema_index];
        auto new_chunk = new Chunk;
        new_chunk->extrema_amount = 0;
        new_chunk->next = nullptr;
        auto start_amount = chunk->extrema_amount;
        for (int i = start_amount/2; i < start_amount; i++)
        {
            chunk->extrema_amount --;
            LOG_ASSERT_MESSAGE_FATAL(chunk->extrema_amount < 0, "Negative chunk?????");
            new_chunk->extrema_amount ++;

            new_chunk->extrema[i - start_amount/2] = chunk->extrema[i];
            if (chunk->extrema[i].type == ExtremaType::Minima)
            {
                new_chunk->extrema[i - start_amount/2].box->storage[index].minima_chunk = new_chunk;
                new_chunk->extrema[i - start_amount/2].box->storage[index].minima_chunk_index = i - start_amount/2;
                LOG_ASSERT_MESSAGE_FATAL(new_chunk->extrema[i - start_amount/2].box->storage[index].minima_chunk_index >= new_chunk->extrema_amount, "Bad bad bad");
            }
            else
            {
                new_chunk->extrema[i - start_amount/2].box->storage[index].maxima_chunk = new_chunk;
                new_chunk->extrema[i - start_amount/2].box->storage[index].maxima_chunk_index = i - start_amount/2;
                LOG_ASSERT_MESSAGE_FATAL(new_chunk->extrema[i - start_amount/2].box->storage[index].maxima_chunk_index >= new_chunk->extrema_amount, "Bad bad bad");
            }

            chunk->extrema[i].box = nullptr;
            chunk->extrema[i].pos = 0;
            chunk->extrema[i].type = Neither;
        }

        // WARNING: If anything goes wrong, it's probably because of this
        // if (extrema_it != array.end())
        // {
        auto new_it = array.insert(extrema_it+1, new_chunk);
        // }
        // else
        // {
        //     array.insert(extrema_it, new_chunk);
        // }

        for (int i = new_it - array.begin(); i < array.size(); i++)
        {
            // This could probably be done more efficiently...
            array[i]->index = i;
        }

        // Figure out where the new extrema fits in
        if (extrema > chunk->extrema[chunk->extrema_amount].pos)
        {
            // The new extrema goes into the new chunk
            extrema_index++;
            extrema_it++;
        }

        // Any checkpoints for the original chunk
        // should also be in the new chunk
        new_chunk->checkpoint = chunk->checkpoint;
    }

    // Finally, actually add the minima
    auto chunk = array[extrema_index];

    // Don't check all the way to the end, because there may be garbage values
    auto chunk_extrema_index = std::upper_bound(chunk->extrema.begin(), chunk->extrema.begin() + chunk->extrema_amount, extrema, 
                [](const float& value, Extrema& info) {
        return value < info.pos;
    }) - chunk->extrema.begin();

    LOG_ASSERT_MESSAGE_FATAL(chunk_extrema_index > chunk->extrema_amount, "New index too big");

    // Clear a space
    for (int i = chunk->extrema_amount-1; i >= chunk_extrema_index; i--)
    {
        chunk->extrema[i+1] = chunk->extrema[i];
        if (chunk->extrema[i+1].type == ExtremaType::Minima)
        {
            chunk->extrema[i+1].box->storage[index].minima_chunk_index = i+1;
            LOG_ASSERT_MESSAGE_FATAL(chunk->extrema[i+1].box->storage[index].minima_chunk_index > chunk->extrema_amount, "Index is bigger than the array");
        }
        else
        {
            chunk->extrema[i+1].box->storage[index].maxima_chunk_index = i+1;
            LOG_ASSERT_MESSAGE_FATAL(chunk->extrema[i+1].box->storage[index].maxima_chunk_index > chunk->extrema_amount, "Index is bigger than the array");
        }
    }

    // Insert it
    chunk->extrema[chunk_extrema_index] = Extrema {type, box, extrema};
    chunk->extrema_amount ++;

    // Add metadata
    if (type == Minima)
    {
        box->storage[index].minima_chunk = chunk;
        box->storage[index].minima_chunk_index = chunk_extrema_index;
    }
    else
    {
        box->storage[index].maxima_chunk = chunk;
        box->storage[index].maxima_chunk_index = chunk_extrema_index;
    }

    return extrema_index;
}

void DS::SegmentedIntervalList::addBoundingBox(BoundingBox *box, float minima, float maxima)
{
    LOG_ASSERT_MESSAGE_FATAL(minima > maxima, "Minima is greater than maxima. TF????");
    auto minima_index = addExtrema(Minima, minima, box);
    auto maxima_index = addExtrema(Maxima, maxima, box);

    // Add a checkpoint to all the chunks in between
    for (int i = minima_index+1; i < maxima_index; i++)
    {
        if (i < array.size())
        {
            array[i]->checkpoint.push_back({Neither, box, 0.0});
        }
    }
}

int DS::SegmentedIntervalList::removeExtrema(DS::ExtremaType type, float extrema, BoundingBox* box)
{
    Chunk* chunk;
    int to_remove = -1;
    if (type == Maxima)
    {
        LOG_ASSERT_MESSAGE_FATAL(box->storage[index].maxima_chunk == nullptr, "Attempting to remove already removed extrema");
        chunk = box->storage[index].maxima_chunk;
        to_remove = box->storage[index].maxima_chunk_index;
    }
    else
    {
        LOG_ASSERT_MESSAGE_FATAL(box->storage[index].minima_chunk == nullptr, "Attempting to remove already removed extrema");
        chunk = box->storage[index].minima_chunk;
        to_remove = box->storage[index].minima_chunk_index;
    }

    LOG_ASSERT_MESSAGE_FATAL(to_remove > chunk->extrema_amount-1, "Cannot remove an index that doesn't exist");

    // int to_remove = -1;

    // Find the extrema
    // for (int i = 0; i < chunk->extrema_amount; i++)
    // {
    //     if (chunk->extrema[i].box == box && chunk->extrema[i].type == type)
    //     {
    //         to_remove = i;
    //     }
    // }

    if (to_remove == -1)
    {
        // ...
        LOG_ERROR("Something broke. Could not find extrema in chunk.");
        return 0;
    }

    // Remove it
    for (int i = to_remove; i < chunk->extrema_amount-1; i++)
    {
        chunk->extrema[i] = chunk->extrema[i+1];
        if (chunk->extrema[i].type == Minima)
        {
            chunk->extrema[i].box->storage[index].minima_chunk_index = i;
        }
        else
        {
            chunk->extrema[i].box->storage[index].maxima_chunk_index = i;
        }
    }

    // For debugging purposes, clear the extra item
    // This shouldn't be nessesary, but I'm doing it anyways
    chunk->extrema[chunk->extrema_amount-1].box = nullptr;
    chunk->extrema[chunk->extrema_amount-1].pos = 0;
    chunk->extrema[chunk->extrema_amount-1].type = Neither;

    chunk->extrema_amount --;
    LOG_ASSERT_MESSAGE_FATAL(chunk->extrema_amount < 0, "Negative chunk?????");

    if (chunk->extrema_amount < 1 && array.size() > 1)
    {
        // Remove the chunk
        auto before_index = chunk->index;
        array.erase(array.begin() + chunk->index);
        delete chunk;

        // Now fix the indexes
        for (int i = before_index; i < array.size(); i++)
        {
            // This could probably be done more efficiently...
            array[i]->index = i;
        }
    }

    if (type == Maxima)
    {
        box->storage[index].maxima_chunk = nullptr;
        box->storage[index].maxima_chunk_index = 0;
    }
    else
    {
        box->storage[index].minima_chunk = nullptr;
        box->storage[index].minima_chunk_index = 0;
    }

    return 0;
}

void DS::SegmentedIntervalList::removeBoundingBox(BoundingBox *box, float minima, float maxima)
{
    // Remove checkpoints
    for (int i = box->storage[index].minima_chunk->index+1; i < box->storage[index].maxima_chunk->index; i++)
    {
        if (i < array.size())
        {
            // We have to go through each one to find the index we're looking for
            int count = -1;
            for (auto ch: array[i]->checkpoint)
            {
                if (ch.box == box)
                {
                    break;
                }
                count++;
            }
            if (count == -1)
            {
                // TODO: Figure out why this was happening
                continue;
            }
            LOG_ASSERT_MESSAGE_FATAL(count == -1, "Could not find checkpoint which _should_ exist");
            array[i]->checkpoint.erase(array[i]->checkpoint.begin() + count);
        }
    }

    removeExtrema(Minima, minima, box);
    removeExtrema(Maxima, maxima, box);
}

static uint64_t pass = 1;

void DS::SegmentedIntervalList::addItemToCollisionList(BoundingBox* box, int i, std::vector<BoundingBox*>& collisions)
{
    if (box == nullptr)
    {
        return;
    }

    if (box->pass < pass)
    {
        collisions.push_back(box);
        box->pass = pass;
        box->collisions[0] = false;
        box->collisions[1] = false;
        box->collisions[2] = false;
        box->collisions[index] = true;
    }
    else
    {
        // Make sure we haven't collided yet
        if (box->collisions[index] == true)
        {
            return;
        }
        
        // Make sure it's collided in our previous tests
        bool all_good = true;
        for (auto in = 0; in < index; in ++)
        {
            if (!box->collisions[in])
            {
                all_good = false;
                break;
            }
        }

        if (all_good)
        {
            box->collisions[index] = true;
            collisions.push_back(box);
        }
    }
}

std::vector<BoundingBox*> DS::SegmentedIntervalList::getCollidingBoxes(BoundingBox* box)
{
    std::vector<BoundingBox*> collisions;
    int minima_chunk = box->storage[index].minima_chunk->index;
    int maxima_chunk = box->storage[index].maxima_chunk->index;

    // If the minima and maxima are in the same chunk,
    // We can save quite a bit of processing time
    if (minima_chunk == maxima_chunk)
    {
        // Anything between these two will collide
        Chunk* chunk = box->storage[index].minima_chunk;

        // Use resize so we only have 2 allocations
        const int sp = box->storage[index].minima_chunk_index+1;
        // collisions.resize(box->storage[index].maxima_chunk_index - box->storage[index].minima_chunk_index);
        for (int i = sp; i < box->storage[index].maxima_chunk_index; i++)
        {
            // collisions.push_back(chunk->extrema[i].box);
            // collisions[i - sp] = chunk->extrema[i].box;
            addItemToCollisionList(chunk->extrema[i].box, i, collisions);
        }

        // Add checkpoints
        // Use a for loop because they're different types
        const int og_size = collisions.size();
        // collisions.resize(collisions.size() + chunk->checkpoint.size());
        for (int i = 0; i < chunk->checkpoint.size(); i++)
        {
            // collisions[og_size + i] = chunk->checkpoint[i].box;
            addItemToCollisionList(chunk->checkpoint[i].box, i, collisions);
        }

        return collisions;
    }

    // We unfortunately now have to do some more complex stuff

    // First chunk
    {
        Chunk* first_chunk = box->storage[index].minima_chunk;
        // collisions.resize(first_chunk->extrema_amount - box->storage[index].minima_chunk_index);

        const int sp = box->storage[index].minima_chunk_index+1;
        for (int i = sp; i < first_chunk->extrema_amount; i++)
        {
            // collisions.push_back(chunk->extrema[i].box);
            addItemToCollisionList(first_chunk->extrema[i].box, i, collisions);
            // collisions[i - sp] = first_chunk->extrema[i].box;
        }

        // Add checkpoints
        // Use a for loop because they're different types
        const int og_size = collisions.size();
        // collisions.resize(collisions.size() + first_chunk->checkpoint.size());
        for (int i = 0; i < first_chunk->checkpoint.size(); i++)
        {
            // collisions[og_size + i] = first_chunk->checkpoint[i].box;
            addItemToCollisionList(first_chunk->checkpoint[i].box, i, collisions);
        }
    }

    // Middle chunks (if any)
    for (int chi = minima_chunk+1; chi < maxima_chunk; chi++)
    {
        Chunk* chunk = array[chi];
        const int sp = collisions.size();
        // collisions.resize(collisions.size() + chunk->extrema_amount);
        for (int i = 0; i < chunk->extrema_amount; i++)
        {
            // collisions.push_back(chunk->extrema[i].box);
            // collisions[sp + i] = chunk->extrema[i].box;
            addItemToCollisionList(chunk->extrema[i].box, i, collisions);
        }

        // Add checkpoints
        // Use a for loop because they're different types
        const int og_size = collisions.size();
        // collisions.resize(collisions.size() + chunk->checkpoint.size());
        for (int i = 0; i < chunk->checkpoint.size(); i++)
        {
            // collisions[og_size + i] = chunk->checkpoint[i].box;
            addItemToCollisionList(chunk->checkpoint[i].box, i, collisions);
        }
    }

    // Last chunk
    {
        Chunk* last_chunk = box->storage[index].maxima_chunk;

        const int sp = collisions.size();
        // collisions.resize(collisions.size() + box->storage[index].maxima_chunk_index+1);
        for (int i = 0; i < box->storage[index].maxima_chunk_index; i++)
        {
            // collisions[sp + i] = last_chunk->extrema[i].box;
            addItemToCollisionList(last_chunk->extrema[i].box, i, collisions);
        }

        // Add checkpoints
        // Use a for loop because they're different types
        const int og_size = collisions.size();
        // collisions.resize(collisions.size() + last_chunk->checkpoint.size());
        for (int i = 0; i < last_chunk->checkpoint.size(); i++)
        {
            // collisions[og_size + i] = last_chunk->checkpoint[i].box;
            addItemToCollisionList(last_chunk->checkpoint[i].box, i, collisions);
        }
    }

    return collisions;
}

// bool DS::SegmentedIntervalList::detectCollision(BoundingBox *boxa, BoundingBox *boxb)
// {
//     // Check if it's even remotely physically possible they're colliding
//     int minima_a = boxa->storage[index].minima_chunk->index;
//     int maxima_a = boxa->storage[index].maxima_chunk->index;
//     int minima_b = boxb->storage[index].minima_chunk->index;
//     int maxima_b = boxb->storage[index].maxima_chunk->index;

//     if (minima_a <= minima_b
//         && maxima_a >= minima_b
//         && maxima_a >= minima_b
//         && minima_b <= minima_a
//         && maxima_b >= minima_a
//         && maxima_b >= minima_a)
//     {
        
//     }
//     else
//     {
//         return false;
//     }
// }

// ==================================================
// Sorted Extrema List
// ==================================================
DS::SortedExtremaList::SortedExtremaList(const int index):
index(index)
{

}

void DS::SortedExtremaList::addBoundingBox(BoundingBox* box, float minima, float maxima)
{
    // Add the 2 extrema in the right place
    auto minima_it = std::upper_bound(extrema.begin(), extrema.end(), minima, 
                [](const float& value, Extrema& info) {
        return value < info.pos;
    });

    auto mit = extrema.insert(minima_it, Extrema {Minima, box, minima});

    auto maxima_it = std::upper_bound(extrema.begin(), extrema.end(), maxima, 
                [](const float& value, Extrema& info) {
        return value < info.pos;
    });

    auto mat = extrema.insert(maxima_it, Extrema {Maxima, box, maxima});

    // The insertion of the Maxima after the minima shouldn't effect the minima's index
    box->storage[index].minima_chunk_index = mit - extrema.begin();
    box->storage[index].maxima_chunk_index = mat - extrema.begin();

    // Sort to update indexes
    sort();
}

void DS::SortedExtremaList::removeBoundingBox(BoundingBox *box, float minima, float maxima)
{
    extrema.erase(extrema.begin() + box->storage[index].maxima_chunk_index);
    extrema.erase(extrema.begin() + box->storage[index].minima_chunk_index);

    // Re-sort
    sort();
}

void DS::SortedExtremaList::sort()
{
    // Update all the extrema
    for (int i = 0; i < extrema.size(); i++)
    {
        Extrema key = extrema[i];

        // Make key up to date
        if (key.type == Minima)
        {
            key.pos = key.box->min_pos[index];
        }
        else
        {
            key.pos = key.box->max_pos[index];
        }

        extrema[i] = key;
    }

    // Basically just your standard insertion sort
    int j;
    for (int i = 1; i < extrema.size(); i++)
    {
        Extrema key = extrema[i];

        j = i;
        while (j > 0 && extrema[j-1].pos > key.pos)
        {
            extrema[j] = extrema[j-1];

            // Update index
            if (extrema[j].type == Minima)
            {
                extrema[j].box->storage[index].minima_chunk_index = j;
            }
            else
            {
                extrema[j].box->storage[index].maxima_chunk_index = j;
            }

            j--;
        }

        extrema[j] = key;
        if (extrema[j].type == Minima)
        {
            extrema[j].box->storage[index].minima_chunk_index = j;
        }
        else
        {
            extrema[j].box->storage[index].maxima_chunk_index = j;
        }
    }

    // std::sort varient
    // A tiny bit slower than insertion sort
    // {
    //     std::sort(extrema.begin(), extrema.end());

    //     // Fix indexes
    //     for (int i = 0; i < extrema.size(); i++)
    //     {
    //         if (extrema[i].type == Minima)
    //         {
    //             extrema[i].box->storage[index].minima_chunk_index = i;
    //         }
    //         else
    //         {
    //             extrema[i].box->storage[index].maxima_chunk_index = i;
    //         }
    //     }
    // }
}

void DS::SortedExtremaList::addItemToCollisionList(BoundingBox* box, int i, std::vector<BoundingBox*>& collisions)
{
    // This if loop is magical
    // Whenever I remove it, performance tanks
    if (box->pass < pass)
    {
        // collisions.push_back(box);
        collisions[i] = box;
        // LOG_INFO("Added thing");
        box->pass = pass;
        box->collisions[0] = false;
        box->collisions[1] = false;
        box->collisions[2] = false;
        box->collisions[index] = true;
    }
    else
    {
        box->collisions[index] = true;
        collisions[i] = box;
    }
}

std::vector<BoundingBox*> DS::SortedExtremaList::getCollidingBoxes(BoundingBox* box)
{
    std::vector<BoundingBox*> collisions;
    const int start = box->storage[index].minima_chunk_index+1;
    collisions.resize(box->storage[index].maxima_chunk_index - start);
    for (int i = box->storage[index].minima_chunk_index+1; i < box->storage[index].maxima_chunk_index; i++)
    {
        // if (extrema[i].box == box)
        // {
        //     LOG_INFO("No");
        // }
        addItemToCollisionList(extrema[i].box, i - start, collisions);
    }

    // Get boxes bigger than us
    if (box->storage[index].minima_chunk_index < extrema.size() - box->storage[index].maxima_chunk_index)
    {
        // Less extrema before
        for (int i = 0; i < box->storage[index].minima_chunk_index; i++)
        {
            // Check if it covers our box
            if (extrema[i].type == Minima)
            {
                // Check against maxima not minima so that we don't waste resources
                // checking, then removing extrema between the box's extrema
                if (extrema[i].box->storage[index].maxima_chunk_index > box->storage[index].maxima_chunk_index)
                {
                    const int size = collisions.size();
                    collisions.resize(size + 1);
                    addItemToCollisionList(extrema[i].box, size, collisions);
                }
            }
        }
    }
    else
    {
        // Less extrema after
        for (int i = box->storage[index].maxima_chunk_index+1; i < extrema.size(); i++)
        {
            // Check if it covers our box
            if (extrema[i].type == Maxima)
            {
                // Check against minima not maxima so that we don't waste resources
                // checking, then removing extrema between the box's extrema
                if (extrema[i].box->storage[index].minima_chunk_index < box->storage[index].minima_chunk_index)
                {
                    const int size = collisions.size();
                    collisions.resize(size + 1);
                    addItemToCollisionList(extrema[i].box, size, collisions);
                }
            }
        }
    }

    return collisions;
}

// ==================================================
// Bounding Box
// ==================================================

BoundingBox::BoundingBox(Renderer::Vertex* mesh, uint32_t size):
min_pos(0, 0, 0),
max_pos(0, 0, 0),
og_min_pos(0, 0, 0),
og_max_pos(0, 0, 0),
current_transform(-1000),
has_display(false),
pass(0)
{
    // Run though the entire mesh and find the higgest and smallest points
    if (size < 1)
    {
        // Why is there an empty mesh?
        LOG_WARN("Mesh is empty");
        return;
    }

    // Reset storage
    storage[0] = {nullptr, -1, nullptr, -1};
    storage[1] = {nullptr, -1, nullptr, -1};
    storage[2] = {nullptr, -1, nullptr, -1};

    // Start with a position in the mesh for the min and max
    min_pos = glm::vec3(mesh[0].x, mesh[0].y, mesh[0].x);
    max_pos = glm::vec3(mesh[0].x, mesh[0].y, mesh[0].x);

    for (int i = 0; i < size; i++)
    {
        auto point = mesh[i];

        // Do each X, Y, and Z individually to make sure we cover everything
        if (point.x < min_pos.x)
        {
            min_pos.x = point.x;
        }
        if (point.y < min_pos.y)
        {
            min_pos.y = point.y;
        }
        if (point.z < min_pos.z)
        {
            min_pos.z = point.z;
        }

        if (point.x > max_pos.x)
        {
            max_pos.x = point.x;
        }
        if (point.y > max_pos.y)
        {
            max_pos.y = point.y;
        }
        if (point.z > max_pos.z)
        {
            max_pos.z = point.z;
        }
    }

    og_min_pos = min_pos;
    og_max_pos = max_pos;

    // Boom. Bounding box made
}

bool BoundingBox::updateTransform(const glm::mat4& global_transform)
{
    if (current_transform == global_transform)
    {
        return false;
    }

    // Rotate the original bounding box to the new position, then use that to build a new bounding box
    // We do this to insure the bounding box is _always_ in global space
    glm::vec4 box_mesh[8];

    glm::vec4 global_min(global_transform * glm::vec4(og_min_pos, 1)), global_max(global_transform * glm::vec4(og_max_pos, 1));

    // Manually add the points
    // Front
    box_mesh[0] =  glm::vec4(global_min.x, global_min.y, global_min.z, 1);
    box_mesh[1] =  glm::vec4(global_max.x, global_min.y, global_min.z, 1);
    box_mesh[2] =  glm::vec4(global_min.x, global_max.y, global_min.z, 1);
    box_mesh[3] =  glm::vec4(global_max.x, global_max.y, global_min.z, 1);

    // Back
    box_mesh[4] =  glm::vec4(global_min.x, global_min.y, global_max.z, 1);
    box_mesh[5] =  glm::vec4(global_max.x, global_min.y, global_max.z, 1);
    box_mesh[6] =  glm::vec4(global_min.x, global_max.y, global_max.z, 1);
    box_mesh[7] =  glm::vec4(global_max.x, global_max.y, global_max.z, 1);

    // Now find the new biggest and smallest
    // Start with a position in the mesh for the min and max
    min_pos = glm::vec3(box_mesh[0].x, box_mesh[0].y, box_mesh[0].z);
    max_pos = glm::vec3(box_mesh[0].x, box_mesh[0].y, box_mesh[0].z);

    for (int i = 0; i < 8; i++)
    {
        auto point = box_mesh[i];

        // Do each X, Y, and Z individually to make sure we cover everything
        if (point.x < min_pos.x)
        {
            min_pos.x = point.x;
        }
        if (point.y < min_pos.y)
        {
            min_pos.y = point.y;
        }
        if (point.z < min_pos.z)
        {
            min_pos.z = point.z;
        }

        if (point.x > max_pos.x)
        {
            max_pos.x = point.x;
        }
        if (point.y > max_pos.y)
        {
            max_pos.y = point.y;
        }
        if (point.z > max_pos.z)
        {
            max_pos.z = point.z;
        }
    }

    if (has_display)
    {
        updateDisplay();
    }

    current_transform = global_transform;

    // Boom. Bounding box updated
    return true;
}

void BoundingBox::setDisplayEntity(EntityRef entity)
{
    Flux::Transform::giveTransform(entity);
    auto shaders = Flux::Renderer::createShaderResource("shaders/basic.vert", "shaders/basic.frag");
    auto mat = Flux::Renderer::createMaterialResource(shaders);
    Renderer::setUniform(mat, "color", glm::vec3(0, 1, 0));
    
    auto meshres = Flux::Resources::createResource(new Renderer::MeshRes);
    meshres->indices = nullptr;
    meshres->vertices = nullptr;
    
    Flux::Renderer::addMesh(entity, meshres, mat);

    has_display = true;
    display = entity;
    updateDisplay();
}

void BoundingBox::updateDisplay()
{
    // TODO: Make this code _not_ wowefully inefficient
    // Generate mesh made out of lines that looks like the bounding box
    auto meshres = Flux::Resources::createResource(new Renderer::MeshRes);
    meshres->draw_mode = Renderer::DrawMode::Lines;

    auto vertices = new Renderer::Vertex[8];

    // Manually add the points
    // Front
    vertices[0] = Renderer::Vertex {min_pos.x, min_pos.y, min_pos.z, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    vertices[1] = Renderer::Vertex {max_pos.x, min_pos.y, min_pos.z, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    vertices[2] = Renderer::Vertex {min_pos.x, max_pos.y, min_pos.z, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    vertices[3] = Renderer::Vertex {max_pos.x, max_pos.y, min_pos.z, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

    // Back
    vertices[4] = Renderer::Vertex {min_pos.x, min_pos.y, max_pos.z, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    vertices[5] = Renderer::Vertex {max_pos.x, min_pos.y, max_pos.z, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    vertices[6] = Renderer::Vertex {min_pos.x, max_pos.y, max_pos.z, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    vertices[7] = Renderer::Vertex {max_pos.x, max_pos.y, max_pos.z, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

    // Hard coded indices
    auto indices = new uint32_t[24] {
        // Front
        0, 1,
        1, 2,
        2, 3,
        3, 0,

        // Back
        4, 5,
        5, 6,
        6, 7,
        7, 4,

        // Between
        0, 4,
        1, 5,
        2, 6,
        3, 7
    };

    // Add to mesh
    meshres->vertices = vertices;
    meshres->vertices_length = 8;

    meshres->indices = indices;
    meshres->indices_length = 24;

    // Add to entity
    display.getComponent<Renderer::MeshCom>()->mesh_resource = meshres;

    // TODO: Do this in a way that is slightly closer to resembling good
    display.removeComponent<Flux::GLRenderer::GLEntityCom>();
    display.removeComponent<Flux::GLRenderer::GLMeshCom>();
}

// ==================================================
// Bounding World
// ==================================================

BoundingWorld::BoundingWorld():
x_axis(0),
y_axis(1),
z_axis(2)
{

}

void BoundingWorld::addBoundingBox(BoundingBox *box)
{
    // Add it to the SILs
    x_axis.addBoundingBox(box, box->min_pos.x, box->max_pos.x);
    y_axis.addBoundingBox(box, box->min_pos.y, box->max_pos.y);
    z_axis.addBoundingBox(box, box->min_pos.z, box->max_pos.z);
}

void BoundingWorld::removeBoundingBox(BoundingBox *box)
{
    // Remove it from the SILs
    x_axis.removeBoundingBox(box, box->min_pos.x, box->max_pos.x);
    y_axis.removeBoundingBox(box, box->min_pos.y, box->max_pos.y);
    z_axis.removeBoundingBox(box, box->min_pos.z, box->max_pos.z);
}

void BoundingWorld::processCollisions(float delta)
{
    x_axis.sort();
    y_axis.sort();
    z_axis.sort();

    // x_axis.started = true;
    // y_axis.started = true;
    // z_axis.started = true;
}

std::vector<BoundingBox*> BoundingWorld::getColliding(BoundingBox* box)
{
    auto collisions_x = x_axis.getCollidingBoxes(box);
    auto collisions_y = y_axis.getCollidingBoxes(box);
    auto collisions_z = z_axis.getCollidingBoxes(box);

    std::vector<BoundingBox*> set;

    // Because apparently it's faster...?
    for (auto i : collisions_x)
    {
        if (i->collisions[0] && i->collisions[1] && i->collisions[2])
        {
            set.push_back(i);

            // Unset it
            i->collisions[0] = false;
            i->collisions[1] = false;
            i->collisions[2] = false;
        }
    }
    // for (auto i : collisions_y)
    // {
    //     if (i->collisions[0] && i->collisions[1] && i->collisions[2])
    //     {
    //         set.push_back(i);

    //         // Unset it
    //         i->collisions[0] = false;
    //         i->collisions[1] = false;
    //         i->collisions[2] = false;
    //     }
    // }
    // for (auto i : collisions_z)
    // {
    //     if (i->collisions[0] && i->collisions[1] && i->collisions[2])
    //     {
    //         set.push_back(i);

    //         // Unset it
    //         i->collisions[0] = false;
    //         i->collisions[1] = false;
    //         i->collisions[2] = false;
    //     }
    // }

    pass ++;

    return set;
}

// ==================================================
// Flux Stuff
// ==================================================

void Flux::Physics::giveBoundingBox(EntityRef entity)
{
    if (!entity.hasComponent<Renderer::MeshCom>())
    {
        LOG_WARN("A mesh is required to make a bounding box");
        return;
    }

    // Add a bounding box made from the entities mesh
    auto com = new BoundingCom;
    
    auto mesh = entity.getComponent<Renderer::MeshCom>()->mesh_resource->vertices;
    auto mesh_size = entity.getComponent<Renderer::MeshCom>()->mesh_resource->vertices_length;

    com->box = new BoundingBox(mesh, mesh_size);
    com->box->entity = entity;

    com->setup = false;
    com->collisions = std::vector<BoundingBox* >();
    entity.addComponent(com);
}

void Flux::Physics::giveBoundingBox(EntityRef entity, glm::vec3 min_pos, glm::vec3 max_pos)
{
    // Add a bounding box made from the entities mesh
    auto com = new BoundingCom;

    com->box = new BoundingBox();

    com->box->storage[1] = {nullptr, -1, nullptr, -1};
    com->box->storage[0] = {nullptr, -1, nullptr, -1};
    com->box->storage[2] = {nullptr, -1, nullptr, -1};

    com->box->og_min_pos = min_pos;
    com->box->og_max_pos = max_pos;
    com->box->min_pos = min_pos;
    com->box->max_pos = max_pos;
    com->box->entity = entity;

    com->setup = false;
    com->collisions = std::vector<BoundingBox* >();
    entity.addComponent(com);
}

// The system
BroadPhaseSystem::BroadPhaseSystem()
:world()
{

}

static uint64_t frames = 0;
void BroadPhaseSystem::onSystemAdded(ECSCtx* ctx) {}
void BroadPhaseSystem::onSystemStart() 
{
    frames ++;
}

void BroadPhaseSystem::runSystem(EntityRef entity, float delta)
{
    if (entity.hasComponent<BoundingCom>())
    {
        auto bc = entity.getComponent<BoundingCom>();
        auto tc = entity.getComponent<Transform::TransformCom>();
        if (!bc->setup)
        {
            // LOG_INFO("=== Initial Add");
            bc->box->updateTransform(tc->model);
            world.addBoundingBox(bc->box);
            bc->setup = true;
            bc->world = &world;
            bc->box->entity = entity;
        }

        if (tc->has_changed)
        // if (bc->box->updateTransform(tc->model))
        {
            // Only update the bounding box,
            // The rest will update itself
            bc->box->updateTransform(tc->model);
            
            // Remove it and re-add it to the bounding world
            // TODO: Potencial for optimisations
            // LOG_INFO("=== Remove");
            // world.removeBoundingBox(bc->box);
            // LOG_INFO("=== Add");
            // world.addBoundingBox(bc->box);
        }

        // bc->collisions = world.getColliding(bc->box);
    }
}

void BroadPhaseSystem::onSystemEnd() 
{
    world.processCollisions(0);
}

std::vector<BoundingBox*> Flux::Physics::getBoundingBoxCollisions(EntityRef entity)
{
    if (entity.hasComponent<BoundingCom>())
    {
        auto bc = entity.getComponent<BoundingCom>();
        if (!bc->setup)
        {
            return std::vector<BoundingBox*>();
        }

        if (bc->frame != frames)
        {
            // LOG_INFO("Redoing BB");
            bc->collisions = bc->world->getColliding(bc->box);
            bc->frame = frames;
        }

        return bc->collisions;
    }

    // LOG_INFO("No Bounding Com");
    return std::vector<BoundingBox*>();
}

// ==================================================
// Narrow Phase
// ==================================================

ConvexCollider::ConvexCollider()
{

}

ConvexCollider::ConvexCollider(std::vector<glm::vec3> vertices)
:vertices(vertices),
og_vertices(vertices),
transform()
{

}

glm::vec3 ConvexCollider::findFurthestPoint(const glm::vec3& direction) const
{
    glm::vec3 max_point;

    // Set max distance to the smallest possible float
    float max_distance = -FLT_MAX;

    // Find point biggest in _direction_
    for (auto v : vertices)
    {
        float distance = glm::dot(v, direction);
        if (distance > max_distance)
        {
            max_distance = distance;
            max_point = v;
        }
    }

    return max_point;
}

void ConvexCollider::updateTransform(const glm::mat4 &global_transform)
{
    if (global_transform != transform)
    {
        // This is very intensive
        vertices = og_vertices;
        for (int i = 0; i < vertices.size(); i++)
        {
            vertices[i] = glm::vec3(global_transform * glm::vec4(vertices[i], 1));
        }
    }
}

glm::vec3 GJK::support(const Collider* col_a, const Collider* col_b, const glm::vec3 &direction)
{
    return col_a->findFurthestPoint(direction) - col_b->findFurthestPoint(-direction);
}

bool GJK::nextSimplex(Simplex& points, glm::vec3& direction)
{
    // Use the apropriate shape
    switch (points.size)
    {
        case 2: return lineSimplex(points, direction);
        case 3: return triangleSimplex(points, direction);
        case 4: return tetrahedronSimplex(points, direction);
    }

    // Impossible
    return false;
}

bool sameDirection(const glm::vec3& direction, const glm::vec3& ao)
{
    return glm::dot(direction, ao) > 0;
}

bool GJK::lineSimplex(Simplex &points, glm::vec3 &direction)
{
    glm::vec3 a = points[0];
    glm::vec3 b = points[1];

    auto ab = b - a;
    auto ao = -a;

    if (sameDirection(ab, ao))
    {
        direction = glm::cross(glm::cross(ab, ao), ab);
    }
    else
    {
        points = {a};
        direction = ao;
    }

    return false;
}

bool GJK::triangleSimplex(Simplex &points, glm::vec3 &direction)
{
    glm::vec3 a = points[0];
    glm::vec3 b = points[1];
    glm::vec3 c = points[2];

    auto ab = b - a;
    auto ac = c - a;
    auto ao = -a;

    auto abc = glm::cross(ab, ac);

    if (sameDirection(glm::cross(abc, ac), ao))
    {
        if (sameDirection(ac, ao))
        {
            points = {a, c};
            direction = glm::cross(glm::cross(ac, ao), ac);
        }
        else
        {
            return lineSimplex(points = {a, b}, direction);
        }
    }
    else
    {
        if (sameDirection(glm::cross(ab, abc), ao))
        {
            return lineSimplex(points = {a, b}, direction);
        }
        else
        {
            if (sameDirection(abc, ao))
            {
                direction = abc;
            }
            else
            {
                points = {a, c, b};
                direction = -abc;
            }
        }
    }

    return false;
}

bool GJK::tetrahedronSimplex(Simplex &points, glm::vec3 &direction)
{
    glm::vec3 a = points[0];
    glm::vec3 b = points[1];
    glm::vec3 c = points[2];
    glm::vec3 d = points[3];

    auto ab = b - a;
    auto ac = c - a;
    auto ad = d - a;
    auto ao = -a;

    auto abc = glm::cross(ab, ac);
    auto acd = glm::cross(ac, ad);
    auto adb = glm::cross(ad, ab);

    if (sameDirection(abc, ao))
    {
        return triangleSimplex(points = {a, b, c}, direction);
    }

    if (sameDirection(acd, ao))
    {
        return triangleSimplex(points = {a, c, d}, direction);
    }

    if (sameDirection(adb, ao))
    {
        return triangleSimplex(points = {a, d, b}, direction);
    }

    return true;
}

std::pair<bool, Flux::Physics::GJK::Simplex> GJK::GJK(const Collider* col_a, const Collider* col_b)
{
    // Start with a random direction
    auto sup_v = support(col_a, col_b, glm::vec3(1, 0, 0));

    // Start the Simplex
    Simplex points;
    points.addPoint(sup_v);

    // New direction is towards the origin (because -x goes towards it)
    glm::vec3 direction = -sup_v;

    while (true)
    {
        sup_v = support(col_a, col_b, direction);

        if (glm::dot(sup_v, direction) <= 0)
        {
            // We could not find a collision in the shape
            return {false, points};
        }

        points.addPoint(sup_v);

        if (nextSimplex(points, direction))
        {
            return {true, points};
        }
    }
}

// std::pair<std::vector<glm::vec4>, uint32_t> GJK::generateNormals(std::vector<glm::vec3>& polytope, std::vector<uint32_t>& faces)
// {
//     std::vector<glm::vec4> normals;

//     // This variable tells us which triangle is closest
//     uint32_t min_tri = 0;
//     float min_distance = FLT_MAX;

//     // Iterate over each triangle
//     for (auto i = 0; i < faces.size(); i += 3)
//     {
//         // Get our vertices
//         glm::vec3 a = polytope[faces[i]];
//         glm::vec3 b = polytope[faces[i+1]];
//         glm::vec3 c = polytope[faces[i+2]];

//         // Generate normal
//         glm::vec3 normal = glm::normalize(glm::cross(b - a, c - a));
//         float distance = glm::dot(normal, a - glm::vec3(0, 0, 0)) * -1;
//         // float distance = glm::dot(normal, glm::vec3(0, 0, 0));

//         if (distance < 0)
//         {
//             // Our winding order is wrong; reverse
//             normal *= -1;
//             distance *= -1;
//         }

//         normals.emplace_back(normal, distance);

//         // Check if it's the closest triangle
//         if (distance < min_distance)
//         {
//             min_tri = i / 3;
//             min_distance = distance;
//         }
//     }

//     return {normals, min_tri};
// }

// void GJK::addEdge(std::vector<std::pair<uint32_t, uint32_t>>& edges, std::vector<uint32_t>& faces, int start, int end)
// {
//     // Check if they're in our list
//     auto edge = std::find(edges.begin(), edges.end(), std::make_pair(faces[end], faces[start]));

//     if (edge != edges.end())
//     {
//         // There's 2 of it - remove both
//         edges.erase(edge);
//     }
//     else
//     {
//         // It's unique (so far). Add it
//         edges.emplace_back(faces[start], faces[end]);
//     }
// }

// CollisionData GJK::EPA(Simplex simplex, const Collider* col_a, const Collider* col_b)
// {
//     // Create polytope as a "mesh"
//     std::vector<glm::vec3> polytope;
//     polytope.push_back(simplex[0]);
//     polytope.push_back(simplex[1]);
//     polytope.push_back(simplex[2]);
//     polytope.push_back(simplex[3]);

//     std::vector<uint32_t> faces = {
//         0, 1, 2,
//         0, 3, 1,
//         0, 2, 3,
//         1, 3, 2
//     };

//     auto [normals, min_face] = generateNormals(polytope, faces);

//     glm::vec3 min_normal;
//     float min_distance = FLT_MAX;
//     uint32_t iterations = 0;
//     while (min_distance == FLT_MAX && iterations < EPA_MAX_ITERATIONS)
//     {
//         min_normal = glm::vec3(normals[min_face]);
//         min_distance = normals[min_face].w;

//         // Find closest point on the actual shape
//         glm::vec3 sup_v = support(col_a, col_b, min_normal);
//         float distance = glm::dot(min_normal, sup_v - glm::vec3(0, 0, 0));

//         if (abs(distance) - abs(min_distance) > 0.005f)
//         {
//             // The support point and the closest triangle aren't in the same place
//             // So we have to extend the polytope
//             min_distance = FLT_MAX;

//             // Tear a giant hole in the polytope
//             std::vector<std::pair<uint32_t, uint32_t>> unique_edges;

//             // Delete any triangles that point towards the new point
//             for (auto i = 0; i < normals.size(); i++)
//             {
//                 if (sameDirection(normals[i], sup_v))
//                 {
//                     // This face must be killed :(
//                     auto f = i * 3;

//                     // Add the unique edges
//                     addEdge(unique_edges, faces, f, f + 1);
//                     addEdge(unique_edges, faces, f + 1, f + 2);
//                     addEdge(unique_edges, faces, f + 2, f);

//                     faces[f+2] = faces.back(); faces.pop_back();
//                     faces[f+1] = faces.back(); faces.pop_back();
//                     faces[f] = faces.back(); faces.pop_back();

//                     normals[i] = normals.back(); normals.pop_back();

//                     i--;
//                 }
//             }

//             // Add new faces
//             std::vector<uint32_t> new_faces;

//             for (auto [start, end] : unique_edges)
//             {
//                 new_faces.push_back(start);
//                 new_faces.push_back(end);
//                 new_faces.push_back(polytope.size());
//             }

//             polytope.push_back(sup_v);

//             auto [new_normals, new_min_face] = generateNormals(polytope, new_faces);

//             // Find the new closest face
//             float old_min_distance = FLT_MAX;
//             uint32_t old_min_face = min_face;

//             // Use the old normals for performance reasons
//             for (auto i = 0; i < normals.size(); i++)
//             {
//                 if (normals[i].w < old_min_distance)
//                 {
//                     old_min_distance = normals[i].w;
//                     min_face = i;
//                 }
//             }

//             // Check if the new normals are closer
//             if (new_normals[new_min_face].w < old_min_distance)
//             {
//                 // new_min_face's index
//                 min_face = new_min_face + normals.size();
//             }

//             // if (old_min_face == min_face)
//             // {
//             //     // I think this means it's done...?
//             //     min_normal = glm::vec3(normals[min_face]);
//             //     min_distance = normals[min_face].w;
//             // }

//             // Add the new faces and normals
//             faces.insert(faces.end(), new_faces.begin(), new_faces.end());
//             normals.insert(normals.end(), new_normals.begin(), new_normals.end());
//         }

//         iterations ++;
//     }

//     if (iterations == EPA_MAX_ITERATIONS)
//     {
//         LOG_WARN("Hit max its");
//     }

//     return {min_normal, min_distance + 0.001f, true};
// }

uint32_t GJK::getClosestTri(std::vector<glm::vec4> &normals, std::vector<glm::vec3> &polytope, std::vector<uint32_t> &faces)
{
    uint32_t closest = 0;
    float closest_distance = FLT_MAX;

    normals.resize(faces.size() / 3);
    for (auto i = 0; i < faces.size(); i += 3)
    {
        // Get our vertices
        glm::vec3 a = polytope[faces[i]];
        glm::vec3 b = polytope[faces[i+1]];
        glm::vec3 c = polytope[faces[i+2]];

        // Generate normal
        glm::vec3 normal = glm::normalize(glm::cross(b - a, c - a));

        // Find distance to origin using plane to point
        auto v = a - glm::vec3(0, 0, 0);
        auto dist = glm::dot(v, normal);

        normals[i/3] = glm::vec4(normal, glm::abs(dist));

        if (dist == 0)
        {
            // Something must've went wrong. Just throw out this normal
            // continue;
        }

        if (dist < closest_distance)
        {
            closest_distance = dist;
            closest = i/3;
        }
    }

    return closest;
}

CollisionData GJK::EPA(Simplex simplex, const Collider* col_a, const Collider* col_b)
{
    bool done = false;
    // Create polytope as a "mesh"
    std::vector<glm::vec3> polytope;
    polytope.push_back(simplex[0]);
    polytope.push_back(simplex[1]);
    polytope.push_back(simplex[2]);
    polytope.push_back(simplex[3]);

    std::vector<uint32_t> faces = {
        0, 1, 2,
        0, 3, 1,
        0, 2, 3,
        1, 3, 2
    };

    std::vector<glm::vec4> normals;
    int adds = 0, removes = 0;

    int iteration_count = 0;

    while (!done)
    {
        normals = std::vector<glm::vec4>();
        uint32_t closest_tri = getClosestTri(normals, polytope, faces);

        auto distance = normals[closest_tri].w;
        auto sup_v = support(col_a, col_b, glm::vec3(normals[closest_tri]));
        auto sup_dist = glm::dot(sup_v - glm::vec3(0, 0, 0), glm::vec3(normals[closest_tri]));

        if (sup_dist - distance > 0.01f)
        {
            // Extend the polytope
            std::vector<std::pair<uint32_t, uint32_t>> edges;
            for (int i = 0; i < normals.size(); i++)
            {
                // if (glm::dot(glm::vec3(normals[i]), sup_v) > 0)
                if (glm::dot(glm::vec3(normals[i]), sup_v - polytope[faces[i*3]]) > 0)
                {
                    // Store it's edges for later
                    uint32_t v1 = faces[i*3];
                    uint32_t v2 = faces[i*3+1];
                    uint32_t v3 = faces[i*3+2];

                    // Remove all traces the face ever existed
                    faces.erase(faces.begin() + i*3+2);
                    faces.erase(faces.begin() + i*3+1);
                    faces.erase(faces.begin() + i*3);
                    removes ++;
                    
                    // Remove it's normal, and correct the if loop's iterations
                    normals.erase(normals.begin() + i);
                    i --; // So that next loop iteration has the same index as this one

                    std::vector<std::pair<uint32_t, uint32_t>> face_edges = {{v1, v2}, {v2, v3}, {v3, v1}};
                    // std::vector<std::pair<uint32_t, uint32_t>> face_edges = {{v2, v1}, {v3, v2}, {v1, v3}};
                    for (auto e : face_edges)
                    {
                        auto ed = std::find(edges.begin(), edges.end(), std::pair<uint32_t, uint32_t>(e.second, e.first));
                        if (ed != edges.end())
                        {
                            edges.erase(ed);
                        }
                        else
                        {
                            edges.push_back(e);
                        }
                    }
                }
            }

            // Add supv
            // We don't have to worry about removing points
            // because if they're not referenced in indices,
            // it's like they don't exist.
            // And we're using a vector so they'll get freed automatically
            uint32_t new_index = polytope.size();
            polytope.push_back(sup_v);

            // Add new triangles
            for (auto i : edges)
            {
                faces.push_back(i.first);
                faces.push_back(i.second);
                faces.push_back(new_index);
                adds ++;
            }
        }
        else
        {
            done = true;
            // Render debug shape
            // std::vector<Renderer::Vertex> vtx(polytope.size());
            // for (int i = 0; i < polytope.size(); i++)
            // {
            //     vtx[i] = Renderer::Vertex {polytope[i].x, polytope[i].y + 5, polytope[i].z};
            //     // Debug::drawPoint(polytope[i] + glm::vec3(0, 5, 0), 0.1, Debug::Blue);
            // }

            // Debug::drawMesh(vtx, faces, Debug::Green, true);

            // Multiply normal by -1, because for some reason it's reversed
            return CollisionData {glm::vec3(normals[closest_tri]) * glm::vec3(-1), distance, true};
        }

        iteration_count ++;
        if (iteration_count > 64)
        {
            done = true;
        }
    }

    // Render debug shape
    // std::vector<Renderer::Vertex> vtx(polytope.size());
    // for (int i = 0; i < polytope.size(); i++)
    // {
    //     vtx[i] = Renderer::Vertex {polytope[i].x, polytope[i].y + 5, polytope[i].z};
    //     // Debug::drawPoint(polytope[i] + glm::vec3(0, 5, 0), 0.1, Debug::Blue);
    // }

    // Debug::drawMesh(vtx, faces, Debug::Red, true);

    return CollisionData {glm::vec3(), 0, false};
}

CollisionData GJK::getColliding(const Collider *col_a, const Collider *col_b)
{
    auto [hit, simplex] = GJK(col_a, col_b);

    if (hit)
    {
        return EPA(simplex, col_a, col_b);
    }

    return {glm::vec3(), 0, false};
}

// ==================================================
// Narrow Phase Flux Stuff
// ==================================================

void Flux::Physics::giveConvexCollider(Flux::EntityRef entity)
{
    if (!entity.hasComponent<Renderer::MeshCom>())
    {
        LOG_ERROR("Mesh required for convex collider creation");
        return;
    }
    auto mc = entity.getComponent<Renderer::MeshCom>();
    std::vector<glm::vec3> points;
    // points.resize(mc->mesh_resource->vertices_length);

    for (auto i = 0 ; i < mc->mesh_resource->vertices_length; i++)
    {
        auto new_point = glm::vec3(mc->mesh_resource->vertices[i].x, mc->mesh_resource->vertices[i].y, mc->mesh_resource->vertices[i].z);
        if (std::find(points.begin(), points.end(), new_point) == points.end())
        {
            // points[i] = new_point;
            points.push_back(new_point);
        }
    }

    auto convc = new ConvexCollider(points);

    auto cc = new ColliderCom;
    cc->collider = convc;
    cc->frame = 0;
    entity.addComponent(cc);
}

// TODO: Move collision detection out of a system,
// only run it on object that specifically ask for it
// because it's expensive af
void NarrowPhaseSystem::runSystem(EntityRef entity, float delta)
{
    
}

std::vector<Collision> Flux::Physics::getCollisions(EntityRef entity)
{
    if (!entity.hasComponent<BoundingCom>() || !entity.hasComponent<ColliderCom>())
    {
        return std::vector<Collision>();
    }

    auto bc = entity.getComponent<BoundingCom>();
    auto tc = entity.getComponent<Transform::TransformCom>();

    bc->collisions = getBoundingBoxCollisions(entity);

    if (bc->collisions.size() < 1 && !tc->has_changed)
    {
        // No collisions
        // LOG_INFO("No collisions");
        
        return std::vector<Collision>();
    }

    auto cc = entity.getComponent<ColliderCom>();

    if (cc->frame == frames)
    {
        return cc->collisions;
    }

    bool recalculate = tc->has_changed;
    for (auto i : bc->collisions)
    {
        if (i->entity.getEntityID() != -1)
        {
            if (i->entity.hasComponent<ColliderCom>())
            {
                auto etc = i->entity.getComponent<Transform::TransformCom>();
                if (etc->has_changed == true)
                {
                    recalculate = true;
                    break;
                }
            }
        }
    }

    if (recalculate)
    {
        // Process all the collisions :(
        auto collisions = std::vector<Collision>();
        cc->collider->updateTransform(tc->model);
        // std::cout << bc->collisions.size() << "\n";

        int c = 0;
        for (auto i : bc->collisions)
        {
            if (i->entity.getEntityID() != -1)
            {
                if (!i->entity.hasComponent<ColliderCom>())
                {
                    // Don't add it
                }
                else
                {
                    // Actually calculate the collision
                    // Step 1: update collider's transform
                    auto ecc = i->entity.getComponent<ColliderCom>();
                    auto etc = i->entity.getComponent<Transform::TransformCom>();
                    ecc->collider->updateTransform(etc->model);

                    // Step 2: GJK
                    auto colliding = GJK::getColliding(cc->collider, ecc->collider);

                    // Step 3: Profit, or don't profit
                    if (colliding.colliding)
                    {
                        collisions.push_back({colliding.normal, colliding.depth, colliding.colliding, i->entity});
                    }
                }
            }
            c++;
        }

        cc->collisions = collisions;
        cc->frame = frames;
        return cc->collisions;
    }
    else
    {
        return cc->collisions;
    }
}

std::vector<Collision> Flux::Physics::move(EntityRef entity, const glm::vec3 &position)
{
    // Translate it initially
    Flux::Transform::translate(entity, position);

    // Check for collisions
    auto collisions = getCollisions(entity);

    // "Solve" collisions
    for (auto i : collisions)
    {
        Flux::Transform::globalTranslate(entity, i.normal * i.depth);
    }

    return collisions;
}
