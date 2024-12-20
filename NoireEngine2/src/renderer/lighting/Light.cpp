#include "Light.hpp"
#include "renderer/scene/Entity.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "renderer/Renderer.hpp"
#include "renderer/scene/SceneManager.hpp"
#include "imgui/imgui.h"

Light::Light(Type type_) :
	type((uint32_t)type_)
{
	useGizmos = true;
}

Light::Light(Type type, Color3 color, float intensity) :
	Light(type)
{
	memcpy(&this->m_Color, color.value_ptr(), sizeof(Color3));
	this->m_Intensity = intensity;
}

Light::Light(Type type, Color3 color, float intensity, float radius) :
	Light(type, color, intensity)
{
	this->m_Radius = radius;
}

Light::Light(Type type, Color3 color, float intensity, float radius, float limit) :
	Light(type, color, intensity, radius)
{
	this->m_Limit = limit;
}

Light::Light(Type type, Color3 color, float intensity, float radius, float fov, float blend) :
	Light(type, color, intensity, radius)
{
	this->m_Fov = fov;
	this->m_Blend = blend;
}

Light::Light(Type type, Color3 color, float intensity, float radius, float fov, float blend, float limit) :
	Light(type, color, intensity, radius, fov, blend)
{
	this->m_Limit = limit;
}

void Light::Update()
{
	Transform* transform = GetTransform();
	glm::vec3 pos = transform->position();
	glm::vec3 dir = transform->Back(); // always point in -z m_Direction

	isDirty |= transform->wasDirtyThisFrame;
	isDirty |= VIEW_CAM->wasDirtyThisFrame;

	if (!isDirty)
		return;

	m_Direction = glm::vec4(dir, 0);
	m_Position = glm::vec4(pos, 0);

	if (type == (uint32_t)Type::Directional)
	{
		if(m_UseShadows)
			UpdateDirectionalLightCascades();
		UpdateDirectionalLightUniform();
	}
	else if (type == (uint32_t)Type::Point)
	{
		if (m_UseShadows) {
			for (uint32_t face = 0; face < OMNI_SHADOWMAPS_COUNT; face++) {
				UpdatePointLightLightSpaces(face);
			}
		}
		UpdatePointLightUniform();
	}
	else  // Type::Spot
	{
		if (m_UseShadows) {
			glm::mat4 depthProjectionMatrix = Mat4::Perspective(glm::radians(m_Fov), 1.0f, m_NearClip, m_FarClip);
			glm::mat4 depthViewMatrix = Mat4::LookAt(pos, pos + dir, transform->Up());
			m_Lightspaces[0] = depthProjectionMatrix * depthViewMatrix;
		}
		UpdateSpotLightUniform();
	}
	isDirty = false;
}

void Light::Render(const glm::mat4& model)
{
	if (!Renderer::UseGizmos || !useGizmos)
		return;

	Color4_4 c{
		static_cast<uint8_t>(m_Color.x * 255),
		static_cast<uint8_t>(m_Color.y * 255),
		static_cast<uint8_t>(m_Color.z * 255),
		255
	};

	if (type == (uint32_t)Type::Point) 
	{
		gizmos.DrawWireSphere(m_Limit, glm::vec3(m_Position), c);
		GetScene()->PushGizmosInstance(&gizmos);
	}
	else if (type == (uint32_t)Type::Spot)
	{
		constexpr float coneRange = 5;
		float inner = glm::tan(glm::radians(m_Fov * (1 - m_Blend) * 0.5f)) * coneRange;
		float outer = glm::tan(glm::radians(m_Fov * 0.5f)) * coneRange;

		gizmos.DrawSpotLight(glm::vec3(m_Position), glm::vec3(m_Direction), inner, outer, m_Limit, c);
		GetScene()->PushGizmosInstance(&gizmos);
	}
	else
	{
		gizmos.DrawDirectionalLight(glm::vec3(m_Direction), glm::vec3(m_Position), c);
		GetScene()->PushGizmosInstance(&gizmos);
	}
}

