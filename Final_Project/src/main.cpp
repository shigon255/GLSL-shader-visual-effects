#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

/*
#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include <stbi_image.h>
#endif
*/

#include <stbi_image.h>

#include "Shader.h"
#include "my_texture_2d.h"
#include "Mesh.h"
#include "Trackball.h"
#include "SphereCamera.h"
#include "Camera.h"
#include "Skybox.h"
#include "Model.h"



#pragma comment( lib, "lib/glew32.lib" )
#pragma comment( lib , "lib/glfw3.lib")
#pragma comment( lib , "lib/assimp-vc143-mtd.lib")
#pragma comment( lib , "lib/soil2-debug.lib")


void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void processInput(GLFWwindow* window);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;
const unsigned int SHADOW_WIDTH = 1024;
const unsigned int SHADOW_HEIGHT = 1024;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

float timer = 0.0f;
float updateTimeMax = 0.01f;
float buttonTimeMax = 0.3f;

// adjusting
float deltaAngle = 1.0f;
float deltaTranslate = 0.1f;
float deltaBloomR = 0.002f;
float invisible = 0.0f;
bool toon = false;
bool stencil = false;
bool bloom = true;
std::string skybox_name("rock");

// Skyboxs
Skybox skybox;

// Meshs
Mesh floorMesh(glm::vec3(0.0f));

