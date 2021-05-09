#include "Renderer.h"
#include "Utils/Utils.h"

#include <spdlog/spdlog.h>

#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <cassert>
#include <vector>
#include <optional>
#include <set>

namespace VaryZulu::Gfx
{
#ifdef WIN32
#pragma warning(push)
#pragma warning(disable : 26812)
#endif
void Renderer::run()
{
    initWindow();
    initVulkan();
    mainLoop();
    cleanup();
}

void Renderer::initWindow()
{
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
    glfwSetWindowUserPointer(window, this);
    glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
}

void Renderer::framebufferResizeCallback(GLFWwindow* window, int, int)
{
    auto app = reinterpret_cast<Renderer*>(glfwGetWindowUserPointer(window));
    app->framebufferResized = true;
}

void Renderer::initVulkan()
{
    createInstance();
    setupDebugMessenger();
    createSurface();
    pickPhysicalDevice();
    createLogicalDevice();
    createSwapChain();
    createImageViews();
    createRenderPass();
    createGraphicsPipeline();
    createFrameBuffers();
    createCommandPool();
    createVertexBuffer();
    createCommandBuffers();
    createSyncObjects();
}

void Renderer::cleanupSwapChain()
{
    std::for_each(swapChainFramebuffers.begin(), swapChainFramebuffers.end(),
        [this](auto& buf) { vkDestroyFramebuffer(device, buf, nullptr); });
    swapChainFramebuffers.clear();
    vkFreeCommandBuffers(
        device, commandPool, static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data());
    vkDestroyPipeline(device, graphicsPipeline, nullptr);
    vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
    vkDestroyRenderPass(device, renderPass, nullptr);
    std::for_each(swapChainImageViews.begin(), swapChainImageViews.end(),
        [&](auto& imageView) { vkDestroyImageView(device, imageView, nullptr); });
    swapChainImageViews.clear();
    vkDestroySwapchainKHR(device, swapChain, nullptr);
}

void Renderer::recreateSwapChain()
{
    int width = 0, height = 0;
    glfwGetFramebufferSize(window, &width, &height);
    while (width == 0 || height == 0) {
        glfwGetFramebufferSize(window, &width, &height);
        glfwWaitEvents();
    }
    vkDeviceWaitIdle(device);

    cleanupSwapChain();

    createSwapChain();
    createImageViews();
    createRenderPass();
    createGraphicsPipeline();
    createFrameBuffers();
    createCommandBuffers();
}

void Renderer::createSyncObjects()
{
    imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
    inFlightImages.resize(swapChainImages.size(), VK_NULL_HANDLE);
    VkSemaphoreCreateInfo createInfo{.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};

    auto makeSem = [&](VkSemaphore& s) {
        VkSemaphore newSem = nullptr;
        auto res = vkCreateSemaphore(device, &createInfo, nullptr, &newSem);
        if (res != VK_SUCCESS) {
            throw std::runtime_error("Failed to create semaphore");
        }
        s = newSem;
    };

    std::for_each(imageAvailableSemaphores.begin(), imageAvailableSemaphores.end(),
        [&](auto& s) { makeSem(s); });

    std::for_each(renderFinishedSemaphores.begin(), renderFinishedSemaphores.end(),
        [&](auto& s) { makeSem(s); });

    VkFenceCreateInfo fenceInfo{
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, .flags = VK_FENCE_CREATE_SIGNALED_BIT};
    std::for_each(inFlightFences.begin(), inFlightFences.end(), [&](auto& f) {
        VkFence newFence = nullptr;
        auto res = vkCreateFence(device, &fenceInfo, nullptr, &newFence);
        if (res != VK_SUCCESS) {
            throw std::runtime_error("Failed to create fence");
        }
        f = newFence;
    });
}

void Renderer::createCommandBuffers()
{
    commandBuffers.resize(swapChainFramebuffers.size());
    VkCommandBufferAllocateInfo allocInfo{.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = commandPool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = static_cast<uint32_t>(commandBuffers.size())};
    auto res = vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data());
    if (res != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate command buffers");
    }

