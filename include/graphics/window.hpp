#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

// std
#include <string>

namespace hnll {

class HveWindow
{
  public:
    HveWindow(const int w, const int h, const std::string name);
    ~HveWindow();

    // delete copy ctor, assignment (for preventing GLFWwindow* from double deleted)
    HveWindow(const HveWindow &) = delete;
    HveWindow& operator= (const HveWindow &) = delete;

    inline bool shouldClose() { return glfwWindowShouldClose(window_); }
    VkExtent2D getExtent() { return { static_cast<uint32_t>(width_m), static_cast<uint32_t>(height_m)}; }
    bool wasWindowResized() { return framebufferResized_m; }
    void resetWindowResizedFlag() { framebufferResized_m = false; }
    GLFWwindow* getGLFWwindow() const { return window_; }
      
    void createWindowSurface(VkInstance instance, VkSurfaceKHR* surface);
    GLFWwindow* window() { return window_; }
      
  private:
    static void framebufferResizeCallBack(GLFWwindow *window, int width, int height);
    void initWindow();

    int width_m;
    int height_m;
    bool framebufferResized_m = false;

    std::string windowName_m;
    GLFWwindow *window_;
};
    
} // namespace hv