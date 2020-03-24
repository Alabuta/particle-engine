#if defined(_DEBUG) || defined(DEBUG)
    #if defined(_MSC_VER)
        #define _CRTDBG_MAP_ALLOC
        #include <crtdbg.h>
    #else
        #include <thread>
        #include <csignal>
        #include <execinfo.h>

        void posix_signal_handler(int signum)
        {
            using namespace std::string_literals;

            auto current_thread = std::this_thread::get_id();

            auto name = "unknown"s;

            switch (signum) {
                case SIGABRT: name = "SIGABRT"s;  break;
                case SIGSEGV: name = "SIGSEGV"s;  break;
                case SIGBUS:  name = "SIGBUS"s;   break;
                case SIGILL:  name = "SIGILL"s;   break;
                case SIGFPE:  name = "SIGFPE"s;   break;
                case SIGTRAP: name = "SIGTRAP"s;  break;
            }

            std::array<void *, 32> callStack;

            auto size = backtrace(std::data(callStack), std::size(callStack));

            std::cerr << fmt::format("Error: signal {}\n"s, name);

            auto symbollist = backtrace_symbols(std::data(callStack), size);

            for (auto i = 0; i < size; ++i)
                std::cerr << fmt::format("{} {} {}\n"s, i, current_thread, symbollist[i]);

            free(symbollist);

            exit(1);
        }
    #endif
#endif

#include "main.hxx"
#include "particle_engine.hxx"


class mouse_handler final : public platform::mouse::handler_interface {
public:

    mouse_handler(std::shared_ptr<app::particle_engine> particle_engine) : particle_engine{particle_engine} { }

private:

    std::weak_ptr<app::particle_engine> particle_engine;

    std::random_device random_device;
    std::mt19937 generator;

    std::uniform_real_distribution<float> uniform_real_distribution;

    float last_x{0.f}, last_y{0.f};

    void on_wheel(float, float) override { }
    void on_down(handler_interface::buttons_t) override { }

    void on_move(float x, float y) override
    {
        last_x = x;
        last_y = y;
    }

    void on_up(handler_interface::buttons_t buttons) override
    {
        if (buttons.to_ulong() != 0x01)
            return;
        
        if (particle_engine.expired())
            return;

        glm::vec4 color{
            uniform_real_distribution(generator),
            uniform_real_distribution(generator),
            uniform_real_distribution(generator),
            1.f
        };

        if (auto observe = particle_engine.lock(); observe)
            observe->spawn_effect(glm::vec2{last_x, last_y}, std::move(color));
    }
};

int main()
{
#if defined(_MSC_VER)
    #if defined(_DEBUG) || defined(DEBUG)
        _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
    #endif
#else
    // static_assert(__cpp_concepts >= 201500); // check compiled with -fconcepts
    static_assert(__cplusplus >= 201703L);

    #if defined(_DEBUG) || defined(DEBUG)
        std::signal(SIGSEGV, posix_signal_handler);
        std::signal(SIGTRAP, posix_signal_handler);
    #endif
#endif

    if (auto result = glfwInit(); result != GLFW_TRUE)
        throw std::runtime_error(fmt::format("failed to init GLFW: {0:#x}\n"s, result));

    platform::window window{"particle-engine"sv, app::SCREEN_WIDTH, app::SCREEN_HEIGHT};

    gfx::context context{window};

    auto input_manager = std::make_shared<platform::input_manager>();
    window.connect_input_handler(input_manager);

    auto particle_engine = std::make_shared<app::particle_engine>();

    auto mouse_handler = std::make_shared<::mouse_handler>(particle_engine);
    input_manager->mouse().connect_handler(mouse_handler);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, app::SCREEN_WIDTH, 0, app::SCREEN_HEIGHT, 0, 40);
	glMatrixMode(GL_MODELVIEW);

    if (auto result = glGetError(); result != GL_NO_ERROR)
        throw std::runtime_error(fmt::format("OpenGL error: {0:#x}\n"s, result));

    auto last = std::chrono::high_resolution_clock::now();

    window.update([&]
    {
        glfwPollEvents();

        auto now = std::chrono::high_resolution_clock::now();
        auto delta_time = std::chrono::duration_cast<std::chrono::milliseconds>(now - last).count();
        last = now;

        particle_engine->update(delta_time);

        glViewport(0, 0, app::SCREEN_WIDTH, app::SCREEN_HEIGHT);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glBegin(GL_POINTS);
            particle_engine->render();
        glEnd();

        glfwSwapBuffers(window.handle());

        if (auto result = glGetError(); result != GL_NO_ERROR)
            throw std::runtime_error(fmt::format("OpenGL error: {0:#x}\n"s, result));
    });

    particle_engine.reset();

    glfwTerminate();
}