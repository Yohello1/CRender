#pragma once

#include <glm/glm.hpp>

namespace
{
    [[nodiscard]] float randf() noexcept;
}

namespace cr::sampling
{
    namespace cook_torrence
    {
        /**
         * Specular D (Normal Distribution Function)
         *
         *                          a ^ 2
         * D(h,a) = -----------------------------------
         *          pi((n * h) ^ 2 (a ^ 2 - 1) + 1) ^ 2
         *
         */
        [[nodiscard]] inline float
          specular_d(float NoH, float roughness)
        {
            constexpr auto pi = 3.141592f;

            const auto a2 = roughness * roughness;
            const auto d = ((NoH * a2 - NoH) * NoH + 1.0f);

            return a2 / (d * d * pi);
        }

        /**
         * Specular G (Geometric Shadowing)
         *
         *                                                          0.5
         * V(v,l,a) = -----------------------------------------------------------------------------------------
         *            n * l sqrt((n * v) ^ 2 (1 - a ^ 2) + a ^ 2) + n * v sqrt((n * l) ^ 2 (1 - a ^ 2) + a ^ 2)
         *
         */
        [[nodiscard]] inline float specular_g(float NoV, float NoL, float roughness)
        {
            const auto a2 = roughness * roughness;

            const auto ggxv = NoL * glm::sqrt(NoV * NoV * (1.0f - a2) + a2);
            const auto ggxl = NoV * glm::sqrt(NoL * NoL * (1.0f - a2) + a2);

            return 0.5f / (ggxv + ggxl);
        }

        /**
         * Specular F (Fresnel)
         *
         * F(v,h,f0,f90) = f0 + (f90 - f0) (1 - v * h) ^ 5
         *
         * Note: This has an approximation within it, so it's not exactly as derived
         *
         */
        [[nodiscard]] inline float specular_f(float u, float f0)
        {
            const auto f = glm::pow(1.0f - u, 5.0f);
            return f + f0 * (1.0f - f);
        }

    }    // namespace cook_torrence

    [[nodiscard]] inline float frensel_reflectance(
      float cos_in,
      float cos_out,
      float eta)
    {
        const auto r_perp = (eta * cos_in - cos_out) / (eta * cos_in + cos_out);
        const auto r_parallel = (cos_in - eta * cos_out) / (cos_in + eta * cos_out);

        return 0.5 * (r_perp * r_perp + r_parallel * r_parallel);
    }

    [[nodiscard]] inline glm::vec3 hemp_rand()
    {
        while (true)
        {
            auto point = glm::vec3(
              ::randf() * 2 - 1,
              ::randf() * 2 - 1,
              ::randf() * 2 - 1);

            if (glm::length2(point) >= 1) continue;

            return glm::normalize(point);
        }
    }

    [[nodiscard]] inline glm::vec3 cos_hemp(const float x, const float y)
    {
        const auto r     = sqrtf(x);
        const auto theta = 6.283f * y;

        const auto u = r * cosf(theta);
        const auto v = r * sinf(theta);

        return { u, v, sqrtf(fmaxf(0.0f, 1.0f - x)) };
    }
}    // namespace cr::sampling
