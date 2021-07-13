#include "octree.h"

namespace
{
    void breakdown(
      const glm::vec3 &         min,
      const glm::vec3 &         max,
      std::array<glm::vec3, 8> &mins,
      std::array<glm::vec3, 8> &maxes)
    {
        const auto half = ((max - min) / 2.0f) + min;

        mins[0] = glm::vec3(min.x, min.y, min.z);
        mins[1] = glm::vec3(half.x, min.y, min.z);
        mins[2] = glm::vec3(min.x, half.y, min.z);
        mins[3] = glm::vec3(half.x, half.y, min.z);
        mins[4] = glm::vec3(min.x, min.y, half.z);
        mins[5] = glm::vec3(half.x, min.y, half.z);
        mins[6] = glm::vec3(min.x, half.y, half.z);
        mins[7] = glm::vec3(half.x, half.y, half.z);

        maxes[0] = glm::vec3(half.x, half.y, half.z);
        maxes[1] = glm::vec3(max.x, half.y, half.z);
        maxes[2] = glm::vec3(half.x, max.y, half.z);
        maxes[3] = glm::vec3(max.x, max.y, half.z);
        maxes[4] = glm::vec3(half.x, half.y, max.z);
        maxes[5] = glm::vec3(max.x, half.y, max.z);
        maxes[6] = glm::vec3(half.x, max.y, max.z);
        maxes[7] = glm::vec3(max.x, max.y, max.z);
    }
}    // namespace

cr::ray::intersection_record cr::octree::intersect(
  const cr::ray &   ray,
  const cr::octree &tree,
  const glm::vec3 & min,
  const glm::vec3 & max)
{
    auto intersection = cr::ray::intersection_record();

    auto mins  = std::array<glm::vec3, 8>();
    auto maxes = std::array<glm::vec3, 8>();

    // "breakdown" the parent size into the min / maxes of each octree child node
    ::breakdown(min, max, mins, maxes);

    const auto left  = _slabs4(mins.data(), maxes.data(), ray.origin, ray.direction);
    const auto right = _slabs4(mins.data() + 4, maxes.data() + 4, ray.origin, ray.direction);

    auto distances = std::array<float, 8>();
    distances[0]   = left[0];
    distances[1]   = left[1];
    distances[2]   = left[2];
    distances[3]   = left[3];
    distances[4]   = right[0];
    distances[5]   = right[1];
    distances[6]   = right[2];
    distances[7]   = right[3];

    for (auto i = 0; i < 8; i++)
    {
        const auto distance = distances[i];
        /*
         * To explain this loop a bit more -
         *
         * If there's an intersection (not equal to infinity)
         * we fire the ray off into that node (assuming it's not a child)
         */
        if (distance != std::numeric_limits<float>::infinity())
        {
            if (tree.children[i] != nullptr)
            {
                // There's more octree, traverse into it
                const auto child_intersection =
                  intersect(ray, *tree.children[i], mins[i], maxes[i]);

                if (child_intersection.distance < intersection.distance)
                    intersection = child_intersection;
            }
            else
            {
                // No more octree, we hit a node that's the final.
                // Now, is it empty or is there a block?
                if (tree.full[i])
                {
                    intersection.distance = distance;
                    intersection.intersection_point = ray.at(distance);
                }
            }
        }
    }

    return intersection;
}

std::array<float, 4> cr::octree::_slabs4(
  const glm::vec3 *p0s,
  const glm::vec3 *p1s,
  const glm::vec3 &origin,
  const glm::vec3 &inv_dir)
{
    auto t0s = std::array<glm::vec3, 4>();
    for (auto i = 0; i < 4; i++) t0s[i] = p0s[i] - origin * inv_dir;

    auto t1s = std::array<glm::vec3, 4>();
    for (auto i = 0; i < 4; i++) t1s[i] = p1s[i] - origin * inv_dir;

    auto tmins = std::array<glm::vec3, 4>();
    for (auto i = 0; i < 4; i++) tmins[i] = glm::min(t0s[i], t1s[i]);

    auto tmaxs = std::array<glm::vec3, 4>();
    for (auto i = 0; i < 4; i++) tmaxs[i] = glm::max(t0s[i], t1s[i]);

    auto intersected = std::array<float, 4>();
    for (auto i = 0; i < 4; i++)
        intersected[i] = (glm::compMax(tmins[i]) <= glm::compMin(tmaxs[i]))
          ? glm::compMin(tmins[i])
          : std::numeric_limits<float>::infinity();

    return intersected;
}
