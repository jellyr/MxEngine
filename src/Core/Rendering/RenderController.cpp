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

#include "RenderController.h"
#include "Utilities/Format/Format.h"
#include "Core/Components/Rendering/MeshRenderer.h"
#include "Core/Components/Camera/CameraController.h"
#include "Core/Components/Camera/CameraEffects.h"
#include "Core/Components/Lighting/DirectionalLight.h"
#include "Core/Components/Lighting/SpotLight.h"
#include "Core/Components/Lighting/PointLight.h"
#include "Core/Components/Rendering/Skybox.h"
#include "Utilities/Profiler/Profiler.h"

namespace MxEngine
{
	constexpr size_t MaxDirLightCount = 4;

	void RenderController::PrepareShadowMaps()
	{
		MAKE_SCOPE_PROFILER("RenderController::PrepareShadowMaps()");

		this->ToggleReversedDepth(false);
		this->ToggleDepthOnlyMode(true);

		{
			MAKE_SCOPE_PROFILER("RenderController::PrepareDirectionalLightMaps()");
			const auto& directionalLightShader = *this->Pipeline.Environment.ShadowMapShader;
			for (auto& directionalLight : this->Pipeline.Lighting.DirectionalLights)
			{
				this->AttachDepthMap(directionalLight.ShadowMap);
				directionalLightShader.SetUniformMat4("LightProjMatrix", directionalLight.ProjectionMatrix);

				for (const auto& renderUnit : this->Pipeline.ShadowCasterUnits)
				{
					this->DrawShadowMap(renderUnit, directionalLight, directionalLightShader);
				}
				directionalLight.ShadowMap->GenerateMipmaps();
			}
		}

		{
			MAKE_SCOPE_PROFILER("RenderController::PrepareSpotLightMaps()");
			const auto& spotLightShader = *this->Pipeline.Environment.ShadowMapShader;
			for (auto& spotLight : this->Pipeline.Lighting.SpotLights)
			{
				this->AttachDepthMap(spotLight.ShadowMap);
				spotLightShader.SetUniformMat4("LightProjMatrix", spotLight.ProjectionMatrix);

				for (const auto& renderUnit : this->Pipeline.ShadowCasterUnits)
				{
					this->DrawShadowMap(renderUnit, spotLight, spotLightShader);
				}
				spotLight.ShadowMap->GenerateMipmaps();
			}
		}

		{
			MAKE_SCOPE_PROFILER("RenderController::PreparePointLightMaps()");
			const auto& pointLightShader = *this->Pipeline.Environment.ShadowCubeMapShader;
			for (auto& pointLight : this->Pipeline.Lighting.PointLights)
			{
				this->AttachDepthMap(pointLight.ShadowMap);
				pointLightShader.SetUniformMat4("LightProjMatrix[0]", pointLight.ProjectionMatrices[0]);
				pointLightShader.SetUniformMat4("LightProjMatrix[1]", pointLight.ProjectionMatrices[1]);
				pointLightShader.SetUniformMat4("LightProjMatrix[2]", pointLight.ProjectionMatrices[2]);
				pointLightShader.SetUniformMat4("LightProjMatrix[3]", pointLight.ProjectionMatrices[3]);
				pointLightShader.SetUniformMat4("LightProjMatrix[4]", pointLight.ProjectionMatrices[4]);
				pointLightShader.SetUniformMat4("LightProjMatrix[5]", pointLight.ProjectionMatrices[5]);
				pointLightShader.SetUniformFloat("zFar", pointLight.Radius);
				pointLightShader.SetUniformVec3("lightPos", pointLight.Position);

				for (const auto& renderUnit : this->Pipeline.ShadowCasterUnits)
				{
					this->DrawShadowMap(renderUnit, pointLight, pointLightShader);
				}
				pointLight.ShadowMap->GenerateMipmaps();
			}
		}
		this->ToggleDepthOnlyMode(false);
	}

	void RenderController::DrawObjects(const CameraUnit& camera, const Shader& shader, const MxVector<RenderUnit>& objects)
	{
		MAKE_SCOPE_PROFILER("RenderController::DrawObjects()");

		if (objects.empty()) return;
		shader.SetUniformMat4("ViewProjMatrix", camera.ViewProjectionMatrix);
		shader.SetUniformFloat("gamma", camera.Gamma);

		for (const auto& unit : objects)
		{
			bool isUnitVisible = unit.InstanceCount > 0 || camera.Culler.IsAABBVisible(unit.MinAABB, unit.MaxAABB);

			if (isUnitVisible) this->DrawObject(unit, shader);
		}
	}

	void RenderController::DrawObject(const RenderUnit& unit, const Shader& shader)
	{
		Texture::TextureBindId textureBindIndex = 0;
		const auto& material = this->Pipeline.MaterialUnits[unit.materialIndex];
		shader.IgnoreNonExistingUniform("material.transparency");

		material.AlbedoMap->Bind(textureBindIndex);
		shader.SetUniformInt("map_albedo", textureBindIndex);
		textureBindIndex++;

		material.SpecularMap->Bind(textureBindIndex);
		shader.SetUniformInt("map_specular", textureBindIndex);
		textureBindIndex++;

		material.EmmisiveMap->Bind(textureBindIndex);
		shader.SetUniformInt("map_emmisive", textureBindIndex);
		textureBindIndex++;

		material.NormalMap->Bind(textureBindIndex);
		shader.SetUniformInt("map_normal", textureBindIndex);
		textureBindIndex++;

		material.HeightMap->Bind(textureBindIndex);
		shader.SetUniformInt("map_height", textureBindIndex);
		textureBindIndex++;

		material.TransparencyMap->Bind(textureBindIndex);
		shader.SetUniformInt("map_transparency", textureBindIndex);
		textureBindIndex++;

		shader.SetUniformFloat("material.specularFactor", material.SpecularFactor);
		shader.SetUniformFloat("material.specularIntensity", material.SpecularIntensity);
		shader.SetUniformFloat("material.emmisive", material.Emmision);
		shader.SetUniformFloat("material.reflection", material.Reflection);
		shader.SetUniformFloat("material.transparency", material.Transparency);
		shader.SetUniformFloat("displacement", material.Displacement);

		this->GetRenderEngine().SetDefaultVertexAttribute(5, unit.ModelMatrix); //-V807
		this->GetRenderEngine().SetDefaultVertexAttribute(9, unit.NormalMatrix);
		this->GetRenderEngine().SetDefaultVertexAttribute(12, material.BaseColor);
		
		this->GetRenderEngine().DrawTrianglesInstanced(*unit.VAO, *unit.IBO, shader, unit.InstanceCount);
	}