    size_t i = 0;
    for (auto& buf : commandBuffers) {
        VkCommandBufferBeginInfo beginInfo{.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
        res = vkBeginCommandBuffer(buf, &beginInfo);
        if (res != VK_SUCCESS) {
            throw std::runtime_error("Failed to begin recording command buffer");
        }

        VkClearValue clearColor = {0.0f, 0.0f, 0.0f, 1.0f};
        VkRenderPassBeginInfo renderPassInfo{.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
            .renderPass = renderPass,
            .framebuffer = swapChainFramebuffers[i],
            .renderArea = VkRect2D{.offset = {0, 0}, .extent = swapChainExtent},
            .clearValueCount = 1,
            .pClearValues = &clearColor};
        vkCmdBeginRenderPass(buf, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        vkCmdBindPipeline(buf, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

        VkBuffer vertexBuffers[] = {vertexBuffer};
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(commandBuffers[i], 0, 1, vertexBuffers, offsets);

        vkCmdDraw(buf, static_cast<uint32_t>(vertices.size()), 1, 0, 0);
        vkCmdEndRenderPass(buf);
        res = vkEndCommandBuffer(buf);
        if (res != VK_SUCCESS) {
            throw std::runtime_error("Failed recording command buffer");
        }
        ++i;
    }
}

void Renderer::createCommandPool()
{
    QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice);
    VkCommandPoolCreateInfo poolInfo{.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .queueFamilyIndex = queueFamilyIndices.graphicsFamily.value()};

    auto res = vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool);
    if (res != VK_SUCCESS) {
        throw std::runtime_error("Failed to create command pool");
    }
}

void Renderer::createFrameBuffers()
{
    for (const auto& imageView : swapChainImageViews) {
        VkImageView attachments[] = {imageView};
        VkFramebufferCreateInfo frameBufferInfo = {
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .renderPass = renderPass,
            .attachmentCount = 1,
            .pAttachments = attachments,
            .width = swapChainExtent.width,
            .height = swapChainExtent.height,
            .layers = 1};

        VkFramebuffer buf = nullptr;
        auto res = vkCreateFramebuffer(device, &frameBufferInfo, nullptr, &buf);
        if (res != VK_SUCCESS) {
            throw std::runtime_error("Failed to create a frame buffer");
        }
        swapChainFramebuffers.push_back(buf);
    }
}

void Renderer::createRenderPass()
{
    VkAttachmentDescription colorAttachment{.format = swapChainImageFormat,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR};

    VkAttachmentReference colorAttachmentRef{
        .attachment = 0, .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};

    VkSubpassDescription subpass{.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount = 1,
        .pColorAttachments = &colorAttachmentRef};

    VkSubpassDependency dependency{.srcSubpass = VK_SUBPASS_EXTERNAL,
        .dstSubpass = 0,
        .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .srcAccessMask = 0,
        .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT};

    VkRenderPassCreateInfo renderPassInfo{.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = 1,
        .pAttachments = &colorAttachment,
        .subpassCount = 1,
        .pSubpasses = &subpass,
        .dependencyCount = 1,
        .pDependencies = &dependency};
    auto res = vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass);
    if (res != VK_SUCCESS) {
        throw std::runtime_error("Failed creating a render pass");
    }
}

VkShaderModule Renderer::createShaderModule(const std::vector<char>& code)
{
    VkShaderModuleCreateInfo createInfo{.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = code.size(),
        .pCode = reinterpret_cast<const uint32_t*>(code.data())};

    VkShaderModule module;
    auto res = vkCreateShaderModule(device, &createInfo, nullptr, &module);
    if (res != VK_SUCCESS) {
        throw std::runtime_error("Failed to create a shader module");
    }
    return module;
}

