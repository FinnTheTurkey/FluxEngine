#include "Flux/Physics.hh"
#include "Flux/Renderer.hh"
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

    delta *= 0.01;

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

    // Step 2: Actually move stuff
    glm::vec3 linear_accel = rc->force / rc->mass;
    rc->linear_velocity += linear_accel * delta;
    Flux::Transform::translate(entity, rc->linear_velocity * delta);

    glm::vec3 angular_accel = rc->global_inertia_inv * rc->torque;
    rc->angular_velocity += angular_accel * delta;
    
    // This won't work
    Flux::Transform::rotate(entity, glm::vec3(1, 0, 0), rc->angular_velocity.x * delta);
    Flux::Transform::rotate(entity, glm::vec3(0, 1, 0), rc->angular_velocity.y * delta);
    Flux::Transform::rotate(entity, glm::vec3(0, 0, 1), rc->angular_velocity.z * delta);
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
    entity.addComponent(rc);
}

void Flux::Physics::addForce(EntityRef entity, const glm::vec3 &position, const glm::vec3 &direction)
{
    if (!entity.hasComponent<Flux::Physics::RigidCom>()) return;

    auto rc = entity.getComponent<RigidCom>();
    rc->forces.push_back({position, direction});
}