	void RenderController::DrawShadowMap(const RenderUnit& unit, const DirectionalLigthUnit& dirLight, const Shader& shader)
	{
		const auto& material = this->Pipeline.MaterialUnits[unit.materialIndex];
		shader.SetUniformFloat("displacement", material.Displacement);
		material.HeightMap->Bind(0);
		shader.SetUniformInt("map_height", 0);

		this->GetRenderEngine().SetDefaultVertexAttribute(5, unit.ModelMatrix); //-V807
		this->GetRenderEngine().SetDefaultVertexAttribute(9, unit.NormalMatrix);
		this->GetRenderEngine().DrawTrianglesInstanced(*unit.VAO, *unit.IBO, shader, unit.InstanceCount);
	}

	void RenderController::DrawShadowMap(const RenderUnit& unit, const SpotLightUnit& spotLight, const Shader& shader)
	{
		const auto& material = this->Pipeline.MaterialUnits[unit.materialIndex];
		shader.SetUniformFloat("displacement", material.Displacement);
		material.HeightMap->Bind(0);
		shader.SetUniformInt("map_height", 0);

		this->GetRenderEngine().SetDefaultVertexAttribute(5, unit.ModelMatrix); //-V807
		this->GetRenderEngine().SetDefaultVertexAttribute(9, unit.NormalMatrix);
		this->GetRenderEngine().DrawTrianglesInstanced(*unit.VAO, *unit.IBO, shader, unit.InstanceCount);
	}

	void RenderController::DrawShadowMap(const RenderUnit& unit, const PointLightUnit& pointLight, const Shader& shader)
	{
		const auto& material = this->Pipeline.MaterialUnits[unit.materialIndex];
		shader.SetUniformFloat("displacement", material.Displacement);
		material.HeightMap->Bind(0);
		shader.SetUniformInt("map_height", 0);

		this->GetRenderEngine().SetDefaultVertexAttribute(5, unit.ModelMatrix);
		this->GetRenderEngine().DrawTrianglesInstanced(*unit.VAO, *unit.IBO, shader, unit.InstanceCount);
	}

	void RenderController::ComputeBloomEffect(CameraUnit& camera)
	{
		auto iterations = camera.BloomIterations - camera.BloomIterations % 2;
		if (iterations == 0) return;
		MAKE_SCOPE_PROFILER("RenderController::PerformBloomIterarations()");

		auto& bloomBuffers = this->Pipeline.Environment.BloomBuffers;
		auto& splitShader = this->Pipeline.Environment.BloomSplitShader;
		auto& iterShader = this->Pipeline.Environment.BloomIterationShader;

		float fogReduceFactor = this->Pipeline.Environment.FogDistance * std::exp(-25.0f * this->Pipeline.Environment.FogDensity);
		float bloomWeight = camera.BloomWeight * fogReduceFactor;

		// first get bloom mask from camera G-buffer
		camera.AlbedoTexture->GenerateMipmaps();
		camera.MaterialTexture->GenerateMipmaps();

		camera.AlbedoTexture->Bind(0);
		splitShader->SetUniformInt("albedoTex", 0);
		camera.MaterialTexture->Bind(1);
		splitShader->SetUniformInt("materialTex", 1);
		splitShader->SetUniformFloat("weight", bloomWeight);
		this->RenderToFrameBuffer(bloomBuffers.back(), splitShader);

		// perform bloom iterations
		iterShader->SetUniformInt("BloomTexture", 0);
		for (uint8_t i = 0; i < iterations; i++)
		{
			auto& target = bloomBuffers[(i & 1)];
			auto& source = bloomBuffers[!(i & 1)];
			iterShader->SetUniformInt("horizontalKernel", int(i & 1));
			GetAttachedTexture(source)->Bind(0);
			this->RenderToFrameBuffer(target, iterShader);
		}
		auto result = GetAttachedTexture(bloomBuffers.back());
		result->GenerateMipmaps();
		
		// use additive blending to apply bloom to camera HDR image
		this->GetRenderEngine().UseBlending(BlendFactor::ONE, BlendFactor::ONE);
		this->AttachFrameBufferNoClear(this->Pipeline.Environment.PostProcessFrameBuffer);
		this->SubmitImage(result);
		this->GetRenderEngine().UseBlending(BlendFactor::ONE, BlendFactor::ZERO);
	}

	TextureHandle RenderController::ComputeAverageWhite(TextureHandle hdrTexture)
	{
		MAKE_SCOPE_PROFILER("RenderController::ComputeAverageWhite()");
		hdrTexture->GenerateMipmaps();

		auto& shader = this->Pipeline.Environment.AverageWhiteShader;
		auto& output = this->Pipeline.Environment.AverageWhiteTexture;
		hdrTexture->Bind(0);
		shader->SetUniformInt("tex", 0);
		this->RenderToTexture(output, shader);
		output->GenerateMipmaps();
		return output;
	}

