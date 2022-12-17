#ifndef TRANSFORM_H
#define TRANSFORM_H

#include <glm/gtc/matrix_transform.hpp>

namespace Tendou
{
	class Transform
	{
	public:
		Transform();
		Transform(glm::vec3 tr, glm::vec3 rot, glm::vec3 scale);

		~Transform();

		void Update(bool rotate = false);

		const glm::mat4 ModelMat() { return modelMat; }
		const glm::vec3 Translation() { return translation; }
		const glm::vec3 Rotation()  { return rotation; }
		const glm::vec3 Scale()  { return scale; }
		const float RotationAngle() { return angleOfRotation; }

		const glm::vec3 PositionVec3();
		const glm::vec4 PositionVec4();

		void SetTranslation(glm::vec3 set) { translation = set; }
		void SetRotation(glm::vec3 rotBy) { rotation = rotBy; }
		void SetScale(glm::vec3 set) { scale = set; }
		void SetRotationAngle(float f) { angleOfRotation = f; }

		glm::mat4 Mat4();
		glm::mat3 NormalMatrix();

	private:
		glm::mat4 modelMat;

		glm::vec3 translation;
		glm::vec3 scale;
		glm::vec3 rotation;

		float angleOfRotation;
	};
}

#endif