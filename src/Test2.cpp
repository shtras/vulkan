#pragma warning(disable : 26812)
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <cassert>
#include <vector>
#include <optional>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

struct QueueFamilyIndices
{
    std::optional<uint32_t> graphicsFamily;

    bool isComplete() const
    {
        return graphicsFamily.has_value();
    }
};

QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device)
{
    QueueFamilyIndices indices;

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    int i = 0;
    for (const auto& queueFamily : queueFamilies) {
        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            indices.graphicsFamily = i;
        }
        i++;
    }

    return indices;
}

class HelloTriangleApplication
{
public:
    void run()
    {
        initWindow();
        initVulkan();
        mainLoop();
        cleanup();
    }

private:
    void initWindow()
    {
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

        window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
    }

    void initVulkan()
    {
        createInstance();
        setupDebugMessenger();
        pickPhysicalDevice();
        createLogicalDevice();
    }

    void createLogicalDevice()
    {
        QueueFamilyIndices indices = findQueueFamilies(physicalDevice);

        float queuePriority = 1.0f;
        VkDeviceQueueCreateInfo queueCreateInfo{.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .queueFamilyIndex = indices.graphicsFamily.value(),
            .queueCount = 1,
            .pQueuePriorities = &queuePriority};

        VkPhysicalDeviceFeatures deviceFeatures{};

        VkDeviceCreateInfo createInfo{.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
            .queueCreateInfoCount = 1,
            .pQueueCreateInfos = &queueCreateInfo,
            .pEnabledFeatures = &deviceFeatures};
        if (enableValidationLayers) {
            // Deprecated. Backwards compatibility
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();
        }

        auto res = vkCreateDevice(physicalDevice, &createInfo, nullptr, &device);
        if (res != VK_SUCCESS) {
            throw std::runtime_error("Failed creating a logical device");
        }
    }

    bool isDeviceSuitable(VkPhysicalDevice device)
    {
        VkPhysicalDeviceProperties deviceProperties;
        vkGetPhysicalDeviceProperties(device, &deviceProperties);
        spdlog::info("Evaluating {}", deviceProperties.deviceName);

        if (deviceProperties.deviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
            spdlog::info("Not a discrete GPU");
            return false;
        }

        VkPhysicalDeviceFeatures deviceFeatures;
        vkGetPhysicalDeviceFeatures(device, &deviceFeatures);
        if (!deviceFeatures.geometryShader) {
            spdlog::info("Doesn't support geometry shader");
            return false;
        }

        QueueFamilyIndices indices = findQueueFamilies(device);
        if (!indices.isComplete()) {
            spdlog::info("Missing required queues");
            return false;
        }

        spdlog::info("Device matches the requirements");
        return true;
    }

    void pickPhysicalDevice()
    {
        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
        if (deviceCount == 0) {
            throw std::runtime_error("No Vulkan devices found");
        }

        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

        for (const auto& device : devices) {
            if (isDeviceSuitable(device)) {
                physicalDevice = device;
                break;
            }
        }
        if (physicalDevice == VK_NULL_HANDLE) {
            throw std::runtime_error("Failed to select usitable device");
        }
    }

    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
    {
        createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        createInfo.pNext = nullptr;
        createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                     VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                                 VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                                 VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        createInfo.pfnUserCallback = debugCallback;
        createInfo.pUserData = nullptr;
    }

