#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <string>
#include <ctime>
#include <cstdlib>
#include "Model.h"
#include "Camera.h"
#include "Shader.h"

using namespace std;

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);

// settings
const unsigned int SCR_WIDTH = 1800;
const unsigned int SCR_HEIGHT = 1000;

// camera
Camera camera(glm::vec3(0.0f, 0.0f, 10.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;
bool paused;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
        camera.ProcessKeyboard(UP, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
        camera.ProcessKeyboard(DOWN, deltaTime);
};

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
};

// glfw: whenever the mouse moves, this callback is called
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
};

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera.ProcessMouseScroll(static_cast<float>(yoffset));
    // Adjust FOV based on mouse scroll
    float fov = camera.Zoom;
    fov -= static_cast<float>(yoffset);
    if (fov < 1.0f)
        fov = 1.0f;
    if (fov > 50.0f)
        fov = 50.0f;

    camera.Zoom = fov;
};

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    // Check if the 'P' key is pressed to toggle pause
    if (key == GLFW_KEY_P && action == GLFW_PRESS) {
        paused = !paused;
    }
};
const int numStars = 3500;  // Adjust the number of stars as needed

float vertices[numStars * 6];  // Each star has x, y, z, r, g, b

const char* vertexShaderSource = R"(
    #version 330 core
    layout(location = 0) in vec3 aPosition;
    layout(location = 1) in vec3 aColor;

    uniform mat4 projection;
    uniform mat4 view;

    out vec3 fragColor;

    void main() {
        gl_Position = projection * view * vec4(aPosition, 1.0);
        fragColor = aColor;
    }
)";

const char* fragmentShaderSource = R"(
    #version 330 core
    out vec4 FragColor;

    in vec3 fragColor;

    void main() {
        FragColor = vec4(fragColor, 1.0);
    }
)";

// method to set up texture. dark flag = 0 id daytime, 1 is night time and 2 is bump file for moon
void loadTexture(unsigned int texture, const char* path1, int darkFlag, Shader ourShader) {
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    // set the texture parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // load image, create texture and generate mipmaps
    int width, height, nrChannels;
    stbi_set_flip_vertically_on_load(true);

    unsigned char* data = stbi_load(path1, &width, &height, &nrChannels, 0);
    if (data)
    {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else
    {
        std::cout << "Failed to load texture at path: " << path1 << std::endl;
        std::cout << "Error: " << stbi_failure_reason() << std::endl;
        return;  // Return without setting uniforms if texture loading fails
    }

    stbi_image_free(data);

    ourShader.use();
    glActiveTexture(GL_TEXTURE0 + darkFlag);
    glBindTexture(GL_TEXTURE_2D, texture);

    // day/night effect
    if (darkFlag == 1) {
        glUniform1i(glGetUniformLocation(ourShader.ID, "darktextureSampler"), darkFlag);
    }
    else if (darkFlag == 0) {
        glUniform1i(glGetUniformLocation(ourShader.ID, "textureSampler"), darkFlag);
    }
    else {
        glUniform1i(glGetUniformLocation(ourShader.ID, "bumpSampler"), darkFlag); // bump.png for moon
    }
};

void renderModel(Shader& ourShader, Model& ourModel, const glm::vec3& translation, const glm::vec3& scale, const glm::vec3& rotationAxis, float rotationAngle)
{
    // Render the loaded model
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, translation);
    model = glm::rotate(model, rotationAngle, rotationAxis);
    model = glm::scale(model, scale);
    ourShader.setMat4("model", model);
    ourModel.Draw(ourShader);
};

