#include "Flux/ECS.hh"
#include "Flux/Log.hh"
#include "Flux/Renderer.hh"
#include "glm/ext/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glm/matrix.hpp"
#include "glm/glm.hpp"

#include <cstdio>
#include <string>
#include <glm/gtc/matrix_transform.hpp>

using namespace Flux;

static Flux::EntityRef camera;

static Flux::ComponentTypeID MeshComponentID = -1;
static Flux::ComponentTypeID TransformComponentID = -1;
static Flux::ComponentTypeID CameraComponentID = -1;

void Flux::Transform::setCamera(EntityRef entity)
{
    if (MeshComponentID == -1)
    {
        // Setup components
        MeshComponentID = Flux::getComponentType("mesh");
        TransformComponentID = Flux::getComponentType("transform");
        CameraComponentID = Flux::getComponentType("camera");
    }

    if (!entity.hasComponent<TransformCom>())
    {
        LOG_WARN("Camera must have transform component.");
        return;
    }

    auto cc = new CameraCom;
    cc->view_matrix = glm::mat4();
    entity.addComponent(cc);
}

void Transform::giveTransform(EntityRef entity)
{
    static float a[16] = {
        1,0,0,0,
        0,1,0,0,
        0,0,1,0,
        0,0,0,1
    };
    auto tc = new Flux::Transform::TransformCom;
    tc->transformation = glm::make_mat4(a);
    tc->model_view = glm::make_mat4(a);
    tc->has_parent = false;
    tc->parent = EntityRef();

    entity.addComponent(tc);
}

void Flux::Transform::rotate(EntityRef entity, const glm::vec3& axis, const float& angle_rad)
{
    if (MeshComponentID == -1)
    {
        // Setup components
        MeshComponentID = Flux::getComponentType("mesh");
        TransformComponentID = Flux::getComponentType("transform");
        CameraComponentID = Flux::getComponentType("camera");
    }

    if (!entity.hasComponent<TransformCom>())
    {
        LOG_WARN("Transform component required for transformation");
        return;
    }

    auto tc = entity.getComponent<TransformCom>();
    
    tc->transformation = glm::rotate(tc->transformation, angle_rad, axis);
}

void Flux::Transform::translate(EntityRef entity, const glm::vec3& offset)
{
    if (!entity.hasComponent<TransformCom>())
    {
        LOG_WARN("Transform component required for transformation");
        return;
    }

    auto tc = entity.getComponent<TransformCom>();
    auto o = glm::vec3(offset.x, offset.y, offset.z);
    tc->transformation = glm::translate(tc->transformation, o);
}

void Flux::Transform::scale(EntityRef entity, const glm::vec3& scalar)
{
    if (MeshComponentID == -1)
    {
        // Setup components
        MeshComponentID = Flux::getComponentType("mesh");
        TransformComponentID = Flux::getComponentType("transform");
        CameraComponentID = Flux::getComponentType("camera");
    }

    if (!entity.hasComponent<TransformCom>())
    {
        LOG_WARN("Transform component required for transformation");
        return;
    }

    auto tc = entity.getComponent<TransformCom>();
    
    tc->transformation = glm::scale(tc->transformation, scalar);
}

void Flux::Transform::setTranslation(EntityRef entity, const glm::vec3 &translation)
{
    if (MeshComponentID == -1)
    {
        // Setup components
        MeshComponentID = Flux::getComponentType("mesh");
        TransformComponentID = Flux::getComponentType("transform");
        CameraComponentID = Flux::getComponentType("camera");
    }

    if (!entity.hasComponent<TransformCom>())
    {
        LOG_WARN("Transform component required for transformation");
        return;
    }

    auto tc = entity.getComponent<TransformCom>();

    tc->transformation[3] = glm::vec4(translation, 1);
}

glm::vec3 Flux::Transform::getTranslation(EntityRef entity)
{
    if (MeshComponentID == -1)
    {
        // Setup components
        MeshComponentID = Flux::getComponentType("mesh");
        TransformComponentID = Flux::getComponentType("transform");
        CameraComponentID = Flux::getComponentType("camera");
    }

    if (!entity.hasComponent<TransformCom>())
    {
        LOG_WARN("Transform component required for transformation");
        return glm::vec3();
    }

    auto tc = entity.getComponent<TransformCom>();

    return glm::vec3(tc->transformation[3]);
}

glm::mat4 getParentTransform(EntityRef entity)
{
    if (entity.hasComponent<Flux::Transform::TransformCom>())
    {
        auto tc = entity.getComponent<Transform::TransformCom>();
        
        if (tc->has_parent)
        {
            return getParentTransform(tc->parent) * tc->transformation;
        }

        return tc->transformation;
    }
    else
    {
        static int a[16] = {
            1, 0, 0, 0,
            0, 1, 0, 0,
            0, 0, 1, 0,
            0, 0, 0, 1
        };
        return glm::make_mat4(a);
    }
}

void Flux::Transform::CameraSystem::runSystem(EntityRef entity, float delta)
{
    if (entity.hasComponent<Flux::Transform::CameraCom>())
    {
        // Calculate camera rotations
        auto tc = entity.getComponent<Transform::TransformCom>();
        auto cc = entity.getComponent<Transform::CameraCom>();

        auto actual = getParentTransform(entity);
        cc->view_matrix = glm::inverse(actual);

        camera = entity;
    }
}

void Flux::Transform::TransformationSystem::runSystem(EntityRef entity, float delta)
{
    if (entity.hasComponent<Flux::Transform::TransformCom>())
    {
        // Calculate model view matrix
        auto tc = entity.getComponent<Transform::TransformCom>();

        // Get camera
        auto cc = camera.getComponent<Transform::CameraCom>();

        // Recursivly add parent transform, as well
        // TODO: Maybe make this more efficient?
        auto pt = getParentTransform(entity);

        auto mv = cc->view_matrix * pt;
        tc->model_view = mv;
        int a = 0;
    }
}

void Flux::Transform::addTransformSystems(ECSCtx *ctx)
{
    ctx->addSystemFront(new TransformationSystem, true);
    ctx->addSystemFront(new CameraSystem, true);
}

void Flux::Transform::setParent(EntityRef entity, EntityRef parent)
{
    if (!entity.hasComponent<Flux::Transform::TransformCom>())
    {
        LOG_ERROR("Parent must have transform component");
        return;
    }

    auto tc = entity.getComponent<Transform::TransformCom>();
    if (tc->has_parent)
    {
        removeParent(entity);
    }

    tc->parent = parent;
    tc->has_parent = true;
}

void Flux::Transform::removeParent(EntityRef entity)
{
    if (!entity.hasComponent<Flux::Transform::TransformCom>())
    {
        LOG_ERROR("Child must have transform component");
        return;
    }

    auto tc = entity.getComponent<Transform::TransformCom>();

    if (tc->has_parent)
    {
        tc->has_parent = false;
    }

    // No point changing the parent variable
}