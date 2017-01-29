#include "glcommon.h"

extern "C"
{
#include "quakedef.h"
}

std::vector<uint8_t> readDataFile(const std::string& filename)
{
    // try open file and get length
    int handle;
    auto len = COM_OpenFile (filename.c_str(), &handle);
    if (handle == -1)
        return {};

    // create buffer
    std::vector<uint8_t> buf(len);

    // read into buffer
    Sys_FileRead (handle, buf.data(), len);
    COM_CloseFile(handle);

    return buf;
}

std::string readTextFile(const std::string& filename)
{
    auto buf = readDataFile(filename);
    return std::string(reinterpret_cast<const char*>(buf.data()), buf.size());
}

Frustum extractViewPlanes(const glm::mat4 mvp)
{
    Frustum planes;
    for (int i = 0; i < 4; ++i)
    {
        // Left Plane
        // col4 + col1
        planes[0][i] = mvp[i][3] + mvp[i][0];
        // Right Plane
        // col4 - col1
        planes[1][i] = mvp[i][3] - mvp[i][0];
        // Bottom Plane
        // col4 + col2
        planes[2][i] = mvp[i][3] + mvp[i][1];
        // Top Plane
        // col4 - col2
        planes[3][i] = mvp[i][3] - mvp[i][1];
        // Near Plane
        // col4 + col3
        planes[4][i] = mvp[i][3] + mvp[i][2];
        // Far Plane
        // col4 - col3
        planes[5][i] = mvp[i][3] - mvp[i][2];
    }
    return planes;
}

bool intersectFrustumAABB(const Frustum& planes, const std::array<glm::vec3, 2>& box)
{
    for(const auto& plane: planes)
    {
        auto px = static_cast<int>(plane.x > 0.0f);
        auto py = static_cast<int>(plane.y > 0.0f);
        auto pz = static_cast<int>(plane.z > 0.0f);
        auto dp = plane.x * box[px].x + plane.y * box[py].y + plane.z * box[pz].z;
        if (dp < -plane.w)
            return false;
    }
    return true;
}