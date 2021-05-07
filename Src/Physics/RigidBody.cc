#include "Flux/Physics.hh"
#include "Flux/Renderer.hh"
#include "glm/detail/type_mat.hpp"
#include "glm/gtc/quaternion.hpp"

glm::mat3 star(const glm::vec3 a)
{
    return glm::mat3   {0,   -a.z, a.y, 
                        a.z,  0,  -a.x,
                       -a.y,  a.x, 0};
}

void Flux::Physics::RigidSystem::runSystem(EntityRef entity, float delta)
{
    if (!entity.hasComponent<RigidCom>())
    {
        return;
    }

    auto rc = entity.getComponent<RigidCom>();
    auto tc = entity.getComponent<Transform::TransformCom>();

    // delta *= 0.01;

    // Step 0: Update inersia tensor
    rc->global_inertia_inv = glm::mat3(tc->model) * rc->inertia_inversed;

    // Step 1: Calculate forces and torques
    // TODO: Calculate this elsewhere
    auto inv_model = glm::inverse(tc->model);
    rc->force = glm::vec3(0);
    rc->torque = glm::vec3(0);

    for (auto i : rc->forces)
    {
        auto r = glm::vec3(inv_model * glm::vec4(i.position, 1));
        rc->torque += glm::cross(r, i.force);
        rc->force += i.force;
    }

    // rc->forces = std::vector<Force>();

    // Step 2: Actually move stuff
    glm::vec3 linear_accel = rc->force / rc->mass;
    rc->linear_velocity += linear_accel * delta;
    // Flux::Transform::globalTranslate(entity, rc->linear_velocity * delta);

    glm::vec3 angular_accel = rc->global_inertia_inv * rc->torque;
    rc->angular_velocity += angular_accel * delta;
    
    // TODO: Integrate this better
    // This won't work
    // Flux::Transform::rotateGlobalAxis(entity, glm::vec3(1, 0, 0), rc->angular_velocity.x * delta);
    // Flux::Transform::rotateGlobalAxis(entity, glm::vec3(0, 1, 0), rc->angular_velocity.y * delta);
    // Flux::Transform::rotateGlobalAxis(entity, glm::vec3(0, 0, 1), rc->angular_velocity.z * delta);
}

/**
Thinking about solver:
https://storage.googleapis.com/google-code-archive-downloads/v2/code.google.com/box2d/Tonge_Richard_PhysicsForGame.pdf

M = [[m, 0, 0, 0, 0, 0]
     [0, m, 0, 0, 0, 0]
     [0, 0, m, 0, 0, 0]
     [0, 0, 0, I, I, I]
     [0, 0, 0, I, I, I]
     [0, 0, 0, I, I, I]

J = [n | r x n]
*/

