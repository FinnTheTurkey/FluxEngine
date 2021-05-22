#ifndef FLUX_PHYSICS_HH
#define FLUX_PHYSICS_HH

#include "Flux/ECS.hh"
#include "Flux/Flux.hh"
#include "Flux/Renderer.hh"
#include "glm/detail/precision.hpp"
#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"
#include <algorithm>
#include <array>
#include <forward_list>
#include <initializer_list>
#include <unordered_set>

#define FLUX_SIL_CHUNK_SIZE 8
#define EPA_MAX_ITERATIONS 64

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

        // bool started = false;
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
        current_transform(-1000),
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

        EntityRef entity;

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

        ~BoundingCom()
        {
            // Remove bounding boxes from bounding world
            if (world && box)
            {
                // TODO: IMPORTANT: Fix
                // world->removeBoundingBox(box);
            }
        }

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

            box->max_pos = box->og_max_pos;
            box->min_pos = box->og_min_pos;

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

    // ====================================
    // Narrow Phase
    // ====================================

    // Thanks to https://blog.winter.dev/2020/gjk-algorithm/ for explaining this well
    // You may notice my code is very similar to their code

    // Use polymorphism because, for once, it's the best tool for the job
    class Collider
    {
    public:
        virtual glm::vec3 findFurthestPoint(const glm::vec3& direction) const {return glm::vec3();};
        virtual void updateTransform(const glm::mat4& global_transform) {};
    };
    
    // TODO: Change transform of colliders
    class ConvexCollider: public Collider
    {
    private:
        std::vector<glm::vec3> og_vertices;
        std::vector<glm::vec3> vertices;
        glm::mat4 transform;
    public:
        ConvexCollider();
        ConvexCollider(std::vector<glm::vec3> vertices);
        glm::vec3 findFurthestPoint(const glm::vec3& direction) const override;

        void updateTransform(const glm::mat4& global_transform) override;
    };

    struct CollisionData
    {
        glm::vec3 normal;
        glm::vec3 offset;
        float depth;
        bool colliding;
    };

    struct Collision
    {
        glm::vec3 normal;
        glm::vec3 offset;
        float depth;
        bool colliding;
        Flux::EntityRef entity;
    };

// Seperate namespace to keep non-user facing GJK away
namespace GJK
{
    std::pair<glm::vec3, glm::vec3> support(const Collider* col_a, const Collider* col_b, const glm::vec3& direction);

    struct PolytopePoint
    {
        glm::vec3 point;
        glm::vec3 sup_a;
    };

    class Simplex
    {
    private:
        std::array<PolytopePoint, 4> points;

    public:
        unsigned int size;

        Simplex()
        // WARNING: This like is sketchy
        : points {glm::vec3(), glm::vec3(), glm::vec3(), glm::vec3()},
        size(0)
        {}

        Simplex& operator=(std::initializer_list<PolytopePoint> list)
        {
            for (auto v = list.begin(); v != list.end(); v++)
            {
                points[std::distance(list.begin(), v)] = *v;
            }
            size = list.size();

            return *this;
        }
        
        PolytopePoint& operator[] (unsigned int i)
        {
            return points[i];
        }

        void addPoint(const PolytopePoint& point)
        {
            points = {point, points[0], points[1], points[2]};
            size = std::min(size + 1, 4u);
        }
    };

    /**
    The smart part: Detecting if the simplex is over the origin, 
    and if not, make it closer
    */
    bool nextSimplex(Simplex& points, glm::vec3& direction);

    /** Now the smart bits for different simplex shapes */
    bool lineSimplex(Simplex& points, glm::vec3& direction);
    bool triangleSimplex(Simplex& points, glm::vec3& direction);
    bool tetrahedronSimplex(Simplex& points, glm::vec3& direction);

    /** Helper functions for EPA */
    std::pair<std::vector<glm::vec4>, uint32_t> generateNormals(std::vector<glm::vec3>& polytope, std::vector<uint32_t>& faces);

