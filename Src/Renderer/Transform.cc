#include "Flux/ECS.hh"
#include "Flux/Log.hh"
#include "Flux/Renderer.hh"
#include "glm/ext/matrix_transform.hpp"
#include "glm/matrix.hpp"

#include <cstdio>
#include <string>
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

void Flux::Transform::scale(ECSCtx* ctx, EntityID entity, const glm::vec3& scalar)
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
    
    tc->transformation = glm::scale(tc->transformation, scalar);
}

void Flux::Transform::setTranslation(ECSCtx* ctx, EntityID entity, const glm::vec3 &translation)
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

    tc->transformation[3] = glm::vec4(translation, 1);
}

glm::vec3 Flux::Transform::getTranslation(ECSCtx* ctx, EntityID entity)
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
        return glm::vec3();
    }

    auto tc = (TransformCom*)Flux::getComponent(ctx, entity, TransformComponentID);

    return glm::vec3(tc->transformation[3]);
}

glm::mat4 getParentTransform(ECSCtx* ctx, EntityID entity)
{
    if (Flux::hasComponent(ctx, entity, TransformComponentID))
    {
        auto tc = (Transform::TransformCom*)Flux::getComponent(ctx, entity, TransformComponentID);
        
        if (tc->has_parent)
        {
            return getParentTransform(ctx, tc->parent) * tc->transformation;
        }

        return tc->transformation;
    }
    else
    {
        return glm::mat4();
    }
}

void cameraSystem(ECSCtx* ctx, EntityID entity, float delta)
{
    if (Flux::hasComponent(ctx, entity, CameraComponentID))
    {
        // Calculate camera rotations
        auto tc = (Transform::TransformCom*)Flux::getComponent(ctx, entity, TransformComponentID);
        auto cc = (Transform::CameraCom*)Flux::getComponent(ctx, entity, CameraComponentID);

        cc->view_matrix = glm::inverse(getParentTransform(ctx, entity));

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

        // Recursivly add parent transform, as well
        // TODO: Maybe make this more efficient?
        auto pt = getParentTransform(ctx, entity);

        auto mv = cc->view_matrix * pt;
        tc->model_view = mv;
        int a = 0;
    }
}

void Flux::Transform::addTransformSystems(ECSCtx *ctx)
{
    Flux::addSystemFront(ctx, transformSystem, true);
    Flux::addSystemFront(ctx, cameraSystem, true);
}

void Flux::Transform::setParent(ECSCtx *ctx, EntityID entity, EntityID parent)
{
    if (!Flux::hasComponent(ctx, parent, TransformComponentID))
    {
        LOG_ERROR("Parent must have transform component");
        return;
    }

    auto tc = (Transform::TransformCom*)Flux::getComponent(ctx, entity, TransformComponentID);
    if (tc->has_parent)
    {
        removeParent(ctx, entity);
    }

    tc->parent = parent;
    tc->has_parent = true;
}

void Flux::Transform::removeParent(ECSCtx *ctx, EntityID entity)
{
    auto tc = (Transform::TransformCom*)Flux::getComponent(ctx, entity, TransformComponentID);

    if (tc->has_parent)
    {
        tc->has_parent = false;
    }

    // No point changing the parent variable
}