	void RenderController::PerformPostProcessing(CameraUnit& camera)
	{
		MAKE_SCOPE_PROFILER("RenderController::PerformPostProcessing()");

		// render skybox & debug buffer (HDR texture is already attached)
		this->Pipeline.Environment.PostProcessFrameBuffer->AttachTexture(camera.DepthTexture, Attachment::DEPTH_ATTACHMENT);
		this->GetRenderEngine().UseDepthBufferMask(false);
		this->DrawSkybox(camera);
		this->DrawTransparentObjects(camera);
		this->DrawDebugBuffer(camera);
		this->GetRenderEngine().UseDepthBufferMask(true);
		this->Pipeline.Environment.PostProcessFrameBuffer->DetachExtraTarget(Attachment::DEPTH_ATTACHMENT);

		this->ComputeBloomEffect(camera);
		this->ApplyFogEffect(camera, camera.HDRTexture, camera.SwapTexture);
		this->ApplyHDRToLDRConversion(camera, camera.HDRTexture, camera.SwapTexture);
		this->ApplyFXAA(camera, camera.HDRTexture, camera.SwapTexture);
		this->ApplyVignette(camera, camera.HDRTexture, camera.SwapTexture);
	}

	void RenderController::DrawDirectionalLights(CameraUnit& camera)
	{
		MAKE_SCOPE_PROFILER("RenderController::DrawDirectionalLights()");
		auto& illumShader = this->Pipeline.Environment.GlobalIlluminationShader;

		illumShader->IgnoreNonExistingUniform("camera.viewProjMatrix");
		illumShader->SetUniformMat3("skyboxTransform", camera.InversedSkyboxRotation);
		this->BindGBuffer(camera, *illumShader);
		this->BindCameraInformation(camera, *illumShader);

		Texture::TextureBindId textureId = 4;

		camera.SkyboxMap->Bind(textureId);
		illumShader->SetUniformInt("skyboxTex", textureId);
		textureId++;

		// submit directional light information
		constexpr size_t MaxDirLightCount = 4;
		const auto& dirLights = this->Pipeline.Lighting.DirectionalLights;
		size_t lightCount = Min(MaxDirLightCount, dirLights.size());

		illumShader->SetUniformInt("lightCount", (int)lightCount);
		illumShader->SetUniformInt("pcfDistance", this->Pipeline.Environment.ShadowBlurIterations);

		for (size_t i = 0; i < lightCount; i++)
		{
			auto& dirLight = this->Pipeline.Lighting.DirectionalLights[i];

			dirLight.ShadowMap->Bind(textureId);
			illumShader->SetUniformInt(MxFormat("lightDepthMaps[{}]", i), textureId);
			textureId++;

			illumShader->SetUniformMat4(MxFormat("lights[{}].transform", i), dirLight.BiasedProjectionMatrix);
			illumShader->SetUniformVec3(MxFormat("lights[{}].ambient", i), dirLight.AmbientColor);
			illumShader->SetUniformVec3(MxFormat("lights[{}].diffuse", i), dirLight.DiffuseColor);
			illumShader->SetUniformVec3(MxFormat("lights[{}].specular", i), dirLight.SpecularColor);
			illumShader->SetUniformVec3(MxFormat("lights[{}].direction", i), dirLight.Direction);
		}
		for (size_t i = lightCount; i < MaxDirLightCount; i++)
		{
			this->Pipeline.Environment.DefaultBlackMap->Bind(textureId);
			illumShader->SetUniformInt(MxFormat("lightDepthMaps[{}]", i), textureId);
			textureId++;
		}
		// render global illumination
		this->RenderToTexture(camera.HDRTexture, illumShader);
	}

	void RenderController::PerformLightPass(CameraUnit& camera)
	{
		this->DrawDirectionalLights(camera);

		this->ToggleFaceCulling(true, true, false);
		this->GetRenderEngine().UseBlending(BlendFactor::ONE, BlendFactor::ONE);

		this->DrawShadowedSpotLights(camera);
		this->DrawNonShadowedSpotLights(camera);
		this->DrawShadowedPointLights(camera);
		this->DrawNonShadowedPointLights(camera);

		this->ToggleFaceCulling(true, true, true);
		this->GetRenderEngine().UseBlending(BlendFactor::ONE, BlendFactor::ZERO);
	}

	void RenderController::DrawTransparentObjects(CameraUnit& camera)
	{
		if (this->Pipeline.TransparentRenderUnits.empty()) return;
		MAKE_SCOPE_PROFILER("RenderController::DrawTransparentObjects()");

		this->GetRenderEngine().UseBlending(BlendFactor::SRC_ALPHA, BlendFactor::ONE_MINUS_SRC_ALPHA);
		this->ToggleFaceCulling(false);

		auto& shader = this->Pipeline.Environment.TransparentShader;

		shader->SetUniformMat3("skyboxTransform", camera.InversedSkyboxRotation);
		shader->SetUniformVec3("viewportPosition", camera.ViewportPosition);
		shader->SetUniformFloat("gamma", camera.Gamma);

		Texture::TextureBindId textureId = 6; // assume there are 6 textures in material structure
		camera.SkyboxMap->Bind(textureId);
		shader->SetUniformInt("skyboxTex", textureId);
		textureId++;

		// submit directional light information
		const auto& dirLights = this->Pipeline.Lighting.DirectionalLights;
		size_t lightCount = Min(MaxDirLightCount, dirLights.size());

		shader->SetUniformInt("lightCount", (int)lightCount);
		shader->SetUniformInt("pcfDistance", this->Pipeline.Environment.ShadowBlurIterations);

		for (size_t i = 0; i < lightCount; i++)
		{
			auto& dirLight = this->Pipeline.Lighting.DirectionalLights[i];

			dirLight.ShadowMap->Bind(textureId);
			shader->SetUniformInt(MxFormat("lightDepthMaps[{}]", i), textureId);
			textureId++;

			shader->SetUniformMat4(MxFormat("lights[{}].transform", i), dirLight.BiasedProjectionMatrix);
			shader->SetUniformVec3(MxFormat("lights[{}].ambient", i), dirLight.AmbientColor);
			shader->SetUniformVec3(MxFormat("lights[{}].diffuse", i), dirLight.DiffuseColor);
			shader->SetUniformVec3(MxFormat("lights[{}].specular", i), dirLight.SpecularColor);
			shader->SetUniformVec3(MxFormat("lights[{}].direction", i), dirLight.Direction);
		}
		for (size_t i = lightCount; i < MaxDirLightCount; i++)
		{
			this->Pipeline.Environment.DefaultBlackMap->Bind(textureId);
			shader->SetUniformInt(MxFormat("lightDepthMaps[{}]", i), textureId);
			textureId++;
		}

		this->DrawObjects(camera, *shader, this->Pipeline.TransparentRenderUnits);

		this->ToggleFaceCulling(true);
		this->GetRenderEngine().UseBlending(BlendFactor::ONE, BlendFactor::ZERO);
	}

