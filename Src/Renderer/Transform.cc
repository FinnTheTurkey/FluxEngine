#include "Flux/ECS.hh"
#include "Flux/Log.hh"
#include "Flux/Renderer.hh"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/detail/type_vec.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glm/matrix.hpp"
#include "glm/glm.hpp"
#include "glm/gtx/euler_angles.hpp"
#include <glm/gtx/matrix_decompose.hpp>

#include <cstdio>
#include <string>
#include <glm/gtc/matrix_transform.hpp>

using namespace Flux;

static Flux::EntityRef camera;

glm::vec3 Transform::camera_position = glm::vec3(0, 0, 0);

void Flux::Transform::setCamera(EntityRef entity)
{
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
    tc->model = glm::make_mat4(a);
    tc->has_parent = false;
    tc->parent = EntityRef();
    tc->visible = true;

    entity.addComponent(tc);
}

void Transform::setVisible(EntityRef entity, bool vis)
{
    entity.getComponent<TransformCom>()->visible = vis;
}

glm::mat4 Transform::getParentTransform(EntityRef entity)
{
    auto b = false;
    return _getParentTransform(entity, &b);
}

glm::mat4 Transform::_getParentTransform(EntityRef entity, bool* has_changed)
{
    if (entity.hasComponent<Flux::Transform::TransformCom>())
    {
        auto tc = entity.getComponent<Transform::TransformCom>();

        if (tc->has_changed == true)
        {
            *has_changed = true;
        }
        
        if (tc->has_parent)
        {
            if (tc->parent.getEntityID() == -1)
            {
                // ...?
                return tc->transformation;
            }
            auto output = _getParentTransform(tc->parent, has_changed) * tc->transformation;
            if (*has_changed != tc->has_changed)
            {
                tc->has_changed = *has_changed;
            }
            return output;
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

bool Flux::Transform::getVisibility(EntityRef entity)
{
    if (entity.hasComponent<Flux::Transform::TransformCom>())
    {
        auto tc = entity.getComponent<Transform::TransformCom>();

        if (!tc->visible)
        {
            return false;
        }
        
        if (tc->has_parent)
        {
            return getVisibility(tc->parent);
        }

        return true;
    }
    else
    {
        return true;
    }
}

void Flux::Transform::rotate(EntityRef entity, const glm::vec3& axis, const float& angle_rad)
{

    if (!entity.hasComponent<TransformCom>())
    {
        LOG_WARN("Transform component required for transformation");
        return;
    }

    auto tc = entity.getComponent<TransformCom>();
    
    tc->transformation = glm::rotate(tc->transformation, angle_rad, axis);
    tc->has_changed = true;
}

void Flux::Transform::rotateGlobalAxis(EntityRef entity, const glm::vec3 &axis, const float &angle_rad)
{
    if (!entity.hasComponent<TransformCom>())
    {
        LOG_WARN("Transform component required for transformation");
        return;
    }

    auto tc = entity.getComponent<TransformCom>();

    // Rotate the axis by the inverse transform to "undo" the object's transform
    tc->transformation = glm::rotate(tc->transformation, angle_rad, glm::vec3(glm::inverse(tc->transformation) * glm::vec4(axis, 0)));
    tc->has_changed = true;
}

void Flux::Transform::globalTranslate(EntityRef entity, const glm::vec3 &offset)
{
    auto global_transform = getParentTransform(entity);

    // To translate globally, we have to apply the inverse global translation
    // To our offset
    // I really hope this works
    Flux::Transform::translate(entity, glm::vec3(glm::inverse(global_transform) * glm::vec4(offset, 0)));;
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
    tc->has_changed = true;
}

void Flux::Transform::scale(EntityRef entity, const glm::vec3& scalar)
{
    if (!entity.hasComponent<TransformCom>())
    {
        LOG_WARN("Transform component required for transformation");
        return;
    }

    auto tc = entity.getComponent<TransformCom>();
    
    tc->transformation = glm::scale(tc->transformation, scalar);
    tc->has_changed = true;
}

void Flux::Transform::setTranslation(EntityRef entity, const glm::vec3 &translation)
{
    if (!entity.hasComponent<TransformCom>())
    {
        LOG_WARN("Transform component required for transformation");
        return;
    }

    auto tc = entity.getComponent<TransformCom>();

    tc->transformation[3] = glm::vec4(translation, 1);
    tc->has_changed = true;
}

glm::vec3 Flux::Transform::getTranslation(EntityRef entity)
{

    if (!entity.hasComponent<TransformCom>())
    {
        LOG_WARN("Transform component required for transformation");
        return glm::vec3();
    }

    auto tc = entity.getComponent<TransformCom>();
    auto o = glm::vec3(tc->transformation[3][0], tc->transformation[3][1], tc->transformation[3][2]);
    return o;
}

glm::vec3 Flux::Transform::getGlobalTranslation(EntityRef entity)
{
    if (!entity.hasComponent<TransformCom>())
    {
        LOG_WARN("Transform component required for transformation");
        return glm::vec3();
    }

    auto trans = getParentTransform(entity);
    auto o = glm::vec3(trans[3][0], trans[3][1], trans[3][2]);
    return o;
}

void Flux::Transform::setGlobalTranslation(EntityRef entity, const glm::vec3 &translation)
{
    if (!entity.hasComponent<TransformCom>())
    {
        LOG_WARN("Transform component required for transformation, duh");
        return;
    }

    auto tc = entity.getComponent<TransformCom>();

    // parent_transform is local position to global position
    // Therefore it's inverse is global position to local position
    // Math.
    auto parent_transform = getParentTransform(entity);

    tc->transformation[3] = glm::inverse(parent_transform) * glm::vec4(translation, 1);
    tc->has_changed = true;
}

glm::vec3 Flux::Transform::getRotation(EntityRef entity)
{
    if (!entity.hasComponent<TransformCom>())
    {
        LOG_WARN("Transform component required for transformation");
        return glm::vec3(0, 0, 0);
    }

    auto tc = entity.getComponent<TransformCom>();

    glm::vec3 rotation;
    glm::extractEulerAngleXYZ(tc->transformation, rotation.x, rotation.y, rotation.z);

    return rotation;
}

glm::vec3 Flux::Transform::getGlobalRotation(EntityRef entity)
{
    if (!entity.hasComponent<TransformCom>())
    {
        LOG_WARN("Transform component required for transformation");
        return glm::vec3(0, 0, 0);
    }

    auto tc = entity.getComponent<TransformCom>();
    auto parent_transform = getParentTransform(entity);

    glm::vec3 rotation;
    glm::extractEulerAngleXYZ(parent_transform * tc->transformation, rotation.x, rotation.y, rotation.z);

    // rotation = glm::inverse(parent_transform) * glm::vec4(rotation, 1);

    return rotation;
}

void Flux::Transform::setRotation(EntityRef entity, const glm::vec3& rotation)
{
    if (!entity.hasComponent<TransformCom>())
    {
        LOG_WARN("Transform component required for transformation");
        return;
    }

    auto tc = entity.getComponent<TransformCom>();

    // There's probably a better way of doing this...
    glm::vec3 scales, translation;
    scales = getScale(entity);
    translation = getTranslation(entity);

    // Create new Matrix with correct rotation
    tc->transformation = glm::eulerAngleXYZ(rotation.x, rotation.y, rotation.z);
    
    // Add back Translation and scale
    setTranslation(entity, translation);
    setScale(entity, scales);
}

void Flux::Transform::setGlobalRotation(EntityRef entity, const glm::vec3& rotation)
{
    if (!entity.hasComponent<TransformCom>())
    {
        LOG_WARN("Transform component required for transformation");
        return;
    }

    auto tc = entity.getComponent<TransformCom>();
    auto parent_transform = getParentTransform(entity);

    glm::vec3 local_rotation = glm::inverse(parent_transform) * glm::vec4(rotation, 1);

    setRotation(entity, local_rotation);
}

glm::vec3 Transform::getScale(EntityRef entity)
{
    if (!entity.hasComponent<TransformCom>())
    {
        LOG_WARN("Transform component required for transformation");
        return glm::vec3(0, 0, 0);
    }

    auto tc = entity.getComponent<TransformCom>();

    // Load in transformation
    glm::vec3 translation;
    glm::vec3 scale;
    glm::quat rotation_q;
    glm::vec3 skew;
    glm::vec4 perspective;
    glm::decompose(tc->transformation, scale, rotation_q, translation, skew, perspective);

    // Apparently the quaternion is incorrect...?
    // We don't care about the rotation so this is not nessesairy
    // rotation_q = glm::conjugate(rotation_q);
    // rotation = glm::eulerAngles(rotation_q);

    return scale;
}

void Transform::setScale(EntityRef entity, const glm::vec3& new_scale)
{
    if (!entity.hasComponent<TransformCom>())
    {
        LOG_WARN("Transform component required for transformation");
        return;
    }

    auto tc = entity.getComponent<TransformCom>();

    // Find scale
    glm::vec3 old_scale;
    old_scale = getScale(entity);

    // Calculate the difference between the current scale and the target
    glm::vec3 diff = new_scale - old_scale;

    // Scale it by the difference
    scale(entity, diff);
}

void Flux::Transform::CameraSystem::runSystem(EntityRef entity, float delta)
{
    if (entity.hasComponent<Flux::Transform::CameraCom>())
    {
        // Calculate camera rotations
        // auto tc = entity.getComponent<Transform::TransformCom>();
        auto cc = entity.getComponent<Transform::CameraCom>();

        auto actual = getParentTransform(entity);
        cc->view_matrix = glm::inverse(actual);

        camera = entity;
        camera_position = getGlobalTranslation(camera);
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
        tc->model = pt;
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
    tc->has_changed = true;
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
    tc->has_changed = true;

    // No point changing the parent variable
}