#pragma once

#include <string>

#include <imgui/imgui.h>
#include <render/renderer.h>
#include <render/timer.h>
#include <render/draft/draft_renderer.h>
#include <util/asset_loader.h>
#include <util/algorithm.h>
#include <stb/stbi_image_write.h>
#include <stb/stb_image.h>
#include <tracy/Tracy.hpp>

namespace cr::ui
{
    struct init_ctx
    {
        ImGuiDockNodeFlags dock_flags;
        ImGuiWindowFlags   window_flags;
        ImGuiViewport *    viewport;
    };
    [[nodiscard]] inline init_ctx init()
    {
        ZoneScopedN("UI Initialize") auto dock_flags = ImGuiDockNodeFlags_PassthruCentralNode;
        auto window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
        auto viewport     = ImGui::GetMainViewport();

        ImGui::SetNextWindowPos(viewport->Pos);
        ImGui::SetNextWindowSize(viewport->Size);
        ImGui::SetNextWindowViewport(viewport->ID);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

        window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
          ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
        window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

        if (dock_flags & ImGuiDockNodeFlags_PassthruCentralNode)
            window_flags |= ImGuiWindowFlags_NoBackground;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

        auto ctx         = init_ctx();
        ctx.dock_flags   = dock_flags;
        ctx.window_flags = window_flags;
        ctx.viewport     = viewport;
        return ctx;
    }

    inline void init_dock(
      const init_ctx &   ctx,
      const std::string &top_left,
      const std::string &bottom_left,
      const std::string &right_panel)
    {
        ZoneScopedN("Dock Initialization");
        auto dockspace_id = ImGui::GetID("DockSpace");
        ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ctx.dock_flags);

