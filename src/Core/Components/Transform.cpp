// Copyright(c) 2019 - 2020, #Momo
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met :
// 
// 1. Redistributions of source code must retain the above copyright notice, this
// list of conditions and the following disclaimer.
// 
// 2. Redistributions in binary form must reproduce the above copyright notice,
// this list of conditions and the following disclaimer in the documentation
// and /or other materials provided with the distribution.
// 
// 3. Neither the name of the copyright holder nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED.IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include "Transform.h"

namespace MxEngine
{
    static Matrix4x4 I(1.0f);

    static Vector3 ForwardVec = MakeVector3( 0.0f, 0.0f, 1.0f);
    static Vector3 RightVec   = MakeVector3(-1.0f, 0.0f, 0.0f);
    static Vector3 UpVec      = MakeVector3( 0.0f, 1.0f, 0.0f);

    void TransformComponent::Copy(const TransformComponent& other) noexcept
    {
        this->translation = other.GetTranslation();
        this->scale = other.GetScale();
        this->rotation = other.GetRotation();
        this->eulerRotation = other.GetEulerRotation();
        other.GetMatrix(this->transform);
        other.GetNormalMatrix(this->transform, this->normalMatrix);
        this->needTransformUpdate = false;
        this->needRotationUpdate = false;
    }

    TransformComponent::TransformComponent(const TransformComponent& other)
    {
        this->Copy(other);
    }

    TransformComponent::TransformComponent(TransformComponent&& other) noexcept
    {
        this->Copy(other);
    }

    TransformComponent& TransformComponent::operator=(const TransformComponent& other)
    {
        this->Copy(other);
        return *this;
    }

    TransformComponent& TransformComponent::operator=(TransformComponent&& other) noexcept
    {
        this->Copy(other);
        return *this;
    }

    const Matrix4x4& TransformComponent::GetMatrix() const
    {
        if (this->needTransformUpdate)
        {
            Matrix4x4 Translation = MxEngine::Translate(I, this->translation);
            Matrix4x4 Rotation = ToMatrix(this->rotation);
            Matrix4x4 Scale = MxEngine::Scale(I, this->scale);
            this->transform = Translation * Rotation * Scale;
            if (this->scale.x == this->scale.y && this->scale.y == this->scale.z)
                this->normalMatrix = Rotation;
            else
                this->normalMatrix = Transpose(Inverse(this->transform));
            this->needTransformUpdate = false;
        }
        return this->transform;
    }

    const Matrix3x3& TransformComponent::GetNormalMatrix() const
    {
        auto _ = GetMatrix();
        return this->normalMatrix;
    }

    void TransformComponent::GetMatrix(Matrix4x4& inPlaceMatrix) const
    {
        Matrix4x4 Translation = MxEngine::Translate(I, this->translation);
        Matrix4x4 Rotation = ToMatrix(this->rotation);
        Matrix4x4 Scale = MxEngine::Scale(I, this->scale);
        inPlaceMatrix = Translation * Rotation * Scale;
    }

    void TransformComponent::GetNormalMatrix(const Matrix4x4& model, Matrix3x3& inPlaceMatrix) const
    {
        if (this->scale.x == this->scale.y && this->scale.y == this->scale.z)
            inPlaceMatrix = Matrix3x3(model);
        else
            inPlaceMatrix = Transpose(Inverse(model));
    }

    const Vector3& TransformComponent::GetTranslation() const
    {
        return this->translation;
    }

    const Quaternion& TransformComponent::GetRotation() const
    {
        return this->rotation;
    }

    const Vector3& TransformComponent::GetScale() const
    {
        return this->scale;
    }

    const Vector3& TransformComponent::GetEulerRotation() const
    {
        if (this->needRotationUpdate)
        {
            this->eulerRotation = MakeEulerAngles(this->rotation);
            this->needRotationUpdate = false;
        }
        return this->eulerRotation;
    }

    const Vector3& TransformComponent::GetPosition() const //-V524
    {
        return this->translation;
    }

    TransformComponent& TransformComponent::SetTranslation(const Vector3& dist)
    {
        this->translation = dist;
        this->needTransformUpdate = true;
        return *this;
    }

    TransformComponent& TransformComponent::SetRotation(float angle, const Vector3& axis)
    {
        return this->SetRotation(MakeQuaternion(angle, axis));
    }

    TransformComponent& TransformComponent::SetRotation(const Quaternion& q)
    {
        this->rotation = q;
        this->needRotationUpdate = true;
        this->needTransformUpdate = true;
        return *this;
    }