    void setupDebugMessenger()
    {
        if (!enableValidationLayers) {
            return;
        }

        VkDebugUtilsMessengerCreateInfoEXT createInfo{};
        populateDebugMessengerCreateInfo(createInfo);

        auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
            instance, "vkCreateDebugUtilsMessengerEXT");
        if (!func) {
            spdlog::error("Could not find vkCreateDebugUtilsMessengerEXT");
            return;
        }
        auto res = func(instance, &createInfo, nullptr, &debugMessenger);
        if (res != VK_SUCCESS) {
            spdlog::error("Debug messenger creation failed");
        }
    }

    void cleanupDebugMessenger()
    {
        if (debugMessenger) {
            auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
                instance, "vkDestroyDebugUtilsMessengerEXT");
            if (!func) {
                spdlog::error("Failed loading vkDestroyDebugUtilsMessengerEXT");
                return;
            }
            func(instance, debugMessenger, nullptr);
        }
    }

    std::vector<const char*> getRequiredExtensions()
    {
        uint32_t glfwExtensionCount = 0;
        const char** glfwExtensions = nullptr;
        glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
        std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

        if (enableValidationLayers) {
            extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }

        return extensions;
    }

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
    {
        if (messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT) {
            spdlog::debug("Validation: {}", pCallbackData->pMessage);
        } else if (messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) {
            spdlog::info("Validation: {}", pCallbackData->pMessage);
        } else if (messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
            spdlog::warn("Validation: {}", pCallbackData->pMessage);
        } else if (messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
            spdlog::error("Validation: {}", pCallbackData->pMessage);
        } else {
            spdlog::warn("Validation unknown: {} - {}", messageSeverity, pCallbackData->pMessage);
        }
        return VK_FALSE;
    }

    void createInstance()
    {
        if (enableValidationLayers) {
            checkValidationLayerSupport();
        }

        VkApplicationInfo appInfo{.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
            .pNext = nullptr,
            .pApplicationName = "Hello Triangle",
            .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
            .pEngineName = "No Engine",
            .engineVersion = VK_MAKE_VERSION(1, 0, 0),
            .apiVersion = VK_API_VERSION_1_0};

        uint32_t extensionCount = 0;
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
        std::vector<VkExtensionProperties> extensions(extensionCount);
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());
        for (const auto& ext : extensions) {
            spdlog::debug("Existing extension: {}", ext.extensionName);
        }

        auto requiredExtensions = getRequiredExtensions();

        for (const auto& requiredExt : requiredExtensions) {
            spdlog::debug("Required extension: {}", requiredExt);
            std::string extName{requiredExt};
            if (!std::any_of(extensions.begin(), extensions.end(),
                    [&](const auto& ext) { return std::string(ext.extensionName) == extName; })) {
                spdlog::error("Missing required extention: {}", extName);
                throw std::runtime_error("Missing extention");
            }
        }

        VkInstanceCreateInfo createInfo{.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
            .pNext = nullptr,
            .pApplicationInfo = &appInfo,
            .enabledLayerCount = 0,
            .enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size()),
            .ppEnabledExtensionNames = requiredExtensions.data()};

        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
        if (enableValidationLayers) {
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();
            populateDebugMessengerCreateInfo(debugCreateInfo);
            createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
        }

        auto result = vkCreateInstance(&createInfo, nullptr, &instance);
        if (result != VK_SUCCESS) {
            throw std::runtime_error("Failed to create instance!");
        }
    }

    void mainLoop()
    {
        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();
        }
    }

    void cleanup()
    {
        vkDestroyDevice(device, nullptr);

        cleanupDebugMessenger();

        vkDestroyInstance(instance, nullptr);
        glfwDestroyWindow(window);
        glfwTerminate();
    }

    void checkValidationLayerSupport()
    {
        uint32_t layerCount;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

        std::vector<VkLayerProperties> availableLayers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());
        for (const auto& layer : availableLayers) {
            spdlog::debug("Layer: {} --- {}", layer.layerName, layer.description);
        }

        for (const auto& requestedLayer : validationLayers) {
            std::string str(requestedLayer);
            if (!std::any_of(availableLayers.begin(), availableLayers.end(),
                    [&str](const auto& l) { return str == std::string{l.layerName}; })) {
                spdlog::error("Requested layer missing: {}", str);
                throw std::runtime_error("Layer missing");
            }
        }
    }

    GLFWwindow* window = nullptr;
    static constexpr uint32_t WIDTH = 800;
    static constexpr uint32_t HEIGHT = 600;

    const std::vector<const char*> validationLayers = {"VK_LAYER_KHRONOS_validation"};
    static constexpr bool enableValidationLayers =
#ifdef DEBUG
        true
#else
        false
#endif
        ;

    VkInstance instance = nullptr;
    VkDebugUtilsMessengerEXT debugMessenger = nullptr;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device = nullptr;
};

int main()
{
    spdlog::set_default_logger(
        spdlog::stdout_color_mt(std::string("logger"), spdlog::color_mode::always));
    spdlog::set_level(spdlog::level::debug);
    spdlog::info("Hello!");
    HelloTriangleApplication app;

    try {
        app.run();
    } catch (const std::exception& e) {
        spdlog::error("{}", e.what());
        return EXIT_FAILURE;
    }
    spdlog::info("Bye");

    return EXIT_SUCCESS;
}
