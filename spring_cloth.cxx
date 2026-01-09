#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include "lib/window.cxx"
#include "lib/vec2.cxx"
#include <iostream>
#include <vector>
#include <sstream>
#include <iomanip>

using namespace std;

/*
 - основано на видео от Coding Train
 - это третяя часть из этого видео где много пружин объедины в подобие цепочки
 - Работает на основе закона Хука: "как разтяжение, так и сила"

 Какие были проблемы при написании:
 - вечно была проблема с тем чтобы учитывать dt, при интеграции и при гашении скорости. Если её забыть, то возможны мутные проблемы. Вообще может в таких примерах стоит просто использовать фиксированный timestap и всё тут. Чтоб не возиться с dt.
 - программа компилировалась, но не линковалась при переноси с CxxDroid на PC. Проблема оказалась в неряшлевом объявлении int main() без аргументов. Нужно именно int main(int argc, char *argv[])... тогда все линкуется. Это вроде какай-то препроцессорная заморочка что SDL переопределяет вызов через SDL_Main() и ему нужно чтоб было куда передать аргументы.

 Bug:
  - Если отянуть грузило под углом, то угол отразиться при отпускании и будет плавно затухать,
    как бы образую уменьшающийся треугольник между исходным и зеркальным углом.
    Это выглядит реалистично и корректно. Но через 3-7 итераций затухания,
    грузило вдруг улеетает резко за этот диапазон, куда-то в сторону,
    пробивая этот мыслительно описанный треугольник, что выглядит странно. Реальная пружина бы просто плавно затухала уменьшая диапозон. тут что-то не так.
*/

// TODO:
// - add sound
// - add sprites
// - make next project where multiple springs are connected to each other


bool hold_by_mouse_or_finger = false;
float hold_x = 0.0f;
float hold_y = 0.0f;
SDL_Color extension_color = {255, 255, 255, 255};
SDL_Color white = {255, 255, 255, 255};
SDL_Color purple = {150, 100, 255, 255};

const int max_trail_points = 500; // Длина шлейфа в кадрах
float damping = 0.991f;
float gravity = 30.0f;
float restLength = 10.0f;
float stiffness = 10.0f;
int segments_no = 24;
bool do_draw_trail = false;

int dragged_particle_index = -1; // -1 означает, что ничего не захвачено

struct Particle {
    Vec2 pos;
    Vec2 vel;
    Vec2 acc;
    float mass = 1.0f;
    bool locked;
    float radius = 6.0f;

    vector<Vec2> trail;

    Particle() : pos(0, 0), vel(0, 0), acc(0, 0), mass(1.0f), locked(false) {}
    Particle(float x, float y) : pos(x, y), vel(0, 0), acc(0, 0), mass(1.0f), locked(false) {}

    void update(lvichki::Game& game) {
        if (locked) return;
        
        vel *= damping;
        // semi-euler integration (first velocity, then position)
        vel += acc * game.fixed_dt;
        vel.y += gravity * game.fixed_dt;
        pos += vel * game.fixed_dt;
        acc = {0, 0};

        //bounceOnWindowRect(game);
        update_trail();
    }

    void draw(lvichki::Game& game) {
        game.draw_circle((int)pos.x, (int)pos.y, radius, purple);
        if (do_draw_trail) {
            draw_trail(game);
        }
    }

    void applyForce(const Vec2& force) {
        acc += force / mass;
    }

    void bounceOnWindowRect(lvichki::Game& game) {
        if (pos.x < 0) {
            pos.x = 0;
            vel.x *= -damping;
        }
        if (pos.x > game.width) {
            pos.x = game.width;
            vel.x *= -damping;
        }
        if (pos.y < 0) {
            pos.y = 0;
            vel.y *= -damping;
        }
        if (pos.y > game.height) {
            pos.y = game.height;
            vel.y *= -damping;
        }
    }

    void draw_trail(lvichki::Game& game) {
        if (trail.size() > 1) {
            for (size_t i = 0; i < trail.size() - 1; ++i) {
                // Вычисляем прозрачность (от 0 до 255)
                // i=0 (самый старый хвост) -> прозрачный
                // i=size-1 (у самого грузика) -> яркий
                Uint8 alpha = (Uint8)((i * 255) / trail.size());
                
                Uint8 whitness = 128;
                SDL_Color t_color = {whitness, whitness, whitness, alpha}; // Белый хвост
                
                // Рисуем отрезок между двумя соседними точками истории
                game.draw_line(trail[i], trail[i+1], t_color);
            }
        }
    }

    void update_trail() {
        // Добавляем текущую позицию в "историю"
        trail.push_back(pos);
        // Если хвост стал слишком длинным, удаляем самую старую точку
        if (trail.size() > max_trail_points) {
            trail.erase(trail.begin());
        }
    }
};

struct SpringJoint {
    Particle* anchor;
    Particle* bob;
    float extension = 0;