    void addEdge(std::vector<std::pair<uint32_t, uint32_t>>& edges, std::vector<uint32_t>& faces, int start, int end);

    // The actual collision detection. Uses GJK and returns true if colliding, or false if not
    std::pair<bool, Simplex> GJK(const Collider* col_a, const Collider* col_b);

    /** Better helper functions for EPA */
    uint32_t getClosestTri(std::vector<glm::vec4>& normals, std::vector<PolytopePoint>& polytope, std::vector<uint32_t>& faces);

    // Gets the useful information from the actual collision detection
    CollisionData EPA(Simplex simplex, const Collider* col_a, const Collider* col_b);

    /** Deals with all the algorithms. Tells you if there's a collision, and if so, how deep  */
    CollisionData getColliding(const Collider* col_a, const Collider* col_b);
}

    void giveConvexCollider(Flux::EntityRef entity);

    struct ColliderCom: public Flux::Component
    {
        FLUX_COMPONENT(ColliderCom, ColiderCom);

        Collider* collider;

        std::vector<Collision> collisions;
        uint64_t frame;
    };

    class NarrowPhaseSystem: public Flux::System
    {
    private:

    public:
        NarrowPhaseSystem() {};

        void onSystemAdded(ECSCtx* ctx) override {};
        void onSystemStart() override {};
        void runSystem(EntityRef entity, float delta) override;
        void onSystemEnd() override {};
    };

    /** Returns collision data about each collision involving this entity */
    std::vector<Collision> getCollisions(EntityRef entity);

    /**
    Basic movement method.
    This will move the entity using the given vector, in the same way
    Transform::translate does. But if it hits a wall, it will exit
    the wall.
    */
    std::vector<Collision> move(EntityRef entity, const glm::vec3& position);


    // ===========================================
    // Rigid body stuff
    // ===========================================

    struct Force
    {
        glm::vec3 position;
        glm::vec3 force;
    };

    struct Constraint
    {
        glm::vec3 normal;
        float (*c)(EntityRef entity);
    };


    /*
    Component that stores info about a rigid body, and what forces are acting apon it
    */
    struct RigidCom: public Component
    {
        FLUX_COMPONENT(RigidCom, RigidCom);

        // Static properties
        float mass;
        glm::mat3 inertia;
        glm::mat3 inertia_inversed;

        // Dynamic properties
        // glm::vec3 velocity;

        // glm::vec3 linear;
        // glm::vec3 angular;

        // State variables
        glm::vec3 dposition; // x(t)
        glm::quat dorientation; // q(t)
        glm::vec3 dscale;
        glm::vec3 linear; // P(t)
        glm::vec3 angular; // L(t)

        // Derived quantities
        glm::mat3 global_inertia_inv; // I-1(t)
        glm::mat3 rotation; // R(t)
        glm::vec3 linear_velocity; // v(t)
        glm::vec3 angular_velocity; // ω(t)

        // Computed quantities
        glm::vec3 force; // F(t)
        glm::vec3 torque; // t(t)

        std::vector<Force> forces;

        // Constraints
        std::vector<Constraint> constraints;
    };

    class RigidSystem: public System
    {
    public:
        void runSystem(EntityRef entity, float delta) override;
    };

    class SolverSystem: public System
    {
    public:
        void runSystem(EntityRef entity, float delta) override;
    };

    void giveRigidBody(EntityRef entity, float mass);

    void addForce(EntityRef entity, const glm::vec3& position, const glm::vec3& direction);

    /** Helper function to apply an impulse */
    std::pair<glm::vec3, glm::vec3> applyImpulse(float delta, RigidCom* rc, const glm::vec3& contact_normal, const glm::vec3& normal_offset, glm::vec3 bias, std::pair<glm::vec3, glm::vec3> lambdas, bool first);

}}

#endif