void Renderer::createGraphicsPipeline()
{
    auto vertShaderCode = Utils::readFile("shaders/shader.vert.spv");
    auto fragShaderCode = Utils::readFile("shaders/shader.frag.spv");

    auto vertShaderModule = createShaderModule(vertShaderCode);
    auto fragShaderModule = createShaderModule(fragShaderCode);

    VkPipelineShaderStageCreateInfo shaderStages[] = {
        VkPipelineShaderStageCreateInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_VERTEX_BIT,
            .module = vertShaderModule,
            .pName = "main"},
        VkPipelineShaderStageCreateInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
            .module = fragShaderModule,
            .pName = "main"}};

    auto bindingDescription = Vertex::getBindingDescription();
    auto attributeDescriptions = Vertex::getAttributeDescriptions();
    VkPipelineVertexInputStateCreateInfo vertexInputInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = 1,
        .pVertexBindingDescriptions = &bindingDescription,
        .vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size()),
        .pVertexAttributeDescriptions = attributeDescriptions.data()};
    VkPipelineInputAssemblyStateCreateInfo inputAssembly{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .primitiveRestartEnable = VK_FALSE};

    VkViewport viewport{.x = 0,
        .y = 0,
        .width = static_cast<float>(swapChainExtent.width),
        .height = static_cast<float>(swapChainExtent.height),
        .minDepth = 0,
        .maxDepth = 1.0f};
    VkRect2D scissor{.offset = {0, 0}, .extent = swapChainExtent};
    VkPipelineViewportStateCreateInfo viewportState{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .pViewports = &viewport,
        .scissorCount = 1,
        .pScissors = &scissor};

    VkPipelineRasterizationStateCreateInfo rasterizer{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .depthClampEnable = VK_FALSE,
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .cullMode = VK_CULL_MODE_BACK_BIT,
        .frontFace = VK_FRONT_FACE_CLOCKWISE,
        .depthBiasEnable = VK_FALSE,
        .lineWidth = 1.0f};

    VkPipelineMultisampleStateCreateInfo multisampling{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
        .sampleShadingEnable = VK_FALSE,
        .minSampleShading = 1.0f};

    VkPipelineColorBlendAttachmentState colorBlendAttachment{.blendEnable = VK_FALSE,
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                          VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT};
    VkPipelineColorBlendStateCreateInfo colorBlending{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .logicOpEnable = VK_FALSE,
        .attachmentCount = 1,
        .pAttachments = &colorBlendAttachment,
    };

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    auto res = vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout);
    if (res != VK_SUCCESS) {
        throw std::runtime_error("Failed to create pipeline layout");
    }

    VkGraphicsPipelineCreateInfo pipelineInfo{
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .stageCount = 2,
        .pStages = shaderStages,
        .pVertexInputState = &vertexInputInfo,
        .pInputAssemblyState = &inputAssembly,
        .pViewportState = &viewportState,
        .pRasterizationState = &rasterizer,
        .pMultisampleState = &multisampling,
        .pDepthStencilState = nullptr,
        .pColorBlendState = &colorBlending,
        .pDynamicState = nullptr,
        .layout = pipelineLayout,
        .renderPass = renderPass,
        .subpass = 0,
        .basePipelineHandle = VK_NULL_HANDLE,
        .basePipelineIndex = -1};

    res = vkCreateGraphicsPipelines(
        device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline);
    if (res != VK_SUCCESS) {
        throw std::runtime_error("Failed to create pipeline");
    }

    vkDestroyShaderModule(device, fragShaderModule, nullptr);
    vkDestroyShaderModule(device, vertShaderModule, nullptr);
}

void Renderer::createImageViews()
{
    for (const auto& image : swapChainImages) {
        VkImageViewCreateInfo createInfo{.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = image,
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = swapChainImageFormat,
            .components = VkComponentMapping{.r = VK_COMPONENT_SWIZZLE_IDENTITY,
                .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                .b = VK_COMPONENT_SWIZZLE_IDENTITY,
                .a = VK_COMPONENT_SWIZZLE_IDENTITY},
            .subresourceRange = VkImageSubresourceRange{.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1}};
        VkImageView view = nullptr;
        auto res = vkCreateImageView(device, &createInfo, nullptr, &view);
        if (res != VK_SUCCESS) {
            throw std::runtime_error("Failed creating image view");
        }
        swapChainImageViews.push_back(view);
    }
}

void Renderer::createSwapChain()
{
    SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice);

    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
    VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
    swapChainExtent = chooseSwapExtent(swapChainSupport.capabilities);
    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
    if (swapChainSupport.capabilities.maxImageCount > 0 &&
        imageCount > swapChainSupport.capabilities.maxImageCount) {
        imageCount = swapChainSupport.capabilities.maxImageCount;
    }
    VkSwapchainCreateInfoKHR createInfo{.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = surface,
        .minImageCount = imageCount,
        .imageFormat = surfaceFormat.format,
        .imageColorSpace = surfaceFormat.colorSpace,
        .imageExtent = swapChainExtent,
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .preTransform = swapChainSupport.capabilities.currentTransform,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = presentMode,
        .clipped = VK_TRUE,
        .oldSwapchain = VK_NULL_HANDLE};

    QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
    uint32_t queueFamilyIndices[] = {indices.graphicsFamily.value(), indices.presentFamily.value()};
    if (indices.graphicsFamily.value() != indices.presentFamily.value()) {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }
    auto res = vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain);
    if (res != VK_SUCCESS) {
        throw std::runtime_error("Failed creating swapchain");
    }

    vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
    swapChainImages.resize(imageCount);
    spdlog::info("Retrieving {} swapchain images", imageCount);
    vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());
    swapChainImageFormat = surfaceFormat.format;
}

