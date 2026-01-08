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
float damping = 0.995f;
float gravity = 30.0f;
float restLength = 10.0f;
float stiffness = 10.0f;
int segments_no = 24;
bool do_draw_trail = false;

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

    void update(lvichki::Window& game) {
        if (locked) return;
        
        vel *= damping;
        // semi-euler integration (first velocity, then position)
        vel += acc * game.fixed_dt;
        vel.y += gravity * game.fixed_dt;
        pos += vel * game.fixed_dt;
        acc = {0, 0};

        bounceOnWindowRect(game);
        update_trail();
    }

    void draw(lvichki::Window& game) {
        game.draw_circle((int)pos.x, (int)pos.y, radius, purple);
        if (do_draw_trail) {
            draw_trail(game);
        }
    }

    void applyForce(const Vec2& force) {
        acc += force / mass;
    }

    void bounceOnWindowRect(lvichki::Window& game) {
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

    void draw_trail(lvichki::Window& game) {
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
    float extension;

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

    void draw(lvichki::Window& game) {
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


void dbg_draw_info(lvichki::Window& game);

int main(int, char**) {
    cout << "=== START SPRING SIMULATON ===" << endl;
    lvichki::Window game;

    
    vector<Particle> particles;
    particles.reserve(segments_no);

    float startX = game.width / 2.0f;
    float startY = 100.0f;

    for (int i = 0; i < segments_no; i++) {
        particles.emplace_back(startX, startY + i * restLength);
    }

    particles[0].locked = true;

    vector<SpringJoint> springs;
    springs.reserve(segments_no - 1);

    for (int i = 0; i < segments_no - 1; i++) {
        springs.push_back({ &particles[i], &particles[i+1], 0 });
    }

    game.on_update = [&]() {
        for (auto& spring : springs) {
            spring.update();
        }
        for (auto& p : particles) {
            p.update(game);
        }
        if (hold_by_mouse_or_finger) {
            particles.back().pos.x = hold_x;
            particles.back().pos.y = hold_y;
            particles.back().vel = {0, 0};
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
        hold_by_mouse_or_finger = false;
        if (e.type == SDL_FINGERDOWN || e.type == SDL_FINGERMOTION) {
            hold_x = e.tfinger.x * game.width;
            hold_y = e.tfinger.y * game.height;
            hold_by_mouse_or_finger = true;
        }
        else if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
            hold_x = e.button.x;
            hold_y = e.button.y;
            hold_by_mouse_or_finger = true;
        }
        else if (e.type == SDL_MOUSEMOTION && (e.motion.state & SDL_BUTTON_LMASK)) {
            hold_x = e.motion.x;
            hold_y = e.motion.y;
            hold_by_mouse_or_finger = true;
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

void dbg_draw_info(lvichki::Window& game) {
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