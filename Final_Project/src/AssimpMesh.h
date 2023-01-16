#ifndef ASSIMP_MESH_H
#define ASSIMP_MESH_H

#include <glad/glad.h> // holds all OpenGL type declarations

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "shader.h"

#include <string>
#include <vector>
#include <iostream>
using namespace std;

#define MAX_BONE_INFLUENCE 4

struct Vertex {
    // position
    glm::vec3 Position;
    // normal
    glm::vec3 Normal;
    // texCoords
    glm::vec2 TexCoords;
    // tangent
    glm::vec3 Tangent;
    // bitangent
    glm::vec3 Bitangent;
    //bone indexes which will influence this vertex
    int m_BoneIDs[MAX_BONE_INFLUENCE];
    //weights from each bone
    float m_Weights[MAX_BONE_INFLUENCE];
};

struct Texture {
    unsigned int id;
    string type;
    string path;
};

class AssimpMesh {
public:
    // mesh Data
    vector<Vertex>       vertices;
    vector<unsigned int> indices;
    vector<Texture>      textures;
    unsigned int VAO;

    // constructor
    AssimpMesh(vector<Vertex> vertices, vector<unsigned int> indices, vector<Texture> textures)
        : xAngle(0.0f), yAngle(0.0f), zAngle(0.0f), xmax(0.0f), xmin(0.0f), ymax(0.0f), ymin(0.0f), zmax(0.0f), zmin(0.0f), rotation(glm::mat4(1.0f)), translateDiff(glm::vec3(0.0f)), bloomR(0.027)
    {
        this->vertices = vertices;
        this->indices = indices;
        this->textures = textures;

        // now that we have all the required data, set the vertex buffers and its attribute pointers.
        setupMesh();
    }

    void bindTextures(Shader& shader, unsigned int textureOffset = 0)
    {
        // textureOffset: How many textures are binded before textures of the model
        // bind appropriate textures
        unsigned int diffuseNr = 1;
        unsigned int specularNr = 1;
        unsigned int normalNr = 1;
        unsigned int heightNr = 1;
        for (unsigned int i = 0; i < textures.size(); i++)
        {            
            glActiveTexture(GL_TEXTURE0 + i + textureOffset); // active proper texture unit before binding
            // retrieve texture number (the N in diffuse_textureN)
            string number;
            string name = textures[i].type;
            if (name == "texture_diffuse")
                number = std::to_string(diffuseNr++);
            else if (name == "texture_specular")
                number = std::to_string(specularNr++); // transfer unsigned int to string
            else if (name == "texture_normal")
                number = std::to_string(normalNr++); // transfer unsigned int to string
            else if (name == "texture_height")
                number = std::to_string(heightNr++); // transfer unsigned int to string

            // now set the sampler to the correct texture unit
            glUniform1i(glGetUniformLocation(shader.ID, (name + number).c_str()), i + textureOffset);
            // and finally bind the texture
            glBindTexture(GL_TEXTURE_2D, textures[i].id);
        }
    }

    glm::mat4 getModelMatrix()
    {
        // model matrix
        glm::mat4 identity(1.0f);
        glm::mat4 model = identity;
        model = rotation * model;
        model = glm::translate(identity, translateDiff) * model;
        model = glm::scale(identity, glm::vec3(0.2f)) * model; // scale it to be smaller
        model = glm::translate(identity, glm::vec3(0.0f, -1.3f, 0.0f));
        return model;
    }


    // render the mesh
    void Draw(Shader& shader)
    {
        shader.use();
        bindTextures(shader, 0);

        // draw mesh
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, static_cast<unsigned int>(indices.size()), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);