void Renderer::createSurface()
{
    auto res = glfwCreateWindowSurface(instance, window, nullptr, &surface);
    if (res != VK_SUCCESS) {
        throw std::runtime_error("failed to create window surface!");
    }
}

void Renderer::createLogicalDevice()
{
    QueueFamilyIndices indices = findQueueFamilies(physicalDevice);

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = {
        indices.graphicsFamily.value(), indices.presentFamily.value()};

    spdlog::info("Creating {} {}", uniqueQueueFamilies.size(),
        (uniqueQueueFamilies.size() == 1 ? "queue" : "queues"));
    float queuePriority = 1.0f;
    for (auto queueFamily : uniqueQueueFamilies) {
        queueCreateInfos.emplace_back(
            VkDeviceQueueCreateInfo{.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                .queueFamilyIndex = queueFamily,
                .queueCount = 1,
                .pQueuePriorities = &queuePriority});
    }

    VkPhysicalDeviceFeatures deviceFeatures{};

    VkDeviceCreateInfo createInfo{.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size()),
        .pQueueCreateInfos = queueCreateInfos.data(),
        .enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size()),
        .ppEnabledExtensionNames = deviceExtensions.data(),
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

    vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphicsQueue);
    vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &presentQueue);
}

uint32_t Renderer::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);
    for (uint32_t i = 0; i < memProperties.memoryTypeCount; ++i) {
        if ((typeFilter & (1 << i)) &&
            (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }
    throw std::runtime_error("Failed to find memory of the required type");
}

void Renderer::createVertexBuffer()
{
    VkBufferCreateInfo bufferInfo{.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = sizeof(vertices[0]) * vertices.size(),
        .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE};
    auto res = vkCreateBuffer(device, &bufferInfo, nullptr, &vertexBuffer);
    if (res != VK_SUCCESS) {
        throw std::runtime_error("Failed to create vertex buffer");
    }

    VkMemoryRequirements memRequirements{};
    vkGetBufferMemoryRequirements(device, vertexBuffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo{.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = memRequirements.size,
        .memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)};

    res = vkAllocateMemory(device, &allocInfo, nullptr, &vertexBufferMemory);
    if (res != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate buffer memory");
    }
    vkBindBufferMemory(device, vertexBuffer, vertexBufferMemory, 0);
    void* data = nullptr;
    vkMapMemory(device, vertexBufferMemory, 0, bufferInfo.size, 0, &data);
    memcpy(data, vertices.data(), static_cast<size_t>(bufferInfo.size));
    vkUnmapMemory(device, vertexBufferMemory);
}

SwapChainSupportDetails Renderer::querySwapChainSupport(VkPhysicalDevice d)
{
    SwapChainSupportDetails details;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(d, surface, &details.capabilities);

    uint32_t formatCount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(d, surface, &formatCount, nullptr);
    if (formatCount > 0) {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(d, surface, &formatCount, details.formats.data());
    }

    uint32_t presentModeCount = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(d, surface, &presentModeCount, nullptr);

    if (presentModeCount > 0) {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(
            d, surface, &presentModeCount, details.presentModes.data());
    }

    return details;
}

QueueFamilyIndices Renderer::findQueueFamilies(VkPhysicalDevice d)
{
    QueueFamilyIndices indices;

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(d, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(d, &queueFamilyCount, queueFamilies.data());

    uint32_t i = 0;
    for (const auto& queueFamily : queueFamilies) {
        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT && !indices.graphicsFamily.has_value()) {
            indices.graphicsFamily = i;
        }
        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(d, i, surface, &presentSupport);
        if (presentSupport && !indices.presentFamily.has_value()) {
            indices.presentFamily = i;
        }
        i++;
    }

    return indices;
}

VkExtent2D Renderer::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities)
{
    if (capabilities.currentExtent.width != UINT32_MAX) {
        return capabilities.currentExtent;
    }
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);

    VkExtent2D actualExtent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};

    if (actualExtent.width > capabilities.maxImageExtent.width) {
        actualExtent.width = capabilities.maxImageExtent.width;
    }
    if (actualExtent.width < capabilities.minImageExtent.width) {
        actualExtent.width = capabilities.minImageExtent.width;
    }
    if (actualExtent.height > capabilities.maxImageExtent.height) {
        actualExtent.height = capabilities.maxImageExtent.height;
    }
    if (actualExtent.height < capabilities.minImageExtent.height) {
        actualExtent.height = capabilities.minImageExtent.height;
    }

    return actualExtent;
}

