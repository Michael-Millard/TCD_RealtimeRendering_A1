#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <my_shader.h>
#include <my_camera.h>
#include <my_model.h>

#include <iostream>
#include <random>
#define _USE_MATH_DEFINES
#include <math.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// Callback function declarations
void frameBufferSizeCallback(GLFWwindow* window, int width, int height);
void mouseCallback(GLFWwindow* window, double xIn, double yIn);
void scrollCallback(GLFWwindow* window, double xOff, double yOff);
void processUserInput(GLFWwindow* window);

// Global params
unsigned int SCREEN_WIDTH = 1920;
unsigned int SCREEN_HEIGHT = 1080;
bool firstMouse = true;

float yaw = -90.0f;	// yaw is initialized to -90.0 degrees since a yaw of 0.0 results in a direction vector pointing to the right so we initially rotate to the left
float pitch = 0.0f;
float xPrev = static_cast<float>(SCREEN_WIDTH) / 2.0f;
float yPrev = static_cast<float>(SCREEN_HEIGHT) / 2.0f;

// Timing params
float deltaTime = 0.0f;	// time between current frame and previous frame
float prevFrame = 0.0f;

// Enum for different lighting methods
enum
{
    LambertianModel = 0,
    PhongModel = 1,
    CookTorranceModel = 2,
    BlinnModel = 3
};

// For random placement of models
float generateRandomNumInRange(float low, float high)
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dis(low, high);

    return dis(gen);
}

// 3D model name
#define TEAPOT // If using teapot model

#ifdef TEAPOT
#define MODEL "models/teapot_smooth.obj"
const float zPosInit = 14.0f; // For teapot
#else
#define MODEL "models/airplane.obj"
const float zPosInit = 18.0f; // For airplane
#endif

// Camera specs (set later, can't call functions here)
const float cameraSpeed = 3.0f;
const float mouseSensitivity = 0.1f;
const float cameraZoom = 50.0f;
const float xPosInit = -2.0f;
const float yPosInit = 0.0f;
Camera camera(glm::vec3(xPosInit, yPosInit, zPosInit));

// Main function
int main()
{
    // glfw init and configure
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_DECORATED, NULL); // Remove title bar

    // Screen params
    GLFWmonitor* MyMonitor = glfwGetPrimaryMonitor(); 
    const GLFWvidmode* mode = glfwGetVideoMode(MyMonitor);
    SCREEN_WIDTH = mode->width; SCREEN_HEIGHT = mode->height;

    // glfw window creation
    GLFWwindow* window = glfwCreateWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Realtime Rendering Assign1", glfwGetPrimaryMonitor(), nullptr);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    // Callback functions
    glfwSetFramebufferSizeCallback(window, frameBufferSizeCallback);
    //glfwSetCursorPosCallback(window, mouseCallback);
    glfwSetScrollCallback(window, scrollCallback);

    // Mouse capture
    //glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED); // If moving mouse with camera
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL); // If moving mouse with IMGUI

    // Load all OpenGL function pointers with GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // Configure global OpenGL state
    glEnable(GL_DEPTH_TEST);    // Depth-testing
    glDepthFunc(GL_LESS);       // Smaller value as "closer" for depth-testing

    // Build and compile shaders
    Shader shader("shaders/vertexShader.vs", "shaders/fragmentShader.fs");

    // Load models
    Model planeModel(MODEL);

    // Fine tune camera params
    camera.setMouseSensitivity(mouseSensitivity);
    camera.setCameraMovementSpeed(cameraSpeed);
    camera.setZoom(cameraZoom);
    camera.setFPSCamera(false, yPosInit);
    camera.setZoomEnabled(false);

    // IMGUI setup
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    // Set font
    io.Fonts->Clear();
    ImFont* myFont = io.Fonts->AddFontFromFileTTF("C:\\fonts\\Open_Sans\\static\\OpenSans_Condensed-Regular.ttf", 30.0f); // Adjust size here

    // Rebuild the font atlas
    unsigned char* pixels;
    int width, height;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

    // Render loop
    float elapsedTime = 0.0f;
    float rotY = 0.0f;
    float rotZ = 0.0f;
    float roughness = 0.25f;
    float specularExponent = 32.0f;
    float fresnelReflectance = 0.1f;
