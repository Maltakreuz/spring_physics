#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include "lib/window.cxx"
#include <iostream>
using namespace std;

struct Spring {
    SDL_FPoint anchor;
    SDL_FPoint bob;
    float restLength;
    float stiffness;
    float velocity;
};

int main(int argc, char *argv[]) {
    cout << "=== START SPRING SIMULATON ===" << endl;
    lvichki::Window win;
    win.fixed_delta_time = false;
    
    SDL_Color white = {255, 255, 255, 255};
    SDL_Color purple = {150, 100, 255, 255};

    
    Spring spring;
    spring.anchor = { win.width / 2.0f, win.height / 2.0f };
    spring.bob = { win.width / 2.0f, win.height / 2.0f + 200};
    spring.restLength = 350;
    spring.stiffness = 0.001f;

    int dbg_step = 0;

    win.on_update = [&]() {
        float extension = spring.bob.y - (spring.restLength + spring.anchor.y);

        if (dbg_step++ % 1000 == 0) {
            cout << "extension: " << extension;
            cout << ", spring.bob.y: " << spring.bob.y;
            cout << ", (spring.restLength + spring.anchor.y): " << (spring.restLength + spring.anchor.y);
            cout << endl;
        }


        float force = -spring.stiffness * extension * win.dt;
        spring.velocity += force;
        spring.bob.y += spring.velocity;
        
        spring.velocity *= 0.9999f;

        

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

    win.run();
    return 0;
}
