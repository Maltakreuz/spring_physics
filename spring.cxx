#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include "lib/window.cxx"

struct Spring {
    SDL_FPoint anchor;
    SDL_FPoint bob;
    float restLength;
    float stiffness;
    float velocity;
};

int main() {
    lvichki::Window win;
    
    SDL_Color white = {255, 255, 255, 255};
    SDL_Color purple = {150, 100, 255, 255};

    
    Spring spring;
    spring.anchor = { win.width / 2.0f, win.height / 2.0f };
    spring.bob = { win.width / 2.0f, win.height / 2.0f  + 200};
    spring.restLength = 350;
    spring.stiffness = .0001f;

    win.on_update = [&]() {
        float extension = spring.bob.y - (spring.restLength + spring.anchor. y);
        float force = -spring.stiffness * extension;
        spring.velocity += force;
        spring.bob.y += spring.velocity;
        
        spring.velocity *= 0.999f;
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
