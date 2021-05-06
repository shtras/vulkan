add_executable (Test1 "Test1.cpp" "Test1.h")
target_link_libraries(Test1 PRIVATE glm::glm glfw Vulkan::Vulkan)
