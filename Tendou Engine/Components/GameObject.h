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

		id_t GetID() { return m_ID; }
		std::string GetTag() { return m_Tag; }

		std::shared_ptr<Model> GetModel() { return model; }
		Transform& GetTransform() { return m_Transform; }


		void SetTag(std::string t) { m_Tag = t; }

		void SetModel(std::shared_ptr<Model> m) { model = m; }

		glm::vec3 color{};
		

	private:
		GameObject(id_t objId)
			: m_ID(objId)
			, m_Tag(std::string())
		{
		}

		id_t m_ID;
		std::string m_Tag;
		std::shared_ptr<Model> model{};
		
		// Components
		Transform m_Transform;
		
	};
}

#endif