int main()
{
    // glfw: initialize and configure
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // glfw window creation
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Planet System", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetKeyCallback(window, keyCallback);

    // tell GLFW to capture our mouse
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    stbi_set_flip_vertically_on_load(false);

    // configure global opengl state
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    // build and compile shaders
    Shader ourShader("res/shaders/vertexShader.glsl", "res/shaders/fragmentShader.glsl");

    // load models
    Model sunModel("res/planetModels/sun/sun.obj");
    Model earthModel("res/planetModels/earth/Earth.obj");
    Model moonModel("res/planetModels/moon/Moon.obj");

    // create textures
    unsigned int sunTexture=0, earthDayTexture=0, earthNightTexture=0, moonDayTexture=0, moonNightTexture=0, moonBumpTexture=0;
    loadTexture(sunTexture, "res/planetModels/sun/sun.jpg", 0, ourShader);
    loadTexture(earthDayTexture, "res/planetModels/earth/Diffuse.png", 0, ourShader);
    loadTexture(earthNightTexture, "res/planetModels/earth/Bump.png", 1, ourShader);
    loadTexture(moonDayTexture, "res/planetModels/moon/Diffuse.png", 0, ourShader);
    loadTexture(moonNightTexture, "res/planetModels/moon/Diffuse.png", 1, ourShader);
    //loadTexture(moonBumpTexture, "res/planetModels/moon/Bump.png", 2, ourShader);

    //random generate stars
    for (int i = 0; i < numStars; ++i) {
        float x = static_cast<float>(rand()) / RAND_MAX * 6000.0f - 3500.0f;
        float y = static_cast<float>(rand()) / RAND_MAX * 6000.0f - 3500.0f;
        float z = static_cast<float>(rand()) / RAND_MAX * 6000.0f - 3500.0f;

        // stars are white
        float r = 255;
        float g = 255;
        float b = 255;

        // Store the coordinates and color in the vertices array
        vertices[i * 6] = x;
        vertices[i * 6 + 1] = y;
        vertices[i * 6 + 2] = z;
        vertices[i * 6 + 3] = r;
        vertices[i * 6 + 4] = g;
        vertices[i * 6 + 5] = b;
    }

    // Vertex Array Object (VAO) and Vertex Buffer Object (VBO) setup
    GLuint vao, vbo;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    // give star data to the VBO
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // Specify the attribute pointers for vertex positions and colors
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // orbit and speed values of earth and moon
    float earthOrbitRadius = 5.0f;
    float earthOrbitSpeed = 0.1f;
    float moonOrbitRadius = 1.5f;
    float moonOrbitSpeed = 0.3f;

    glm::vec3 earthLastPosition;
    float earthLastRotationAngle;
    float earthLastOrbitAngle;
    camera.Zoom = 50.0f;
    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Set up view and projection transformations 
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = camera.GetViewMatrix();

        //render stars
        glBindVertexArray(vao);
        glDrawArrays(GL_POINTS, 0, numStars);
        glBindVertexArray(0);

        // setup shader for celestial objects
        ourShader.use();
        ourShader.setMat4("projection", projection);
        ourShader.setMat4("view", view);
        ourShader.setVec3("lightPos", glm::vec3(1.0f, 1.0f, 1.0f));
        ourShader.setVec3("lightColor", glm::vec3(1.0f, 1.0f, 1.0f));
        ourShader.setFloat("ambientStrength", 0.5f);
        ourShader.setFloat("diffuseStrength", 0.9f);

        // Sun
        glm::vec3 translationSun(0.0f, 0.0f, 0.0f);
        glm::vec3 scaleSun(1.0f);

        //// Earth
        float earthOrbitAngle = static_cast<float>(glfwGetTime()) * earthOrbitSpeed;
        float earthPosX = cos(earthOrbitAngle) * earthOrbitRadius;
        float earthPosZ = sin(earthOrbitAngle) * earthOrbitRadius;

        glm::vec3 translationEarth(earthPosX, 0.0f, earthPosZ);
        glm::vec3 scaleEarth(0.2f);

        // // Moon
        float moonOrbitAngle = static_cast<float>(glfwGetTime()) * moonOrbitSpeed;
        float moonPosX = cos(moonOrbitAngle) * moonOrbitRadius + earthPosX;
        float moonPosZ = sin(moonOrbitAngle) * moonOrbitRadius + earthPosZ;

        glm::vec3 translationMoon(moonPosX, 0.0f, moonPosZ);
        glm::vec3 scaleMoon(0.05f);
        glm::vec3 earthRotationAxis(0.0f, 1.0f, 0.0f);
        glm::vec3 sunRotationAxis(0.0f, 1.0f, 0.0f);
        glm::vec3 moonRotationAxis(0.0f, 1.0f, 0.0f);

        float earthRotationAngle, sunRotationAngle, moonRotationAngle;
        // Update orbit angle when not paused
        earthOrbitAngle += earthOrbitSpeed * deltaTime;

        // Calculate new position based on the updated orbit angle
        earthPosX = cos(earthOrbitAngle) * earthOrbitRadius;
        earthPosZ = sin(earthOrbitAngle) * earthOrbitRadius;

        // code for when pausing
        if (!paused) {
            earthOrbitSpeed = 0.1f;
            moonOrbitSpeed = 0.3f;
        }
        else {
            // stop moving
            earthOrbitSpeed = 0.0f;
            moonOrbitSpeed = 0.0f;

            // If paused, store the current state
            earthLastPosition = glm::vec3(earthPosX, 0.0f, earthPosZ);
            earthLastOrbitAngle = earthOrbitAngle;
        }

        earthRotationAngle = static_cast<float>(glfwGetTime()) * 0.5f;

        sunRotationAngle = static_cast<float>(glfwGetTime()) * 0.1f;

        moonRotationAngle = static_cast<float>(glfwGetTime()) * 0.8f;

        // render celestial objects. planet = 1 for sun, 2 for earth, 3 for moon
        // based on the planet the shader behaves differently
        ourShader.setInt("planet", 1);
        renderModel(ourShader, sunModel, translationSun, scaleSun, sunRotationAxis, sunRotationAngle);
        ourShader.setInt("planet", 2);
        renderModel(ourShader, earthModel, translationEarth, scaleEarth, earthRotationAxis, earthRotationAngle);

        // Calculate Moon's rotation by adding Earth's rotation
        moonRotationAngle += earthRotationAngle;
        ourShader.setInt("planet", 3);
        // Draw Moon
        renderModel(ourShader, moonModel, translationMoon, scaleMoon, moonRotationAxis, moonRotationAngle);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
    glfwTerminate();
    return 0;
}

