#ifndef FLUX_PHYSICS_HH
#define FLUX_PHYSICS_HH

#include "Flux/ECS.hh"
#include "Flux/Flux.hh"
#include "Flux/Renderer.hh"
#include <forward_list>
#include <unordered_set>

#define FLUX_SIL_CHUNK_SIZE 8

/**
References:
https://www.toptal.com/game/video-game-physics-part-ii-collision-detection-for-solid-objects
Sweep & prune space partitioning algorithm
http://www.math.ucsd.edu/~sbuss/ResearchWeb/EnhancedSweepPrune/SAP_paper_online.pdf
*/

namespace Flux { namespace Physics {

    class BoundingBox;

/** Namespace for specialized data structures */
namespace DS
{
    /** Segmented interval list:
    Diagram:

                 Chunk List:
    Linked List as well as Array (for Binary search)
      ┌───┬───┬───┬───┬───┬───┬───┬───┬───┬───┬───┐
      │   │   │   │   │   │   │   │   │   │   │   │
      └───┴─┬─┴───┴───┴───┴───┴───┴───┴───┴───┴───┘
            │
            │
            │
            │
            ▼
    ┌─────────────────────────────────────────────────┐
    │   Sorted list of extrema                        │
    │  ┌───┬───┬───┬───┬───┬───┬───┬───┬───┬───┬───┐  │
    │  │A[ │   │   │B] │   │A] │   │C[ │   │   │   │  │
    │  └───┴───┴───┴───┴───┴───┴───┴───┴───┴───┴───┘  │
    │                                                 │
    │   Sorted list of checkpoints:                   │
    │   ┌───┬───┐                                     │
    │   │ C │ D │                                     │
    │   └───┴───┘                                     │
    │                                                 │
    └─────────────────────────────────────────────────┘

    Vocabulary:
    Extrema: Min or max position
    Checkpoint: Shape where either no extrema, or only the minima are in the chunk, but it's space covers the chunk

    Explaination:
    To add:   Binary search the chunks for the one that's closest to the point
              you want to add, then insert it (sorted) into that chunk.
              If that chunk if full, then split off the half that contains the new element,
              and make that a new chunk
    To erase: Figure out which chunks the erased object is in by using AABB collision,
              then seare it from those chunks
    */

    enum ExtremaType
    {
        Minima, Maxima, Neither
    };

    struct Extrema
    {
        ExtremaType type;
        BoundingBox* box;
        float pos;

        friend bool operator < (Extrema& a, Extrema& b)
        {
            return a.pos < b.pos;
        }

        friend bool operator < (Extrema& a, float& b)
        {
            return a.pos < b;
        }

        friend bool operator > (Extrema& a, Extrema& b)
        {
            return a.pos > b.pos;
        }

        friend bool operator > (Extrema& a, float& b)
        {
            return a.pos > b;
        }

        friend bool operator <= (Extrema& a, Extrema& b)
        {
            return a.pos <= b.pos;
        }

        friend bool operator <= (Extrema& a, float& b)
        {
            return a.pos <= b;
        }

        friend bool operator >= (Extrema& a, Extrema& b)
        {
            return a.pos >= b.pos;
        }

        friend bool operator >= (Extrema& a, float& b)
        {
            return a.pos >= b;
        }

        friend bool operator == (Extrema& a, Extrema& b)
        {
            return a.pos == b.pos;
        }

        friend bool operator == (Extrema& a, float& b)
        {
            return a.pos == b;
        }
    };

    struct Chunk
    {
        std::array<Extrema, FLUX_SIL_CHUNK_SIZE> extrema;
        int extrema_amount;
        std::vector<Extrema> checkpoint;
        Chunk* next;

        int index;
    };

    struct BBInfo
    {
        Chunk* minima_chunk;
        int minima_chunk_index;

        Chunk* maxima_chunk;
        int maxima_chunk_index;
    };

    /** 
    Index is the index in the storage BoundingBox's storage array that this SIL will use.
    Normally, X axis would be 0, Y axis 1, Z axis 2 */
    class SegmentedIntervalList
    {
    public:
        SegmentedIntervalList(const int index);
        ~SegmentedIntervalList();
        
        /** Adds a bounding box. Simple as that */
        void addBoundingBox(BoundingBox* box, float minima, float maxima);

        /** Remove your bounding box */
        void removeBoundingBox(BoundingBox* box, float minima, float maxima);

        /** Detect of 2 bounding boxes are colliding */
        // bool detectCollision(BoundingBox* boxa, BoundingBox* boxb);

        /** Get all other boxes colliding with this box. */
        std::vector<BoundingBox*> getCollidingBoxes(BoundingBox* box);

    private:
        int addExtrema(DS::ExtremaType type, float extrema, BoundingBox* box);
        int removeExtrema(DS::ExtremaType type, float extrema, BoundingBox* box);

        void addItemToCollisionList(BoundingBox* box, int i, std::vector<BoundingBox*>& collisions);

        const int index;
        Chunk* linked_list;
        std::vector<Chunk*> array;
    };

    class SortedExtremaList
    {
    public:
        SortedExtremaList(const int index);
        ~SortedExtremaList();