static glm::mat4 BIAS_MAT = glm::mat4(
	0.5f, 0.0f, 0.0f, 0.0f, // Column 1
	0.0f, 0.5f, 0.0f, 0.0f, // Column 2
	0.0f, 0.0f, 1.0f, 0.0f, // Column 3
	0.5f, 0.5f, 0.0f, 1.0f  // Column 4
);

void Light::Inspect() 
{
	ImGui::PushID("##LI");
	{
		ImGui::Columns(2);
		ImGui::Text("Light Type");
		ImGui::NextColumn();
		switch (type)
		{
		case 0:
			ImGui::Text("Directional");
			break;
		case 1:
			ImGui::Text("Point");
			break;
		case 2:
			ImGui::Text("Spot");
			break;
		}
		ImGui::Columns(1);

		if(ImGui::ColorEdit3("Light Color", (float*)&m_Color))
			isDirty = true;
		
		ImGui::Columns(2);
		ImGui::Text("Intensity");
		ImGui::NextColumn();
		if(ImGui::DragFloat("###Intensity", &m_Intensity, 0.01f, 0.0f, FLT_MAX, "%.2f", ImGuiSliderFlags_AlwaysClamp))
			isDirty = true;
		ImGui::Columns(1);

		ImGui::Columns(2);
		ImGui::Text("Gizmos");
		ImGui::NextColumn();
		ImGui::Checkbox("###USEGIZMOS", &useGizmos);
		ImGui::Columns(1);
		ImGui::Separator(); // -----------------------------------------------------

		if (type != /*Type::Directional*/0)
		{
			ImGui::Columns(2);
			ImGui::Text("Radius");
			ImGui::NextColumn();
			if(ImGui::DragFloat("###Radius", &m_Radius, 0.03f, 0.0f, FLT_MAX, "%.2f", ImGuiSliderFlags_AlwaysClamp))
				isDirty = true;
			ImGui::Columns(1);

			ImGui::Columns(2);
			ImGui::Text("Limit");
			ImGui::NextColumn();
			if(ImGui::DragFloat("###Limit", &m_Limit, 0.01f, 0.0f, FLT_MAX, "%.2f", ImGuiSliderFlags_AlwaysClamp))
				isDirty = true;
			ImGui::Columns(1);
		}

		if (type == /*Type::Spot*/2)
		{
			ImGui::Columns(2);
			ImGui::Text("FOV");
			ImGui::NextColumn();
			if(ImGui::DragFloat("###FOV", &m_Fov, 0.5f, 0.0f, 180, "%.2f", ImGuiSliderFlags_AlwaysClamp))
				isDirty = true;
			ImGui::Columns(1);

			ImGui::Columns(2);
			ImGui::Text("Blend");
			ImGui::NextColumn();
			if(ImGui::DragFloat("###BLEND", &m_Blend, 0.002f, 0, 1, "%.2f", ImGuiSliderFlags_AlwaysClamp))
				isDirty = true;
			ImGui::Columns(1);
		}

		ImGui::Columns(2);
		ImGui::Text("Shadow Strength");
		ImGui::NextColumn();
		if(ImGui::DragFloat("###ShadowStrength", &m_ShadowAttenuation, 0.01f, 0.0f, 1.0f, "%.05f", ImGuiSliderFlags_AlwaysClamp))
			isDirty = true;
		ImGui::Columns(1);
	}
	ImGui::PopID();
}

