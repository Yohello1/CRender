#include "renderer.h"

namespace
{
    [[nodiscard]] float randf() noexcept
    {
        thread_local std::mt19937                          gen;
        thread_local std::uniform_real_distribution<float> dist(0.f, 1.f);
        return dist(gen);
    }

    struct processed_hit
    {
        float     emission;
        glm::vec3 albedo;
        cr::ray   ray;
    };
    [[nodiscard]] processed_hit
      process_hit(const cr::ray::intersection_record &record, const cr::ray &ray)
    {
        auto out = processed_hit();

        // Calculate some "required" values
        const auto cos_theta = glm::abs(glm::dot(out.ray.direction, record.normal));

        out.emission = record.material->info.emission;
        if (record.material->info.tex.has_value())
            out.albedo = record.material->info.tex->get_uv(record.uv.x, record.uv.y);
        else
            out.albedo = record.material->info.colour;

        switch (record.material->info.type)
        {
        case cr::material::metal:
            out.ray.origin    = record.intersection_point + record.normal * 0.0001f;
            out.ray.direction = glm::reflect(ray.direction, record.normal);
            break;
        case cr::material::smooth:
            auto cos_hemp_dir =
              cr::sampling::hemp_cos(record.normal, glm::vec2(::randf(), ::randf()));

            out.ray.origin    = record.intersection_point + record.normal * 0.0001f;
            out.ray.direction = glm::normalize(cos_hemp_dir);
            break;
        }

        return out;
    }

}    // namespace

cr::renderer::renderer(
  const uint64_t                    res_x,
  const uint64_t                    res_y,
  const uint64_t                    bounces,
  std::unique_ptr<cr::thread_pool> *pool,
  std::unique_ptr<cr::scene> *      scene)
    : _camera(scene->get()->registry()->camera()), _buffer(res_x, res_y), _normals(res_x, res_y),
      _albedo(res_x, res_y), _res_x(res_x), _res_y(res_y), _max_bounces(bounces),
      _thread_pool(pool), _scene(scene), _raw_buffer(res_x, res_y)
{
    _management_thread = std::thread([this]() {
        while (_run_management)
        {
            ZoneScopedN("Main Loop");
            const auto tasks = _get_tasks();

            if (!tasks.empty() && (_current_sample < _spp_target || _spp_target == 0))
            {
                ZoneScopedN("Task submit and execution");
                _thread_pool->get()->wait_on_tasks(tasks);
                _current_sample++;
            }
            else
            {
                if (_current_sample == _spp_target && _current_sample != 0)
                    cr::logger::info(
                      "Finished rendering [{}] samples at resolution [X: {}, Y: {}], took: [{}]s",
                      _spp_target,
                      _res_x,
                      _res_y,
                      _timer.time_since_start());

                {
                    auto guard = std::unique_lock(_pause_mutex);
                    _pause_cond_var.notify_one();
                }
                auto guard = std::unique_lock(_start_mutex);
                _start_cond_var.wait(guard);
            }
        }
    });
}

cr::renderer::~renderer()
{
    _run_management = false;
    start();    // if we're paused, start it up again
    _management_thread.join();
}

bool cr::renderer::start()
{
    if (_pause)
    {
        _pause = false;
        _buffer.clear();
        _timer.reset();
        _raw_buffer.clear();
        _current_sample = 0;
        _total_rays     = 0;

        auto guard = std::unique_lock(_start_mutex);
        _start_cond_var.notify_all();
        return true;
    }
    return false;
}

bool cr::renderer::pause()
{
    if (!_pause)
    {
        _pause = true;

        auto guard = std::unique_lock(_pause_mutex);
        _pause_cond_var.wait(guard);
        return true;
    }
    return false;
}

void cr::renderer::update(const std::function<void()> &update)
{
    pause();

    update();

    start();
}