        static auto first_time = true;
        if (first_time)
        {
            first_time = false;

            ImGui::DockBuilderRemoveNode(dockspace_id);    // clear any previous layout
            ImGui::DockBuilderAddNode(dockspace_id, ctx.dock_flags | ImGuiDockNodeFlags_DockSpace);
            ImGui::DockBuilderSetNodeSize(dockspace_id, ctx.viewport->Size);

            auto dock_id_right = ImGui::DockBuilderSplitNode(
              dockspace_id,
              ImGuiDir_Right,
              0.2f,
              nullptr,
              &dockspace_id);

            auto dock_id_down = ImGui::DockBuilderSplitNode(
              dockspace_id,
              ImGuiDir_Down,
              0.2f,
              nullptr,
              &dockspace_id);

            ImGui::DockBuilderGetNode(dock_id_right)->LocalFlags |= ImGuiDockNodeFlags_NoTabBar;
            ImGui::DockBuilderGetNode(dock_id_down)->LocalFlags |= ImGuiDockNodeFlags_NoTabBar;
            ImGui::DockBuilderGetNode(dockspace_id)->LocalFlags |= ImGuiDockNodeFlags_NoTabBar;

            ImGui::DockBuilderDockWindow(top_left.c_str(), dockspace_id);
            ImGui::DockBuilderDockWindow(right_panel.c_str(), dock_id_right);
            ImGui::DockBuilderDockWindow(bottom_left.c_str(), dock_id_down);

            ImGui::DockBuilderFinish(dockspace_id);
        }
    }

    inline void root_node(init_ctx ui_ctx)
    {
        ImGui::Begin("DockSpace", nullptr, ui_ctx.window_flags);
        ImGui::PopStyleVar(3);

        // Setup the dock
        cr::ui::init_dock(ui_ctx, "Scene Preview", "Console", "Misc");

        ImGui::End();
    }

    inline void scene_preview(
      cr::renderer *      renderer,
      cr::draft_renderer *draft_renderer,
      GLuint              target_texture,
      GLuint              scene_texture,
      GLuint              compute_program,
      bool                in_draft_mode)
    {
        ZoneScopedN("Scene Preview");
        ImGui::Begin("Scene Preview");
        auto window_size = ImGui::GetContentRegionAvail();

        // Set uniforms
        {
            auto        io                  = ImGui::GetIO();
            static auto current_translation = glm::vec2(0.0f, 0.0f);
            static auto current_zoom        = float(1);
            if (ImGui::IsWindowHovered())
            {
                current_zoom += io.MouseWheel * -.05;

                if (ImGui::IsMouseDown(0))
                {
                    const auto delta =
                      glm::vec2(io.MouseDelta.x, io.MouseDelta.y) * glm::vec2(-1, -1);
                    current_translation.x += delta.x;
                    current_translation.y += delta.y;
                }
            }

            glUniform1f(glGetUniformLocation(compute_program, "zoom"), current_zoom);

            glUniform2fv(
              glGetUniformLocation(compute_program, "translation"),
              1,
              glm::value_ptr(current_translation));

            glUniform2i(
              glGetUniformLocation(compute_program, "target_size"),
              window_size.x,
              window_size.y);

            glUniform2i(
              glGetUniformLocation(compute_program, "scene_size"),
              renderer->current_resolution().x,
              renderer->current_resolution().y);
        }

        {
            glBindTexture(GL_TEXTURE_2D, target_texture);
            glTexImage2D(
              GL_TEXTURE_2D,
              0,
              GL_RGBA8,
              static_cast<int>(window_size.x),
              static_cast<int>(window_size.y),
              0,
              GL_RGBA,
              GL_UNSIGNED_BYTE,
              nullptr);
            glClearTexImage(target_texture, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
            glBindImageTexture(0, target_texture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA8);

            if (in_draft_mode)
            {
                draft_renderer->render();

                glUseProgram(compute_program);
                const auto rendered = draft_renderer->rendered_texture();
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, rendered);
            }
            else
            {
                ZoneScopedN("CPU to GPU Scene Texture") const auto current_progress =
                  renderer->current_progress();
                // Upload rendered scene to GPU
                glUseProgram(compute_program);
                glBindTexture(GL_TEXTURE_2D, scene_texture);
                glTexImage2D(
                  GL_TEXTURE_2D,
                  0,
                  GL_RGBA8,
                  current_progress->width(),
                  current_progress->height(),
                  0,
                  GL_RGBA,
                  GL_FLOAT,
                  current_progress->data());
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, scene_texture);
            }

            glDispatchCompute(
              static_cast<int>(glm::ceil(window_size.x / 8)),
              static_cast<int>(glm::ceil(window_size.y / 8)),
              1);

            glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
        }
        ImGui::Image((void *) target_texture, window_size);
        ImGui::End();
    }

    inline void setting_render(
      cr::renderer *                    renderer,
      cr::draft_renderer *              draft_renderer,
      std::unique_ptr<cr::thread_pool> &pool)
    {
        ZoneScopedN("Setting Render");
        static auto resolution   = glm::ivec2();
        static auto bounces      = int(5);
        static auto thread_count = static_cast<int>(std::thread::hardware_concurrency());

        if (resolution.x == 0) resolution.x = renderer->current_resolution().x;

        if (resolution.y == 0) resolution.y = renderer->current_resolution().y;

        ImGui::InputInt2("Resolution", glm::value_ptr(resolution));
        ImGui::InputInt("Max Bounces (?)", &bounces);
        if (ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();

            ImGui::Text("How many times can the ray bounce around before getting terminated");
            ImGui::Separator();
            ImGui::Text("Suggested amounts");
            ImGui::Text("---------");
            ImGui::Text("< 4 | Not suggested");
            ImGui::Text("5-12 | Good for general use");
            ImGui::Text("12-20 | Enough for near perfect lighting");
            ImGui::Text("> 20 | Not suggested, performance to lighting tradeoff not optimal");
            ImGui::EndTooltip();
        }
        ImGui::InputInt("Thread Count", &thread_count);

        if (ImGui::Button("Update"))
        {
            ZoneScopedN("Update Renderer");
            renderer->update([renderer, draft_renderer, &pool]() {
                renderer->set_max_bounces(bounces);
                renderer->set_resolution(resolution.x, resolution.y);
                draft_renderer->set_resolution(resolution.x, resolution.y);
                pool = std::make_unique<cr::thread_pool>(thread_count);
            });
        }

        ImGui::Text(
          "%s",
          fmt::format("Current sample count: [{}]", renderer->current_sample_count()).c_str());

        ImGui::NewLine();
        ImGui::Separator();

        if (ImGui::Button("Pause"))
        {
            const auto success = renderer->pause();

            if (success)
                cr::logger::info("Paused the renderer successfully");
            else
                cr::logger::warn("Cannot pause the renderer when it's already paused");
        }
        else if (ImGui::SameLine(); ImGui::Button("Start"))
        {
            const auto success = renderer->start();

            if (success)
                cr::logger::info("Started the renderer successfully");
            else
                cr::logger::warn("Cannot start the renderer when it's started");
        }
        static auto target_spp = int(0);
        ImGui::Text("Target Sample Count (?)");
        ImGui::InputInt("Count", &target_spp, 16, 64);
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Set amount of samples per pixel you want to render, 0 for no limit");
        if (ImGui::Button("Set target sample count")) renderer->set_target_spp(target_spp);
    }

    inline void setting_export(std::unique_ptr<cr::renderer> *renderer)
    {
        ZoneScopedN("Setting Export") static auto file_string = std::array<char, 32>();
        ImGui::InputTextWithHint("File Name", "Max 32 chars", file_string.data(), 32);

        static const auto export_types = std::array<std::string, 3>({ "PNG", "JPG", "EXR" });

        static auto current_type = 0;

        if (ImGui::BeginCombo("Export Type", export_types[current_type].c_str()))
        {
            for (auto i = 0; i < 3; i++)
                if (ImGui::Button(export_types[i].c_str())) current_type = i;
            ImGui::EndCombo();
        }

        auto selected_type = cr::asset_loader::image_type::PNG;

        switch (current_type)
        {
        case 0: selected_type = asset_loader::image_type::PNG; break;
        case 1: selected_type = asset_loader::image_type::JPG; break;
        case 2: selected_type = asset_loader::image_type::EXR; break;
        }

        if (ImGui::Button("Save"))
        {
            ZoneScopedN("Save Buffer");
            cr::logger::info("Starting to export image [{}]", file_string.data());
            auto timer = cr::timer();

            const auto data = renderer->get()->current_progress();

            cr::asset_loader::export_framebuffer(*data, file_string.data(), selected_type);
            cr::logger::info("Finished exporting image in [{}s]", timer.time_since_start());
        }
    }

    inline void setting_materials(cr::renderer *renderer, cr::scene *scene)
    {
        ZoneScopedN("Setting Materials");
        const auto &models = scene->models();

        if (models.size() > 0)
        {
            ImGui::BeginChild(
              "setting-materials-models-child",
              { 0, ImGui::GetContentRegionAvail().y / 5 });
            static auto selected_index   = 0;
            bool        selected_changed = false;
            for (auto i = 0; i < models.size(); i++)
            {
                const auto &model = models[i];
                if (ImGui::Button(model.object_name.c_str()))
                {
                    if (selected_index != i) selected_changed = true;
                    selected_index = i;
                }
            }
            ImGui::EndChild();

            ImGui::BeginChild("settings-materials-materials-list");

            static auto material_search_string = std::array<char, 65>();
            material_search_string[64]         = '\0';
            const auto changed                 = ImGui::InputTextWithHint(
              "Material name",
              "Max 64 chars",
              material_search_string.data(),
              64);

            static auto materials  = std::vector<cr::material>();
            static auto first_time = true;
            if (selected_changed || first_time)
            {
                selected_changed = false;
                materials.clear();
                auto &registry_materials =
                  scene->registry()
                    ->entities
                    .get<cr::entity::model_materials>(models[selected_index].entity_handle)
                    .materials;
                for (auto i = models[selected_index].index_start;
                     i < models[selected_index].index_end;
                     i++)
                    materials.push_back(scene->meshes()[i].material);
            }

            // This is a cool Imgui thing im going to make (search thing)
            static auto found_material_indices = std::vector<size_t>();

            if (changed || first_time)
            {
                ZoneScopedN("Search materials");
                found_material_indices = cr::algorithm::find_string_matches<cr::material>(
                  std::string(material_search_string.data()),
                  materials,
                  [](const cr::material &material) { return material.info.name; });
            }

            for (const auto index : found_material_indices)
            {
                auto &material = materials[index];
                ImGui::Separator();
                ImGui::Indent(4.f);
                ImGui::Text("%s", material.info.name.c_str());
                ImGui::Indent(4.f);
                static const auto material_types =
                  std::array<std::string, 3>({ "Metal", "Smooth", "Glass" });
                auto current_type = material.info.type == material::metal ? 0
                  : material.info.type == material::smooth                ? 1
                                                                          : 2;

                if (ImGui::BeginCombo(
                      ("Type##" + material.info.name).c_str(),
                      material_types[current_type].c_str()))
                {
                    for (auto i = 0; i < material_types.size(); i++)
                        if (ImGui::Button(material_types[i].c_str())) current_type = i;
                    ImGui::EndCombo();
                }

                switch (current_type)
                {
                case 0: material.info.type = material::metal; break;
                case 1: material.info.type = material::smooth; break;
                case 2: material.info.type = material::glass; break;
                }

                ImGui::Separator();

                switch (material.info.type)
                {
                case material::metal:
                    ImGui::SliderFloat(
                      ("Roughness##" + material.info.name).c_str(),
                      &material.info.roughness,
                      0,
                      1);
                    break;

                case material::smooth: break;

                case material::glass:
                    ImGui::SliderFloat(
                      ("IOR##" + material.info.name).c_str(),
                      &material.info.ior,
                      1,
                      2);
                    break;
                }

                ImGui::SliderFloat(
                  ("Emission##" + material.info.name).c_str(),
                  &material.info.emission,
                  0,
                  50);

                if (!material.info.tex.has_value())
                    ImGui::ColorEdit3(
                      ("Colour##" + material.info.name).c_str(),
                      glm::value_ptr(material.info.colour));

                ImGui::Unindent(8.f);
            }

            if (ImGui::Button("Update Materials"))
            {
                const auto &model = models[selected_index];
                renderer->update([materials = materials, scene, &model] {
                    for (auto i = model.index_start; i < model.index_end; i++)
                    {
                        //                        scene->meshes()[i].material = materials[i];
                        auto &registry_materials =
                          scene->registry()
                            ->entities.get<cr::entity::model_materials>(model.entity_handle)
                            .materials;
                        registry_materials = materials;
                    }
                });
            }

            ImGui::EndChild();
            first_time = false;
        }
    }

    inline void setting_asset_loader(
      std::unique_ptr<cr::renderer> *renderer,
      std::unique_ptr<cr::scene> *   scene,
      bool                           in_draft_mode)
    {
        ZoneScopedN("Setting Asset Loader");
        static std::string current_directory;
        static std::string current_model;
        bool               throw_away = false;
        ImGui::Indent(4.f);

        ImGui::Text("Model Loader");
        ImGui::Indent(4.f);
        if (ImGui::BeginCombo("Select Model", current_directory.c_str()))
        {
            for (const auto &entry : std::filesystem::directory_iterator("./assets/models"))
            {
                if (!entry.is_directory()) continue;

                const auto model_path = cr::asset_loader::valid_directory(entry);
                if (!model_path.has_value()) continue;

                // Go through each file in the directory
                if (ImGui::Selectable(entry.path().filename().string().c_str(), &throw_away))
                {
                    current_directory = entry.path().string();
                    current_model     = model_path.value();
                    break;
                }
            }
            ImGui::EndCombo();
        }

        if (current_model != std::filesystem::path() && ImGui::Button("Load Model"))
        {
            ZoneScopedN("Load Model");
            cr::logger::info("Starting to load model [{}]", current_model);
            auto timer = cr::timer();
            // Load model in
            const auto model_data = cr::asset_loader::load_model(current_model, current_directory);

            if (!in_draft_mode)
                renderer->get()->update(
                  [&scene, &model_data] { scene->get()->add_model(model_data); });
            else
                scene->get()->add_model(model_data);
            cr::logger::info("Finished loading model in [{}s]", timer.time_since_start());

            auto texture_count = 0;
            for (const auto &material : model_data.materials)
                if (material.info.tex.has_value()) texture_count++;

            cr::logger::info(
              "-- Model Stats\n\tVertices: [{}]\n\tTriangles: [{}]\n\tMaterials: [{}]\n\tTextures: "
              "[{}]",
              model_data.vertices.size(),
              model_data.vertex_indices.size() / 3,
              model_data.materials.size(),
              texture_count);
        }

        ImGui::Unindent(4.f);
        ImGui::Separator();
        ImGui::NewLine();
        ImGui::Text("Skybox Loader");
        ImGui::Indent(4.f);

        static std::filesystem::path current_skybox;
        if (ImGui::BeginCombo("Select Skybox", current_skybox.string().c_str()))
        {
            for (const auto &entry : std::filesystem::directory_iterator("./assets/skybox"))
            {
                if (entry.is_directory()) continue;

                if (ImGui::Selectable(entry.path().filename().string().c_str(), &throw_away))
                {
                    current_skybox = entry.path();
                    break;
                }
            }
            ImGui::EndCombo();
        }

        if (!current_skybox.empty() && ImGui::Button("Load Skybox"))
        {
            ZoneScopedN("Load Skybox");
            auto timer = cr::timer();
            // Load skybox in
            cr::logger::info("Started to load skybox [{}]", current_skybox.stem().string());
            renderer->get()->update([&scene, current_skybox = current_skybox, &timer] {
                auto image  = cr::asset_loader::load_picture(current_skybox.string());
                auto skybox = cr::image(image.colour, image.res.x, image.res.y);

                scene->get()->set_skybox(std::move(skybox));

                cr::logger::info("Finished loading skybox in [{}s]", timer.time_since_start());
                cr::logger::info(
                  "-- Skybox Stats\n\tResolution:\n\t\tX: [{}]\n\t\tY: [{}]",
                  image.res.x,
                  image.res.y);
            });
        }

        static auto rotation = glm::vec2();
        ImGui::DragFloat2("Rotation", glm::value_ptr(rotation), 0.f, 1.f);

        ImGui::SameLine();
        if (ImGui::Button("Update"))
            renderer->get()->update([&scene]() { scene->get()->set_skybox_rotation(rotation); });

        ImGui::Unindent(8.f);
    }

    inline void setting_stats(cr::renderer *renderer)
    {
        ZoneScopedN("Setting Stats");
        ImGui::Indent(4.f);

        const auto stats = renderer->current_stats();

        ImGui::Text("%s", fmt::format("Rays per second: [{}]", stats.rays_per_second).c_str());
        ImGui::Text(
          "%s",
          fmt::format("Samples per second: [{}]", stats.samples_per_second).c_str());
        ImGui::Text("%s", fmt::format("Total Rays Fired: [{}]", stats.total_rays).c_str());
        ImGui::Text("%s", fmt::format("Running Time: [{}]", stats.running_time).c_str());

        ImGui::Unindent(4.f);
    }

    inline std::optional<cr::ImGuiThemes::theme> new_theme;
    inline void                                  setting_style()
    {
        ZoneScopedN("Setting Style");
        ImGui::Separator();
        ImGui::Indent(4.0f);
        ImGui::Text("Custom Theme (?)");
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Don't like the default colours? Spice it up a bit!");

        static std::string current_directory;
        static std::string current_font;
        bool               throw_away = false;
        ImGui::Indent(4.0f);

        ImGui::BeginChild("style-child-region", { 0, ImGui::GetContentRegionAvail().y / 5 });

        static const auto styles =
          std::array<std::string, 4>({ "Red", "Corporate Grey", "Cherry", "Dark Charcoal" });

        for (auto i = 0; i < styles.size(); i++)
            if (ImGui::Button(styles[i].c_str()))
                if (i == 0)
                    new_theme = cr::ImGuiThemes::theme::RED;
                else if (i == 1)
                    new_theme = cr::ImGuiThemes::theme::CORPORATE_GREY;
                else if (i == 2)
                    new_theme = cr::ImGuiThemes::theme::CHERRY;
                else if (i == 3)
                    new_theme = cr::ImGuiThemes::theme::DARK_CHARCOAL;
                else if (i == 4)
                    new_theme = cr::ImGuiThemes::theme::VISUAL_STUDIO;
                else if (i == 5)
                    new_theme = cr::ImGuiThemes::theme::GREEN;

        ImGui::EndChild();

        ImGui::Unindent(8.0f);
    }

    inline void settings(
      std::unique_ptr<cr::renderer> *      renderer,
      std::unique_ptr<cr::draft_renderer> *draft_renderer,
      std::unique_ptr<cr::scene> *         scene,
      std::unique_ptr<cr::thread_pool> *   pool,
      bool                                 draft_mode)
    {
        ZoneScopedN("Setting");
        ImGui::Begin("Misc");

        // List all of the different settings
        static const auto window_settings = std::array<std::string, 6>(
          { "Render", "Export", "Materials", "Asset Loader", "Stats", "Settyle" });

        static auto selected_window = 0;

        for (auto i = 0; i < window_settings.size(); i++)
        {
            if (i % 4 != 0) ImGui::SameLine();
            if (ImGui::Button(window_settings[i].c_str())) selected_window = i;
        }

        ImGui::Separator();

        ImGui::BeginChild(window_settings[selected_window].c_str(), ImVec2(), true);

        ImGui::Text("%s", window_settings[selected_window].c_str());

        ImGui::Separator();

        switch (selected_window)
        {
        case 0: setting_render(renderer->get(), draft_renderer->get(), *pool); break;
        case 1: setting_export(renderer); break;
        case 2: setting_materials(renderer->get(), scene->get()); break;
        case 3: setting_asset_loader(renderer, scene, draft_mode); break;
        case 4: setting_stats(renderer->get()); break;
        case 5: setting_style(); break;
        }

        ImGui::EndChild();

        ImGui::End();
    }

    inline void console(const std::vector<std::string> &lines = {})
    {
        ZoneScopedN("UI Console");
        ImGui::Begin("Console");

        ImGui::Text("Console Output");
        ImGui::Separator();

        ImGui::BeginChild("Console-Text-Region");

        static auto lines_to_display = std::vector<std::string>();
        lines_to_display.reserve(lines_to_display.size() + lines.size());
        for (const auto &string : lines) lines_to_display.push_back(string);

        for (const auto &line : lines_to_display) ImGui::Text("%s", line.c_str());

        if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) ImGui::SetScrollHereY(1.0f);

        ImGui::EndChild();

        ImGui::End();
    }

    //    inline void export_ui

}    // namespace cr::ui