    void update() {
        /**
         * calc spring force, the core of this example, based on
         * Hooke's law: F = -k * x
         * where x is extension, k is stiffness and F is spring force */
        Vec2 spring_force = bob->pos - anchor->pos;
        extension = spring_force.length() - restLength;
        spring_force.normalize();
        spring_force *= -stiffness * extension;

        anchor->applyForce(-spring_force);
        bob->applyForce(spring_force);
    }

    void draw(lvichki::Game& game) {
        // Включаем режим смешивания цветов для прозрачности
        SDL_SetRenderDrawBlendMode(game.get_renderer(), SDL_BLENDMODE_BLEND);
        // just optics, make color dynamicly
        float abs_ext = abs(extension);
        Uint8 intensity = (Uint8)mapValue(abs_ext, 0.0f, 400.0f, 0.0f, 255.0f);
        extension_color = {255, (Uint8)(255 - intensity), (Uint8)(255 - intensity), 255};

        bob->draw(game);
        anchor->draw(game);
        game.draw_line(anchor->pos, bob->pos, extension_color);
    }
};


void dbg_draw_info(lvichki::Game& game);

void create_triangle(vector<Particle> &particles, vector<SpringJoint> &springs) {
    particles.emplace_back(400, 400);
    particles.emplace_back(300, 700);
    particles.emplace_back(500, 700);

    springs.push_back({ &particles[0], &particles[1] });
    springs.push_back({ &particles[1], &particles[2] });
    springs.push_back({ &particles[2], &particles[0] });

    particles[0].locked = true;

    restLength = 100;
}

void create_quad(vector<Particle> &particles, vector<SpringJoint> &springs) {
    particles.emplace_back(400, 400);
    particles.emplace_back(400, 800);
    particles.emplace_back(800, 800);
    particles.emplace_back(800, 400);

    springs.push_back({ &particles[0], &particles[1] });
    springs.push_back({ &particles[1], &particles[2] });
    springs.push_back({ &particles[2], &particles[3] });
    springs.push_back({ &particles[3], &particles[0] });

    springs.push_back({ &particles[0], &particles[2] });
    springs.push_back({ &particles[1], &particles[3] });

    particles[0].locked = true;

    restLength = 300;
    stiffness = 200;
}

void create_quad_bridge(vector<Particle> &particles, vector<SpringJoint> &springs) {
    int segments = 10;     // Количество сегментов моста
    float size = 50.0f;    // Размер одного квадрата
    float start_x = 200;
    float start_y = 400;

    // 1. Создаем частицы (верхний и нижний ряд)
    for (int i = 0; i <= segments; i++) {
        particles.emplace_back(start_x + i * size, start_y);          // Верхняя точка
        particles.emplace_back(start_x + i * size, start_y + size);   // Нижняя точка
    }

    // 2. Создаем пружины
    for (int i = 0; i < segments; i++) {
        int top_curr = i * 2;
        int bot_curr = i * 2 + 1;
        int top_next = (i + 1) * 2;
        int bot_next = (i + 1) * 2 + 1;

        // Вертикальная связь (текущая секция)
        springs.push_back({ &particles[top_curr], &particles[bot_curr] });
        
        // Горизонтальные связи
        springs.push_back({ &particles[top_curr], &particles[top_next] });
        springs.push_back({ &particles[bot_curr], &particles[bot_next] });

        // Диагональные связи (крест-накрест для жесткости)
        springs.push_back({ &particles[top_curr], &particles[bot_next] });
        springs.push_back({ &particles[bot_curr], &particles[top_next] });
    }
    
    // Замыкающая вертикаль в конце
    springs.push_back({ &particles[segments * 2], &particles[segments * 2 + 1] });

    // 3. Закрепляем края (первую и последнюю пару)
    particles[0].locked = true;
    particles[1].locked = true;
    particles[segments * 2].locked = true;
    particles[segments * 2 + 1].locked = true;

    restLength = size;
    stiffness = 100;
    gravity = 1000.0f;
}

void create_cloth(vector<Particle> &particles, vector<SpringJoint> &springs) {
    int cols = 66;
    int rows = 21;
    
    float spacing = 10.0f;
    float start_x = 40;
    float start_y = 200;

    // 1. Создаем сетку частиц
    for (int y = 0; y < rows; y++) {
        for (int x = 0; x < cols; x++) {
            particles.emplace_back(start_x + x * spacing, start_y + y * spacing);
            
            // Закрепляем весь верхний ряд
            if (y == 0) {
                particles.back().locked = true;
            }
        }
    }

    // 2. Соединяем пружинами
    for (int y = 0; y < rows; y++) {
        for (int x = 0; x < cols; x++) {
            int current = y * cols + x;

            // Горизонтальная связь (Structural)
            if (x < cols - 1) {
                springs.push_back({ &particles[current], &particles[current + 1] });
            }
            // Вертикальная связь (Structural)
            if (y < rows - 1) {
                springs.push_back({ &particles[current], &particles[current + cols] });
            }
            // Диагонали (Shear)
            if (x < cols - 1 && y < rows - 1) {
                // Сверху-слева направо-вниз
                springs.push_back({ &particles[current], &particles[current + cols + 1] });
                // Сверху-справа налево-вниз
                springs.push_back({ &particles[current + 1], &particles[current + cols] });
            }
        }
    }

    restLength = spacing;
    stiffness = 100;
    gravity = 800.0f;
}


