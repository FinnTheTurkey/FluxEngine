#include "Flux/Physics.hh"
#include "Flux/Log.hh"
#include "Flux/Renderer.hh"
#include "Flux/Resources.hh"

// TODO: Remove ASAP
#include "Flux/OpenGL/GLRenderer.hh"
#include <algorithm>

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

    auto maxima_it = std::upper_bound(extrema.begin(), extrema.end(), minima, 
                [](const float& value, Extrema& info) {
        return value < info.pos;
    });

    auto mat = extrema.insert(maxima_it, Extrema {Maxima, box, maxima});

    // The insertion of the Maxima after the minima shouldn't effect the minima's index
    box->storage[index].minima_chunk_index = mit - extrema.begin();
    box->storage[index].maxima_chunk_index = mat - extrema.begin();
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
    // Basically just your standard insertion sort
    int j;
    // Extrema& key;
    for (int i = 0; i < extrema.size(); i++)
    {
        Extrema& key = extrema[i];

        // Make key up to date
        if (key.type == Minima)
        {
            key.pos = key.box->min_pos[index];
        }
        else
        {
            key.pos = key.box->max_pos[index];
        }

        j = i;
        while (j > 0 && extrema[j-1] > key)
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
}

void DS::SortedExtremaList::addItemToCollisionList(BoundingBox* box, int i, std::vector<BoundingBox*>& collisions)
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

std::vector<BoundingBox*> DS::SortedExtremaList::getCollidingBoxes(BoundingBox* box)
{
    std::vector<BoundingBox*> collisions;
    
    for (int i = box->storage[index].minima_chunk_index+1; i < box->storage[index].maxima_chunk_index; i++)
    {
        addItemToCollisionList(box, i, collisions);
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
current_transform(),
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
    min_pos = glm::vec3(box_mesh[0].x, box_mesh[0].y, box_mesh[0].x);
    max_pos = glm::vec3(box_mesh[0].x, box_mesh[0].y, box_mesh[0].x);

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
        set.push_back(i);
    }
    for (auto i : collisions_y)
    {
        set.push_back(i);
    }
    for (auto i : collisions_z)
    {
        set.push_back(i);
    }

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
            world.addBoundingBox(bc->box);
            bc->setup = true;
            bc->world = &world;
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
        if (bc->frame != frames)
        {
            bc->collisions = bc->world->getColliding(bc->box);
        }

        return bc->collisions;
    }

    return std::vector<BoundingBox*>();
}