	void RenderController::ApplyFogEffect(CameraUnit& camera, TextureHandle& input, TextureHandle& output)
	{
		if (this->Pipeline.Environment.FogDistance == 1.0 && this->Pipeline.Environment.FogDensity == 0.0f)
			return; // such parameters produce no fog. Do not do extra work calling this shader

		MAKE_SCOPE_PROFILER("RenderController::ApplyFogEffect()");

		auto fogShader = this->Pipeline.Environment.FogShader;
		fogShader->IgnoreNonExistingUniform("camera.viewProjMatrix");
		fogShader->IgnoreNonExistingUniform("normalTex");
		fogShader->IgnoreNonExistingUniform("albedoTex");
		fogShader->IgnoreNonExistingUniform("materialTex");

		input->Bind(4);
		fogShader->SetUniformInt("cameraOutput", 4);

		this->BindGBuffer(camera, *fogShader);
		this->BindFogInformation(*fogShader);
		this->BindCameraInformation(camera, *fogShader);

		this->RenderToTexture(output, fogShader);
		std::swap(input, output);
	}

	void RenderController::ApplyHDRToLDRConversion(CameraUnit& camera, TextureHandle& input, TextureHandle& output)
	{
		MAKE_SCOPE_PROFILER("RenderController::ApplyHDRToLDRConversion()");

		auto& HDRToLDRShader = this->Pipeline.Environment.HDRToLDRShader;
		auto averageWhite = camera.EnableToneMapping ? this->ComputeAverageWhite(camera.HDRTexture) : this->Pipeline.Environment.DefaultGreyMap;

		input->Bind(0);
		averageWhite->Bind(1);
		HDRToLDRShader->SetUniformInt("HDRTex", 0);
		HDRToLDRShader->SetUniformInt("averageWhiteTex", 1);

		HDRToLDRShader->SetUniformFloat("exposure", camera.Exposure);
		HDRToLDRShader->SetUniformFloat("colorMultiplier", camera.ColorScale);
		HDRToLDRShader->SetUniformFloat("whitePoint", camera.WhitePoint);
		HDRToLDRShader->SetUniformVec3("ABCcoefsACES", { camera.ACESCoefficients.A, camera.ACESCoefficients.B, camera.ACESCoefficients.C });
		HDRToLDRShader->SetUniformVec3("DEFcoefsACES", { camera.ACESCoefficients.D, camera.ACESCoefficients.E, camera.ACESCoefficients.F });

		HDRToLDRShader->SetUniformFloat("gamma", camera.Gamma);

		this->RenderToTexture(output, HDRToLDRShader);
		std::swap(input, output);
	}

	void RenderController::ApplyFXAA(CameraUnit& camera, TextureHandle& input, TextureHandle& output)
	{
		if (!camera.EnableFXAA) return;
		MAKE_SCOPE_PROFILER("RenderController::ApplyFXAA");

		auto& fxaaShader = this->Pipeline.Environment.FXAAShader;
		input->GenerateMipmaps();
		input->Bind(0);
		fxaaShader->SetUniformInt("tex", 0);
		
		this->RenderToTexture(output, fxaaShader);
		std::swap(input, output);
	}

	void RenderController::ApplyVignette(CameraUnit& camera, TextureHandle& input, TextureHandle& output)
	{
		if (camera.VignetteRadius <= 0.0f) return;
		MAKE_SCOPE_PROFILER("RenderController::ApplyVignette");

		auto& vignetteShader = this->Pipeline.Environment.VignetteShader;
		input->Bind(0);
		vignetteShader->SetUniformInt("tex", 0);

		vignetteShader->SetUniformFloat("radius", camera.VignetteRadius);
		vignetteShader->SetUniformFloat("intensity", camera.VignetteIntensity);

		this->RenderToTexture(output, vignetteShader);
		std::swap(input, output);
	}

	void RenderController::DrawShadowedSpotLights(CameraUnit& camera)
	{
		const auto& spotLights = this->Pipeline.Lighting.SpotLights;
		if (spotLights.empty()) return;
		MAKE_SCOPE_PROFILER("RenderController::DrawShadowedSpotLights()");

		auto shader = this->Pipeline.Environment.SpotLightShader;
		auto& pyramid = this->Pipeline.Lighting.PyramidLight;
		auto viewportSize = MakeVector2((float)camera.OutputTexture->GetWidth(), (float)camera.OutputTexture->GetHeight());

		shader->SetUniformVec2("viewportSize", viewportSize);
		shader->SetUniformInt("pcfDistance", this->Pipeline.Environment.ShadowBlurIterations);
		this->BindGBuffer(camera, *shader);
		this->BindCameraInformation(camera, *shader);

		for (size_t i = 0; i < spotLights.size(); i++)
		{
			const auto& spotLight = spotLights[i];

			spotLight.ShadowMap->Bind(4);
			shader->SetUniformInt("lightDepthMap", 4);

			shader->SetUniformMat4("worldToLightTransform", spotLight.BiasedProjectionMatrix);
			shader->SetUniformInt("castsShadows", true);

			this->GetRenderEngine().SetDefaultVertexAttribute(5,  spotLight.Transform);
			this->GetRenderEngine().SetDefaultVertexAttribute(9,  Vector4(spotLight.Position, spotLight.InnerAngle));
			this->GetRenderEngine().SetDefaultVertexAttribute(10, Vector4(spotLight.Direction, spotLight.OuterAngle));
			this->GetRenderEngine().SetDefaultVertexAttribute(11, spotLight.AmbientColor);
			this->GetRenderEngine().SetDefaultVertexAttribute(12, spotLight.DiffuseColor);
			this->GetRenderEngine().SetDefaultVertexAttribute(13, spotLight.SpecularColor);

			this->GetRenderEngine().DrawTriangles(pyramid.GetVAO(), pyramid.GetIBO(), *shader);
		}
	}

