
#ifdef WIN32
#pragma warning(push)
#pragma warning(disable : 26812)
#define GLFW_EXPOSE_NATIVE_WIN32
#define VK_USE_PLATFORM_WIN32_KHR
#else
#define GLFW_EXPOSE_NATIVE_X11
#define VK_USE_PLATFORM_XLIB_KHR
#endif

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <cassert>
#include <vector>
#include <optional>
#include <set>

namespace VaryZulu::Gfx
{
constexpr int MAX_FRAMES_IN_FLIGHT = 2;

struct QueueFamilyIndices
{
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    bool isComplete() const
    {
        if (!graphicsFamily.has_value()) {
            return false;
        }
        if (!presentFamily.has_value()) {
            return false;
        }
        return true;
    }
};

struct SwapChainSupportDetails
{
    VkSurfaceCapabilitiesKHR capabilities{};
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

class Renderer
{
public:
    void run();

private:
    void initWindow();

    static void framebufferResizeCallback(GLFWwindow* window, int, int);

    void initVulkan();
    void cleanupSwapChain();
    void recreateSwapChain();
    void createSyncObjects();
    void createCommandBuffers();
    void createCommandPool();
    void createFrameBuffers();
    void createRenderPass();
    VkShaderModule createShaderModule(const std::vector<char>& code);
    void createGraphicsPipeline();
    void createImageViews();
    void createSwapChain();
    void createSurface();
    void createLogicalDevice();
    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice d);
    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice d);
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
    VkPresentModeKHR chooseSwapPresentMode(
        const std::vector<VkPresentModeKHR>& availablePresentModes);
    VkSurfaceFormatKHR chooseSwapSurfaceFormat(
        const std::vector<VkSurfaceFormatKHR>& availableFormats);
    bool isDeviceSuitable(VkPhysicalDevice d);
    bool checkDeviceExtensionSupport(VkPhysicalDevice d);
    void pickPhysicalDevice();
    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
    void setupDebugMessenger();
    void cleanupDebugMessenger();
    std::vector<const char*> getRequiredExtensions();
    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void*);
    void createInstance();
    void mainLoop();
    bool drawFrame();
    void cleanup();
    void checkValidationLayerSupport();

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
    const std::vector<const char*> deviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
    VkInstance instance = nullptr;
    VkDebugUtilsMessengerEXT debugMessenger = nullptr;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device = nullptr;
    VkQueue graphicsQueue = nullptr;
    VkQueue presentQueue = nullptr;
    VkSurfaceKHR surface = nullptr;
    VkSwapchainKHR swapChain = nullptr;
    std::vector<VkImage> swapChainImages;
    std::vector<VkImageView> swapChainImageViews;
    std::vector<VkFramebuffer> swapChainFramebuffers;
    std::vector<VkCommandBuffer> commandBuffers;
    VkFormat swapChainImageFormat = VK_FORMAT_UNDEFINED;
    VkExtent2D swapChainExtent{};
    VkRenderPass renderPass = nullptr;
    VkPipelineLayout pipelineLayout = nullptr;
    VkPipeline graphicsPipeline = nullptr;
    VkCommandPool commandPool = nullptr;
    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;
    std::vector<VkFence> inFlightImages;
    size_t currentFrame = 0;
    bool framebufferResized = false;
};

} // namespace VaryZulu::Gfx
