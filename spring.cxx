#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include "lib/window.cxx"
#include "lib/vec2.cxx"
#include <iostream>
using namespace std;

/*
 - основано на видео от Coding Train
 - это первая часть из этого видео где 1 пружина (на векторах, тоесть двигается в 2d-направлении, а не только в одном)
 - Работает на основе закона Хука: "как разтяжение, так и сила"
 - Сама суть кода в объявлении структуры Spring + начало кода в udpate().

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

struct Spring {
    Vec2 anchor;
    Vec2 bob;
    Vec2 velocity;
    float mass = 1.0f;
    float restLength;
    float stiffness;
};

// Функция для преобразования диапазона (как map() в Arduino или Processing)
float mapValue(float val, float in_min, float in_max, float out_min, float out_max) {
    // Ограничиваем входное значение, чтобы цвет не "сломался" за пределами диапазона
    if (val < in_min) val = in_min;
    if (val > in_max) val = in_max;
    return out_min + (out_max - out_min) * ((val - in_min) / (in_max - in_min));
}

int main(int argc, char *argv[]) {
    cout << "=== START SPRING SIMULATON ===" << endl;
    lvichki::Window win;
    win.fixed_delta_time = false;
    
    SDL_Color white = {255, 255, 255, 255};
    SDL_Color purple = {150, 100, 255, 255};
    float gravity = 2000.0f;
    
    Spring spring;
    spring.anchor = { win.width / 2.0f, win.height / 2.0f };
    spring.bob = { win.width / 2.0f + 200, win.height / 2.0f + 200};
    spring.restLength = 300;
    spring.stiffness = 150; // чем меньше тем более медленная и "тугая" пружина. Чем выше, тем более она быстро колеблется и более свободная

    win.on_update = [&]() {
        /**
         * calc spring force, the core of this example, based on
         * Hooke's law: F = -k * x
         * where x is extension, k is stiffness and F is spring force */
        Vec2 spring_force = spring.bob - spring.anchor;
        float extension = spring_force.length() - spring.restLength;
        spring_force.normalize();
        spring_force *= -spring.stiffness * extension;

        // euler integration
        Vec2 acc = spring_force / spring.mass;
        spring.velocity += acc * win.dt;
        spring.velocity.y +=  gravity * win.dt;
        spring.bob += spring.velocity * win.dt;
        
        // In on_update, replace the damping line with:
        float damping_per_sec = pow(0.99f, 60.0f);  // Effective decay over 1 second at 60 FPS
        spring.velocity *= pow(damping_per_sec, win.dt);

        // draw debug info
        int y = 55;
        win.draw_text( ("extension: " + to_string(extension)).c_str(), 30, y += 30);
        win.draw_text( ("dt: " + to_string(win.dt)).c_str(), 30, y += 30);
        win.draw_text( ("velocity: " + to_string(spring.velocity.x) + ", " + to_string(spring.velocity.y)).c_str(), 30, y += 30);
        win.draw_text( ("spring_force: (" + to_string(spring_force.x) + ", " + to_string(spring_force.y) + ")" ).c_str(), 30, y += 30);

        // just optics, make color dynamicly
        float abs_ext = std::abs(extension);
        Uint8 intensity = (Uint8)mapValue(abs_ext, 0.0f, 400.0f, 0.0f, 255.0f);
        extension_color = {255, (Uint8)(255 - intensity), (Uint8)(255 - intensity), 255};

        if (hold_by_mouse_or_finger) {
            spring.bob.x = hold_x;
            spring.bob.y = hold_y;
            spring.velocity = {0, 0};
        }

    };

    win.on_draw = [&]() {
        int radius = 24;
        win.draw_circle((int)spring.anchor.x, (int)spring.anchor.y, radius, purple);
        win.draw_circle((int)spring.bob.x, (int)spring.bob.y, radius, white);
        win.draw_line(spring.anchor, spring.bob, extension_color);
    };
    
    win.on_event = [&](const SDL_Event& e) {
        hold_by_mouse_or_finger = false;
        if (e.type == SDL_FINGERDOWN || e.type == SDL_FINGERMOTION) {
            hold_x = e.tfinger.x * win.width;
            hold_y = e.tfinger.y * win.height;
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

    };

    win.run();
    return 0;
}
