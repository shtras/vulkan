#pragma once

#ifdef WIN32
#pragma warning(push)
#pragma warning(disable : 26812)
#pragma warning(disable : 4201)
#pragma warning(disable : 4244)
#pragma warning(disable : 4324)
#define GLFW_EXPOSE_NATIVE_WIN32
#define VK_USE_PLATFORM_WIN32_KHR
#else
#define GLFW_EXPOSE_NATIVE_X11
#define VK_USE_PLATFORM_XLIB_KHR
#endif

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#ifdef WIN32
#pragma warning(pop)
#endif // WIN32
