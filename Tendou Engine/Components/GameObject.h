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

		static GameObject CreateGameObject(std::string tag = std::string(), std::string name = std::string())
		{
			static id_t currId = 0;
			return GameObject(currId++, tag, name);
		}

		GameObject(const GameObject&) = delete;
		GameObject& operator=(const GameObject&) = delete;
		GameObject(GameObject&&) = default;
		GameObject& operator=(GameObject&&) = default;

		id_t GetID() { return m_ID; }

		std::string GetName() { return m_Name; }
		std::string GetTag() { return m_Tag; }

		bool GetRender() { return render; }

		std::shared_ptr<Model> GetModel() { return model; }
		Transform& GetTransform() { return m_Transform; }


		void SetName(std::string t) { m_Name = t; }
		void SetTag(std::string t) { m_Tag = t; }
		void SetRender(bool b) { render = b; }

		void SetModel(std::shared_ptr<Model> m) { model = m; }

		glm::vec3 color{};
		

	private:
		GameObject(id_t objId, std::string tag, std::string name)
			: m_ID(objId)
			, m_Tag(tag)
			, m_Name(name)
		{
		}

		id_t m_ID;

		std::string m_Name;
		std::string m_Tag;

		bool render = true;

		std::shared_ptr<Model> model{};
		
		// Components
		Transform m_Transform;
		
	};
}

#endif