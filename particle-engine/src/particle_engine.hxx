#pragma once

#include <atomic>
#include <vector>
#include <random>
#include <array>

#include "utility/barrier.hxx"
#include "math/math.hxx"


namespace app
{
    auto constexpr EFFECTS_COUNT = 2048u;
    auto constexpr PER_EFFECT_PARTICLES_COUNT = 64u;
    auto constexpr TOTAL_PARTICLES_COUNT = EFFECTS_COUNT * PER_EFFECT_PARTICLES_COUNT;
    auto constexpr DEAD_PARTICLE_EXPLOSION_CHANCE = .25f;
    
    struct particle final {
        std::chrono::milliseconds born_time{0}; // ms
        std::chrono::milliseconds time_stamp{0}; // ms

        glm::vec2 position{0};
        glm::vec2 velocity{0};
        glm::vec4 color{0};
    };

    struct effect final {
        std::uint32_t count{0};

        glm::vec2 position{0};
        glm::vec4 color{0};
    };

    struct frame_data final {
        frame_data() : particles_count{0}
        {
            particles.resize(TOTAL_PARTICLES_COUNT);
        }

        std::uint32_t particles_count;

        std::vector<app::particle> particles;
        app::effect effect;
    };
}

namespace app
{
    struct worker_context final {
        app::frame_data const *read_frame{nullptr};
        app::frame_data *write_frame{nullptr};

        std::chrono::milliseconds elapsed_time{0};
        std::chrono::milliseconds from_start_time{0};

        std::uint32_t worker_index;

        std::random_device random_device;
        std::mt19937 generator;

        std::bernoulli_distribution bernoulli_distribution;
        std::uniform_real_distribution<float> uniform_real_distribution;

        std::uint64_t dt{0};

        worker_context(std::uint32_t worker_index) : worker_index{worker_index}
        {
            generator = std::mt19937{random_device()};

            bernoulli_distribution = std::bernoulli_distribution{DEAD_PARTICLE_EXPLOSION_CHANCE};
            uniform_real_distribution = std::uniform_real_distribution<float>{0.f, 1.f};
        }
    };
}

namespace app
{
    class particle_engine final {
    public:

        particle_engine();

        ~particle_engine();

        void render();

        void update(std::int64_t dt);

        void spawn_effect(glm::vec2 &&position, glm::vec4 &&color);

    private:

        static std::chrono::milliseconds constexpr UPDATE_PERIOD{5};
        static std::chrono::milliseconds constexpr PARTICLE_LIFE_TIME{2'000};

        static auto constexpr FRAMES_COUNT{4u};

        static auto constexpr C_DRAG{.999f};
        static auto constexpr G_ACCEL{9.81f};

        std::atomic_int64_t global_timer{0};

        std::atomic_bool stop_workers;

        std::array<std::atomic_flag, FRAMES_COUNT> acquired_indices;

        std::vector<app::frame_data> frames_data;

        std::vector<std::thread> workers;

        std::atomic_uint32_t shared_frame_read_index{0};
        std::atomic_uint32_t shared_frame_write_index{1};

        std::atomic_uint32_t shared_effect_read_index{0};

        std::atomic_uint32_t shared_particle_read_index{0};
        std::atomic_uint32_t shared_particle_write_index{0};

        std::atomic_uint32_t idle_workers{0};

        std::atomic_uint32_t job_count{0};

        std::unique_ptr<utility::barrier> barrier;

        void worker_object(std::uint32_t worker_index);

        void update_worker_time_points(app::worker_context &worker_context);

        void process_particles(app::worker_context &worker_context);

        void add_particles(app::worker_context &worker_context);

        std::uint32_t acquire_available_frame_index();
        std::uint32_t acquire_frame_index(std::uint32_t index);
        std::uint32_t release_frame_index(std::uint32_t index);

        void randomize_velocity_vector(app::worker_context &worker_context, glm::vec2 &velocity);

        static bool is_particle_outside(app::particle const &particle);
    };
}