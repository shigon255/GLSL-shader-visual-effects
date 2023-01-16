#ifndef MESH_H
#define MESH_H
#include "shader.h"
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <assert.h>
#include <vector>
#include <utility>

// My mesh (different from assimp mesh)
class Mesh 
{
public:
	Mesh(glm::vec3 initialPosition) 
		: xAngle(0.0f), yAngle(0.0f), zAngle(0.0f), numIndices(0), numVertices(0), xmax(0.0f), xmin(0.0f), ymax(0.0f), ymin(0.0f), zmax(0.0f), zmin(0.0f), rotation(glm::mat4(1.0f)), translateDiff(initialPosition), bloomR(0.01) {}

	void draw_blinn_phong(Shader& shader, const glm::mat4& projection, const glm::mat4& view, const glm::vec3 viewPos, const glm::vec3* lightPos)
	{
		// For blinn-phong rendering(3 light)

		// use the shader
		shader.use();

		// position uniform
		shader.setVec3("lightPos", lightPos[0]);
		shader.setVec3("lightPos2", lightPos[1]);
		shader.setVec3("lightPos3", lightPos[2]);
		shader.setVec3("viewPos", viewPos);

		// view/projection transformations
		shader.setMat4("projection", projection);
		shader.setMat4("view", view);

		// translate matrix
		glm::mat4 identity(1.0f);
		glm::mat4 model = identity;
		model = rotation * model;
		model = glm::translate(identity, glm::vec3(-(xmax + xmin) / 2.0f, -(ymax + ymin) / 2.0f, -(zmax + zmin) / 2.0f));
		auto scaleEff = 1.0f / std::max(xmax - xmin, std::max( ymax - ymin, zmax - zmin));
		model = glm::scale(identity, glm::vec3(scaleEff, scaleEff, scaleEff)) * model;
		//model = glm::translate(identity, glm::vec3(xAngle, yAngle, zAngle)) * model;
		shader.setMat4("model", model);

		// normal matrix
		glm::mat3 normal_matrix(glm::transpose(glm::inverse(model)));
		shader.setMat3("normal_matrix", normal_matrix);

		// render 
		glBindVertexArray(this->VAO);

		glDrawElements(GL_TRIANGLES, numIndices, GL_UNSIGNED_INT, (void*)0);

		glBindVertexArray(0);
	}

