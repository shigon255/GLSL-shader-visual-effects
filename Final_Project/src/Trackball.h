#ifndef TRACK_BALL_H
#define TRACK_BALL_H
#include <glm/glm.hpp>
#include <utility>
#include <cmath>

class Trackball
{
public:
	Trackball(float _r) : x(0.0f), y(0.0f), r(_r), nowVec(glm::vec3(0.0f, 0.0f, 1.0f)) {};
	Trackball(float _r, float width, float height) : x(width/2.0f), y(height/2.0f), r(_r), nowVec(glm::vec3(0.0f, 0.0f, 1.0f)) {}
	Trackball(float _r, float width, float height, glm::vec3 _front, glm::vec3 _up) : x(width / 2.0f), y(height / 2.0f), r(_r), front(glm::normalize(_front)), up(glm::normalize(_up))
	{
		nowVec = glm::normalize(-front);
		right = glm::normalize(glm::cross(front, up));
	}

	void refreshScreenCoord(float _x, float _y)
	{
		x = _x;
		y = _y;
		nowVec = -front;
	}
	
	// Refresh the camera's coordinate
	void refreshViewCoord(glm::vec3 _front, glm::vec3 _up)
	{
		front = _front;
		up = _up;
		nowVec = glm::normalize(-front);
		right = glm::normalize(glm::cross(front, up));
	}

	std::pair<glm::vec3, float> getRotation(float screenX, float screenY)
	{
		//get normalized new direction
		float newDirX = screenX - x;
		float newDirY = - (screenY - y); // y in screen space is upside-down

		glm::vec3 newVec;

		if (std::pow(newDirX, 2) + std::pow(newDirY, 2) > std::pow(r, 2))
		{
			newVec = glm::normalize(glm::vec3(newDirX, newDirY, 0.0f));
		}
		else
		{
			// x^2+y^2+z^2 = r^2, z>=0
			float newDirZ = std::sqrt(std::pow(r, 2) - std::pow(newDirX, 2) - std::pow(newDirY, 2));
			newVec = glm::normalize(glm::vec3(newDirX, newDirY, newDirZ));
			newVec = glm::normalize(newVec.x * right + newVec.y * up + newVec.z * (-front));
		}

		// We need to add the threshold, otherwise their will be no difference between nowVec and newVec
		// The threshold is the minimum difference of nowVec and newVec
		if (glm::length(newVec - nowVec) < 0.01) // threshold
		{
			// no rotation
			return std::pair < glm::vec3, float>(glm::vec3(0.0f, 0.0f, 1.0f), 0.0f);
		}
		else
		{
			float angle = glm::acos(glm::dot(nowVec, newVec));
			glm::vec3 rotateAxis = glm::normalize(glm::cross(nowVec, newVec));

			nowVec = newVec;

			return std::pair<glm::vec3, float>(rotateAxis, angle);
		}
	}

private:
	float x;
	float y;

	float r;

	glm::vec3 nowVec; //normalized
	glm::vec3 front;
	glm::vec3 up;
	glm::vec3 right;
};



#endif