void cr::renderer::set_resolution(int x, int y)
{
    _res_x = x;
    _res_y = y;

    _aspect_correction = static_cast<float>(_res_x) / _res_y;

    _buffer         = cr::image<uint8_t>(x, y);
    _raw_buffer     = cr::image<uint32_t>(x, y);
    _current_sample = 0;
}

void cr::renderer::set_max_bounces(int bounces)
{
    _max_bounces = bounces;
}

void cr::renderer::set_target_spp(uint64_t target)
{
    _spp_target = target;
}

cr::image<std::uint8_t> *cr::renderer::current_progress() noexcept
{
    return &_buffer;
}

cr::image<std::uint8_t> *cr::renderer::current_normals() noexcept
{
    return &_normals;
}

cr::image<std::uint8_t> *cr::renderer::current_albedos() noexcept
{
    return &_albedo;
}

std::vector<std::function<void()>> cr::renderer::_get_tasks()
{
    ZoneScopedN("Get Tasks");
    auto tasks = std::vector<std::function<void()>>();

    if (_pause) return tasks;

    tasks.reserve(_res_y);

    for (auto y = 0; y < _res_y; y++)
        tasks.emplace_back([this, y] {
            ZoneScopedN("Scanline Task");
            auto fired_rays = size_t(0);
            for (auto x = 0; x < _res_x; x++) this->_sample_pixel(x, y, fired_rays);
            _total_rays += fired_rays;
        });

    return tasks;
}

void cr::renderer::_sample_pixel(uint64_t x, uint64_t y, size_t &fired_rays)
{
    ZoneScopedN("Sample Pixel");
    auto ray = _camera->get_ray(
      ((static_cast<float>(x) + ::randf()) / _res_x) * _aspect_correction,
      (static_cast<float>(y) + ::randf()) / _res_y);

    auto throughput = glm::vec3(1.0f, 1.0f, 1.0f);
    auto final      = glm::vec3(0.0f, 0.0f, 0.0f);

    auto total_bounces = 1;
    for (auto i = 0; i < _max_bounces; i++, total_bounces++)
    {
        auto intersection = _scene->get()->cast_ray(ray);

        if (intersection.distance == std::numeric_limits<float>::infinity())
        {
            const auto miss_uv = glm::vec2(
              0.5f + atan2f(ray.direction.z, ray.direction.x) / (2 * 3.1415f),
              0.5f - asinf(ray.direction.y) / 3.1415f);

            const auto miss_sample = _scene->get()->sample_skybox(miss_uv.x, miss_uv.y);

            final += throughput * miss_sample;
            break;
        }
        else
        {
            const auto processed = ::process_hit(intersection, ray);

            throughput *= processed.albedo;
            final += throughput * processed.emission;

            ray = processed.ray;
        }
    }
    fired_rays += total_bounces;

    // flip Y
    y = _res_y - 1 - y;

    const auto previous = _raw_buffer.get(x, y);
    const auto added    = previous + glm::vec4(final, 1.0f);

    _raw_buffer.set(x, y, added);

    const auto tonemapped = glm::clamp(added / static_cast<float>(_current_sample + 1), 0.0f, 1.0f);

    _buffer.set(
      x,
      y,
      glm::vec3(glm::pow(tonemapped.x, 1.0f / 2.2), glm::pow(tonemapped.y, 1.0f / 2.2),
                  glm::pow(tonemapped.z, 1.0f / 2.2)));
}

glm::ivec2 cr::renderer::current_resolution() const noexcept
{
    return { _res_x, _res_y };
}

uint64_t cr::renderer::current_sample_count() const noexcept
{
    return _current_sample.load();
}

cr::renderer::renderer_stats cr::renderer::current_stats()
{
    auto stats               = cr::renderer::renderer_stats();
    stats.rays_per_second    = _total_rays / _timer.time_since_start();
    stats.samples_per_second = _current_sample / _timer.time_since_start();
    stats.total_rays         = _total_rays;
    stats.running_time       = _timer.time_since_start();
    return stats;
}
