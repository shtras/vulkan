#include "Utils.h"

#include <fstream>
#include <chrono>

namespace VaryZulu::Utils
{
std::vector<char> readFile(const std::string& fileName)
{
    std::ifstream file(fileName, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        throw std::runtime_error("failed to open file!");
    }

    size_t fileSize = static_cast<size_t>(file.tellg());
    std::vector<char> buffer(fileSize);
    file.seekg(0);
    file.read(buffer.data(), fileSize);
    file.close();
    return buffer;
}

int64_t GetCurrentTimeMs()
{
    auto now = std::chrono::system_clock::now();
    auto msNow = std::chrono::time_point_cast<std::chrono::milliseconds>(now);
    return msNow.time_since_epoch().count();
}

} // namespace VaryZulu::Utils
