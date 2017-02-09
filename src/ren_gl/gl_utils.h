#pragma once

#include <vector>
#include <string>
#include <chrono>
#include <glm/glm.hpp>

template<typename T>
glm::vec3 qvec2glm(const T* v) { return {v[0], v[2], -v[1]}; }

std::vector<uint8_t> readDataFile(const std::string& filename);
std::string readTextFile(const std::string& filename);

using Frustum = std::array<glm::vec4, 6>;
using Box = std::array<glm::vec3, 2>;

template<typename T>
Box qbox2glm(const T* min, const T* max)
{
    Box result;
    result[0].x = min[0]; result[0].y = min[2]; result[0].z = -max[1];
    result[1].x = max[0]; result[1].y = max[2]; result[1].z = -min[1];
    return result;
}

Frustum extractViewPlanes(const glm::mat4 mvp);
bool intersectFrustumAABB(const Frustum& planes, const std::array<glm::vec3, 2>& box);

class Timing
{
    std::chrono::steady_clock::time_point _start;
public:
    Timing()
    {
        _start = std::chrono::steady_clock::now();
    }
    void print(const char* msg)
    {
        using namespace std::chrono;
        auto now = steady_clock::now();
        auto t = now - _start;
        _start = now;
        printf("%s : %d ms\n", msg, duration_cast<milliseconds>(t).count());
    }
};