int main(int, char**) {
    cout << "=== START SPRING SIMULATON ===" << endl;
    lvichki::Game game;

    
    vector<Particle> particles;
    vector<SpringJoint> springs;
    //create_quad_bridge(particles, springs);
    create_cloth(particles, springs);

    

    game.on_update = [&]() {
        for (auto& spring : springs) {
            spring.update();
        }
        for (auto& p : particles) {
            p.update(game);
        }
        if (hold_by_mouse_or_finger) {
            particles[dragged_particle_index].pos.x = hold_x;
            particles[dragged_particle_index].pos.y = hold_y;
            particles[dragged_particle_index].vel = {0, 0};
        }
    };

    game.on_draw = [&]() {
        for (auto& spring : springs) {
            spring.draw(game);
        }
        for (auto& p : particles) {
            if (!p.locked) {
                p.draw(game);
            }
        }
        dbg_draw_info(game);
    };
    
    game.on_event = [&](const SDL_Event& e) {
        if (e.type == SDL_MOUSEBUTTONDOWN || e.type == SDL_FINGERDOWN) {
            hold_by_mouse_or_finger = true;
            
            // Получаем координаты клика
            if (e.type == SDL_FINGERDOWN) {
                hold_x = e.tfinger.x * game.width;
                hold_y = e.tfinger.y * game.height;
            } else {
                hold_x = e.button.x;
                hold_y = e.button.y;
            }

            // --- ПОИСК БЛИЖАЙШЕЙ ЧАСТИЦЫ ---
            float min_dist = 1000000.0f;
            dragged_particle_index = -1;
            
            for (size_t i = 0; i < particles.size(); i++) {
                float dx = particles[i].pos.x - hold_x;
                float dy = particles[i].pos.y - hold_y;
                float dist_sq = dx*dx + dy*dy; // Используем квадрат расстояния (быстрее)
                
                if (dist_sq < min_dist) {
                    min_dist = dist_sq;
                    dragged_particle_index = i;
                }
            }
            // Опционально: можно добавить проверку min_dist < 50*50, 
            // чтобы не хватать частицу с другого конца экрана.
        } else if (e.type == SDL_MOUSEMOTION || e.type == SDL_FINGERMOTION) {
            if (hold_by_mouse_or_finger) {
                if (e.type == SDL_FINGERMOTION) {
                    hold_x = e.tfinger.x * game.width;
                    hold_y = e.tfinger.y * game.height;
                } else {
                    hold_x = e.motion.x;
                    hold_y = e.motion.y;
                }
            }
        } else if (e.type == SDL_MOUSEBUTTONUP || e.type == SDL_FINGERUP) {
            hold_by_mouse_or_finger = false;
            dragged_particle_index = -1;
        }

        if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_w) {
            stiffness += 10.0f;
        }
        if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_q) {
            stiffness -= 10.0f;
            if (stiffness < 1.0f) stiffness = 1.0f;
        }
        if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_s) {
            gravity += 10.0f;
        }
        if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_a) {
            gravity -= 10.0f;
        }
        if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_t) {
            restLength += 10.0f;
        }
        if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_r) {
            restLength -= 10.0f;
            if (restLength < 0.0f) restLength = 0.0f;
        }
        if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_g) {
            do_draw_trail = !do_draw_trail;
        }
    };

    game.run();
    return 0;
}

void dbg_draw_info(lvichki::Game& game) {
    int y = 55;
    //game.draw_text( ("dt: " + to_string(game.dt)).c_str(), 30, y += 30);
    //stringstream ss; ss << fixed << setprecision(2) << "velocity: " << spring.bob.vel.x << ", " << spring.bob.vel.y;
    //game.draw_text(ss.str().c_str(), 30, y += 30);
    game.draw_text("Q/W to adjust stiffness, A/S gravity, R/T rest length, G to toggle trail", 30, y += 30, purple);
    game.draw_text( ("stiffness: " + to_string((int) stiffness)).c_str(), 30, y += 30);
    game.draw_text( ("gravity: " + to_string((int) gravity)).c_str(), 30, y += 30);
    game.draw_text( ("rest length: " + to_string((int) restLength)).c_str(), 30, y += 30);
    game.draw_text( ("Segments no: " + to_string((int) segments_no)).c_str(), 30, y += 30);
    game.draw_text( ("draw trail: " + to_string(do_draw_trail)).c_str(), 30, y += 30);
}
