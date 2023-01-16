#ifndef SPHERECAMERA_H
#define SPHERECAMERA_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <utility>
#include <iostream>
#include "trackball.h"

// A camera rotate with a center.
// the camera will always face the center

class SphereCamera 
{
public:
	SphereCamera(glm::vec3 _rotateCenter, float _r):
		rotateCenter(_rotateCenter), r(_r), theta(0.0f), phi(2.0f) 
	{
		updateAll();
	}

	SphereCamera(glm::vec3 _rotateCenter, float _r, float _theta, float _phi) :
		rotateCenter(_rotateCenter), r(_r), theta(_theta), phi(_phi)
	{
		updateAll();
	}

	void updateAll()
	{
		glm::vec3 diff = glm::vec3(1.0f, 0.0f, 0.0f) * (r * glm::cos(glm::radians(phi)) * glm::cos(glm::radians(theta))) + glm::vec3(0.0f, 0.0f, -1.0f) * (r * glm::cos(glm::radians(phi)) * glm::sin(glm::radians(theta))) + glm::vec3(0.0f, 1.0f, 0.0f) * (r * glm::sin(glm::radians(phi)));
		position = rotateCenter + diff;

		front = glm::normalize(rotateCenter - position);

		glm::vec3 extendPoint(rotateCenter + r * (1.0f / glm::cos(glm::radians(phi))) * glm::normalize(glm::vec3(1.0f, 0.0f, 0.0f) * (r * glm::cos(glm::radians(phi)) * glm::cos(glm::radians(theta))) + glm::vec3(0.0f, 0.0f, -1.0f) * (r * glm::cos(glm::radians(phi)) * glm::sin(glm::radians(theta)))));

		up = glm::normalize(position - extendPoint);
		if (phi < 0.0f)
			up = -up;
		
		//showStatus();
	}

	void showStatus()
	{
		std::cout << "Phi: " << phi << std::endl;
		std::cout << "Theta: " << theta << std::endl;
		std::cout << "r: " << r << std::endl;

		std::cout << "Front: ";
		for (int i = 0; i < 3; i++)
		{
			std::cout << front[i] << " ";
		}
		std::cout << std::endl;
		std::cout << "Up: ";
		for (int i = 0; i < 3; i++)
		{
			std::cout << up[i] << " ";
		}
		std::cout << std::endl;
		std::cout << "Position: ";
		for (int i = 0; i < 3; i++)
		{
			std::cout << position[i] << " ";
		}
		std::cout << std::endl;
		std::cout << "RotateCenter: ";
		for (int i = 0; i < 3; i++)
		{
			std::cout << rotateCenter[i] << " ";
		}
		std::cout << std::endl;
		std::cout << std::endl;
	}

	void updateCenter(glm::vec3 diff)
	{
		rotateCenter += diff;
		updateAll();
	}

	void updateR(float _r)
	{
		r += _r;
		if (r < 0.1f) r = 0.1f;
		updateAll();
	}

	void updateTheta(float angle)
	{
		theta += angle;
		if(theta > 360.0f)
			theta = 0.0f;
		
		if (theta < 0.0f)
			theta = 360.0f;

		updateAll();
	}

	void updatePhi(float angle)
	{
		phi += angle;
		if (phi > 88.0f)
			phi = 88.0f;

		if (phi < 10.0f)
			phi = 10.0f;

		updateAll();
	}

	glm::vec3 getPosition()
	{
		return position;
	}

	glm::vec3 getFront()
	{
		return front;
	}

	glm::vec3 getUp()
	{
		return up;
	}

	glm::vec3 getRight()
	{
		return glm::cross(front, up);
	}

	glm::mat4 getViewMatrix()
	{
		
		return glm::lookAt(position, front, up);
	}
private:
	glm::vec3 rotateCenter;
	glm::vec3 front;
	glm::vec3 up;
	glm::vec3 position;

	float theta;
	float phi;
	float r;
};

#endif