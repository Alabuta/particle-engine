
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


namespace app
{
    struct state final {
        std::array<std::int32_t, 2> window_size{0, 0};
    };
}

void update(app::state const &state)
{
    ;
}

void render(app::state const &state)
{
    auto [width, height] = state.window_size;

    glViewport(0, 0, width, height);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

class window_event_handler final : public platform::window::event_handler_interface {
public:

    window_event_handler(app::state &app_state) noexcept : app_state{app_state} { }

    void on_resize(std::int32_t width, std::int32_t height) override
    {
        if (width * height == 0)
            return;

        app_state.window_size = std::array{width, height};
    }

private:

    app::state &app_state;
};


int main()
{
#if defined(_MSC_VER)
    #if defined(_DEBUG) || defined(DEBUG)
        _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
    #endif
#else
    static_assert(__cpp_concepts >= 201500); // check compiled with -fconcepts
    static_assert(__cplusplus >= 201703L);

    #if defined(_DEBUG) || defined(DEBUG)
        std::signal(SIGSEGV, posix_signal_handler);
        std::signal(SIGTRAP, posix_signal_handler);
    #endif
#endif

    if (auto result = glfwInit(); result != GLFW_TRUE)
        throw std::runtime_error(fmt::format("failed to init GLFW: {0:#x}\n"s, result));

    app::state state{std::array{800, 600}};

    auto [width, height] = state.window_size;

    platform::window window{"particle-engine"sv, width, height};

    gfx::context context{window};

    auto app_window_events_handler = std::make_shared<window_event_handler>(state);
    window.connect_event_handler(app_window_events_handler);

    auto input_manager = std::make_shared<platform::input_manager>();
    window.connect_input_handler(input_manager);

    if (auto result = glGetError(); result != GL_NO_ERROR)
        throw std::runtime_error(fmt::format("OpenGL error: {0:#x}\n"s, result));

    auto last = std::chrono::high_resolution_clock::now();

    window.update([&]
    {
        glfwPollEvents();

        auto now = std::chrono::high_resolution_clock::now();
        auto delta_time = static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(now - last).count());
        last = now;

        update(state);

        render(state);

        glfwSwapBuffers(window.handle());

        if (auto result = glGetError(); result != GL_NO_ERROR)
            throw std::runtime_error(fmt::format("OpenGL error: {0:#x}\n"s, result));
    });

    glfwTerminate();
}