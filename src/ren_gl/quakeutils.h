#pragma once

#include <glm/glm.hpp>

inline glm::vec3 qvec2glm(const float* v)
{
    return {v[0], v[2], -v[1]};
}

std::vector<uint8_t> readDataFile(const std::string& filename);
std::string readTextFile(const std::string& filename);
