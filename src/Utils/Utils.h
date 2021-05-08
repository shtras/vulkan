#pragma once

#include <vector>
#include <string>

namespace VaryZulu::Utils
{
std::vector<char> readFile(const std::string& fileName);
int64_t GetCurrentTimeMs();
} // namespace VaryZulu::Utils
