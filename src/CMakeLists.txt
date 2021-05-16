add_executable (Test2 "Test2.cpp" "Utils/Utils.cpp" "Gfx/Vertex.cpp" "Gfx/Renderer.cpp" "Utils/Utils.h" "Gfx/Vertex.h" "Gfx/Renderer.h" "vk_wrap.h" "stb_image.h")
target_link_libraries(Test2 PRIVATE glm::glm glfw Vulkan::Vulkan spdlog::spdlog)

compile_shader(Test2 FORMAT spv SOURCES shader.vert shader.frag)
