#ifndef CAMERA_H
#define CAMERA_H

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

enum class CameraDirection
{
	NONE = 0,
	FORWARD,
	BACKWARDS,
	LEFT,
	RIGHT,
	UP,
	DOWN
};

namespace Tendou
{
	class Camera
	{
	public:
		glm::vec3 cameraPos;

		glm::vec3 front, up, right;

		float yaw, pitch, speed, zoom;
		float n, f;

		int currW;
		int currH;

		bool rotateCamera = false;

		Camera(glm::vec3 pos = glm::vec3(-6.0f, -1.0f, 0.0f));

		void UpdateCameraDir(double dx, double dy);
		void UpdateCameraPos(CameraDirection d, double dt);
		void UpdateCameraZoom(double dy);

		__inline glm::mat4 view() const { return glm::lookAt(cameraPos, cameraPos + front, up); };
		__inline glm::mat4 perspective() const
		{
			return glm::perspective(glm::radians(zoom),
				static_cast<float>(currW) / static_cast<float>(currH),
				n, f);
		}

		__inline float GetZoom() { return zoom; };

	private:
		void UpdateCameraVectors();
	};
}

#endif