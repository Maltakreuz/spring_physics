#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include "lib/window.cxx"
#include "lib/vec2.cxx"
#include <iostream>
using namespace std;

struct Spring {
    Vec2 anchor;
    Vec2 bob;
    float restLength;
    float stiffness;
    Vec2 velocity;
};

int main(int argc, char *argv[]) {
    cout << "=== START SPRING SIMULATON ===" << endl;
    lvichki::Window win;
    win.fixed_delta_time = false;
    
    
    SDL_Color white = {255, 255, 255, 255};
    SDL_Color purple = {150, 100, 255, 255};
    Vec2 gravity(0, 1.0f);
    

    
    Spring spring;
    spring.anchor = { win.width / 2.0f, win.height / 2.0f };
    spring.bob = { win.width / 2.0f + 200, win.height / 2.0f + 200};
    spring.restLength = 350;
    spring.stiffness = 0.051f;

    int dbg_step = 0;

    win.on_update = [&]() {
         Vec2 force = spring.bob - spring.anchor;
         float extension = force.length() - spring.restLength;
         force.normalize();
         force *= -spring.stiffness * extension * win.dt;
        spring.velocity += force;
        spring.velocity +=  gravity * win.dt;
        spring.bob += spring.velocity;
        
        spring.velocity *= 0.999f;

        

        char buf[32];
        sprintf(buf, "extension: %.1f", extension);
        win.draw_text(buf, 30, 70);
    };

    win.on_draw = [&]() {
        int radius = 24;
        win.draw_circle((int)spring.anchor.x, (int)spring.anchor.y, radius, purple);
        win.draw_circle((int)spring.bob.x, (int)spring.bob.y, radius, white);
        win.draw_line(spring.anchor, spring.bob, purple);
        
    };
    
    win.on_event = [&](const SDL_Event& e) {
    float fx = 0, fy = 0;
    bool should_update = false;

    if (e.type == SDL_FINGERDOWN || e.type == SDL_FINGERMOTION) {
        fx = e.tfinger.x * win.width;
        fy = e.tfinger.y * win.height;
        should_update = true;
    }
    else if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
        fx = e.button.x;
        fy = e.button.y;
        should_update = true;
    }
    else if (e.type == SDL_MOUSEMOTION && (e.motion.state & SDL_BUTTON_LMASK)) {
        // SDL_BUTTON_LMASK значит: левая кнопка зажата во время движения
        fx = e.motion.x;
        fy = e.motion.y;
        should_update = true;
    }

    if (should_update) {
        spring.bob.x = fx;
        spring.bob.y = fy;
        spring.velocity = {0, 0};
    }
};

    win.run();
    return 0;
}
