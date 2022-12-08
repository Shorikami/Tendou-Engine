#ifndef TRANSFORM_H
#define TRANSFORM_H

#include <glm/gtc/matrix_transform.hpp>

namespace Tendou
{
	class TransformComponent
	{
	public:
		TransformComponent();
		TransformComponent(glm::vec3 tr, glm::vec3 rot, glm::vec3 scale);

		~TransformComponent();

		void Update();

		const glm::vec3 Translation() { return translation; }
		const glm::vec3 Rotation()  { return rotation; }
		const glm::vec3 Scale()  { return scale; }

		void SetTranslation(glm::vec3 set) { translation = set; }
		void SetRotation(glm::vec3 rotBy) { rotation = rotBy; }
		void SetScale(glm::vec3 set) { scale = set; }

		glm::mat4 Mat4();
		glm::mat3 NormalMatrix();

	private:
		glm::vec3 translation;
		glm::vec3 scale;
		glm::vec3 rotation;
	};
}

#endif