        // always good practice to set everything back to defaults once configured.
        glActiveTexture(GL_TEXTURE0);
    }

    void draw_only_model(Shader& shader)
    {
        // render the scene only with model matrix (for creating shadow)

        // model matrix
        auto model = getModelMatrix();
        
        shader.setMat4("model", model);

        // render 
        glBindVertexArray(this->VAO);

        glDrawElements(GL_TRIANGLES, static_cast<unsigned int>(indices.size()), GL_UNSIGNED_INT, 0);

        glBindVertexArray(0);
    }

    void draw_single_color(Shader& singleColorShader, const glm::mat4& projection, const glm::mat4& view)
    {
        singleColorShader.use();
        auto model = getModelMatrix();
        singleColorShader.setMat4("model", model);
        singleColorShader.setMat4("projection", projection);
        singleColorShader.setMat4("view", view);
        singleColorShader.setFloat("bloomR", bloomR);
        glBindVertexArray(this->VAO);
        glDrawElements(GL_TRIANGLES, static_cast<unsigned int>(indices.size()), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }

    // Can accept toon or blinn-phong(non-toon)
    void draw_point_shadow(Shader& pointShadowShader, Shader& singleColorShader, const glm::mat4& projection, const glm::mat4& view, const glm::vec3 viewPos, const glm::vec3 lightPos, myTexture2D& texture, unsigned int& depthCubeMap, unsigned int& sceneTexture, const float& invisible, const float& far_plane, const bool& drawShadow)
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

        pointShadowShader.setBool("meshOrModel", false);
        pointShadowShader.setBool("drawShadow", drawShadow);

        // model matrix
        auto model = getModelMatrix();
        pointShadowShader.setMat4("model", model);

        glActiveTexture(GL_TEXTURE0);
        texture.bind();

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_CUBE_MAP, depthCubeMap);

        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, sceneTexture);

        // bind the textures of the model
        // 2: Mesh texture & shadowMap
        bindTextures(pointShadowShader, 3);

        // render the frame of the object
        glBindVertexArray(this->VAO);

        glDrawElements(GL_TRIANGLES, static_cast<unsigned int>(indices.size()), GL_UNSIGNED_INT, 0);

        glBindVertexArray(0);
    }

    // render the mesh
    void draw_blinn_phong(Shader& blinnPhongShader, const glm::vec3& lightPos, const glm::vec3& viewPos, const glm::mat4& view, const glm::mat4& projection)
    {
        blinnPhongShader.use();
        bindTextures(blinnPhongShader);

        blinnPhongShader.setMat4("projection", projection);
        blinnPhongShader.setMat4("view", view);

        blinnPhongShader.setVec3("viewPos", viewPos);
        blinnPhongShader.setVec3("lightPos", lightPos);

        // model matrix
        glm::mat4 identity(1.0f);
        glm::mat4 model = identity;
        model = rotation * model;
        model = glm::translate(identity, translateDiff) * model;
        model = glm::scale(identity, glm::vec3(0.3f)) * model;

        blinnPhongShader.setMat4("model", model);

        // draw mesh
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, static_cast<unsigned int>(indices.size()), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);

        // always good practice to set everything back to defaults once configured.
        glActiveTexture(GL_TEXTURE0);
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

private:
    // render data 
    unsigned int VBO, EBO;

    // initializes all the buffer objects/arrays
    void setupMesh()
    {
        // create buffers/arrays
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);

        glBindVertexArray(VAO);
        // load data into vertex buffers
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        // A great thing about structs is that their memory layout is sequential for all its items.
        // The effect is that we can simply pass a pointer to the struct and it translates perfectly to a glm::vec3/2 array which
        // again translates to 3/2 floats which translates to a byte array.
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

        // set the vertex attribute pointers
        // vertex Positions
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
        // vertex normals
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Normal));
        // vertex texture coords
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, TexCoords));
        // vertex tangent
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Tangent));
        // vertex bitangent
        glEnableVertexAttribArray(4);
        glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Bitangent));
        // ids
        glEnableVertexAttribArray(5);
        glVertexAttribIPointer(5, 4, GL_INT, sizeof(Vertex), (void*)offsetof(Vertex, m_BoneIDs));

        // weights
        glEnableVertexAttribArray(6);
        glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, m_Weights));
        glBindVertexArray(0);
    }

    // My mesh
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