VkPresentModeKHR Renderer::chooseSwapPresentMode(
    const std::vector<VkPresentModeKHR>& availablePresentModes)
{
    for (const auto& availablePresentMode : availablePresentModes) {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return availablePresentMode;
        }
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}

VkSurfaceFormatKHR Renderer::chooseSwapSurfaceFormat(
    const std::vector<VkSurfaceFormatKHR>& availableFormats)
{
    if (availableFormats.empty()) {
        throw std::runtime_error("Empty formats list");
    }
    for (const auto& format : availableFormats) {
        if (format.format == VK_FORMAT_B8G8R8A8_SRGB &&
            format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return format;
        }
    }
    return *availableFormats.begin();
}

bool Renderer::isDeviceSuitable(VkPhysicalDevice d)
{
    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(d, &deviceProperties);
    spdlog::info("Evaluating {}", deviceProperties.deviceName);

    if (deviceProperties.deviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
        spdlog::info("Not a discrete GPU");
        return false;
    }

    VkPhysicalDeviceFeatures deviceFeatures;
    vkGetPhysicalDeviceFeatures(d, &deviceFeatures);
    if (!deviceFeatures.geometryShader) {
        spdlog::info("Doesn't support geometry shader");
        return false;
    }

    QueueFamilyIndices indices = findQueueFamilies(d);
    if (!indices.isComplete()) {
        spdlog::info("Missing required queues");
        return false;
    }

    if (!checkDeviceExtensionSupport(d)) {
        spdlog::info("Extensions not supported");
        return false;
    }

    SwapChainSupportDetails swapChainDetails = querySwapChainSupport(d);
    if (swapChainDetails.formats.empty() || swapChainDetails.presentModes.empty()) {
        spdlog::info("Swap chain inadequate");
        return false;
    }

    spdlog::info("Device matches the requirements");
    return true;
}

bool Renderer::checkDeviceExtensionSupport(VkPhysicalDevice d)
{
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(d, nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(d, nullptr, &extensionCount, availableExtensions.data());
    for (const auto& requestedExtension : deviceExtensions) {
        std::string extName{requestedExtension};
        if (!std::any_of(availableExtensions.begin(), availableExtensions.end(),
                [&requestedExtension](const auto& ext) {
                    return requestedExtension == std::string{ext.extensionName};
                })) {
            spdlog::info("Extension {} not supported", requestedExtension);
            return false;
        }
    }
    return true;
}

void Renderer::pickPhysicalDevice()
{
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
    if (deviceCount == 0) {
        throw std::runtime_error("No Vulkan devices found");
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

    for (const auto& d : devices) {
        if (isDeviceSuitable(d)) {
            physicalDevice = d;
            break;
        }
    }
    if (physicalDevice == VK_NULL_HANDLE) {
        throw std::runtime_error("Failed to select usitable device");
    }
}

void Renderer::populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
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

void Renderer::setupDebugMessenger()
{
    if (!enableValidationLayers) {
        return;
    }

    VkDebugUtilsMessengerCreateInfoEXT createInfo{};
    populateDebugMessengerCreateInfo(createInfo);

    auto func = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
        vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"));
    if (!func) {
        spdlog::error("Could not find vkCreateDebugUtilsMessengerEXT");
        return;
    }
    auto res = func(instance, &createInfo, nullptr, &debugMessenger);
    if (res != VK_SUCCESS) {
        spdlog::error("Debug messenger creation failed");
    }
}

void Renderer::cleanupDebugMessenger()
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

std::vector<const char*> Renderer::getRequiredExtensions()
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

VKAPI_ATTR VkBool32 VKAPI_CALL Renderer::debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void*)
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

void Renderer::createInstance()
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

void Renderer::mainLoop()
{
    int64_t lastTimeMs = Utils::GetCurrentTimeMs();
    int64_t frames = 0;
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        if (!drawFrame()) {
            break;
        }
        ++frames;
        auto timeMs = Utils::GetCurrentTimeMs();
        if (timeMs - lastTimeMs > 1000) {
            spdlog::debug("{} FPS", frames);
            frames = 0;
            lastTimeMs = timeMs;
        }
    }
    vkDeviceWaitIdle(device);
}

