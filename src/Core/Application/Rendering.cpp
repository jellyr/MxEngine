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

#include "Rendering.h"
#include "Core/Application/Application.h"

namespace MxEngine
{
    CameraController::Handle MxEngine::Rendering::GetViewport()
    {
        return Application::Get()->GetRenderAdaptor().Viewport;
    }

    void Rendering::SetViewport(const CameraController::Handle& viewport)
    {
        Application::Get()->GetRenderAdaptor().Viewport = viewport;
    }

    void Rendering::ResizeViewport(size_t width, size_t height)
    {
        auto viewport = Rendering::GetViewport();
        if (viewport.IsValid())
        {
            viewport->ResizeRenderTexture(width, height);
        }
    }

    VectorInt2 Rendering::GetViewportSize()
    {
        VectorInt2 size{ 0, 0 };
        auto viewport = Rendering::GetViewport();
        if (viewport.IsValid())
        {
            auto texture = viewport->GetRenderTexture();
            size.x = (int)texture->GetWidth();
            size.y = (int)texture->GetHeight();
        }
        return size;
    }

    RenderController& Rendering::GetController()
    {
        return Application::Get()->GetRenderAdaptor().Renderer;
    }

    RenderAdaptor& Rendering::GetAdaptor()
    {
        return Application::Get()->GetRenderAdaptor();
    }

    #define FWD(func_name, ...) Application::Get()->GetRenderAdaptor().func_name(__VA_ARGS__)

    void Rendering::SetRenderToDefaultFrameBuffer(bool value)
    {
        FWD(SetRenderToDefaultFrameBuffer, value);
    }

    bool Rendering::IsRenderedToDefaultFrameBuffer()
    {
        return FWD(IsRenderedToDefaultFrameBuffer);
    }

    void Rendering::SetFogColor(const Vector3& color)
    {
        FWD(SetFogColor, color);
    }

    const Vector3& Rendering::GetFogColor()
    {
        return FWD(GetFogColor);
    }

    void Rendering::SetFogDensity(float density)
    {
        FWD(SetFogDensity, density);
    }

    float Rendering::GetFogDensity()
    {
        return FWD(GetFogDensity);
    }

    void Rendering::SetFogDistance(float distance)
    {
        FWD(SetFogDistance, distance);
    }

    float Rendering::GetFogDistance()
    {
        return FWD(GetFogDistance);
    }

    void Rendering::SetShadowBlurIterations(size_t iterations)
    {
        FWD(SetShadowBlurIterations, iterations);
    }

    size_t Rendering::GetShadowBlurIterations()
    {
        return FWD(GetShadowBlurIterations);
    }

    #define DRW Application::Get()->GetRenderAdaptor().DebugDrawer

    void Rendering::Draw(const Line& line, const Vector4& color)
    {
        DRW.Submit(line, color);
    }

    void Rendering::Draw(const AABB& box, const Vector4& color)
    {
        DRW.Submit(box, color);
    }

    void Rendering::Draw(const BoundingBox& box, const Vector4& color)
    {
        DRW.Submit(box, color);
    }

    void Rendering::Draw(const BoundingSphere& sphere, const Vector4& color)
    {
        DRW.Submit(sphere, color);
    }

    void Rendering::Draw(const Frustrum& frustrum, const Vector4& color)
    {
        DRW.Submit(frustrum, color);
    }

    void Rendering::Draw(const Cone& cone, const Vector4& color)
    {
        DRW.Submit(cone, color);
    }

    void Rendering::Draw(const Capsule& capsule, const Vector4& color)
    {
        DRW.Submit(capsule, color);
    }

    void Rendering::Draw(const Cylinder& cylinder, const Vector4& color)
    {
        DRW.Submit(cylinder, color);
    }
}