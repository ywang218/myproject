#pragma once
#include <glm/glm.hpp>

struct alignas(16) Point {
    glm::vec4 pos;   // x, y, z, intensity
    glm::vec4 color; // r, g, b, a
};