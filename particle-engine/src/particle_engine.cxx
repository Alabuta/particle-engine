#include "particle_engine.hxx"

#ifdef max
#undef max
#endif

#ifdef min
#undef min
#endif


namespace app
{
    particle_engine::particle_engine()
    {
        stop_workers = false;

        for (auto &&index : acquired_indices)
            index.clear();

        acquired_indices.at(0).test_and_set();

        frames_data.resize(FRAMES_COUNT);

        auto const workers_count = std::max(std::thread::hardware_concurrency() - 1u, 1u);

        barrier = std::make_unique<utility::barrier>(workers_count);

        workers.reserve(workers_count);

        for (auto worker_index = 0u; worker_index < workers_count; ++worker_index)
            workers.emplace_back(&particle_engine::worker_object, this, worker_index);
    }

    particle_engine::~particle_engine()
    {
        stop_workers = true;
        barrier->drop();

        for (auto &worker : workers)
            worker.join();
    }

    void app::particle_engine::update(std::int64_t dt)
    {
        global_timer.fetch_add(dt);
    }

    void particle_engine::render()
    {
        auto frame_index = acquire_frame_index(shared_frame_read_index.load());

        auto &&frame_data = frames_data.at(frame_index);

        for (auto i = 0u; i < frame_data.particles_count; ++i) {
            auto &&particle = frame_data.particles.at(i);
            auto &&position = particle.position;
            auto &&color = particle.color;

            gfx::draw_point(position.x, position.y, color.r, color.g, color.b, color.a);
        }

        release_frame_index(frame_index);
    }

    void particle_engine::spawn_effect(glm::vec2 &&position, glm::vec4 &&color)
    {
        auto frame_index = acquire_available_frame_index();

        auto &&frame_data = frames_data.at(frame_index);

        frame_data.effect.count = PER_EFFECT_PARTICLES_COUNT;
        frame_data.effect.position = std::move(position);
        frame_data.effect.color = std::move(color);

        release_frame_index(frame_index);
    }

    void particle_engine::worker_object(std::uint32_t worker_index)
    {
        auto frame_read_index = 0u, frame_write_index = 1u;

        barrier->wait();

        app::worker_context worker_context{worker_index};

        while (!stop_workers) {
            update_worker_time_points(worker_context);

            if (worker_context.dt == 0) {
                std::this_thread::sleep_for(UPDATE_PERIOD - std::chrono::milliseconds{worker_context.dt});
                continue;
            }

            frame_write_index = shared_frame_write_index.load();
            
            worker_context.read_frame = &frames_data.at(frame_read_index);
            worker_context.write_frame = &frames_data.at(frame_write_index);

            auto &&read_frame = *worker_context.read_frame;
            auto &&write_frame = *worker_context.write_frame;

            add_particles(worker_context);

            process_particles(worker_context);

            frame_read_index = frame_write_index;

            if ((++idle_workers) == workers.size()) {
                const_cast<app::frame_data &>(read_frame).effect.count = 0;
                
                write_frame.particles_count = std::min(TOTAL_PARTICLES_COUNT, job_count.load());
                
                shared_frame_read_index = frame_read_index;

                frame_write_index = acquire_available_frame_index();
                shared_frame_write_index = frame_write_index;

                release_frame_index(frame_write_index);

                shared_effect_read_index = 0;

                shared_particle_read_index = 0;
                shared_particle_write_index = 0;

                job_count = 0;

                idle_workers = 0;

                barrier->wait();
            }

            else barrier->wait();
        }
    }

    std::uint32_t particle_engine::acquire_available_frame_index()
    {
        auto index = shared_frame_read_index.load();

        do {
            index = (index + 1) % FRAMES_COUNT;
        } while (index == shared_frame_read_index.load() || acquired_indices.at(index).test_and_set());

        return index;
    }

    std::uint32_t particle_engine::acquire_frame_index(std::uint32_t index)
    {
        acquired_indices.at(index).test_and_set();

        return index;
    }

    std::uint32_t particle_engine::release_frame_index(std::uint32_t index)
    {
        acquired_indices.at(index).clear();

        return index;
    }