	void RenderController::DrawShadowedPointLights(CameraUnit& camera)
	{
		const auto& pointLights = this->Pipeline.Lighting.PointLights;
		if (pointLights.empty()) return;
		MAKE_SCOPE_PROFILER("RenderController::DrawShadowedPointLights()");

		auto shader = this->Pipeline.Environment.PointLightShader;
		auto& sphere = this->Pipeline.Lighting.SphereLight;
		auto viewportSize = MakeVector2((float)camera.OutputTexture->GetWidth(), (float)camera.OutputTexture->GetHeight());

		shader->SetUniformVec2("viewportSize", viewportSize);
		this->BindGBuffer(camera, *shader);
		this->BindCameraInformation(camera, *shader);

		for (size_t i = 0; i < pointLights.size(); i++)
		{
			const auto& pointLight = pointLights[i];

			pointLight.ShadowMap->Bind(4);
			shader->SetUniformInt("lightDepthMap", 4);
			shader->SetUniformInt("castsShadows", true);

			this->GetRenderEngine().SetDefaultVertexAttribute(5,  pointLight.Transform);
			this->GetRenderEngine().SetDefaultVertexAttribute(9,  Vector4(pointLight.Position, pointLight.Radius));
			this->GetRenderEngine().SetDefaultVertexAttribute(10, pointLight.AmbientColor);
			this->GetRenderEngine().SetDefaultVertexAttribute(11, pointLight.DiffuseColor);
			this->GetRenderEngine().SetDefaultVertexAttribute(12, pointLight.SpecularColor);

			this->GetRenderEngine().DrawTriangles(sphere.GetVAO(), sphere.GetIBO(), *shader);
		}
	}

	void RenderController::DrawNonShadowedPointLights(CameraUnit& camera)
	{
		auto& instancedPointLights = this->Pipeline.Lighting.PointLigthsInstanced;
		if (instancedPointLights.Instances.empty()) return;
		MAKE_SCOPE_PROFILER("RenderController::DrawNonShadowedPointLights()");

		auto shader = this->Pipeline.Environment.PointLightShader;
		auto viewportSize = MakeVector2((float)camera.OutputTexture->GetWidth(), (float)camera.OutputTexture->GetHeight());

		this->BindGBuffer(camera, *shader);
		this->BindCameraInformation(camera, *shader);

		this->Pipeline.Environment.DefaultBlackCubeMap->Bind(4);
		shader->SetUniformVec2("viewportSize", viewportSize);
		shader->SetUniformInt("lightDepthMap", 4);
		shader->SetUniformInt("castsShadows", false);

		instancedPointLights.SubmitToVBO();
		this->GetRenderEngine().DrawTrianglesInstanced(instancedPointLights.GetVAO(), instancedPointLights.GetIBO(), *shader, instancedPointLights.Instances.size());
	}

	void RenderController::DrawNonShadowedSpotLights(CameraUnit& camera)
	{
		auto& instancedSpotLights = this->Pipeline.Lighting.SpotLightsInstanced;
		if (instancedSpotLights.Instances.empty()) return;
		MAKE_SCOPE_PROFILER("RenderController::DrawNonShadowedSpotLights()");

		auto shader = this->Pipeline.Environment.SpotLightShader;
		auto viewportSize = MakeVector2((float)camera.OutputTexture->GetWidth(), (float)camera.OutputTexture->GetHeight());

		this->BindGBuffer(camera, *shader);
		this->BindCameraInformation(camera, *shader);

		this->Pipeline.Environment.DefaultBlackCubeMap->Bind(4);
		shader->SetUniformVec2("viewportSize", viewportSize);
		shader->SetUniformInt("lightDepthMap", 4);
		shader->SetUniformInt("castsShadows", false);

		instancedSpotLights.SubmitToVBO();
		this->GetRenderEngine().DrawTrianglesInstanced(instancedSpotLights.GetVAO(), instancedSpotLights.GetIBO(), *shader, instancedSpotLights.Instances.size());
	}

	void RenderController::BindFogInformation(const Shader& shader)
	{
		shader.SetUniformFloat("fog.distance", this->Pipeline.Environment.FogDistance);
		shader.SetUniformFloat("fog.density", this->Pipeline.Environment.FogDensity);
		shader.SetUniformVec3("fog.color", this->Pipeline.Environment.FogColor);
	}

	void RenderController::BindCameraInformation(const CameraUnit& camera, const Shader& shader)
	{
		shader.SetUniformVec3("camera.position", camera.ViewportPosition);
		shader.SetUniformMat4("camera.viewProjMatrix", camera.ViewProjectionMatrix);
		shader.SetUniformMat4("camera.invViewMatrix", camera.InverseViewMatrix);
		shader.SetUniformMat4("camera.invProjMatrix", camera.InverseProjectionMatrix);
	}

	void RenderController::BindGBuffer(const CameraUnit& camera, const Shader& shader)
	{
		camera.AlbedoTexture->Bind(0);
		shader.SetUniformInt("albedoTex", 0);

		camera.NormalTexture->Bind(1);
		shader.SetUniformInt("normalTex", 1);

		camera.MaterialTexture->Bind(2);
		shader.SetUniformInt("materialTex", 2);

		camera.DepthTexture->Bind(3);
		shader.SetUniformInt("depthTex", 3);
	}

