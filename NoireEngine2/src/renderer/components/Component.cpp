#include "Component.hpp"
#include "utils/Logger.hpp"
#include <iostream>

#include "renderer/scene/Entity.hpp"

#define CHECK_ENTITY 	if (!entity) { NE_WARN("Entity is null!"); return nullptr; }

void Component::SetEntity(Entity* thisEntity)
{
	entity = thisEntity;
}

Transform* Component::GetTransform()
{
	CHECK_ENTITY
	return entity->transform();
}

Scene* Component::GetScene()
{
	CHECK_ENTITY
	return entity->scene();
}