bool Renderer::drawFrame()
{
    vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);
    uint32_t imageIdx = 0;
    auto res = vkAcquireNextImageKHR(device, swapChain, UINT64_MAX,
        imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIdx);
    if (res != VK_SUCCESS) {
        if (res == VK_ERROR_OUT_OF_DATE_KHR) {
            spdlog::info("Swap chain out of date. Recreating");
            recreateSwapChain();
            return true;
        } else if (res != VK_SUBOPTIMAL_KHR) {
            spdlog::error("Failed to acquire the next image: {}", res);
            return false;
        }
    }
    if (inFlightImages[imageIdx] != VK_NULL_HANDLE) {
        vkWaitForFences(device, 1, &inFlightImages[imageIdx], VK_TRUE, UINT64_MAX);
    }
    inFlightImages[imageIdx] = inFlightFences[currentFrame];

    vkResetFences(device, 1, &inFlightFences[currentFrame]);
    VkSemaphore waitSemaphores[] = {imageAvailableSemaphores[currentFrame]};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    VkSemaphore signalSemaphores[] = {renderFinishedSemaphores[currentFrame]};
    VkSubmitInfo submitInfo{.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = waitSemaphores,
        .pWaitDstStageMask = waitStages,
        .commandBufferCount = 1,
        .pCommandBuffers = &commandBuffers[imageIdx],
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = signalSemaphores};
    res = vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]);
    if (res != VK_SUCCESS) {
        spdlog::error("Submitting to queue failed");
        return false;
    }

    VkSwapchainKHR swapChains[] = {swapChain};
    VkPresentInfoKHR presentInfo{.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = signalSemaphores,
        .swapchainCount = 1,
        .pSwapchains = swapChains,
        .pImageIndices = &imageIdx};

    res = vkQueuePresentKHR(presentQueue, &presentInfo);
    if (res != VK_SUCCESS) {
        if (res == VK_ERROR_OUT_OF_DATE_KHR || res == VK_SUBOPTIMAL_KHR || framebufferResized) {
            spdlog::info("Out of date or suboptimal swap chain: {}. Recreating.", res);
            framebufferResized = false;
            recreateSwapChain();
        } else {
            spdlog::error("Presenting queue failed: {}", res);
            return false;
        }
    }
    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    return true;
}

void Renderer::cleanup()
{
    cleanupSwapChain();
    vkDestroyBuffer(device, vertexBuffer, nullptr);
    vkFreeMemory(device, vertexBufferMemory, nullptr);
    std::for_each(renderFinishedSemaphores.begin(), renderFinishedSemaphores.end(),
        [this](auto& s) { vkDestroySemaphore(device, s, nullptr); });
    std::for_each(imageAvailableSemaphores.begin(), imageAvailableSemaphores.end(),
        [this](auto& s) { vkDestroySemaphore(device, s, nullptr); });
    std::for_each(inFlightFences.begin(), inFlightFences.end(),
        [this](auto& s) { vkDestroyFence(device, s, nullptr); });
    vkDestroyCommandPool(device, commandPool, nullptr);

    vkDestroyDevice(device, nullptr);

    vkDestroySurfaceKHR(instance, surface, nullptr);

    cleanupDebugMessenger();
    vkDestroyInstance(instance, nullptr);
    glfwDestroyWindow(window);
    glfwTerminate();
}

void Renderer::checkValidationLayerSupport()
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
#ifdef WIN32
#pragma warning(pop)
#endif // WIN32
} // namespace VaryZulu::Gfx