// Camera & lights
//SphereCamera camera(glm::vec3(0.0f, 0.0f, 0.0f), 8.0);
Camera camera(glm::vec3(0.0f, 10.0f, 10.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

SphereCamera pointLight(glm::vec3(0.0f, 0.0f, 0.0f), 20.0f, 0.0f, 45.0f);

float trackballSize = static_cast<float>(SCR_HEIGHT);
Trackball trackball(trackballSize, SCR_WIDTH, SCR_HEIGHT, camera.getFront(), camera.getUp());

int mouseState = GLFW_RELEASE;

float mousePosX = static_cast<float>(SCR_WIDTH);
float mousePosY = static_cast<float>(SCR_HEIGHT);

myTexture2D loadTextureFromFile(const char* file, bool alpha);

void renderQuad();

int main()
{
    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw window creation
    // --------------------
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Render 3D Mesh", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetInputMode(window, GLFW_STICKY_MOUSE_BUTTONS, GLFW_TRUE);


    // tell GLFW to capture our mouse
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // tell stb_image.h to flip loaded texture's on the y-axis (before loading model).
    stbi_set_flip_vertically_on_load(true);

    // configure global opengl state
    // -----------------------------
    glEnable(GL_DEPTH_TEST);
    
    glDepthFunc(GL_LESS);
    glEnable(GL_STENCIL_TEST);
    glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

    glEnable(GL_CULL_FACE);

    // build and compile shaders
    Shader singleColorShader("shaders/singleColorShader.vert", "shaders/singleColorShader.frag");
    std::cout << "SingleColorShader end" << std::endl;

    Shader pointShadowToonShader("shaders/pointShadowToonShader.vert", "shaders/pointShadowToonShader.frag");
    std::cout << "pointShadowToonShader end" << std::endl;
    Shader pointShadowShader("shaders/pointShadowShader.vert", "shaders/pointShadowShader.frag");
    std::cout << "pointShadowShader end" << std::endl;

    Shader simplePointDepthShader("shaders/simplePointDepthShader.vert", "shaders/simplePointDepthShader.frag", "shaders/simplePointDepthShader.geo");
    std::cout << "simplePointDepthShader end" << std::endl;

    Shader bloomShader("shaders/bloomShader.vert", "shaders/bloomShader.frag"); // Final bloom shader
    std::cout << "bloomShader end" << std::endl;
    Shader blurShader("shaders/blurShader.vert", "shaders/blurShader.frag");
    std::cout << "blurShader end" << std::endl;

    Shader skyboxShader("shaders/skyboxShader.vert", "shaders/skyboxShader.frag");
    std::cout << "skyboxShader end" << std::endl;

    pointShadowToonShader.use();
    pointShadowToonShader.setInt("meshTexture", 0);
    pointShadowToonShader.setInt("shadowMap", 1);
    pointShadowToonShader.setInt("scene", 2);

    pointShadowShader.use();
    pointShadowShader.setInt("meshTexture", 0);
    pointShadowShader.setInt("shadowMap", 1);
    pointShadowShader.setInt("scene", 2);

    blurShader.use();
    blurShader.setInt("image", 0);

    bloomShader.use();
    bloomShader.setInt("scene", 0);
    bloomShader.setInt("bloomBlur", 1);

    skyboxShader.setInt("skybox", 0);

    // load textures

    myTexture2D spotTexture = loadTextureFromFile("textures/others/spot_texture.png", false);
    myTexture2D hmap = loadTextureFromFile("textures/others/hmap.jpg", false); //test
    myTexture2D floorTexture = loadTextureFromFile("textures/skybox_rock/bottom.png", true);

    // load models
    floorMesh.load_vtn("meshs/others/floor.obj");
    
    Model ourModel("meshs/nanosuit/nanosuit.obj");

    // load skybox's texture & vertices

    skybox.load_vertices();

    std::vector<std::string> faces
    {
        "textures/skybox_rock/right.png",
        "textures/skybox_rock/left.png",
        "textures/skybox_rock/top.png",
        "textures/skybox_rock/bottom.png",
        "textures/skybox_rock/front.png",
        "textures/skybox_rock/back.png"
    };

    skybox.loadCubemap(faces, true);

    // Cube depth map (For point shadow)
     
    // Create FBO for storing depth map(cube)    
    unsigned int depthCubeMapFBO;
    glGenFramebuffers(1, &depthCubeMapFBO);
    // Create depth cubemap texture
    unsigned int depthCubemap;
    glGenTextures(1, &depthCubemap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, depthCubemap);
    for (unsigned int i = 0; i < 6; ++i)
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    //Attach texture to FBO
    glBindFramebuffer(GL_FRAMEBUFFER, depthCubeMapFBO);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depthCubemap, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    

    // Create FBO for post-processing
    unsigned int blurFBO;
    glGenFramebuffers(1, &blurFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, blurFBO);

    // Create 2 color attachment to this FBO. One for the real scene, and the other for the bright part
    unsigned int colorBuffers[2];
    glGenTextures(2, colorBuffers);
    for (unsigned int i = 0; i < 2; i++)
    {
        glBindTexture(GL_TEXTURE_2D, colorBuffers[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);  // we clamp to the edge as the blur filter would otherwise sample repeated texture values!
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        // attach texture to framebuffer
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, colorBuffers[i], 0);
    }

    // create depth & stencil buffer (renderbuffer)
    // note that we need both depth and stencil buffer
    unsigned int rboDepthStencil;
    glGenRenderbuffers(1, &rboDepthStencil);
    glBindRenderbuffer(GL_RENDERBUFFER, rboDepthStencil);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, SCR_WIDTH, SCR_HEIGHT);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rboDepthStencil);
    
    // tell OpenGL which color attachments we'll use (of this framebuffer) for rendering 
    unsigned int attachments[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
    glDrawBuffers(2, attachments);
    // finally check if framebuffer is complete
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "Framebuffer not complete!" << std::endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // ping-pong-framebuffer for blurring
    // ping-pong: blur horizentally and then vertically, repeat these 2 steps to save the blur time(Ex. 1024 -> 32+32)
    unsigned int pingpongFBO[2];
    unsigned int pingpongColorbuffers[2];
    glGenFramebuffers(2, pingpongFBO);
    glGenTextures(2, pingpongColorbuffers);
    // Create 2 color attachment to ping-pong FBO
    for (unsigned int i = 0; i < 2; i++)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[i]);
        glBindTexture(GL_TEXTURE_2D, pingpongColorbuffers[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); // we clamp to the edge as the blur filter would otherwise sample repeated texture values!
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pingpongColorbuffers[i], 0);
        // also check if framebuffers are complete (no need for depth buffer)
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            std::cout << "Framebuffer not complete!" << std::endl;
    }

    // render loop
    // -----------
    while (!glfwWindowShouldClose(window))
    {
        // per-frame time logic
        // --------------------
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        timer += deltaTime;

        // input
        // -----
        processInput(window);

        // render
        // ------
        glClearColor(0.3f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT); // stencil buffer

        // update rotation by trackball
        if (mouseState == GLFW_PRESS)
        {
            auto p = trackball.getRotation(mousePosX, mousePosY);
            ourModel.updateRotation(p);
        }
        
        // projection & view
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = camera.GetViewMatrix();
        glm::vec3 viewPos = camera.getPosition();
        glm::vec3 lightPos = pointLight.getPosition();

        // Step 1. Render to depth map
        // Setting transform matrices
        float point_near_plane = 1.0f;
        float point_far_plane = 250.0f;
        glm::mat4 shadowProj = glm::perspective(glm::radians(90.0f), (float)SHADOW_WIDTH / (float)SHADOW_HEIGHT, point_near_plane, point_far_plane);
        std::vector<glm::mat4> shadowTransforms;
        // Light will face 6 different direction and record the depth of objects on cubemap
        shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)));
        shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)));
        shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)));
        shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f)));
        shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f)));
        shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, -1.0f, 0.0f)));

        glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
        simplePointDepthShader.use();
        for (unsigned int i = 0; i < 6; ++i)
            simplePointDepthShader.setMat4("shadowMatrices[" + std::to_string(i) + "]", shadowTransforms[i]);
        simplePointDepthShader.setFloat("far_plane", point_far_plane);
        simplePointDepthShader.setVec3("lightPos", lightPos);

        // Bind the framebuffer to depth FBO to store the depth of objects
        glBindFramebuffer(GL_FRAMEBUFFER, depthCubeMapFBO);
        glClear(GL_DEPTH_BUFFER_BIT);

        // Draw to store the depths
        floorMesh.draw_only_model(simplePointDepthShader, false);
        ourModel.draw_only_model(simplePointDepthShader);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // Step 2. Draw the scene onto the blurFBO. Using the depthFBO to create shadow

        // Bind the framebuffer to blurFBO
        glBindFramebuffer(GL_FRAMEBUFFER, blurFBO);
        glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);

        
        // Draw the skybox
        skybox.draw(skyboxShader, view, projection);
        glClear(GL_STENCIL_BUFFER_BIT);

        // Draw the real scene
        Shader& pointShader = toon ? pointShadowToonShader : pointShadowShader;
        floorMesh.draw_point_shadow(pointShader, singleColorShader, projection, view, viewPos, lightPos, floorTexture, depthCubemap, colorBuffers[0], 0.0f, point_far_plane, false, false, (invisible < 0.1f));
        glClear(GL_STENCIL_BUFFER_BIT);
        ourModel.draw_point_shadow(pointShader, singleColorShader, projection, view, viewPos, lightPos, floorTexture, depthCubemap, colorBuffers[0], invisible, point_far_plane, stencil, true);;

        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // Step 3. Blur. Use the hdrFBO, which contain normal scene and bloom part, to create the blur effect.

        // Use ping-pong skill to blur the bloom part of the FBO(using gauss blur)
        bool horizontal = true, first_iteration = true;
        if (bloom)
        {
            unsigned int amount = 14;
            blurShader.use();
            for (unsigned int i = 0; i < amount; i++)
            {
                glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[horizontal]);
                blurShader.setInt("horizontal", horizontal);
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, first_iteration ? colorBuffers[1] : pingpongColorbuffers[!horizontal]);  // bind texture of other framebuffer (or scene if first iteration)
                renderQuad();
                horizontal = !horizontal;
                if (first_iteration)
                    first_iteration = false;
            }
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }


        // Step 4. Render the blurred scene onto the screen.
        glClear(GL_COLOR_BUFFER_BIT);
        bloomShader.use();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, colorBuffers[0]);
        glActiveTexture(GL_TEXTURE1);
        if (bloom)
        {
            glBindTexture(GL_TEXTURE_2D, pingpongColorbuffers[!horizontal]);
        }
        else
        {
            glBindTexture(GL_TEXTURE_2D, colorBuffers[1]);
        }
        renderQuad();

        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();
    return 0;
}


