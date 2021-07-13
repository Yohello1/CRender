#pragma once

#include <array>
#include <memory>
#include <bitset>

#include <render/ray.h>
#include <render/material/material.h>

#include <glm/gtx/component_wise.hpp>

namespace cr
{
    class octree
    {
    public:
        octree() = default;

        [[nodiscard]] static cr::ray::intersection_record
          intersect(const cr::ray &ray, const cr::octree &tree, const glm::vec3 &min, const glm::vec3 &max);

    private:
        /*
         * If children[i] == nullptr, then that's a leaf
         * if full[i] == false, then it's empty else full
         */
        std::bitset<8>                         full;
        std::array<std::unique_ptr<octree>, 8> children;

        [[nodiscard]] static std::array<float, 4> _slabs4(
          const glm::vec3 *p0s,
          const glm::vec3 *p1s,
          const glm::vec3 &origin,
          const glm::vec3 &inv_dir);
    };
}    // namespace cr
