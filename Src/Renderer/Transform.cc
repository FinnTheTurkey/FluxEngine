#include "Flux/ECS.hh"
#include "Flux/Log.hh"
#include "Flux/Renderer.hh"
#include "glm/ext/matrix_transform.hpp"
#include "glm/matrix.hpp"

#include <glm/gtc/matrix_transform.hpp>

using namespace Flux;

static Flux::EntityID camera;

static Flux::ComponentTypeID MeshComponentID = -1;
static Flux::ComponentTypeID TransformComponentID = -1;
static Flux::ComponentTypeID CameraComponentID = -1;

void Flux::Transform::setCamera(ECSCtx *ctx, EntityID entity)
{
    if (MeshComponentID == -1)
    {
        // Setup components
        MeshComponentID = Flux::getComponentType("mesh");
        TransformComponentID = Flux::getComponentType("transform");
        CameraComponentID = Flux::getComponentType("camera");
    }

    if (!Flux::hasComponent(ctx, entity, TransformComponentID))
    {
        LOG_WARN("Camera must have transform component.");
        return;
    }

    auto cc = new CameraCom;
    Flux::addComponent(ctx, entity, CameraComponentID, cc);
}

void Flux::Transform::rotate(ECSCtx *ctx, EntityID entity, const glm::vec3& axis, const float& angle_rad)
{
    if (MeshComponentID == -1)
    {
        // Setup components
        MeshComponentID = Flux::getComponentType("mesh");
        TransformComponentID = Flux::getComponentType("transform");
        CameraComponentID = Flux::getComponentType("camera");
    }

    if (!Flux::hasComponent(ctx, entity, TransformComponentID))
    {
        LOG_WARN("Transform component required for transformation");
        return;
    }

    auto tc = (TransformCom*)Flux::getComponent(ctx, entity, TransformComponentID);
    
    tc->transformation = glm::rotate(tc->transformation, angle_rad, axis);
}

void Flux::Transform::translate(ECSCtx *ctx, EntityID entity, const glm::vec3& offset)
{
    if (MeshComponentID == -1)
    {
        // Setup components
        MeshComponentID = Flux::getComponentType("mesh");
        TransformComponentID = Flux::getComponentType("transform");
        CameraComponentID = Flux::getComponentType("camera");
    }

    if (!Flux::hasComponent(ctx, entity, TransformComponentID))
    {
        LOG_WARN("Transform component required for transformation");
        return;
    }

    auto tc = (TransformCom*)Flux::getComponent(ctx, entity, TransformComponentID);
    
    tc->transformation = glm::translate(tc->transformation, offset);
}

void cameraSystem(ECSCtx* ctx, EntityID entity, float delta)
{
    if (Flux::hasComponent(ctx, entity, CameraComponentID))
    {
        // Calculate camera rotations
        auto tc = (Transform::TransformCom*)Flux::getComponent(ctx, entity, TransformComponentID);
        auto cc = (Transform::CameraCom*)Flux::getComponent(ctx, entity, CameraComponentID);

        // TODO: Fix random segfault
        // For some reason entity 1023 has a camera - sometimes
        cc->view_matrix = glm::inverse(tc->transformation);

        camera = entity;
    }
}

void transformSystem(ECSCtx* ctx, EntityID entity, float delta)
{
    if (Flux::hasComponent(ctx, entity, TransformComponentID))
    {
        // Calculate model view matrix
        auto tc = (Transform::TransformCom*)Flux::getComponent(ctx, entity, TransformComponentID);

        // Get camera
        auto cc = (Transform::CameraCom*)Flux::getComponent(ctx, camera, CameraComponentID);

        auto mv = cc->view_matrix * tc->transformation;
        tc->model_view = mv;
        int a = 0;
    }
}

void Flux::Transform::addTransformSystems(ECSCtx *ctx)
{
    Flux::addSystemFront(ctx, transformSystem, true);
    Flux::addSystemFront(ctx, cameraSystem, true);
}