    TransformComponent& TransformComponent::SetScale(const Vector3& scale)
    {
        this->scale = scale;
        this->needTransformUpdate = true;
        return *this;
    }

    TransformComponent& TransformComponent::SetScale(float scale) 
    {
        return this->SetScale(MakeVector3(scale));
    }

    TransformComponent& TransformComponent::SetPosition(const Vector3& position)
    {
        return this->SetTranslation(position);
    }

    TransformComponent& TransformComponent::Scale(float scale) 
    {
        return this->Scale(MakeVector3(scale));
    }

    TransformComponent& TransformComponent::Scale(const Vector3& scale)
    {
        this->scale *= scale;
        this->needTransformUpdate = true;
        return *this;
    }

    TransformComponent& TransformComponent::ScaleX(float scale) 
    {
        return this->Scale(MakeVector3(scale, 1.0f, 1.0f));
    }

    TransformComponent& TransformComponent::ScaleY(float scale) 
    {
        return this->Scale(MakeVector3(1.0f, scale, 1.0f));
    }

    TransformComponent& TransformComponent::ScaleZ(float scale) 
    {
        return this->Scale(MakeVector3(1.0f, 1.0f, scale));
    }

    TransformComponent& TransformComponent::Rotate(float angle, const Vector3& axis)
    {
        return this->Rotate(MakeQuaternion(Radians(angle), axis));
    }

    TransformComponent& TransformComponent::Rotate(const Quaternion& q)
    {
        this->rotation *= q;
        this->needRotationUpdate = true;
        this->needTransformUpdate = true;
        return *this;
    }

    TransformComponent& TransformComponent::RotateX(float angle)
    {
        return this->Rotate(angle, MakeVector3(1.0f, 0.0f, 0.0f));
    }

    TransformComponent& TransformComponent::RotateY(float angle)
    {
        return this->Rotate(angle, MakeVector3(0.0f, 1.0f, 0.0f));
    }

    TransformComponent& TransformComponent::RotateZ(float angle)
    {
        return this->Rotate(angle, MakeVector3(0.0f, 0.0f, 1.0f));
    }

    TransformComponent& TransformComponent::Translate(const Vector3& dist)
    {
        this->translation += dist;
        this->needTransformUpdate = true;
        return *this;
    }

    TransformComponent& TransformComponent::TranslateX(float x)
    {
        return this->Translate(MakeVector3(x, 0.0f, 0.0f));
    }

    TransformComponent& TransformComponent::TranslateY(float y)
    {
        return this->Translate(MakeVector3(0.0f, y, 0.0f));
    }

    TransformComponent& TransformComponent::TranslateZ(float z)
    {
        return this->Translate(MakeVector3(0.0f, 0.0f, z));
    }

    TransformComponent& TransformComponent::TranslateForward(float dist)
    {
        this->Translate(this->rotation * ForwardVec * dist);
        return *this;
    }

    TransformComponent& TransformComponent::TranslateRight(float dist)
    {
        this->Translate(this->rotation * RightVec * dist);
        return *this;
    }

    TransformComponent& TransformComponent::TranslateUp(float dist)
    {
        this->Translate(this->rotation * UpVec * dist);
        return *this;
    }

    TransformComponent& TransformComponent::LookAt(const Vector3& point)
    {
        auto q = LookAtRotation(Normalize(this->translation - point), MakeVector3(0.00001f, 1.0f, 0.0f));
        this->SetRotation(q);
        return *this;
    }

    TransformComponent& TransformComponent::LookAtXY(const Vector3& point)
    {
        auto v = this->translation - point;
        auto q = LookAtRotation(Normalize(MakeVector3(v.x, v.y, 0.0f)), MakeVector3(0.00001f, 0.0f, 1.0f));
        this->SetRotation(q);
        return *this;
    }

    TransformComponent& TransformComponent::LookAtXZ(const Vector3& point)
    {
        auto v = this->translation - point;
        auto q = LookAtRotation(Normalize(MakeVector3(v.x, 0.0f, v.z)), MakeVector3(0.00001f, 1.0f, 0.0f));
        this->SetRotation(q);
        return *this;
    }

    TransformComponent& TransformComponent::LookAtYZ(const Vector3& point)
    {
        auto v = this->translation - point;
        auto q = LookAtRotation(Normalize(MakeVector3(0.0f, v.y, v.z)), MakeVector3(1.0f, 0.0f, 0.00001f));
        this->SetRotation(q);
        return *this;
    }
}