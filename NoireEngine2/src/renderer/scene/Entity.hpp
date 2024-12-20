#pragma once

#include <memory>
#include <vector>
#include <list>
#include <set>
#include <utils/UUID.hpp>
#include <string>

#include "Transform.hpp"
#include "renderer/components/Components.hpp"
#include "Scene.hpp"

#define ADD_CHILD 		m_Children.emplace_back(std::make_unique<Entity>(m_Scene, args...));\
						m_Children.back()->SetParent(this);\
						return m_Children.back().get();\

class TransformMatrixStack;

class Entity
{
public:
	static Entity& root()
	{
		static Entity r;
		return r;
	}

	Entity() = default;

	template<typename... TArgs>
	Entity(Scene* scene, TArgs&... args) :
		m_Scene(scene), s_Transform(std::make_unique<Transform>(args...)) {
	}

	template<typename... TArgs>
	Entity(Scene* scene, const std::string& name, TArgs&... args) :
		m_Scene(scene), m_Name(name), s_Transform(std::make_unique<Transform>(args...)) {
	}

	template<typename... TArgs>
	Entity(Scene* scene, const char* name, TArgs&... args) :
		m_Scene(scene), m_Name(name), s_Transform(std::make_unique<Transform>(args...)) {
	}

	~Entity();

public:
	template<typename... TArgs>
	Entity* AddChild(TArgs&... args)
	{
		ADD_CHILD
	}

	// Variation of AddChild where it takes in a scene to overwrite current scene.
	template<typename... TArgs>
	Entity* AddChild(Scene* scene, TArgs&... args)
	{
		m_Scene = scene;
		ADD_CHILD
	}

	// Adds a component to an entity
	template<typename T, typename... Args>
	T& AddComponent(Args&&... args)
	{
		m_Components.emplace_back(std::make_unique<T>(std::forward<Args>(args)...));
		m_Components.back()->SetEntity(this);
		T& newComponent = *dynamic_cast<T*>(m_Components.back().get());
		m_Scene->OnComponentAdded(*this, newComponent);
		return newComponent;
	}

	template<typename T>
	T* GetComponent() const
	{
		for (const auto& component : m_Components)
		{
			if (dynamic_cast<T*>(component.get()))
				return (T*)component.get();
			else
				continue;
		}
		return nullptr;
	}

	void SetParent(Entity* newParent);

	// dfs the scene graph and update components
	void Update();

	// stores transforms in a matrix stack, and push transform matrices in a reverse order
	void RenderPass(TransformMatrixStack& matrixStack);

public:
	inline Entity* parent()												{ return m_Parent; }
	inline Transform* transform()										{ return s_Transform.get(); }
	inline UUID& id()													{ return m_Id; }
	inline std::string& name()											{ return m_Name; }
	inline std::list<std::unique_ptr<Entity>>& children()				{ return m_Children; }
	const std::vector<std::unique_ptr<Component>>& components() const { return m_Components; }

	void SetScene(Scene* newScene) { m_Scene = newScene; }
	Scene* scene() { return m_Scene; }

private:
	friend class Scene;
	void PrepareAcceleration(TransformMatrixStack& matrixStack);

private:

	std::unique_ptr<Transform>				s_Transform;
	Entity*									m_Parent = nullptr;
	std::list<std::unique_ptr<Entity>>		m_Children; // manages its children
	std::vector<std::unique_ptr<Component>>	m_Components;
	Scene*									m_Scene = nullptr;

	UUID									m_Id;
	std::string								m_Name;
};

