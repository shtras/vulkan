#pragma once

#include "vk_wrap.h"

#include <array>

namespace VaryZulu::Gfx
{
struct UniformBufferObject
{
    alignas(16) glm::mat4 model;

    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;
};

struct Vertex
{
    glm::vec2 pos;
    glm::vec3 color;
    glm::vec2 texCoord;

    static VkVertexInputBindingDescription getBindingDescription();
    static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions();
};

} // namespace VaryZulu::Gfx
