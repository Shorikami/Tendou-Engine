#include "Camera.h"

#include <cassert>
#include <limits>

namespace Drevis
{
	Camera::Camera(glm::vec3 pos)
		: cameraPos(pos)
		, yaw(0.0f)
		, pitch(0.0f)
		, speed(2.5f)
		, zoom(45.0f)
		, front(glm::vec3(0.0f, 0.0f, -1.0f))
		, n(0.1f)
		, f(100.0f)
	{
		// Todo: please make these tied to initial width/height of screen
		currW = 1280;
		currH = 720;
		UpdateCameraVectors();
	}

	void Camera::UpdateCameraDir(double dx, double dy)
	{
		if (rotateCamera)
		{
			yaw += static_cast<float>(dx);
			pitch -= static_cast<float>(dy);

			if (pitch > 89.0f)
			{
				pitch = 89.0f;
			}
			else if (pitch < -89.0f)
			{
				pitch = -89.0f;
			}

			UpdateCameraVectors();
		}
	}

	void Camera::UpdateCameraPos(CameraDirection dir, double dt)
	{
		float velocity = (float)dt * speed;

		switch (dir)
		{
		case CameraDirection::FORWARD:
			cameraPos += front * velocity;
			break;
		case CameraDirection::BACKWARDS:
			cameraPos -= front * velocity;
			break;
		case CameraDirection::RIGHT:
			cameraPos += glm::normalize(glm::cross(front, up)) * velocity;
			break;
		case CameraDirection::LEFT:
			cameraPos -= glm::normalize(glm::cross(front, up)) * velocity;
			break;
		case CameraDirection::UP:
			cameraPos += glm::normalize(glm::cross(front, right)) * velocity;
			break;
		case CameraDirection::DOWN:
			cameraPos -= glm::normalize(glm::cross(front, right)) * velocity;
			break;
		}
	}

	void Camera::UpdateCameraZoom(double dy)
	{
		if (zoom >= 1.0f && zoom <= 45.0f)
		{
			zoom -= static_cast<float>(dy);
		}
		else if (zoom < 1.0f)
		{
			zoom = 1.0f;
		}
		else
		{
			zoom = 45.0f;
		}
	}

	void Camera::UpdateCameraVectors()
	{
		glm::vec3 dir;
		dir.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
		dir.y = sin(glm::radians(pitch));
		dir.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
		front = glm::normalize(dir);

		right = glm::normalize(glm::cross(front, glm::vec3(0.0f, 1.0f, 0.0f)));
		up = glm::normalize(glm::cross(right, front));
	}
}