void Light::UpdateDirectionalLightCascades()
{
	Camera* camera = VIEW_CAM;

	float cascadeSplits[SHADOW_MAP_CASCADE_COUNT];

	const float cascadeSplitLambda = 0.95f;
	const float nearClip = camera->nearClipPlane;
	const float farClip = camera->farClipPlane;
	const float clipRange = farClip - nearClip;

	float minZ = nearClip;
	float maxZ = nearClip + clipRange;

	float range = clipRange;
	float ratio = maxZ / minZ;

	// Calculate split depths based on view camera frustum
	// Based on method presented in https://developer.nvidia.com/gpugems/GPUGems3/gpugems3_ch10.html
	for (uint32_t i = 0; i < SHADOW_MAP_CASCADE_COUNT; i++) {
		float p = (i + 1) / static_cast<float>(SHADOW_MAP_CASCADE_COUNT);
		float log = minZ * std::pow(ratio, p);
		float uniform = minZ + range * p;
		float d = cascadeSplitLambda * (log - uniform) + uniform;
		cascadeSplits[i] = (d - nearClip) / clipRange;
	}

	glm::mat4 invCam = inverse(camera->getWorldToClipMatrix());

	// Calculate orthographic projection matrix for each cascade
	float lastSplitDist = 0.0;
	for (uint32_t i = 0; i < SHADOW_MAP_CASCADE_COUNT; i++) 
	{
		float splitDist = cascadeSplits[i];

		glm::vec3 frustumCorners[8] = {
			glm::vec3(-1.0f,  1.0f, 0.0f),
			glm::vec3(1.0f,  1.0f, 0.0f),
			glm::vec3(1.0f, -1.0f, 0.0f),
			glm::vec3(-1.0f, -1.0f, 0.0f),
			glm::vec3(-1.0f,  1.0f,  1.0f),
			glm::vec3(1.0f,  1.0f,  1.0f),
			glm::vec3(1.0f, -1.0f,  1.0f),
			glm::vec3(-1.0f, -1.0f,  1.0f),
		};

		// Project frustum corners into world space
		for (uint32_t j = 0; j < 8; j++) {
			glm::vec4 invCorner = invCam * glm::vec4(frustumCorners[j], 1.0f);
			frustumCorners[j] = invCorner / invCorner.w;
		}

		for (uint32_t j = 0; j < 4; j++) {
			glm::vec3 dist = frustumCorners[j + 4] - frustumCorners[j];
			frustumCorners[j + 4] = frustumCorners[j] + (dist * splitDist);
			frustumCorners[j] = frustumCorners[j] + (dist * lastSplitDist);
		}

		// Get frustum center
		glm::vec3 frustumCenter = glm::vec3(0.0f);
		for (uint32_t j = 0; j < 8; j++) {
			frustumCenter += frustumCorners[j];
		}
		frustumCenter /= 8.0f;

		float radius = 0.0f;
		for (uint32_t j = 0; j < 8; j++) {
			float distance = glm::length(frustumCorners[j] - frustumCenter);
			radius = glm::max(radius, distance);
		}
		radius = std::ceil(radius * 16.0f) / 16.0f;

		glm::vec3 maxExtents = glm::vec3(radius);
		glm::vec3 minExtents = -maxExtents;

		glm::mat4 lightViewMatrix = Mat4::LookAt(frustumCenter - glm::vec3(m_Direction) * -minExtents.z, frustumCenter, GetTransform()->Up());
		glm::mat4 lightOrthoMatrix = glm::ortho(minExtents.x, maxExtents.x, minExtents.y, maxExtents.y, -1000.0f, maxExtents.z - minExtents.z);
		lightOrthoMatrix[1][1] *= -1;

		// Store split distance and matrix in cascade
		m_CascadeSplitDepths[i] = (nearClip + splitDist * clipRange) * -1.0f;
		m_Lightspaces[i] = lightOrthoMatrix * lightViewMatrix;

		lastSplitDist = cascadeSplits[i];
	}
}