#ifdef TEAPOT
    float distApart = 3.5f; // For teapot
    glm::vec3 modelOriginOffset(0.0f, 1.5f, 0.0f); // For teapot (positive because subtracted later)
#else
    float distApart = 5.0f; // For airplane
    glm::vec3 modelOriginOffset(0.0f, 0.8f, 0.0f); // For airplane (positive because subtracted later)
#endif
    float ambientFloat = 0.1f;
    float lightOffsetFloat = 2.0f;
    glm::vec3 ambientLight(ambientFloat, ambientFloat, ambientFloat);
    glm::vec3 lightOffset(lightOffsetFloat, lightOffsetFloat, lightOffsetFloat);
    glm::vec3 lightColour(1.0f, 1.0f, 1.0f);
    glm::vec3 objectColour(0.3f, 0.6f, 0.8f);   
    while (!glfwWindowShouldClose(window))
    {
        // Per-frame time logic
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - prevFrame;
        elapsedTime += deltaTime;
        prevFrame = currentFrame;

        // User input handling
        processUserInput(window);

        // Clear screen colour and buffers
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // IMGUI window
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Set updated IMGUI params
        ambientLight = glm::vec3(ambientFloat, ambientFloat, ambientFloat);
        lightOffset = glm::vec3(lightOffsetFloat, lightOffsetFloat, lightOffsetFloat);

        // Enable shader before setting uniforms
        shader.use();
        shader.setVec3("ambient", ambientLight);
        shader.setFloat("roughness", roughness);
        shader.setFloat("specularExponent", specularExponent);
        shader.setFloat("fresnelReflectance", fresnelReflectance);
        shader.setVec3("lightColour", lightColour);
        shader.setVec3("objectColour", objectColour);

        // Camera position
        shader.setVec3("viewPos", camera.position);

        // Model, View & Projection transformations, set uniforms in shader
        glm::mat4 view = camera.getViewMatrix();
        shader.setMat4("view", view);
        glm::mat4 projection = glm::perspective(glm::radians(camera.zoom), static_cast<float>(SCREEN_WIDTH) / static_cast<float>(SCREEN_HEIGHT), 0.1f, 1000.0f);
        shader.setMat4("projection", projection);

        // Rotate the model slowly around the y axis at 20 degrees per second
        rotY += 20.0f * deltaTime;
        rotY = fmodf(rotY, 360.0f);
        // Rotate the propeller around the z axis at 360 degrees per second
        rotZ += 360.0f * deltaTime;
        rotZ = fmodf(rotZ, 360.0f);

        // First plane (TL): Lambertian
        shader.setInt("lightingID", LambertianModel);
        glm::vec3 planePosition = glm::vec3(-distApart, distApart, 0.0f) - modelOriginOffset;
        shader.setVec3("lightPos", planePosition + lightOffset);
        glm::mat4 model = glm::identity<glm::mat4>();
        model = glm::translate(model, planePosition);
        model = glm::rotate(model, glm::radians(rotY), glm::vec3(0.0f, 1.0f, 0.0f));
        shader.setMat4("model", model);
        planeModel.drawHierarchy(shader, model, rotZ);

        // Second plane (TR): Phong
        shader.setInt("lightingID", PhongModel);
        planePosition = glm::vec3(distApart, distApart, 0.0f) - modelOriginOffset;
        shader.setVec3("lightPos", planePosition + lightOffset);
        model = glm::identity<glm::mat4>();
        model = glm::translate(model, planePosition);
        model = glm::rotate(model, glm::radians(rotY), glm::vec3(0.0f, 1.0f, 0.0f));
        shader.setMat4("model", model);
        planeModel.drawHierarchy(shader, model, rotZ);

        // Third plane (BL): Cook-Torrance
        shader.setInt("lightingID", CookTorranceModel);
        planePosition = glm::vec3(-distApart, -distApart, 0.0f) - modelOriginOffset;
        shader.setVec3("lightPos", planePosition + lightOffset);
        model = glm::identity<glm::mat4>();
        model = glm::translate(model, planePosition);
        model = glm::rotate(model, glm::radians(rotY), glm::vec3(0.0f, 1.0f, 0.0f));
        shader.setMat4("model", model);
        planeModel.drawHierarchy(shader, model, rotZ);

        // Fourth plane (BR): Blinn
        shader.setInt("lightingID", BlinnModel);
        planePosition = glm::vec3(distApart, -distApart, 0.0f) - modelOriginOffset;
        shader.setVec3("lightPos", planePosition + lightOffset);
        model = glm::identity<glm::mat4>();
        model = glm::translate(model, planePosition);
        model = glm::rotate(model, glm::radians(rotY), glm::vec3(0.0f, 1.0f, 0.0f));
        shader.setMat4("model", model);
        planeModel.drawHierarchy(shader, model, rotZ);

        // IMGUI drawing
        ImGui::SetNextWindowSize(ImVec2(500, 300));
        ImGui::Begin("IMGUI");
        ImGui::Text("Adjust Parameter Sliders:");
        ImGui::SliderFloat("Specular Exponent", &specularExponent, 2.0f, 128.0f);
        ImGui::SliderFloat("Roughness", &roughness, 0.01f, 1.0f);
        ImGui::SliderFloat("Fresnal Reflectance", &fresnelReflectance, 0.01f, 1.0f);
        ImGui::SliderFloat("Ambient light", &ambientFloat, 0.01f, 1.0f);
        ImGui::SliderFloat("Light Offset", &lightOffsetFloat, 1.0f, 10.0f);
        ImGui::End();

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // Swap buffers and poll events
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Shutdown procedure
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    // Destroy window
    glfwDestroyWindow(window);

    // Terminate and return success
    glfwTerminate();
    return 0;
}