	const Renderer& RenderController::GetRenderEngine() const
	{
		return this->renderer;
	}

	Renderer& RenderController::GetRenderEngine()
	{
		return this->renderer;
	}

	void RenderController::Render() const
	{
		this->GetRenderEngine().Flush();
	}

	void RenderController::Clear() const
	{
		this->GetRenderEngine().Clear();
	}

	void RenderController::AttachDepthMap(const TextureHandle& texture)
	{
		auto& framebuffer = this->Pipeline.Environment.DepthFrameBuffer;
		framebuffer->AttachTexture(texture, Attachment::DEPTH_ATTACHMENT);
		this->AttachFrameBuffer(framebuffer);
	}

	void RenderController::AttachDepthMap(const CubeMapHandle& cubemap)
	{
		auto& framebuffer = this->Pipeline.Environment.DepthFrameBuffer;
		framebuffer->AttachCubeMap(cubemap, Attachment::DEPTH_ATTACHMENT);
		this->AttachFrameBuffer(framebuffer);
	}

	void RenderController::AttachFrameBuffer(const FrameBufferHandle& framebuffer)
	{
		this->AttachFrameBufferNoClear(framebuffer);
		this->Clear();
	}

	void RenderController::AttachFrameBufferNoClear(const FrameBufferHandle& framebuffer)
	{
		framebuffer->Bind();
		this->SetViewport(0, 0, (int)framebuffer->GetWidth(), (int)framebuffer->GetHeight());
	}

	void RenderController::AttachDefaultFrameBuffer()
	{
		// we can unbind any buffer to set target framebuffer to default one (id = 0)
		// doing this allows us to render to application window
		this->Pipeline.Environment.DepthFrameBuffer->Unbind();
		this->SetViewport(0, 0, this->Pipeline.Environment.Viewport.x, this->Pipeline.Environment.Viewport.y);
		this->Clear();
	}

	void RenderController::RenderToFrameBuffer(const FrameBufferHandle& framebuffer, const ShaderHandle& shader)
	{
		this->AttachFrameBuffer(framebuffer);
		auto& rectangle = this->Pipeline.Environment.RectangularObject;
		this->GetRenderEngine().DrawTriangles(rectangle.GetVAO(), rectangle.VertexCount, *shader);
	}

	void RenderController::RenderToTexture(const TextureHandle& texture, const ShaderHandle& shader, Attachment attachment)
	{
		this->Pipeline.Environment.PostProcessFrameBuffer->AttachTexture(texture, attachment);
		this->RenderToFrameBuffer(this->Pipeline.Environment.PostProcessFrameBuffer, shader);
	}

	void RenderController::CopyTexture(const TextureHandle& input, const TextureHandle& output)
	{
		MAKE_SCOPE_PROFILER("RenderController::CopyTexture");
		this->Pipeline.Environment.PostProcessFrameBuffer->AttachTexture(output);
		this->AttachFrameBufferNoClear(this->Pipeline.Environment.PostProcessFrameBuffer);
		this->SubmitImage(input);
	}

    void RenderController::ToggleDepthOnlyMode(bool value)
    {
		bool useColor = !value;
		this->GetRenderEngine().UseColorMask(useColor, useColor, useColor, useColor);
    }

	void RenderController::ToggleReversedDepth(bool value)
	{
		this->GetRenderEngine().UseReversedDepth(value);
	}

	void RenderController::ToggleFaceCulling(bool value, bool counterClockWise, bool cullBack)
	{
		this->GetRenderEngine().UseCulling(value, counterClockWise, cullBack);
	}

	void RenderController::SetAnisotropicFiltering(float value)
	{
		this->GetRenderEngine().UseAnisotropicFiltering(value);
	}

	void RenderController::SetViewport(int x, int y, int width, int height)
	{
		this->GetRenderEngine().SetViewport(x, y, width, height);
	}

	void RenderController::DrawSkybox(const CameraUnit& camera)
	{
		MAKE_SCOPE_PROFILER("RenderController::DrawSkybox()");

		auto& shader = *this->Pipeline.Environment.SkyboxShader;
		auto& skybox = this->Pipeline.Environment.SkyboxCubeObject;

		float skyLuminance = 0.0f;
		Vector3 luminanceRGB(0.2125f, 0.7154f, 0.0721f);
		for (size_t i = 0; i < Min(this->Pipeline.Lighting.DirectionalLights.size(), MaxDirLightCount); i++)
		{
			const auto& dirLight = this->Pipeline.Lighting.DirectionalLights[i];
			skyLuminance += Dot(luminanceRGB, dirLight.AmbientColor + dirLight.DiffuseColor);
		}

		this->BindFogInformation(shader);

		shader.SetUniformMat4("StaticViewProjection", camera.StaticViewProjectionMatrix);
		shader.SetUniformMat3("Rotation", Transpose(camera.InversedSkyboxRotation));
		shader.SetUniformFloat("gamma", camera.Gamma);
		shader.SetUniformFloat("luminance", skyLuminance);

		camera.SkyboxMap->Bind(0);
		shader.SetUniformInt("skybox", 0);

		this->GetRenderEngine().DrawTriangles(skybox.GetVAO(), skybox.VertexCount, shader);
	}

	void RenderController::DrawDebugBuffer(const CameraUnit& camera)
	{
		if (this->Pipeline.Environment.DebugBufferObject.VertexCount == 0) return;
		MAKE_SCOPE_PROFILER("RenderController::DrawDebugBuffer()");

		this->GetRenderEngine().UseDepthBuffer(!this->Pipeline.Environment.OverlayDebugDraws); //-V807

		auto& shader = *this->Pipeline.Environment.DebugDrawShader;
		shader.SetUniformMat4("ViewProjMatrix", camera.ViewProjectionMatrix);

		this->GetRenderEngine().DrawLines(*this->Pipeline.Environment.DebugBufferObject.VAO, this->Pipeline.Environment.DebugBufferObject.VertexCount, shader);

		this->GetRenderEngine().UseDepthBuffer(true);
	}