	void draw_only_model(Shader& shader, const bool& normalize)
	{
		// render the scene only with model matrix (for creating shadow)

		// model matrix
		auto model = getModelMatrix(normalize);
		shader.setMat4("model", model);

		// render 
		glBindVertexArray(this->VAO);

		glDrawArrays(GL_TRIANGLES, 0, numVertices);

		glBindVertexArray(0);
	}
	// Can accept toon or blinn-phong(non-toon)
	void draw_point_shadow(Shader& pointShadowShader, Shader& singleColorShader, const glm::mat4& projection, const glm::mat4& view, const glm::vec3 viewPos, const glm::vec3 lightPos, myTexture2D& texture, unsigned int& depthCubeMap, unsigned int& sceneTexture, const float& invisible, const float& far_plane, const bool& normalize, const bool& stencil, const bool& drawShadow)
	{
		// use the shader
		pointShadowShader.use();

		pointShadowShader.setVec3("viewPos", viewPos);
		pointShadowShader.setVec3("lightPos", lightPos);

		// view/projection transformations
		pointShadowShader.setMat4("projection", projection);
		pointShadowShader.setMat4("view", view);

		pointShadowShader.setFloat("far_plane", far_plane);
		pointShadowShader.setFloat("invisible", invisible);

		pointShadowShader.setBool("meshOrModel", true);
		pointShadowShader.setBool("drawShadow", drawShadow);

		// model matrix
		auto model = getModelMatrix(normalize);
		pointShadowShader.setMat4("model", model);

		glActiveTexture(GL_TEXTURE0);
		texture.bind();

		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_CUBE_MAP, depthCubeMap);

		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, sceneTexture);

		if (stencil)
		{
			// record stencil buffer
			glStencilFunc(GL_ALWAYS, 1, 0xFF);
			glStencilMask(0xFF);
		}

		// render the frame of the object
		glBindVertexArray(this->VAO);

		glDrawArrays(GL_TRIANGLES, 0, numVertices);
		
		if (stencil)
		{
			// render the frame
			glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
			glStencilMask(0x00);
			glDisable(GL_DEPTH_TEST);
			singleColorShader.use();
			float scale = 1.01f;
			//model = glm::scale(identity, glm::vec3(scale, scale, scale)) * model;
			singleColorShader.setMat4("model", model);
			singleColorShader.setMat4("projection", projection);
			singleColorShader.setMat4("view", view);
			singleColorShader.setFloat("bloomR", bloomR);

			glDrawArrays(GL_TRIANGLES, 0, numVertices);
		}

		glBindVertexArray(0);
		if (stencil)
		{
			// reset the OpenGL settings
			glStencilMask(0xFF);
			glStencilFunc(GL_ALWAYS, 0, 0xFF);
			glEnable(GL_DEPTH_TEST);
		}
	}


	glm::mat4 getModelMatrix(const bool& normalize)
	{
		glm::mat4 identity(1.0f);
		glm::mat4 model = identity;
		model = rotation * model;
		if (normalize)
		{
			// place the object to (0,0,0) and normalize the size to 1

			model = glm::translate(identity, glm::vec3(-(xmax + xmin) / 2.0f, -(ymax + ymin) / 2.0f, -(zmax + zmin) / 2.0f));
			auto scaleEff = 1.0f / std::max(xmax - xmin, std::max(ymax - ymin, zmax - zmin));
			model = glm::scale(identity, glm::vec3(scaleEff, scaleEff, scaleEff)) * model;
		}
		else
		{
			model = glm::translate(identity, translateDiff) * model;
		}
		return model;
	}

	void updateTranslateDiff(glm::vec3 diff)
	{
		translateDiff += diff;
	}

	void updateRotation(std::pair<glm::vec3, float> p)
	{
		glm::vec3 axis = p.first;
		float angle = p.second;

		glm::mat4 newRotation = glm::rotate(glm::mat4(1.0f), angle, axis);
		rotation = newRotation * rotation;
	}

	void updateAngle(const float& deltaAngle, const unsigned int& angleIndex)
	{
		switch (angleIndex)
		{
		case 0:
			xAngle += deltaAngle;
			break;
		case 1:
			yAngle += deltaAngle;
			break;
		case 2:
			zAngle += deltaAngle;
			break;
		}
	}

	void updateBloomR(const float deltaBloomR)
	{
		bloomR += deltaBloomR;
		if (bloomR <= 0.005f) bloomR = 0.005f;
		if (bloomR >= 0.5f) bloomR = 0.5f;
	}

	// load for specific obj file format
	// If the obj file format is general, use assimp to load the model.
	void load_vtn(const char* filepath)
	{
		// std::cout << "start load"<<std::endl;
		// redirect standard I/O to read file
		std::ifstream in(filepath);
		std::streambuf* cinbuf = std::cin.rdbuf(); //save old buf
		std::cin.rdbuf(in.rdbuf()); //redirect std::cin to in.txt!

		//read obj file
		std::vector<float> vertices;

		std::vector<float> tmpVertices;
		std::vector<float> tmpNormals;
		std::vector<float> tmpUVs;

		float tmpMaxMin[6];

		for (int i = 0; i < 3; i++)
		{
			tmpMaxMin[2 * i] = -1;
			tmpMaxMin[2 * i + 1] = 10000;
		}

		std::string tmpS;
		char tmpC;
		float tmpf;

		while ((std::cin >> tmpS) && tmpS != "f")
		{
			for (int i = 0; i < 3; i++)
			{
				if (tmpS == "vn")
				{
					for (int j = 0; j < 3; j++)
					{
						std::cin >> tmpf;
						tmpNormals.push_back(tmpf);
					}
				}
				else if (tmpS == "vt")
				{
					for (int j = 0; j < 2; j++)
					{
						std::cin >> tmpf;
						tmpUVs.push_back(tmpf);
					}
				}
				else
				{
					for (int j = 0; j < 3; j++)
					{
						std::cin >> tmpf;
						tmpVertices.push_back(tmpf);

						tmpMaxMin[2 * j] = std::max(tmpMaxMin[2 * j], tmpf);
						tmpMaxMin[2 * j + 1] = std::min(tmpMaxMin[2 * j + 1], tmpf);
					}
				}
				if (i != 2)
				{
					std::cin >> tmpS;
				}
			}
		}


		xmax = tmpMaxMin[0];
		xmin = tmpMaxMin[1];
		ymax = tmpMaxMin[2];
		ymin = tmpMaxMin[3];
		zmax = tmpMaxMin[4];
		zmin = tmpMaxMin[5];

		// note that index means 3 component

		for (int i = 0; i < 3; i++)
		{
			unsigned int tmpU;
			unsigned int tmpUA[3];
			for (int j = 0; j < 3; j++)
			{
				std::cin >> tmpU;
				tmpUA[j] = tmpU;
				if (j != 2)
				{
					std::cin >> tmpC;
				}
			}
			auto normalIndex = tmpUA[0] - 1;
			auto UVIndex = tmpUA[1] - 1;
			auto vertexIndex = tmpUA[2] - 1;

			for (int j = 0; j < 3; j++)
			{
				vertices.push_back(tmpVertices[vertexIndex * 3 + j]);
			}
			for (int j = 0; j < 3; j++)
			{
				vertices.push_back(tmpNormals[normalIndex * 3 + j]);
			}
			for (int j = 0; j < 2; j++)
			{
				vertices.push_back(tmpUVs[UVIndex * 2 + j]);
			}

		}

		while (std::cin >> tmpS)
		{
			for (int i = 0; i < 3; i++)
			{
				unsigned int tmpU;
				unsigned int tmpUA[3];
				for (int j = 0; j < 3; j++)
				{
					std::cin >> tmpU;
					tmpUA[j] = tmpU;
					if (j != 2)
					{
						std::cin >> tmpC;
					}
				}
				auto normalIndex = tmpUA[0] - 1;
				auto UVIndex = tmpUA[1] - 1;
				auto vertexIndex = tmpUA[2] - 1;

				for (int j = 0; j < 3; j++)
				{
					vertices.push_back(tmpVertices[vertexIndex * 3 + j]);
				}
				for (int j = 0; j < 3; j++)
				{
					vertices.push_back(tmpNormals[normalIndex * 3 + j]);
				}
				for (int j = 0; j < 2; j++)
				{
					vertices.push_back(tmpUVs[UVIndex * 2 + j]);
				}

			}
		}

		assert(vertices.size() % 8 == 0);

		numVertices = vertices.size() / 8;

		unsigned int VBO;

		// VAO
		glGenVertexArrays(1, &this->VAO);

		// Attribute Array and Buffer
		glBindVertexArray(this->VAO);

		// VBO
		glGenBuffers(1, &VBO);
		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), &vertices.front(), GL_STATIC_DRAW);


		// position
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(0);

		// normal
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
		glEnableVertexAttribArray(1);

		// UV
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
		glEnableVertexAttribArray(2);

		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);
	}

	void load_block()
	{
		// for block object

		std::vector<float> vertices = {
		-0.5f, -0.5f, -0.5f,  0.0f, 0.0f, -1.0f, 0.0f, 0.0f,
		 0.5f, -0.5f, -0.5f,  0.0f, 0.0f, -1.0f, 1.0f, 0.0f,
		 0.5f,  0.5f, -0.5f,  0.0f, 0.0f, -1.0f, 1.0f, 1.0f,
		 0.5f,  0.5f, -0.5f,  0.0f, 0.0f, -1.0f, 1.0f, 1.0f,
		-0.5f,  0.5f, -0.5f,  0.0f, 0.0f, -1.0f, 0.0f, 1.0f,
		-0.5f, -0.5f, -0.5f,  0.0f, 0.0f, -1.0f, 0.0f, 0.0f,

		-0.5f, -0.5f,  0.5f,  0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
		 0.5f, -0.5f,  0.5f,  0.0f, 0.0f, 1.0f, 1.0f, 0.0f,
		 0.5f,  0.5f,  0.5f,  0.0f, 0.0f, 1.0f, 1.0f, 1.0f,
		 0.5f,  0.5f,  0.5f,  0.0f, 0.0f, 1.0f, 1.0f, 1.0f,
		-0.5f,  0.5f,  0.5f,  0.0f, 0.0f, 1.0f, 0.0f, 1.0f,
		-0.5f, -0.5f,  0.5f,  0.0f, 0.0f, 1.0f, 0.0f, 0.0f,

		-0.5f,  0.5f,  0.5f,  -1.0f, 0.0f, 0.0f, 1.0f, 0.0f,
		-0.5f,  0.5f, -0.5f,  -1.0f, 0.0f, 0.0f, 1.0f, 1.0f,
		-0.5f, -0.5f, -0.5f,  -1.0f, 0.0f, 0.0f, 0.0f, 1.0f,
		-0.5f, -0.5f, -0.5f,  -1.0f, 0.0f, 0.0f, 0.0f, 1.0f,
		-0.5f, -0.5f,  0.5f,  -1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
		-0.5f,  0.5f,  0.5f,  -1.0f, 0.0f, 0.0f, 1.0f, 0.0f,

		 0.5f,  0.5f,  0.5f,  1.0f, 0.0f, 0.0f, 1.0f, 0.0f,
		 0.5f,  0.5f, -0.5f,  1.0f, 0.0f, 0.0f, 1.0f, 1.0f,
		 0.5f, -0.5f, -0.5f,  1.0f, 0.0f, 0.0f, 0.0f, 1.0f,
		 0.5f, -0.5f, -0.5f,  1.0f, 0.0f, 0.0f, 0.0f, 1.0f,
		 0.5f, -0.5f,  0.5f,  1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
		 0.5f,  0.5f,  0.5f,  1.0f, 0.0f, 0.0f, 1.0f, 0.0f,

		-0.5f, -0.5f, -0.5f,  0.0f, -1.0f, 0.0f, 0.0f, 1.0f,
		 0.5f, -0.5f, -0.5f,  0.0f, -1.0f, 0.0f, 1.0f, 1.0f,
		 0.5f, -0.5f,  0.5f,  0.0f, -1.0f, 0.0f, 1.0f, 0.0f,
		 0.5f, -0.5f,  0.5f,  0.0f, -1.0f, 0.0f, 1.0f, 0.0f,
		-0.5f, -0.5f,  0.5f,  0.0f, -1.0f, 0.0f, 0.0f, 0.0f,
		-0.5f, -0.5f, -0.5f,  0.0f, -1.0f, 0.0f, 0.0f, 1.0f,

		-0.5f,  0.5f, -0.5f,  0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
		 0.5f,  0.5f, -0.5f,  0.0f, 1.0f, 0.0f, 1.0f, 1.0f,
		 0.5f,  0.5f,  0.5f,  0.0f, 1.0f, 0.0f, 1.0f, 0.0f,
		 0.5f,  0.5f,  0.5f,  0.0f, 1.0f, 0.0f, 1.0f, 0.0f,
		-0.5f,  0.5f,  0.5f,  0.0f, 1.0f, 0.0f, 0.0f, 0.0f,
		-0.5f,  0.5f, -0.5f,  0.0f, 1.0f, 0.0f, 0.0f, 1.0f
		};

		xmax = 0.5f;
		xmin = -0.5f;
		ymax = 0.5f;
		ymin = -0.5f;
		zmax = 0.5f;
		zmin = -0.5f;

		numVertices = vertices.size() / 8;

		unsigned int VBO;

		// VAO
		glGenVertexArrays(1, &this->VAO);

		// Attribute Array and Buffer
		glBindVertexArray(this->VAO);

		// VBO
		glGenBuffers(1, &VBO);
		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), &vertices.front(), GL_STATIC_DRAW);


		// position
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(0);

		// normal
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
		glEnableVertexAttribArray(1);

		// UV
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
		glEnableVertexAttribArray(2);

		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);
	}

	// for obj file that only contain vertices and faces
	void load(const char* filepath)
	{
		// redirect standard I/O to read file
		std::ifstream in(filepath);
		std::streambuf* cinbuf = std::cin.rdbuf(); //save old buf
		std::cin.rdbuf(in.rdbuf()); //redirect std::cin to in.txt!

		//read obj file
		std::vector<float> vertices;
		std::vector<unsigned int> indices;


		float tmpMaxMin[6];

		char tmpc;
		unsigned int tmpu;
		float tmpf;

		//take first value to initialize maxmin
		std::cin >> tmpc;
		for (int i = 0; i < 3; i++)
		{
			std::cin >> tmpf;
			vertices.push_back(tmpf);
			tmpMaxMin[2 * i] = tmpf;
			tmpMaxMin[2 * i + 1] = tmpf;
		}
		for (int i = 0; i < 3; i++)
		{
			// default normal
			vertices.push_back(0.0f);
		}

		while (std::cin >> tmpc)
		{
			if (tmpc != 'v') break;
			for (int i = 0; i < 3; i++)
			{
				std::cin >> tmpf;
				vertices.push_back(tmpf);
				tmpMaxMin[2 * i] = tmpMaxMin[2 * i] > tmpf ? tmpMaxMin[2 * i] : tmpf;
				tmpMaxMin[2 * i + 1] = tmpMaxMin[2 * i + 1] < tmpf ? tmpMaxMin[2 * i + 1] : tmpf;
			}
			for (int i = 0; i < 3; i++)
			{
				// default normal
				vertices.push_back(0.0f);
			}
		}

		xmax = tmpMaxMin[0];
		xmin = tmpMaxMin[1];
		ymax = tmpMaxMin[2];
		ymin = tmpMaxMin[3];
		zmax = tmpMaxMin[4];
		zmin = tmpMaxMin[5];

		auto vertSize = vertices.size();

		while (std::cin >> tmpc)
		{
			unsigned int tmpind[3];
			for (int i = 0; i < 3; i++)
			{
				std::cin >> tmpu;
				tmpind[i] = tmpu;
				indices.push_back(tmpu - 1);
			}
			updateNormal(vertices, tmpind);
		}

		numIndices = indices.size();

		unsigned int VBO;
		unsigned int EBO;

		// VAO
		glGenVertexArrays(1, &this->VAO);

		// Attribute Array and Buffer
		glBindVertexArray(this->VAO);

		// VBO
		glGenBuffers(1, &VBO);
		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), &vertices.front(), GL_STATIC_DRAW);


		// EBO
		glGenBuffers(1, &EBO);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices.front(), GL_STATIC_DRAW);

		// position
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(0);

		// normal
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
		glEnableVertexAttribArray(1);

		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);
	}
