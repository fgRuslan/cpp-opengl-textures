#include <GLXW/glxw.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/transform.hpp>
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>
#include <defer.h>
#include <chrono>
#include "utils/utils.h"
#include "utils/camera.h"

static const GLfloat globVertexBufferData[] = {
  -1.0f, -1.0f,  0.0f,
   1.0f, -1.0f,  0.0f,
   0.0f,  1.0f,  0.0f,
};

void windowSizeCallback(GLFWwindow *, int width, int height) {
  glViewport(0, 0, width, height);
}

// TODO: box instead of triangle
// TODO: rename project in CMakeList.txt

int main() {
  if(glfwInit() == GL_FALSE) {
    std::cerr << "Failed to initialize GLFW" << std::endl;
    return -1;
  }
  defer(std::cout << "Calling glfwTerminate()" << std::endl; glfwTerminate());

  glfwDefaultWindowHints();

  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);

  GLFWwindow* window = glfwCreateWindow(800, 600, "Triangle", nullptr, nullptr);
  if(window == nullptr) {
    std::cerr << "Failed to open GLFW window" << std::endl;
    return -1;
  }
  defer(std::cout << "Calling glfwDestroyWindow()" << std::endl; glfwDestroyWindow(window));

  glfwMakeContextCurrent(window);

  if(glxwInit()) {
    std::cerr << "Failed to init GLXW" << std::endl;
    return -1;
  }

  glfwSwapInterval(1);

  glfwSetWindowSizeCallback(window, windowSizeCallback);

  glfwShowWindow(window);

  glEnable(GL_DEPTH_TEST | GL_DOUBLEBUFFER | GL_CULL_FACE); // TODO: describe GL_CULL_FACE!
  glDepthFunc(GL_LESS);

  glClearColor(0, 0, 0, 1);

  bool errorFlag = false;
  std::vector<GLuint> shaders;

  GLuint vertexShaderId = loadShader("shaders/vertexShader.glsl", GL_VERTEX_SHADER, &errorFlag);
  if(errorFlag) {
    std::cerr << "Failed to load vertex shader (invalid working directory?)" << std::endl;
    return -1;
  }

  shaders.push_back(vertexShaderId);

  GLuint fragmentShaderId = loadShader("shaders/fragmentShader.glsl", GL_FRAGMENT_SHADER, &errorFlag);
  if(errorFlag) {
    std::cerr << "Failed to load fragment shader (invalid working directory?)" << std::endl;
    return -1;
  }
  shaders.push_back(fragmentShaderId);

  GLuint programId = prepareProgram(shaders, &errorFlag);
  if(errorFlag) {
    std::cerr << "Failed to prepare program" << std::endl;
    return -1;
  }
  glDeleteShader(vertexShaderId);
  glDeleteShader(fragmentShaderId);

  GLuint vbo;
  glGenBuffers(1, &vbo);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(globVertexBufferData), globVertexBufferData, GL_STATIC_DRAW);

  GLuint vao;
  glGenVertexArrays(1, &vao);

  glBindVertexArray(vao);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
  glBindBuffer(GL_ARRAY_BUFFER, 0); // unbind VBO
  glBindVertexArray(0); // unbind VAO

  glm::mat4 projection = glm::perspective(90.0f, 4.0f / 3.0f, 0.3f, 100.0f);

  GLint matrixId = glGetUniformLocation(programId, "MVP");

  auto startTime = std::chrono::high_resolution_clock::now();
  auto prevTime = startTime;

  // hide cursor
  glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

  Camera camera(window, glm::vec3(0, 0, 5), 3.14f /* toward -Z */, 0.0f /* look at the horizon */);

  while(glfwWindowShouldClose(window) == GL_FALSE) {
    if(glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) break;

    auto currentTime = std::chrono::high_resolution_clock::now();
    float startDeltaTimeMs = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - startTime).count();
    float prevDeltaTimeMs = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - prevTime).count();
    prevTime = currentTime;

    float rotationTimeMs = 3000.0f;
    float currentRotation = startDeltaTimeMs / rotationTimeMs;
    float angle = 360.0f*(currentRotation - (long)currentRotation);

    glm::mat4 view;
    camera.getViewMatrix(prevDeltaTimeMs, &view);

    glm::mat4 model = glm::rotate(angle, 0.0f, 1.0f, 0.0f);
    glm::mat4 mvp = projection * view * model; // matrix multiplication is the other way around
    glUniformMatrix4fv(matrixId, 1, GL_FALSE, &mvp[0][0]);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(programId);

    glBindVertexArray(vao);
    glEnableVertexAttribArray(0);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glDisableVertexAttribArray(0);
    
    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  glDeleteVertexArrays(1, &vao);
  glDeleteBuffers(1, &vbo);
  glDeleteProgram(programId);

  return 0;
}
