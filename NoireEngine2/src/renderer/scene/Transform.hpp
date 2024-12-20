#pragma once

#include "math/Math.hpp"

class Transform 
{
public:
    Transform() = default;
    Transform(const Transform& other);

    void Update();

    Transform(glm::vec3 t);
    Transform(glm::vec3 t, glm::vec3 euler);
    Transform(glm::vec3 t, glm::vec3 euler, glm::vec3 s);
    Transform(glm::vec3 t, glm::quat q);
    Transform(glm::vec3 t, glm::quat q, glm::vec3 s);

    // by default, these return ws positions
    inline glm::vec3 position() const;
    inline glm::quat rotation() const;
    inline glm::vec3 eulerAngles() const { return glm::eulerAngles(rotation()); }
    inline glm::vec3 scale() const;
    inline Transform* parent() const { return m_Parent; }

    void SetPosition(glm::vec3);
    void SetPosition(glm::vec3&);

    void SetRotation(glm::quat);
    void SetRotation(glm::quat&);

    void SetScale(glm::vec3);
    void SetScale(glm::vec3&);

    void SetParent(Transform* newParent) { m_Parent = newParent; }

public:
    void Inspect();

public:
    // the local transformation matrix
    glm::mat4 Local() const;
    const glm::mat4& GetLocal() const;

    // the transformation matrix in worldspace, and local to world operations
    glm::mat4 World() const;
    glm::mat4 WorldDirty();

    // the local operations
    glm::mat4 LocalInverse() const;
    glm::vec3 LocalInverseLocation() const;
    glm::vec3 LocalInverseScale() const;
    glm::quat LocalInverseRotation() const;

    // the world to local transformation matrix and operations
    glm::mat4 WorldInverse() const;
    glm::vec3 WorldInverseLocation() const;
    glm::vec3 WorldInverseScale() const;
    glm::quat WorldInverseRotation() const;

    glm::vec3 Forward() const; // the forward vector in relation to the transform
    glm::vec3 Back() const; // the Back vector in relation to the transform
    glm::vec3 Right() const; // the Right vector in relation to the transform
    glm::vec3 Left() const; // the Left vector in relation to the transform
    glm::vec3 Up() const; // the Up vector in relation to the transform
    glm::vec3 Down() const; // the Down vector in relation to the transform

    // decomposes a transformation matrix `m`
    static void Decompose(const glm::mat4& m, glm::vec3& pos, glm::quat& rot, glm::vec3& scale);

    void Decompose(const glm::mat4& m);

    // applies a transformation matrix
    void Apply(glm::mat4& transformation);

    // returns a rotation quad to rotate the transform along a certian axis by `angle` degrees
    static glm::quat Rotation(const float& angle, const glm::vec3& axis, bool useRadians = false);

    // rotates the transform along an `axis` by `angle`
    void Rotate(const float& angle, const glm::vec3& axis, bool useRadians = false);

    // Rotates the transform about axis passing through point in local coordinates by angle DEGREES.
    void RotateAround(const glm::vec3& point, const float& angle, const glm::vec3& axis);

    // Rotates the transform about axis passing through point in local space
    void RotateAround(const glm::vec3& point, const float& angle, const glm::quat& rot);

    // Moves the transform in the direction and distance of translation.
    void Translate(const glm::vec3& translation, bool useWorldspace = false);

    void LerpTo(const glm::vec3& targetPosition, float t);

    bool operator!() const {
        return this == nullptr;
    }

    bool isDirty = true;
    bool wasDirtyThisFrame = false;

private:
    // attach a new transform as parent
    void AttachParent(Transform& parentTransform);

    // remove the current parent in hierarchy
    void RemoveParent();

    glm::mat4 localMat;

    Transform* m_Parent = nullptr;
    glm::vec3 m_Position = Vec3::Zero;
    glm::quat m_Rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    glm::vec3 m_Scale = Vec3::One;
};