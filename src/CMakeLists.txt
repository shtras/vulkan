add_executable (Test1 "Test1.cpp")
target_link_libraries(Test1 PRIVATE glm::glm glfw Vulkan::Vulkan)

add_executable (Test2 "Test2.cpp")
target_link_libraries(Test2 PRIVATE glm::glm glfw Vulkan::Vulkan spdlog::spdlog)