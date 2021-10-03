#include "display.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb/stbi_image_write.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

cr::display::display()
{
    glfwSetErrorCallback([](int error, const char *description) {
        cr::logger::error("GLFW Failed with error [{}], description [{}]", error, description);
    });

    const auto init_glfw = glfwInit();
    if (!init_glfw) exit("Failed to initialize GLFW", 1);

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    _glfw_window = glfwCreateWindow(1920, 1080, "CRender", nullptr, nullptr);
    if (!_glfw_window) exit("Failed to create GLFW window", 2);

    glfwMakeContextCurrent(_glfw_window);

    const auto load_gl = gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);
    if (!load_gl) exit("Failed to load GLAD OpenGL functions", 20);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    imnodes::Initialize();

    glGenTextures(1, &_scene_texture_handle);
    glBindTexture(GL_TEXTURE_2D, _scene_texture_handle);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glGenTextures(1, &_target_texture);
    glBindTexture(GL_TEXTURE_2D, _target_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Load the compute shader into the string
    {
        auto shader_file_in_stream = std::ifstream(std::string(CRENDER_ASSET_PATH) + "shaders/scene_zoom.comp");
        auto shader_string_stream  = std::stringstream();
        shader_string_stream << shader_file_in_stream.rdbuf();
        const auto shader_source = shader_string_stream.str();

        // Create OpenGL shader
        auto       shader_handle = glCreateShader(GL_COMPUTE_SHADER);
        const auto shader_string = shader_source.c_str();
        glShaderSource(shader_handle, 1, &shader_string, nullptr);
        glCompileShader(shader_handle);

        auto success = int(0);
        auto log     = std::array<char, 512>();
        glGetShaderiv(shader_handle, GL_COMPILE_STATUS, &success);

        if (!success)
        {
            glGetShaderInfoLog(shader_handle, 512, nullptr, log.data());
            cr::logger::error("Compiling shader [{}], with error [{}]\n", "vertex", log.data());
        }
        _compute_shader_id = shader_handle;
    }

    {
        // Create OpenGL program
        auto program_handle = glCreateProgram();

        glAttachShader(program_handle, _compute_shader_id);
        glLinkProgram(program_handle);

        auto success = int(0);
        auto log     = std::array<char, 512>();
        glGetProgramiv(program_handle, GL_LINK_STATUS, &success);

        // If it failed, show the error message
        if (!success)
        {
            glGetProgramInfoLog(program_handle, 512, nullptr, log.data());
            cr::logger::error(
              "Linking program [{}], with error [{}]\n",
              program_handle,
              log.data());
        }
        _compute_shader_program = program_handle;
    }

    glfwSetWindowUserPointer(_glfw_window, this);

    glfwSetCursorPosCallback(_glfw_window, [](GLFWwindow *window, double x, double y) {
        auto ptr = reinterpret_cast<display *>(glfwGetWindowUserPointer(window));

        ptr->_mouse_pos.x = x;
        ptr->_mouse_pos.y = y;

        ptr->_mouse_change_prev = ptr->_mouse_pos_prev - ptr->_mouse_pos;

        // coordinates are reversed on y axis (top left vs bottom left)
        ptr->_mouse_change_prev.x *= -1;

        ptr->_mouse_pos_prev = ptr->_mouse_pos;
    });

    glfwSetKeyCallback(
      _glfw_window,
      [](GLFWwindow *window, int key, int scancode, int action, int mods) {
          auto ptr = reinterpret_cast<display *>(glfwGetWindowUserPointer(window));

          if (key < ptr->_key_states.size())
          {
              if (action == GLFW_RELEASE)
                  ptr->_key_states[key] = key_state::released;
              else if (action == GLFW_REPEAT)
                  ptr->_key_states[key] = key_state::repeat;
              else if (action == GLFW_PRESS)
                  ptr->_key_states[key] = key_state::pressed;
          }
      });
}

void cr::display::start(
  std::unique_ptr<cr::scene> &         scene,
  std::unique_ptr<cr::renderer> &      renderer,
  std::unique_ptr<cr::thread_pool> &   thread_pool,
  std::unique_ptr<cr::draft_renderer> &draft_renderer,
  std::unique_ptr<cr::post_processor> &post_processor)
{
    auto work_group_max = std::array<int, 3>();

    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 0, &work_group_max[0]);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 1, &work_group_max[1]);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 2, &work_group_max[2]);
    cr::logger::info(
      "Maximum compute work group count [x: {}, y: {}, z: {}]\n",
      work_group_max[0],
      work_group_max[1],
      work_group_max[2]);

    auto &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    const auto font = io.Fonts->AddFontFromFileTTF((std::string(CRENDER_ASSET_PATH) + "fonts/Oxygen-Regular.ttf").c_str(), 18.f);