// Process keyboard inputs
void processUserInput(GLFWwindow* window)
{
    // Escape to exit
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    // WASD to move, parse to camera processing commands
    // Positional constraints implemented in camera class
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.processKeyboardInput(GLFW_KEY_W, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.processKeyboardInput(GLFW_KEY_A, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.processKeyboardInput(GLFW_KEY_S, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.processKeyboardInput(GLFW_KEY_D, deltaTime);

    // QE for up/down
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
        camera.processKeyboardInput(GLFW_KEY_Q, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
        camera.processKeyboardInput(GLFW_KEY_E, deltaTime);
}

// Window size change callback
void frameBufferSizeCallback(GLFWwindow* window, int width, int height)
{
    // Ensure viewport matches new window dimensions
    glViewport(0, 0, width, height);

    // Adjust screen width and height params that set the aspect ratio in the projection matrix
    SCREEN_WIDTH = width;
    SCREEN_HEIGHT = height;
}

// Mouse input callback
void mouseCallback(GLFWwindow* window, double xIn, double yIn)
{
    // Cast doubles to floats
    float x = static_cast<float>(xIn);
    float y = static_cast<float>(yIn);

    // Check if first time this callback function is being used, set last variables if so
    if (firstMouse)
    {
        xPrev = x;
        yPrev = y;
        firstMouse = false;
    }

    // Compute offsets relative to last positions
    float xOff = x - xPrev;
    // Reverse since y-coordinates are inverted (bottom to top)
    float yOff = yPrev - y; 
    xPrev = x; yPrev = y;

    // Tell camera to process new mouse offsets
    camera.processMouseMovement(xOff, yOff);
}

// Mouse scroll wheel input callback - camera zoom must be enabled for this to work
void scrollCallback(GLFWwindow* window, double xOff, double yOff)
{
    // Tell camera to process new y-offset from mouse scroll whell
    camera.processMouseScroll(static_cast<float>(yOff));
}