void Light::UpdatePointLightLightSpaces(uint32_t faceIndex)
{
	// TODO: add projection matrix
	glm::mat4 lightViewMatrix;
	glm::mat4 depthProjectionMatrix = Mat4::Perspective(glm::pi<float>() / 2.0f, 1.0f, 0.1f, 1024.0f);

	glm::vec3 pos = glm::vec3(m_Position);

	switch (faceIndex)
	{
	case 0: // POSITIVE_X
		lightViewMatrix = Mat4::LookAt(pos, pos + glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		break;
	case 1:	// NEGATIVE_X
		lightViewMatrix = Mat4::LookAt(pos, pos + glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		break;
	case 2:	// POSITIVE_Y
		lightViewMatrix = Mat4::LookAt(pos, pos + glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f));
		break;
	case 3:	// NEGATIVE_Y
		lightViewMatrix = Mat4::LookAt(pos, pos + glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		break;
	case 4:	// POSITIVE_Z
		lightViewMatrix = Mat4::LookAt(pos, pos + glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		break;
	case 5:	// NEGATIVE_Z
		lightViewMatrix = Mat4::LookAt(pos, pos + glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		break;
	}

	assert(faceIndex >= 0 && faceIndex < 6);
	m_Lightspaces[faceIndex] = depthProjectionMatrix * lightViewMatrix;
}

void Light::UpdateDirectionalLightUniform()
{
	DirectionalLightUniform uniform;

	// copy lightspaces
	if (m_UseShadows) {
		memcpy(uniform.lightspaces.data(), m_Lightspaces.data(), sizeof(glm::mat4) * SHADOW_MAP_CASCADE_COUNT);
		for (int i = 0; i < SHADOW_MAP_CASCADE_COUNT; ++i)
		{
			uniform.lightspaces[i] = BIAS_MAT * uniform.lightspaces[i];
		}
	}

	uniform.color = m_Color;
	uniform.direction = m_Direction;
	uniform.angle = 0.0f;
	uniform.intensity = m_Intensity;
	uniform.shadowOffset = m_UseShadows ? SHADOW_MAP_CASCADE_COUNT : 0;
	uniform.shadowStrength = m_ShadowAttenuation;
	uniform.splitDepths = m_CascadeSplitDepths;

	m_Uniform = uniform;
}

void Light::UpdatePointLightUniform()
{
	PointLightUniform uniform;

	// copy lightspaces
	if (m_UseShadows) {
		memcpy(uniform.lightspaces.data(), m_Lightspaces.data(), sizeof(glm::mat4) * OMNI_SHADOWMAPS_COUNT);
		for (int i = 0; i < OMNI_SHADOWMAPS_COUNT; ++i)
		{
			uniform.lightspaces[i] = BIAS_MAT * uniform.lightspaces[i];
		}
	}

	uniform.color = m_Color;
	uniform.position = m_Position;
	uniform.intensity = m_Intensity;
	uniform.radius = m_Radius;
	uniform.limit = m_Limit;
	uniform.shadowOffset = m_UseShadows ? OMNI_SHADOWMAPS_COUNT : 0;
	uniform.shadowStrength = m_ShadowAttenuation;

	m_Uniform = uniform;
}

void Light::UpdateSpotLightUniform()
{
	SpotLightUniform uniform;
	if (m_UseShadows) {
		uniform.lightspace = BIAS_MAT * m_Lightspaces[0];
	}
	uniform.color = m_Color;
	uniform.position = m_Position;
	uniform.direction = m_Direction;
	uniform.intensity = m_Intensity;
	uniform.radius = m_Radius;
	uniform.limit = m_Limit;
	uniform.fov = m_Fov;
	uniform.blend = m_Blend;
	uniform.shadowOffset = m_UseShadows ? 1 : 0;
	uniform.shadowStrength = m_ShadowAttenuation;
	uniform.nearClip = m_NearClip;

	m_Uniform = uniform;
}

template<>
void Scene::OnComponentAdded<Light>(Entity& entity, Light& component)
{
	m_SceneLights.emplace_back(&component);
}

template<>
void Scene::OnComponentRemoved<Light>(Entity& entity, Light& component)
{
	m_SceneLights.erase(
		std::remove_if(m_SceneLights.begin(), m_SceneLights.end(), [&component](Light* light) {
			return light->entity->id() == component.entity->id();
			}),
		m_SceneLights.end()
	);
}