private:

	glm::vec3 getNormal(const glm::vec3* vertices)
	{
		glm::vec3 AtoB = vertices[1] - vertices[0];
		glm::vec3 BtoC = vertices[2] - vertices[1];
		return glm::cross(AtoB, BtoC)/glm::length(glm::cross(AtoB, BtoC));
	}

	void updateNormal(std::vector<float>& vertices, const unsigned int * index)
	{
		// update 3 vertices normal

		glm::vec3 three_vert[3];
		for (int i = 0; i < 3; i++)
		{
			three_vert[i] = glm::vec3(vertices[(index[i] - 1) * 6], vertices[(index[i] - 1) * 6 + 1], vertices[(index[i] - 1) * 6 + 2]);
		}

		auto normal = getNormal(three_vert);

		for (int i = 0; i < 3; i++)
		{
			glm::vec3 nowNormal(vertices[(index[i] - 1) * 6 + 3], vertices[(index[i] - 1) * 6 + 4], vertices[(index[i] - 1) * 6 + 5]);
			nowNormal = (nowNormal + normal) / glm::length(nowNormal + normal);
			for (int j = 0; j < 3; j++)
			{
				vertices[(index[i] - 1) * 6 + 3 + j] = nowNormal[j];
			}
		}
	}

	unsigned int VAO;
	unsigned int numIndices;
	unsigned int numVertices; // for spot.obj

	float xAngle;
	float yAngle;
	float zAngle;

	glm::mat4 rotation;
	glm::vec3 translateDiff;

	float bloomR;

	float xmax;
	float xmin;
	float ymax;
	float ymin;
	float zmax;
	float zmin;
};



#endif