	EnvironmentUnit& RenderController::GetEnvironment()
	{
		return this->Pipeline.Environment;
	}

	const EnvironmentUnit& RenderController::GetEnvironment() const
	{
		return this->Pipeline.Environment;
	}

	LightingSystem& RenderController::GetLightInformation()
	{
		return this->Pipeline.Lighting;
	}

	const LightingSystem& RenderController::GetLightInformation() const
	{
		return this->Pipeline.Lighting;
	}

	void RenderController::ResetPipeline()
	{
		this->Pipeline.Lighting.DirectionalLights.clear();
		this->Pipeline.Lighting.PointLigthsInstanced.Instances.clear();
		this->Pipeline.Lighting.SpotLightsInstanced.Instances.clear();
		this->Pipeline.Lighting.PointLights.clear();
		this->Pipeline.Lighting.SpotLights.clear();
		this->Pipeline.OpaqueRenderUnits.clear();
		this->Pipeline.TransparentRenderUnits.clear();
		this->Pipeline.ShadowCasterUnits.clear();
		this->Pipeline.MaterialUnits.clear();
		this->Pipeline.Cameras.clear();
	}

	void RenderController::SubmitLightSource(const DirectionalLight& light, const TransformComponent& parentTransform)
	{
		auto& dirLight = this->Pipeline.Lighting.DirectionalLights.emplace_back();

		dirLight.AmbientColor = light.AmbientColor;
		dirLight.DiffuseColor = light.DiffuseColor;
		dirLight.SpecularColor = light.SpecularColor;
		dirLight.Direction = light.Direction;
		dirLight.ShadowMap = light.GetDepthTexture();
		dirLight.ProjectionMatrix = light.GetMatrix(parentTransform.GetPosition());
		dirLight.BiasedProjectionMatrix = MakeBiasMatrix() * light.GetMatrix(parentTransform.GetPosition());
	}

	void RenderController::SubmitLightSource(const PointLight& light, const TransformComponent& parentTransform)
	{
		PointLightBaseData* baseLightData = nullptr;
		if (light.IsCastingShadows())
		{
			auto& pointLight = this->Pipeline.Lighting.PointLights.emplace_back();
			baseLightData = &pointLight;

			pointLight.ShadowMap = light.GetDepthCubeMap();
			for (size_t i = 0; i < std::size(pointLight.ProjectionMatrices); i++)
				pointLight.ProjectionMatrices[i] = light.GetMatrix(i, parentTransform.GetPosition());
		}
		else
		{
			auto& pointLight = this->Pipeline.Lighting.PointLigthsInstanced.Instances.emplace_back();
			baseLightData = &pointLight;
		}

		baseLightData->AmbientColor = light.AmbientColor;
		baseLightData->DiffuseColor = light.DiffuseColor;
		baseLightData->SpecularColor = light.SpecularColor;
		baseLightData->Position = parentTransform.GetPosition();
		baseLightData->Radius = light.GetRadius();
		baseLightData->Transform = light.GetSphereTransform(parentTransform.GetPosition());
	}

	void RenderController::SubmitLightSource(const SpotLight& light, const TransformComponent& parentTransform)
	{
		SpotLightBaseData* baseLightData = nullptr;
		if (light.IsCastingShadows())
		{
			auto& spotLight = this->Pipeline.Lighting.SpotLights.emplace_back();
			baseLightData = &spotLight;

			spotLight.ProjectionMatrix = light.GetMatrix(parentTransform.GetPosition());
			spotLight.BiasedProjectionMatrix = MakeBiasMatrix() * light.GetMatrix(parentTransform.GetPosition());
			spotLight.ShadowMap = light.GetDepthTexture();
		}
		else
		{
			auto& spotLight = this->Pipeline.Lighting.SpotLightsInstanced.Instances.emplace_back();
			baseLightData = &spotLight;
		}

		baseLightData->AmbientColor = light.AmbientColor;
		baseLightData->DiffuseColor = light.DiffuseColor;
		baseLightData->SpecularColor = light.SpecularColor;
		baseLightData->Position = parentTransform.GetPosition();
		baseLightData->Direction = light.Direction;
		baseLightData->Transform = light.GetPyramidTransform(parentTransform.GetPosition());
		baseLightData->InnerAngle = light.GetInnerCos();
		baseLightData->OuterAngle = light.GetOuterCos();
	}

	void RenderController::SubmitCamera(const CameraController& controller, const TransformComponent& parentTransform, const Skybox& skybox, const CameraEffects& effects)
	{
		auto& camera = this->Pipeline.Cameras.emplace_back();

		camera.ViewportPosition           = parentTransform.GetPosition();
		camera.StaticViewProjectionMatrix = controller.GetMatrix(MakeVector3(0.0f));
		camera.InverseProjectionMatrix    = Inverse(controller.GetProjectionMatrix());
		camera.InverseViewMatrix          = Inverse(controller.GetViewMatrix(parentTransform.GetPosition()));
		camera.ViewProjectionMatrix       = controller.GetMatrix(parentTransform.GetPosition());
		camera.Culler                     = controller.GetFrustrumCuller();
		camera.IsPerspective              = controller.GetCameraType() == CameraType::PERSPECTIVE;
		camera.GBuffer                    = controller.GetGBuffer();
		camera.AlbedoTexture              = controller.GetAlbedoTexture();
		camera.NormalTexture              = controller.GetNormalTexture();
		camera.MaterialTexture            = controller.GetMaterialTexture();
		camera.DepthTexture               = controller.GetDepthTexture();
		camera.HDRTexture                 = controller.GetHDRTexture();
		camera.SwapTexture                = controller.GetSwapHDRTexture();
		camera.BloomIterations            = (uint8_t)effects.GetBloomIterations();
		camera.BloomWeight                = effects.GetBloomWeight();
		camera.Exposure                   = effects.GetExposure();
		camera.ACESCoefficients           = effects.GetACESCoefficients();
		camera.ColorScale                 = effects.GetColorScale();
		camera.WhitePoint                 = effects.GetWhitePoint();
		camera.Gamma                      = effects.GetGamma();
		camera.VignetteRadius             = effects.GetVignetteRadius();
		camera.VignetteIntensity          = effects.GetVignetteIntensity();
		camera.EnableFXAA                 = effects.IsFXAAEnabled();
		camera.EnableToneMapping          = effects.IsToneMappingEnabled();
		camera.SkyboxMap                  = skybox.Texture.IsValid() ? skybox.Texture : this->Pipeline.Environment.DefaultBlackCubeMap;
		camera.InversedSkyboxRotation     = Transpose(ToMatrix(parentTransform.GetRotation()));
		camera.OutputTexture              = controller.GetRenderTexture();
		camera.RenderToTexture            = controller.IsRendered();
	}

