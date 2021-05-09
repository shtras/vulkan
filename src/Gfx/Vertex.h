#pragma once

#include "vk_wrap.h"

#include <glm/glm.hpp>

#include <array>

namespace VaryZulu::Gfx
{
struct Vertex
{
    glm::vec2 pos;
    glm::vec3 color;

    static VkVertexInputBindingDescription getBindingDescription();
    static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions();
};

} // namespace VaryZulu::Gfx
