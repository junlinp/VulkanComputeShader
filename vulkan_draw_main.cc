#define GLFW_INCLUDE_VULKAN
#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <vulkan/vulkan.h>
#include <vector>

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

class HelloTriangleApplication {
public:
    void run() {
        initVulkan();
        mainLoop();
        cleanup();
    }

private:
    VkInstance instance_;
    void initVulkan() {
        createInstance();
    }

    void mainLoop() {
    }

    void cleanup() {
        // vkDestroyInstance(instance_, nullptr);
    }

    void createInstance() {
        VkApplicationInfo appInfo;
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "Hello World!";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "No Engine";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_0;

        VkInstanceCreateInfo createInfo{};
        createInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;
        std::vector<const char*> requiredExtensions;
        requiredExtensions.emplace_back(
            VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
        createInfo.enabledExtensionCount = (uint32_t)requiredExtensions.size();
        createInfo.ppEnabledExtensionNames = requiredExtensions.data();

        std::cout << "CreateInstance " << std::endl;
        VkResult res = vkCreateInstance(&createInfo, nullptr, &instance_);
        if (res != VK_SUCCESS) {
            std::cerr << "Error code : " << res << std::endl;
            throw std::runtime_error("failed to create instance!");
        }

        uint32_t physical_device_count = 0;
        std::cout << "here" << std::endl;
        vkEnumeratePhysicalDevices(instance_, &physical_device_count, nullptr);
        std::cout << physical_device_count << std::endl;
    }
};

int main() {
    HelloTriangleApplication app;

    try {
        std::cout << "he" << std::endl;
        app.run();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}