    void RenderController::SubmitPrimitive(const SubMesh& object, const Material& material, const TransformComponent& parentTransform, size_t instanceCount)
    {
		RenderUnit* primitivePtr = nullptr;
		// filter transparent object to render in separate order
		if (material.Transparency < 1.0f)
			primitivePtr = &this->Pipeline.TransparentRenderUnits.emplace_back();
		else
			primitivePtr = &this->Pipeline.OpaqueRenderUnits.emplace_back();
		auto& primitive = *primitivePtr;

		primitive.VAO = object.Data.GetVAO();
		primitive.IBO = object.Data.GetIBO();
		primitive.materialIndex = this->Pipeline.MaterialUnits.size();
		primitive.ModelMatrix  = parentTransform.GetMatrix() * object.GetTransform()->GetMatrix(); //-V807
		primitive.NormalMatrix = parentTransform.GetNormalMatrix() * object.GetTransform()->GetNormalMatrix();
		primitive.InstanceCount = instanceCount;

		// compute aabb of primitive object for later frustrum culling
		auto aabb = object.Data.GetBoundingBox() * primitive.ModelMatrix;
		primitive.MinAABB = aabb.Min;
		primitive.MaxAABB = aabb.Max;

		auto& renderMaterial = this->Pipeline.MaterialUnits.emplace_back(material); // create a copy of material for future work

		// we need to change displacement to account object scale, so we take average of object scale components as multiplier
		renderMaterial.Displacement *= Dot(parentTransform.GetScale() * object.GetTransform()->GetScale(), MakeVector3(1.0f / 3.0f));
		// set default textures if they are not exist
		if (!renderMaterial.AlbedoMap.IsValid())       renderMaterial.AlbedoMap       = this->Pipeline.Environment.DefaultMaterialMap;
		if (!renderMaterial.SpecularMap.IsValid())     renderMaterial.SpecularMap     = this->Pipeline.Environment.DefaultMaterialMap;
		if (!renderMaterial.EmmisiveMap.IsValid())     renderMaterial.EmmisiveMap     = this->Pipeline.Environment.DefaultMaterialMap;
		if (!renderMaterial.TransparencyMap.IsValid()) renderMaterial.TransparencyMap = this->Pipeline.Environment.DefaultMaterialMap;
		if (!renderMaterial.NormalMap.IsValid())       renderMaterial.NormalMap       = this->Pipeline.Environment.DefaultNormalMap;
		if (!renderMaterial.HeightMap.IsValid())       renderMaterial.HeightMap       = this->Pipeline.Environment.DefaultBlackMap;

		if (renderMaterial.CastsShadow) this->Pipeline.ShadowCasterUnits.push_back(primitive);
    }

	void RenderController::SubmitImage(const TextureHandle& texture)
	{
		auto& finalShader = *this->Pipeline.Environment.ImageForwardShader;
		auto& rectangle = this->Pipeline.Environment.RectangularObject;

		finalShader.SetUniformInt("tex", 0);
		texture->Bind(0);

		this->GetRenderEngine().DrawTriangles(rectangle.GetVAO(), rectangle.VertexCount, finalShader);
	}

	void RenderController::StartPipeline()
	{
		MAKE_SCOPE_PROFILER("RenderController::StartPipeline()");
		if (this->Pipeline.Cameras.empty())
		{
			if(this->Pipeline.Environment.RenderToDefaultFrameBuffer)
				this->AttachDefaultFrameBuffer();
			return;
		}

		this->PrepareShadowMaps();

		for (auto& camera : this->Pipeline.Cameras)
		{
			if (!camera.RenderToTexture) continue;

			this->GetRenderEngine().UseBlending(BlendFactor::ONE, BlendFactor::ZERO);
			this->ToggleReversedDepth(camera.IsPerspective);
			this->AttachFrameBuffer(camera.GBuffer);

			this->DrawObjects(camera, *this->Pipeline.Environment.GBufferShader, this->Pipeline.OpaqueRenderUnits);
			this->PerformLightPass(camera);
			this->PerformPostProcessing(camera);

			this->CopyTexture(camera.HDRTexture, camera.OutputTexture);
			camera.OutputTexture->GenerateMipmaps();
		}
	}

	void RenderController::EndPipeline()
	{
		MAKE_SCOPE_PROFILER("RenderController::SubmitFinalImage");
		this->AttachDefaultFrameBuffer();
		if (this->Pipeline.Environment.RenderToDefaultFrameBuffer && this->Pipeline.Environment.MainCameraIndex < this->Pipeline.Cameras.size())
		{
			const auto& mainCamera = this->Pipeline.Cameras[this->Pipeline.Environment.MainCameraIndex];
			MX_ASSERT(mainCamera.OutputTexture.IsValid());

			this->SubmitImage(mainCamera.OutputTexture);
		}
	}
}