void Flux::Physics::SolverSystem::runSystem(EntityRef entity, float delta)
{
    if (!entity.hasComponent<RigidCom>())
    {
        return;
    }

    auto rc = entity.getComponent<RigidCom>();
    auto contacts = getCollisions(entity);

    // delta *= 0.01;

    // Step 3: Resolve constraints
    // for (int i = 0; i < 4; i++)
    // {
    // TODO: Re-introduce sequenciality
    for (auto c : contacts)
    {
        // TODO: Make offset a real thing
        glm::vec3 linear_j = c.normal;
        glm::vec3 angular_j = glm::cross(c.normal, c.offset);

        glm::mat3 M = {{rc->mass, 0, 0},
                        {0, rc->mass, 0},
                        {0, 0, rc->mass}};
        
        // All of this is almost certainally wrong
        glm::vec3 m_l = (linear_j * glm::inverse(M)) + glm::vec3(0.0001);
        glm::vec3 m_a = (angular_j * rc->global_inertia_inv) + glm::vec3(0.0001);

        // glm::vec3 linear_lambda = (-m_l * (linear_j * (rc->linear_velocity + rc->force)));
        glm::vec3 linear_lambda = (-m_l * (linear_j + (rc->force)));
        // glm::vec3 angular_lambda = (-m_a * (angular_j * (rc->angular_velocity + rc->torque)));
        glm::vec3 angular_lambda = (-m_a * (angular_j + ( rc->torque)));

        glm::vec3 t_l = linear_lambda;
        glm::vec3 t_a = angular_lambda;

        // TODO: bias
        float bias = 0;
        // linear_lambda = linear_lambda - (glm::vec3(1)/m_l) * (linear_j * rc->linear_velocity + bias);
        linear_lambda = linear_lambda - (glm::vec3(1)/m_l) * (linear_j + bias);
        // angular_lambda = angular_lambda - (glm::vec3(1)/m_a) * (angular_j * rc->angular_velocity + bias);
        angular_lambda = angular_lambda - (glm::vec3(1)/m_a) * (angular_j  + bias);

        linear_lambda = glm::max(glm::vec3(0), linear_lambda);
        angular_lambda = glm::max(glm::vec3(0), angular_lambda);

        rc->linear_velocity = rc->linear_velocity + glm::inverse(M)*linear_j*(linear_lambda - t_l);
        rc->angular_velocity = rc->angular_velocity + rc->global_inertia_inv*angular_j*(angular_lambda - t_a);

        // rc->linear_velocity = rc->linear_velocity + glm::inverse(M)*linear_j;
        // rc->angular_velocity = rc->angular_velocity + rc->global_inertia_inv*angular_j;

        // rc->linear_velocity *= delta;
        // rc->angular_velocity *= delta;
    }
    // }

    // Apply position
    Flux::Transform::globalTranslate(entity, rc->linear_velocity * delta);

    // TODO: Integrate this better
    // This won't work
    Flux::Transform::rotateGlobalAxis(entity, glm::vec3(1, 0, 0), rc->angular_velocity.x * delta);
    Flux::Transform::rotateGlobalAxis(entity, glm::vec3(0, 1, 0), rc->angular_velocity.y * delta);
    Flux::Transform::rotateGlobalAxis(entity, glm::vec3(0, 0, 1), rc->angular_velocity.z * delta);
}

// https://storage.googleapis.com/google-code-archive-downloads/v2/code.google.com/box2d/Tonge_Richard_PhysicsForGame.pdf
void Flux::Physics::applyImpulse(float delta, RigidCom *rc, const glm::vec3& contact_normal, const glm::vec3& normal_offset)
{
    
    // glm::mat3 x;
    // glm::vec3 linear_j = contact_normal;
    // glm::vec3 angular_j = glm::cross(normal_offset, contact_normal);
    // glm::vec3 v_rel_lin = rc->linear_velocity * linear_j;
    // glm::vec3 v_rel_ang = rc->angular_velocity * angular_j;
    
    // glm::vec3 linear_i = linear_j * lambda;
    // glm::vec3 angular_i = angular_j * lambda;
    // rc->linear_velocity = rc->linear_velocity + rc->mass * linear_i;
    // rc->angular_velocity = rc->angular_velocity + rc->global_inertia_inv * angular_i;
}

void Flux::Physics::giveRigidBody(EntityRef entity, float mass)
{
    if (!entity.hasComponent<Flux::Transform::TransformCom>() && !entity.hasComponent<Flux::Physics::BoundingCom>())
    {
        return;
    }

    // For now, use bounding box to calculate moment of inertia
    auto bc = entity.getComponent<Flux::Physics::BoundingCom>();
    glm::vec3 sizes = bc->box->og_max_pos - bc->box->og_min_pos;

    glm::mat3 inertia;
    inertia[0][0] = (1.0f/12.0f) * mass * ((sizes.x * sizes.x) + (sizes.z * sizes.z));
    inertia[1][1] = (1.0f/12.0f) * mass * ((sizes.z * sizes.z) + (sizes.y * sizes.y));
    inertia[2][2] = (1.0f/12.0f) * mass * ((sizes.x * sizes.x) + (sizes.y * sizes.y));

    auto rc = new RigidCom;
    rc->mass = mass;
    rc->inertia = inertia;
    rc->inertia_inversed = glm::inverse(inertia);

    rc->force = glm::vec3(0, 0, 0);
    rc->torque = glm::vec3(0, 0, 0);
    rc->linear_velocity = glm::vec3(0);
    rc->angular_velocity = glm::vec3(0);
    entity.addComponent(rc);
}

void Flux::Physics::addForce(EntityRef entity, const glm::vec3 &position, const glm::vec3 &direction)
{
    if (!entity.hasComponent<Flux::Physics::RigidCom>()) return;

    auto rc = entity.getComponent<RigidCom>();
    rc->forces.push_back({position, direction});
}