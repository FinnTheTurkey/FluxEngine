#include "Flux/Debug.hh"
#include "Flux/Log.hh"
#include "Flux/Physics.hh"
#include "Flux/Renderer.hh"
#include "glm/detail/func_common.hpp"
#include "glm/detail/func_trigonometric.hpp"
#include "glm/detail/type_mat.hpp"
#include "glm/gtc/quaternion.hpp"
#include "glm/gtx/matrix_decompose.hpp"
#include <cstdio>
#include <iostream>

// #define SLOWMO

glm::mat3 star(const glm::vec3 a)
{
    return glm::mat3   {0,   -a.z, a.y, 
                        a.z,  0,  -a.x,
                       -a.y,  a.x, 0};
}

#define DRAG_FACTOR -0.25f

void Flux::Physics::RigidSystem::runSystem(EntityRef entity, float delta)
{
    if (!entity.hasComponent<RigidCom>())
    {
        return;
    }

    auto rc = entity.getComponent<RigidCom>();
    auto tc = entity.getComponent<Transform::TransformCom>();

    // delta *= 0.01;
#ifdef SLOWMO
    delta *= 0.4;
#endif

    // Step 0: Update inersia tensor
    rc->global_inertia_inv = glm::mat3(tc->model) * rc->inertia_inversed;

    // Step 1: Calculate forces and torques
    // TODO: Calculate this elsewhere
    auto inv_model = glm::inverse(tc->model);
    rc->force = glm::vec3(0);
    rc->torque = glm::vec3(0);

    for (auto i : rc->forces)
    {
        // auto r = glm::vec3(inv_model * glm::vec4(i.position, 1));
        auto r = i.position;
        // rc->torque += glm::cross(r, i.force);
        rc->force += i.force * rc->mass;
    }

    // rc->forces = std::vector<Force>();

    // Apply "friction"

    // Step 2: Actually move stuff
    glm::vec3 linear_accel = rc->force / rc->mass;
    rc->linear_velocity += linear_accel * delta;
    // Flux::Transform::globalTranslate(entity, rc->linear_velocity * delta);

    glm::vec3 angular_accel = rc->global_inertia_inv * rc->torque;
    rc->angular_velocity += angular_accel * delta;

    // rc->linear_velocity += rc->linear_velocity * DRAG_FACTOR * delta;
    // rc->angular_velocity += rc->angular_velocity * DRAG_FACTOR * delta;
    
    // TODO: Integrate this better
    // This won't work
    // Flux::Transform::rotateGlobalAxis(entity, glm::vec3(1, 0, 0), rc->angular_velocity.x * delta);
    // Flux::Transform::rotateGlobalAxis(entity, glm::vec3(0, 1, 0), rc->angular_velocity.y * delta);
    // Flux::Transform::rotateGlobalAxis(entity, glm::vec3(0, 0, 1), rc->angular_velocity.z * delta);

    // Integrate
    // Flux::Transform::globalTranslate(entity, rc->linear_velocity);
    // Flux::Transform::rotateGlobalAxis(entity, glm::vec3(1, 0, 0), rc->angular_velocity.x);
    // Flux::Transform::rotateGlobalAxis(entity, glm::vec3(0, 1, 0), rc->angular_velocity.y);
    // Flux::Transform::rotateGlobalAxis(entity, glm::vec3(0, 0, 1), rc->angular_velocity.z);
    
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

    auto tc = entity.getComponent<Transform::TransformCom>();

#ifdef SLOWMO
    delta *= 0.4;
#endif

    // Step 3: Resolve constraints
    // std::cout << contacts.size() << "\n=============\n";
    std::vector<std::pair<glm::vec3, glm::vec3>> lambdas;
    lambdas.resize(contacts.size());

    for (int i = 0; i < 4; i++)
    {
        int cc = 0;
        for (auto c : contacts)
        {
            printf("%f, %f, %f\n", c.offset.x, c.offset.y, c.offset.z);
            glm::vec3 thing = glm::vec3(tc->model * glm::vec4(0, 0, 0, 1)) + c.offset;
            Flux::Debug::drawPoint(thing + (c.normal * c.depth), 0.25, Debug::Colors::Blue);
            Flux::Debug::drawPoint(glm::vec3(tc->model * glm::vec4(0, 0, 0, 1)) + c.offset, 0.25, Flux::Debug::Pink);
            lambdas[cc] = applyImpulse(delta, rc, c.normal * c.depth, c.offset, c.normal * c.depth, lambdas[cc], i == 0);

            cc++;
        }
    }

    // rc->linear_velocity += rc->linear_velocity * DRAG_FACTOR * delta;
    // rc->angular_velocity += rc->angular_velocity * DRAG_FACTOR * delta;

    // Integrate
    Flux::Transform::globalTranslate(entity, rc->linear_velocity * delta);
    Flux::Transform::rotateGlobalAxis(entity, glm::vec3(1, 0, 0), rc->angular_velocity.x * delta);
    Flux::Transform::rotateGlobalAxis(entity, glm::vec3(0, 1, 0), rc->angular_velocity.y * delta);
    Flux::Transform::rotateGlobalAxis(entity, glm::vec3(0, 0, 1), rc->angular_velocity.z * delta);

    // auto tc = entity.getComponent<Transform::TransformCom>();

    // glm::vec3 scale, trans, skew;
    // glm::vec4 pers;
    // glm::quat orientation;

    // glm::decompose(tc->model, scale, orientation, trans, skew, pers);

    // rc->dposition = trans;
    // rc->dorientation = orientation;
    // rc->dscale = scale;

    // rc->dposition = rc->dposition + rc->linear_velocity * delta;
    // glm::vec3 ang = rc->angular_velocity / glm::abs(rc->angular_velocity);
    // glm::vec3 theta = glm::abs(delta * rc->angular_velocity);
    // rc->dorientation = glm::vec2(glm::cos(theta/2), ang * glm::sin(theta/2)) * rc->dorientation;
    // rc->dorientation = glm::rotate(rc->dorientation, rc->angular_velocity[0], glm::vec3(1, 0, 0));
    // rc->dorientation = glm::rotate(rc->dorientation, rc->angular_velocity[1], glm::vec3(0, 1, 0));
    // rc->dorientation = glm::rotate(rc->dorientation, rc->angular_velocity[2], glm::vec3(0, 0, 1));

}