    void particle_engine::update_worker_time_points(app::worker_context &worker_context)
    {
        thread_local static std::int64_t last_time = 0;

        auto global_time = global_timer.load();
        worker_context.dt = static_cast<std::uint64_t>(global_time - last_time);
        last_time = global_time;

        worker_context.from_start_time += std::chrono::milliseconds{worker_context.dt};
    }

    void particle_engine::process_particles(app::worker_context &worker_context)
    {
        auto &&generator = worker_context.generator;
        auto &&bernoulli_distribution = worker_context.bernoulli_distribution;
        auto &&uniform_real_distribution = worker_context.uniform_real_distribution;

        auto &&read_frame = *worker_context.read_frame;
        auto &&write_frame = *worker_context.write_frame;

        auto is_dead = false;
        auto is_outside = false;
        auto is_exploded = false;

        for (auto idx = shared_particle_read_index++; idx < read_frame.particles_count; idx = shared_particle_read_index++) {
            auto &&read_particle = read_frame.particles.at(idx);

            auto elapsed = static_cast<float>((worker_context.from_start_time - read_particle.born_time).count());
            auto life_time = static_cast<float>(PARTICLE_LIFE_TIME.count()) * (uniform_real_distribution(generator) * .5f + .5f);

            //is_dead = worker_context.from_start_time - read_particle.born_time > PARTICLE_LIFE_TIME;
            is_dead = elapsed > life_time;
            is_outside = is_particle_outside(read_particle);
            is_exploded = !is_outside && is_dead && bernoulli_distribution(generator);

            if (!is_outside & !is_dead) {
                auto job_write_index = shared_particle_write_index++;

                if (job_write_index >= TOTAL_PARTICLES_COUNT)
                    continue;

                auto &&write_particle = write_frame.particles.at(job_write_index);

                write_particle.born_time = read_particle.born_time;
                write_particle.time_stamp = worker_context.from_start_time;
                write_particle.color = read_particle.color;

                auto dt = static_cast<float>(worker_context.dt) * 1e-3f;

                write_particle.position = read_particle.position + read_particle.velocity * dt;
                write_particle.velocity = read_particle.velocity * C_DRAG - glm::vec2{0, G_ACCEL * dt};

                ++job_count;
            }

            else if (is_exploded) {
                auto j = shared_particle_write_index++;

                for (auto i = 0u; j < TOTAL_PARTICLES_COUNT; ++i, j = shared_particle_write_index++) {
                    if (i >= PER_EFFECT_PARTICLES_COUNT)
                        break;

                    auto &&particle = write_frame.particles.at(j);

                    particle.born_time = worker_context.from_start_time;

                    particle.position = read_particle.position;
                    particle.color = read_particle.color;

                    randomize_velocity_vector(worker_context, particle.velocity);

                    ++job_count;
                }
            }
        }
    }

    void particle_engine::add_particles(app::worker_context &worker_context)
    {
        auto &&read_frame = *worker_context.read_frame;
        auto &&write_frame = *worker_context.write_frame;

        auto &&position = read_frame.effect.position;
        auto &&color = read_frame.effect.color;

        auto j = 0u;

        for (auto i = shared_effect_read_index++; i < read_frame.effect.count; i = shared_effect_read_index++) {
            j = shared_particle_write_index++;

            if (j >= TOTAL_PARTICLES_COUNT)
                break;

            auto &&write_particle = write_frame.particles.at(j);

            write_particle.born_time = worker_context.from_start_time;

            write_particle.position = position;
            write_particle.color = color;

            randomize_velocity_vector(worker_context, write_particle.velocity);

            ++job_count;
        }
    }

    void particle_engine::randomize_velocity_vector(app::worker_context &worker_context, glm::vec2 &velocity)
    {
        auto &&generator = worker_context.generator;
        auto &&uniform_real_distribution = worker_context.uniform_real_distribution;

        auto angle = uniform_real_distribution(generator) * kPI * 2.f;
        auto speed = (uniform_real_distribution(generator) * .75f + .25f) * 100.f;

        velocity = glm::vec2{std::cos(angle), std::sin(angle)} * speed;
    }

    bool particle_engine::is_particle_outside(app::particle const &particle)
    {
        auto &&position = particle.position;
        return position.x < 0 || position.x > app::SCREEN_WIDTH || position.y < 0 || position.y > app::SCREEN_HEIGHT;
    }
}