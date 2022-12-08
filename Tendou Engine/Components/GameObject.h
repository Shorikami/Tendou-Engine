#ifndef GAMEOBJECT_H
#define GAMEOBJECT_H

#include "../Rendering/Model.h"

#include "../Components/Transform.h"

#include <memory>
#include <unordered_map>

namespace Tendou
{
	class GameObject
	{
	public:
		using id_t = unsigned int;
		using Map = std::unordered_map<id_t, GameObject>;

		static GameObject CreateGameObject()
		{
			static id_t currId = 0;
			return GameObject(currId++);
		}

		GameObject(const GameObject&) = delete;
		GameObject& operator=(const GameObject&) = delete;
		GameObject(GameObject&&) = default;
		GameObject& operator=(GameObject&&) = default;

		TransformComponent& Transform() { return m_Transform; }

		id_t GetID() { return m_ID; }

		std::shared_ptr<Model> model{};
		glm::vec3 color{};
		

	private:
		GameObject(id_t objId)
			: m_ID(objId)
		{
		}

		TransformComponent m_Transform;
		id_t m_ID;
	};
}

#endif