// https://storage.googleapis.com/google-code-archive-downloads/v2/code.google.com/box2d/Tonge_Richard_PhysicsForGame.pdf
std::pair<glm::vec3, glm::vec3> Flux::Physics::applyImpulse(float delta, RigidCom *rc, const glm::vec3& contact_normal, const glm::vec3& normal_offset, glm::vec3 bias, std::pair<glm::vec3, glm::vec3> lambdas, bool first)
{
    glm::vec3 lin_i, ang_i;

    // Create J matrix
    // This is kinda a guess...
    glm::mat3 lin_J = {{contact_normal.x, 0, 0},
                        {0, contact_normal.y, 0},
                        {0, 0, contact_normal.z}};
    
    glm::vec3 r_cross_n = glm::cross(normal_offset, contact_normal);
    glm::mat3 ang_J = {{r_cross_n.x, 0, 0},
                        {0, r_cross_n.y, 0},
                        {0, 0, r_cross_n.z}};

    glm::mat3 m = { {rc->mass, 0,        0       },
                    {0,        rc->mass, 0       },
                    {0,        0,        rc->mass} };

    // Get relative velocity box
    glm::vec3 lin_vrel = lin_J * glm::vec3(1, 1, 1);
    glm::vec3 ang_vrel = ang_J * glm::vec3(1, 1, 1);

    // The dark magic/math box
    glm::vec3 lin_lambda, ang_lambda;
    // lin_lambda = glm::inverse(-(lin_J * glm::inverse(m) * glm::transpose(lin_J))) * (lin_J * (rc->linear_velocity + (delta * rc->force)));
    // ang_lambda = glm::inverse(-(ang_J * rc->global_inertia_inv * glm::transpose(ang_J))) * (ang_J * (rc->angular_velocity + (delta * rc->torque)));
    glm::vec3 lin_t(0), ang_t(0);

    if (first)
    {
        // lin_lambda = glm::inverse(-(lin_J * glm::inverse(m) * glm::transpose(lin_J))) * (lin_J * (lin_vrel + (delta * rc->force)));
        // lin_lambda = (-glm::inverse((lin_J * glm::inverse(m) * glm::transpose(lin_J)))) * (lin_J * (lin_vrel + (delta * rc->force)));
        // lin_lambda = (lin_J * glm::inverse(m) * glm::transpose(lin_J)) * (lin_J * (lin_vrel + (delta * rc->force)));
        // ang_lambda = glm::inverse(-(ang_J * rc->global_inertia_inv * glm::transpose(ang_J))) * (ang_J * (ang_vrel  + (delta * rc->torque)));
        // ang_lambda = (ang_J * rc->global_inertia_inv * glm::transpose(ang_J)) * (ang_J * (ang_vrel  + (delta * rc->torque)));

        // lin_lambda = (lin_J * glm::inverse(m) * glm::transpose(lin_J)) * (lin_J * (rc->linear_velocity + (delta * rc->force)));
        // // ang_lambda = (ang_J * rc->global_inertia_inv * glm::transpose(ang_J)) * (ang_J * (rc->angular_velocity  + (delta * rc->torque)));

        lin_lambda = glm::vec3(0);
        ang_lambda = glm::vec3(0);
    }
    else
    {
        lin_lambda = lambdas.first;
        ang_lambda = lambdas.second;
        // bias = glm::vec3(0, 0, 0);
    }

    lin_t = lin_lambda;
    ang_t = ang_lambda;

    // lin_lambda = lin_lambda - (glm::vec3(1)/rc->mass) * ((lin_J * lin_vrel) + ((-0.8f/delta) * bias));
    // ang_lambda = ang_lambda - (glm::vec3(1)/rc->mass) * ((ang_J * ang_vrel) + ((-0.8f/delta) * bias));

    glm::mat3 lin_m = lin_J * glm::inverse(m) * glm::transpose(lin_J);
    glm::mat3 ang_m = ang_J * rc->global_inertia_inv * glm::transpose(ang_J);

    // glm::vec3 lin_m = lin_J * glm::vec3(); // glm::inverse(m); // * glm::transpose(lin_J);
    // glm::vec3 ang_m = ang_J * glm::vec3(); // rc->global_inertia_inv; // * glm::transpose(ang_J);

    lin_lambda = lin_lambda -(glm::vec3(1)/lin_m) * ((lin_J * rc->linear_velocity) + ((-0.8f/delta) * bias));
    ang_lambda = ang_lambda -(glm::vec3(1)/ang_m) * ((ang_J * rc->angular_velocity) + ((0.8f/delta) * glm::cross(normal_offset, bias)));

    lin_lambda = glm::max(glm::vec3(0), lin_lambda);
    ang_lambda = glm::max(glm::vec3(0), ang_lambda);

    if (lin_lambda.x != lin_lambda.x)
    {
        auto end_bit = (lin_J * (lin_vrel + (delta * rc->force)));
        auto start_bit = glm::inverse(-(lin_J * glm::inverse(m) * glm::transpose(lin_J)));
        auto uninv_startbit = -(lin_J * glm::inverse(m) * glm::transpose(lin_J));
        auto inv_m = glm::inverse(m);
        auto t_linj = glm::transpose(lin_J);
        LOG_ERROR("Nooooooooooooooooooooooooo");
    }

    // lin_lambda = glm::max(lin_lambda, glm::vec3(0));
    // ang_lambda = glm::max(ang_lambda, glm::vec3(0));

    // Impulse to RB impulse box
    lin_i = glm::transpose(lin_J) * (lin_lambda - lin_t);
    ang_i = glm::transpose(ang_J) * (ang_lambda - lin_t);

    // Apply impulse box
    rc->linear_velocity = rc->linear_velocity + (glm::inverse(m) * lin_i); // * delta;
    rc->angular_velocity = rc->angular_velocity + (rc->global_inertia_inv * ang_i); // * delta;

    if (rc->linear_velocity.x != rc->linear_velocity.x || rc->linear_velocity.y != rc->linear_velocity.y || rc->linear_velocity.z != rc->linear_velocity.z ||
        rc->angular_velocity.x != rc->angular_velocity.x || rc->angular_velocity.y != rc->angular_velocity.y || rc->angular_velocity.z != rc->angular_velocity.z)
    {
        LOG_ERROR("Nooooooooooooooooooooooooo");
    }

    return {lin_lambda, ang_lambda};
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
    // glm::vec3 sizes = bc->box->max_pos - bc->box->min_pos;

    glm::mat3 inertia;
    inertia[0][0] = (1.0f/12.0f) * mass * ((sizes.y * sizes.y) + (sizes.z * sizes.z));
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