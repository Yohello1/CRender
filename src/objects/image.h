#pragma once

#include <vector>
#include <cstdint>
#include <limits>

#include <glm/glm.hpp>

#include <util/exception.h>

namespace cr
{
    template<typename T, typename = typename std::enable_if<std::is_integral<T>::value>::type>
    class image
    {
    private:
        inline static constexpr auto ValType_Max = std::numeric_limits<T>::max();

        uint64_t             _width  = std::numeric_limits<uint64_t>::max();
        uint64_t             _height = std::numeric_limits<uint64_t>::max();
        std::unique_ptr<T[]> _image_data;

        [[nodiscard]] inline static constexpr T ValTypeFromFloat(float in)
        {
            return glm::clamp(in * ValType_Max, 0.0f, static_cast<float>(ValType_Max));
        }

        [[nodiscard]] inline static constexpr float FloatFromValType(T in)
        {
            return in / static_cast<float>(ValType_Max);
        }

        [[nodiscard]] inline constexpr float FloatAt(size_t index) const
        {
            return FloatFromValType(_image_data[index]);
        }

        inline constexpr void SetFloat(size_t index, float val)
        {
            _image_data[index] = ValTypeFromFloat(val);
        }

    public:
        image() = default;

        image(const image<T> &image) : _width(image.width()), _height(image.height())
        {
            _image_data = std::make_unique<T[]>(size());

            std::memcpy(_image_data.get(), image._image_data.get(), sizeof(T) * size());
        }

        image<T> &operator=(const image<T> &other)
        {
            if (this != &other)
            {
                _image_data = std::make_unique<T[]>(other.size());
                std::memcpy(_image_data.get(), other._image_data.get(), sizeof(T) * size());
            }
            return *this;
        }

        image(const std::unique_ptr<float[]> &data, uint64_t width, uint64_t height)
            : _width(width), _height(height)
        {
            _image_data = std::make_unique<T[]>(size());

            std::transform(data.get(), data.get() + size(), _image_data.get(), ValTypeFromFloat);
        }

        template<typename P>
        image(const std::unique_ptr<P[]> &data, uint64_t width, uint64_t height)
            : _width(width), _height(height)
        {
            _image_data = std::make_unique<T[]>(size());

            std::transform(data.get(), data.get() + size(), _image_data.get(), [](T in) {
                return image<T>::FloatFromValType(in) * std::numeric_limits<P>::max();
            });
        }

        image(uint64_t width, uint64_t height) : _width(width), _height(height)
        {
            _image_data = std::make_unique<T[]>(size());
            clear();
        }

        void clear() { std::fill(_image_data.get(), _image_data.get() + size(), ValType_Max); }

        template<typename P>
        [[nodiscard]] image<P> to_accuracy() const
        {
            return image<P>(_image_data, _width, _height);
        }

        [[nodiscard]] bool valid() const noexcept
        {
            return _width != std::numeric_limits<std::uint64_t>::max() &&
              _height != std::numeric_limits<std::uint64_t>::max();
        }

        [[nodiscard]] float at(size_t index) const { return FloatFromValType(_image_data[index]); }

        [[nodiscard]] T *data() noexcept { return _image_data.get(); }

        [[nodiscard]] const T *data() const noexcept { return _image_data.get(); }

        [[nodiscard]] uint64_t width() const noexcept { return _width; }

        [[nodiscard]] uint64_t height() const noexcept { return _height; }

        [[nodiscard]] uint64_t size() const noexcept { return _width * _height; }

        [[nodiscard]] glm::vec3 get_uv(float u, float v) const noexcept
        {
            return get(
              static_cast<uint64_t>(u * _width) % _width,
              static_cast<uint64_t>(v * _height) % _height);
        }

        [[nodiscard]] glm::vec4 get(uint64_t x, uint64_t y) const noexcept
        {
            const auto base_index = (x + y * _width) * 4;

            return { FloatAt(base_index + 0),
                     FloatAt(base_index + 1),
                     FloatAt(base_index + 2),
                     FloatAt(base_index + 3) };
        }

        void set(uint64_t x, uint64_t y, const glm::vec3 &colour) noexcept
        {
            set(x, y, glm::vec4(colour, 1.0f));
        }

        void set(uint64_t x, uint64_t y, const glm::vec4 &colour) noexcept
        {
            const auto base_index = (x + y * _width) * 4;

            SetFloat(base_index + 0, colour.r);
            SetFloat(base_index + 1, colour.g);
            SetFloat(base_index + 2, colour.b);
            SetFloat(base_index + 3, colour.a);
        }
    };
}    // namespace cr