// Render the recorded texture/framebuffer.
unsigned int quadVAO = 0;
unsigned int quadVBO;
void renderQuad()
{
    if (quadVAO == 0)
    {
        float quadVertices[] = {
            // positions        // texture Coords
            -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
            -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
             1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
             1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
        };
        // setup plane VAO
        glGenVertexArrays(1, &quadVAO);
        glGenBuffers(1, &quadVBO);
        glBindVertexArray(quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    }
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow* window)
{
    // rotate by trackball
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    
    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS && timer > updateTimeMax)
    {
        timer = 0.0f;
        pointLight.updatePhi(deltaAngle);
    }
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS && timer > updateTimeMax)
    {
        timer = 0.0f;
        pointLight.updatePhi(-deltaAngle);
    }
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS && timer > updateTimeMax)
    {
        timer = 0.0f;
        pointLight.updateTheta(deltaAngle);
    }
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS && timer > updateTimeMax)
    {
        timer = 0.0f;
        pointLight.updateTheta(-deltaAngle);
    }
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS && timer > updateTimeMax)
    {
        timer = 0.0f;
        camera.ProcessKeyboard(FORWARD, deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS && timer > updateTimeMax)
    {
        timer = 0.0f;
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS && timer > updateTimeMax)
    {
        timer = 0.0f;
        camera.ProcessKeyboard(LEFT, deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS && timer > updateTimeMax)
    {
        timer = 0.0f;
        camera.ProcessKeyboard(RIGHT, deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS && timer > buttonTimeMax)
    {
        timer = 0.0f;
        toon = !toon;
        std::cout << "Shading type: " << (toon ? "Toon" : "Blinn-phong") << std::endl;
    }
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS && timer > buttonTimeMax)
    {
        timer = 0.0f;
        stencil = !stencil;
        std::cout << "Frame: "<< (stencil ? "On" : "Off") << std::endl;
    }
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS && timer > buttonTimeMax)
    {
        timer = 0.0f;
        bloom = !bloom;
        std::cout << "Blur: "<< (bloom ? "On" : "Off") << std::endl;
    }
    if (glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS && timer > buttonTimeMax)
    {
        timer = 0.0f;
        invisible = invisible > 0.1f ? 0.0f : 0.85f;
        std::cout << "Invisible: " << (invisible > 0.0f ? "On" : "Off") << std::endl;
    }
}


void mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

    lastX = xpos;
    lastY = ypos;

    camera.ProcessMouseMovement(xoffset, yoffset);
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and 
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

myTexture2D loadTextureFromFile(const char* file, bool alpha)
{
    // create texture 
    myTexture2D texture;

    // change format if the image has the alpha channel
    if (alpha)
    {
        texture.internalFormat = GL_RGBA;
        texture.imageFormat = GL_RGBA;
    }

    // load image
    int width, height, numChannels;
    stbi_set_flip_vertically_on_load(true); // preventing from upside down
    unsigned char* data = stbi_load(file, &width, &height, &numChannels, 0);

    // error check
    if (!data) std::cout << "load fail, file: " << file << std::endl;

    // generate texture
    texture.generate(width, height, data);

    // free image data
    stbi_image_free(data);
    return texture;
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT && action != mouseState)
    {
        mouseState = action;
        if (action == GLFW_PRESS)
        {
            trackball.refreshScreenCoord(mousePosX, mousePosY);
        }
    }
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera.ProcessMouseScroll(static_cast<float>(yoffset));
}