        /** Adds a bounding box. Simple as that */
        void addBoundingBox(BoundingBox* box, float minima, float maxima);

        /** Remove your bounding box */
        void removeBoundingBox(BoundingBox* box, float minima, float maxima);

        /** Update the bounding box with new positional values */
        // void updateBoundingBox(BoundingBox* box, float minima, float maxima);

        /** Sort the list, which has the small, tiny, barely worth mentioning side effect
            of _calculating every single collision in the entire scene_ */
        void sort();

        /** Get all other boxes colliding with this box. */
        std::vector<BoundingBox*> getCollidingBoxes(BoundingBox* box);
    private:
        void addItemToCollisionList(BoundingBox* box, int i, std::vector<BoundingBox*>& collisions);

        const int index;
        std::vector<Extrema> extrema;
    };
}

    class BoundingBox
    {
    public:
        BoundingBox():
        min_pos(0, 0, 0),
        max_pos(0, 0, 0),
        og_min_pos(0, 0, 0),
        og_max_pos(0, 0, 0),
        current_transform(),
        has_display(false),
        pass(0) {};
        BoundingBox(Renderer::Vertex* mesh, uint32_t size);

        /**
        Updates the bounding box to reflect the new transformation.
        The bounding box will _always_ be Axis Aligned, and in global space
        */
        bool updateTransform(const glm::mat4& global_transform);

        /**
        Gives the bounding box an entity that it will use to display itself
        */
        void setDisplayEntity(EntityRef entity);

        glm::vec3 min_pos;
        glm::vec3 max_pos;
        glm::vec3 og_min_pos;
        glm::vec3 og_max_pos;

        DS::BBInfo storage[3];

        // Info for collision detection
        uint64_t pass;
        bool collisions[3];

    private:
        bool has_display;
        EntityRef display;

        glm::mat4 current_transform;

        void updateDisplay();
    };

    /**
    Class that represents a world of bounding boxes.
    It stores all the collision information for the broad phase.
    */
    class BoundingWorld
    {
    public:
        BoundingWorld();

        /** 
        Add a bounding box to the bounding world.
        Bounding boxes can only be in a single bounding world
        at once. 
        */
        void addBoundingBox(BoundingBox* box);

        void removeBoundingBox(BoundingBox* box);

        /**
        Run collision detection and update any bounding boxes that have changed
        */
        void processCollisions(float delta);

        /**
        Checks if two bounding boxes are colliding
        */
        // bool isColliding(BoundingBox* boxa, BoundingBox* boxb);

        /**
        Returns all boxes coliding with the given box
        */
        std::vector<BoundingBox*> getColliding(BoundingBox* box);

    private:

        // DS::SegmentedIntervalList x_axis;
        // DS::SegmentedIntervalList y_axis;
        // DS::SegmentedIntervalList z_axis;

        DS::SortedExtremaList x_axis;
        DS::SortedExtremaList y_axis;
        DS::SortedExtremaList z_axis;
    };

    struct BoundingCom: public Flux::Component
    {
        FLUX_COMPONENT(BoundingCom, BoundingCom);

        bool serialize(Resources::Serializer *serializer, FluxArc::BinaryFile *output) override
        {
            output->set(box->og_min_pos.x);
            output->set(box->og_min_pos.y);
            output->set(box->og_min_pos.z);

            output->set(box->og_max_pos.x);
            output->set(box->og_max_pos.y);
            output->set(box->og_max_pos.z);

            return true;
        }

        void deserialize(Resources::Deserializer *deserializer, FluxArc::BinaryFile *file) override
        {
            // Manually fill the bounding box
            box = new BoundingBox;

            box->storage[1] = {nullptr, -1, nullptr, -1};
            box->storage[0] = {nullptr, -1, nullptr, -1};
            box->storage[2] = {nullptr, -1, nullptr, -1};
            
            file->get(&box->og_min_pos.x);
            file->get(&box->og_min_pos.y);
            file->get(&box->og_min_pos.z);

            file->get(&box->og_max_pos.x);
            file->get(&box->og_max_pos.y);
            file->get(&box->og_max_pos.z);

            setup = false;
            collisions = std::vector<BoundingBox* >();
        }

        BoundingBox* box;
        BoundingWorld* world;

        uint64_t frame;
        std::vector<BoundingBox*> collisions;

        bool setup;
    };

    /**
    Gives an entity a bounding box.
    A bounding box can be created from any entity with a mesh,
    or a collision mesh (TODO: implement)
    */
    void giveBoundingBox(EntityRef entity);
    void giveBoundingBox(EntityRef entity, glm::vec3 min_pos, glm::vec3 max_pos);

    std::vector<BoundingBox*> getBoundingBoxCollisions(EntityRef entity);

    class BroadPhaseSystem: public Flux::System
    {
    private:
        BoundingWorld world;

    public:
        BroadPhaseSystem();

        void onSystemAdded(ECSCtx* ctx) override;
        void onSystemStart() override;
        void runSystem(EntityRef entity, float delta) override;
        void onSystemEnd() override;
    };

}}

#endif