//this this this this this this 


    // CorporateGrey();

    ImGui_ImplGlfw_InitForOpenGL((GLFWwindow *) _glfw_window, true);
    ImGui_ImplOpenGL3_Init("#version 450");

    auto current_frame = 0;

    cr::logger::info("Starting main display loop");
    bool draft_mode_changed = false;
    while (!glfwWindowShouldClose(_glfw_window))
    {

        _timer.frame_start();
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        auto ui_ctx = cr::ui::init();

        if (!_in_draft_mode && draft_mode_changed)
        {
            draft_mode_changed = false;
            renderer.get()->start();
        }
        else if (_in_draft_mode && draft_mode_changed)
        {
            draft_mode_changed = false;
            renderer.get()->pause();
        }

        // Root imgui node (Not visible)
        ui::root_node(ui_ctx);

        ImGui::PushFont(font);

        ui::scene_preview(
          renderer.get(),
          draft_renderer.get(),
          scene.get(),
          _target_texture,
          _scene_texture_handle,
          _compute_shader_program,
          _in_draft_mode);

        static auto messages = std::vector<std::string>();

        if (current_frame == 0)
            messages.push_back("Welcome to CRender - The discord for support / updates is https://discord.gg/ZjrRyKXpWg");


        cr::logger::read_messages(messages);

        ui::console(messages);
        messages.clear();

        ui::settings(&renderer, &draft_renderer, &scene, &thread_pool, &post_processor, _key_states, _in_draft_mode, speed_multipliers);

        ImGui::PopFont();

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        ImGui::Render();
        glClearColor(1, 1, 1, 1);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        _timer.frame_stop();
        glfwSwapBuffers(_glfw_window);

        if (
          _key_states[static_cast<int>(key_code::KEY_R)] == key_state::pressed &&
          !io.WantCaptureKeyboard)
        {
            _in_draft_mode     = !_in_draft_mode;
            draft_mode_changed = true;
            if (_in_draft_mode)
                cr::logger::info("Switched to draft mode");
            else
                cr::logger::info("Switched to path tracing mode");
        }

        glfwSetInputMode(
          _glfw_window,
          GLFW_CURSOR,
          _in_draft_mode ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);

        if (_in_draft_mode) _update_camera(scene.get()->registry()->camera());
        _poll_events();

        current_frame++;
    }
    glDeleteTextures(1, &_scene_texture_handle);
    stop();
}

void cr::display::_poll_events()
{
    for (auto &key : _key_states)
    {
        if (key == key_state::released) key = key_state::none;
        if (key == key_state::pressed) key = key_state::held;
    }
    glfwPollEvents();
}

void cr::display::stop()
{
    glfwSetWindowShouldClose(_glfw_window, true);
}

cr::display::~display()
{
    glDeleteTextures(1, &_target_texture);
    glDeleteTextures(1, &_scene_texture_handle);
    glDeleteShader(_compute_shader_id);
    glDeleteProgram(_compute_shader_program);
}

void cr::display::_update_camera(cr::camera *camera)
{
    auto translation = glm::vec3();
    auto rotation    = glm::vec3();

    if (
      _key_states[static_cast<int>(key_code::SPACE)] == key_state::held ||
      _key_states[static_cast<int>(key_code::SPACE)] == key_state::repeat)
        translation.y += 3.0f;

    if (
      _key_states[static_cast<int>(key_code::KEY_LEFT_CONTROL)] == key_state::held ||
      _key_states[static_cast<int>(key_code::KEY_LEFT_CONTROL)] == key_state::repeat)
        translation.y -= 3.0f;

    if (
      _key_states[static_cast<int>(key_code::KEY_W)] == key_state::held ||
      _key_states[static_cast<int>(key_code::KEY_W)] == key_state::repeat)
        translation.z += 3.0f;

    if (
      _key_states[static_cast<int>(key_code::KEY_S)] == key_state::held ||
      _key_states[static_cast<int>(key_code::KEY_S)] == key_state::repeat)
        translation.z -= 3.0f;

    if (
      _key_states[static_cast<int>(key_code::KEY_D)] == key_state::held ||
      _key_states[static_cast<int>(key_code::KEY_D)] == key_state::repeat)
        translation.x -= 3.0f;

    if (
      _key_states[static_cast<int>(key_code::KEY_A)] == key_state::held ||
      _key_states[static_cast<int>(key_code::KEY_A)] == key_state::repeat)
        translation.x += 3.0f;

    translation *= static_cast<float>(_timer.since_last_frame()) * speed_multipliers.x * 5.75f;

    if (
      _key_states[static_cast<int>(key_code::KEY_LEFT_SHIFT)] == key_state::held ||
      _key_states[static_cast<int>(key_code::KEY_LEFT_SHIFT)] == key_state::repeat)
        translation *= 5;

    camera->translate(translation);

    rotation.x -= _mouse_change_prev.x * 2.0 * _timer.since_last_frame();
    rotation.y -= _mouse_change_prev.y * 2.0 * _timer.since_last_frame();
    rotation *= speed_multipliers.y;

    _mouse_change_prev = {};

    camera->rotate(rotation);
}
