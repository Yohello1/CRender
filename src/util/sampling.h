#pragma once

#include <glm/glm.hpp>
#include <util/numbers.h>
#include <render/entities/components.h>

namespace
{
    [[nodiscard]] float randf() noexcept;
}

namespace cr::sampling
{
    struct local_coords
    {
        glm::vec3 normal;
        glm::vec3 tangent;
        glm::vec3 bi_tangent;
    };

    [[nodiscard]] inline local_coords build_local(const glm::vec3 &normal)
    {
        auto       coords = local_coords();
        const auto s      = (normal.z < 0.0) ? -1.0f : 1.0f;
        const auto a      = -1.0f / (s + normal.z);
        const auto b      = normal.x * normal.y * a;

        coords.normal     = normal;
        coords.tangent    = glm::vec3(1.0f + s * normal.x * normal.x * a, s * b, -s * normal.x);
        coords.bi_tangent = glm::vec3(b, s + normal.y * normal.y * a, -normal.y);

        return coords;
    }

    [[nodiscard]] inline glm::vec3 map_to_solid_angle(const glm::vec2 uv, float theta_max)
    {
        const auto phi       = cr::numbers<float>::tau * uv.x;
        const auto cos_theta = 1.0f - uv.y * (1.0f - glm::cos(theta_max));
        const auto sin_theta = glm::sqrt(1.0f - cos_theta * cos_theta);

        return glm::vec3(glm::cos(phi) * sin_theta, cos_theta, glm::sin(phi) * sin_theta);
    }

    [[nodiscard]] inline float solid_angle_mapping_pdf(float theta_max)
    {
        return 1.0f / (cr::numbers<float>::tau * (1.0f - glm::cos(theta_max)));
    }

    [[nodiscard]] inline float hemp_cos_pdf(float cos_theta) { return cos_theta / cr::numbers<float>::pi; }

    namespace sun
    {
        [[nodiscard]] inline glm::vec3 sky_colour(const glm::vec3 &direction, const cr::entity::sun &sun)
        {
            const auto sun_angle = glm::acos(glm::dot(direction, -sun.direction));
            return (sun_angle < sun.size) ? (sun.colour * sun.intensity) : glm::vec3(0.0f);
        }

        struct incoming
        {
            glm::vec3       pos;
            glm::vec3       normal;
            glm::mat3       sun_transform;
            cr::entity::sun sun;
        };
        struct pdf_cos
        {
            float     pdf;
            float     cosine;
            glm::vec3 dir;
        };
        [[nodiscard]] inline pdf_cos sample(const incoming& sample)
        {
            auto out = pdf_cos();
            out.dir  = sample.sun_transform *
              map_to_solid_angle(glm::vec2(::randf(), ::randf()), sample.sun.size);
            out.pdf = solid_angle_mapping_pdf(sample.sun.size);
            out.cosine = glm::clamp(glm::dot(sample.normal, out.dir), 0.0f, 1.0f);
            return out;
        }
    }    // namespace sun

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
        [[nodiscard]] inline float specular_d(float NoH, float roughness)
        {
            const auto a2 = roughness * roughness;
            const auto d  = ((NoH * a2 - NoH) * NoH + 1.0f);

            return a2 / (d * d * cr::numbers<float>::pi);
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
         */
        [[nodiscard]] inline glm::vec3 specular_f(float u, float f0, float f90)
        {
            return f0 + (glm::vec3(f90, f90, f90) - f0) * glm::pow(1.0f - u, 5.0f);
        }

        /**
         * Specular F (Fresnel)
         *
         * F(v,h,f0,f90) = f0 + (f90 - f0) (1 - v * h) ^ 5
         *
         */
        [[nodiscard]] inline glm::vec3 specular_f(float u, const float f0)
        {
            const auto f = glm::pow(1.0f - u, 5.0f);
            return glm::vec3(f + f0 * (1.0f - f));
        }

    }    // namespace cook_torrence

    [[nodiscard]] inline glm::vec3 hemp_rand()
    {
        while (true)
        {
            auto point = glm::vec3(::randf() * 2 - 1, ::randf() * 2 - 1, ::randf() * 2 - 1);

            if (glm::length(point) >= 1) continue;

            return glm::normalize(point);
        }
    }

    [[nodiscard]] inline glm::vec3 sphere(const glm::vec2 uv)
    {
        const auto cos_theta = 2.0f * uv.x - 1.0f;
        const auto sin_theta = glm::sqrt(1.0f - cos_theta * cos_theta);

        const auto phi     = cr::numbers<float>::tau * uv.y;
        const auto sin_phi = glm::sin(phi);
        const auto cos_phi = glm::cos(phi);

        return glm::vec3(sin_theta * cos_phi, cos_theta, sin_theta * sin_phi);
    }

    [[nodiscard]] inline glm::vec3 hemp_cos(const glm::vec3 normal, const glm::vec2 uv)
    {
        const auto p = sphere(uv);
        return normal + p;
    }

    [[nodiscard]] inline glm::vec3 cos_hemp(const float x, const float y)
    {
        const auto r     = sqrtf(x);
        const auto theta = cr::numbers<float>::tau * y;

        const auto u = r * cosf(theta);
        const auto v = r * sinf(theta);

        return { u, v, sqrtf(fmaxf(0.0f, 1.0f - x)) };
    }
}    // namespace cr::sampling
