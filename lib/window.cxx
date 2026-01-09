#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <cmath>
#include <cstdio>
#include <functional>
#include "vec2.cxx"
using namespace std;

namespace lvichki {

class Game {
public:
    int width = 1080;
    int height = 1340; // 2184
    float dt = 0;
    float fps = 0.0f;
    bool show_fps = true;  // По умолчанию показываем FPS
    const float fixed_dt = 1.0f / 60.0f; // Фиксированный шаг времени для физики
    function<void()> on_update = [](){};
    function<void()> on_draw   = [](){};
    function<void(const SDL_Event&)> on_event = [](const SDL_Event&){};
    uint64_t update_count = 0;
    uint64_t draw_count = 0;

    Game() {
        if (SDL_Init(SDL_INIT_VIDEO) < 0) {
            SDL_Log("SDL_Init error: %s", SDL_GetError());
            return;
        }

        if (TTF_Init() == -1) {
            SDL_Log("TTF_Init error: %s", TTF_GetError());
        }

        window = SDL_CreateWindow("lvichki",
            SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
            width, height, SDL_WINDOW_SHOWN);

        if (!window) {
            SDL_Log("Window creation error: %s", SDL_GetError());
            return;
        }

        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
        if (!renderer) {
            SDL_Log("Renderer creation error: %s", SDL_GetError());
            return;
        }

        // === ШРИФТ ПО УМОЛЧАНИЮ ===
        font = TTF_OpenFont("FreeSans.ttf", 32);
        if (!font) {
            font = TTF_OpenFont("/storage/emulated/0/Download/freesans/FreeSans.ttf", 32);
        }
        if (!font) {
            SDL_Log("Не удалось загрузить шрифт по умолчанию: %s", TTF_GetError());
            SDL_Log("Текст (включая FPS) не будет отображаться.");
        }

        // FPS счётчик
        fps_start_time = SDL_GetTicks();
        last_frame_time = fps_start_time;

        is_valid = true;
    }

    ~Game() {
        if (font) TTF_CloseFont(font);
        if (renderer) SDL_DestroyRenderer(renderer);
        if (window)   SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
    }

    void run() {
        if (!is_valid) return;

        bool running = true;
        SDL_Event e;
        
        float accumulator = 0.0f;
        Uint32 last_time = SDL_GetTicks();

        while (running) {
            update_delta_and_fps();
            //dbg_print_frames();
            Uint32 now = SDL_GetTicks();
            float frame_time = (now - last_time) / 1000.0f;
            if (frame_time > 0.25f) frame_time = 0.25f; // "Защита от спирали смерти"
            last_time = now;

            accumulator += frame_time;

            while (SDL_PollEvent(&e)) {
                if (e.type == SDL_QUIT) {
                    running = false;
                }
                if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE) {
                    running = false;
                }
                on_event(e);
            }

            
            // Пока накопилось достаточно времени, крутим физику
            while (accumulator >= fixed_dt) {
                if (on_update) on_update();
                update_count++;
                accumulator -= fixed_dt;
            }

            draw_count++;
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            SDL_RenderClear(renderer);
            if (on_draw) on_draw();

            // Автоматический FPS
            if (show_fps && font) {
                char buf[32];
                sprintf(buf, "FPS: %.1f", fps);
                draw_text(buf, 30, 30);
            }

            SDL_RenderPresent(renderer);
        }
    }

    SDL_Renderer* get_renderer() const { return renderer; }

    // Если всё-таки захочешь другой шрифт — можешь перезагрузить
    void set_font(const char* path, int size) {
        if (font) TTF_CloseFont(font);
        font = TTF_OpenFont(path, size);
        if (!font) {
            SDL_Log("Failed to load custom font %s: %s", path, TTF_GetError());
        }
    }

    void draw_text(const char* text, int x, int y, SDL_Color color = {255,255,255,255}) {
        if (!font) return;

        SDL_Surface* surf = TTF_RenderText_Blended(font, text, color);
        if (!surf) return;

        SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer, surf);
        if (tex) {
            SDL_Rect dst = {x, y, surf->w, surf->h};
            SDL_RenderCopy(renderer, tex, nullptr, &dst);
            SDL_DestroyTexture(tex);
        }
        SDL_FreeSurface(surf);
    }
    
    void draw_circle(int cx, int cy, int radius, SDL_Color color) {
    int segments = std::fmax(6, radius);
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    float angle_step = 2.0f * M_PI / segments;

    for (int i = 0; i < segments; ++i) {
        float a0 = i * angle_step;
        float a1 = (i + 1) * angle_step;

        int x0 = cx + (int)(cosf(a0) * radius);
        int y0 = cy + (int)(sinf(a0) * radius);
        int x1 = cx + (int)(cosf(a1) * radius);
        int y1 = cy + (int)(sinf(a1) * radius);

        SDL_RenderDrawLine(renderer, x0, y0, x1, y1);
    }
}

void draw_line(Vec2 a, Vec2 b, SDL_Color color ) {
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    int x0 = (int) a.x;
    int y0 = (int) a.y;
    int x1 = (int) b.x;
    int y1 = (int) b.y;
    SDL_RenderDrawLine(renderer, x0, y0, x1, y1);
}

private:
    SDL_Window*   window = nullptr;
    SDL_Renderer* renderer = nullptr;
    TTF_Font*     font = nullptr;
    Uint32 fps_start_time = 0;
    Uint32 last_frame_time = 0;
    int    fps_frames = 0;
    bool is_valid = false;

    void update_delta_and_fps() {
        Uint32 now = SDL_GetTicks();

        fps_frames++;
        if (now - fps_start_time >= 1000) {
            fps = fps_frames * 1000.0f / (now - fps_start_time);
            fps_frames = 0;
            fps_start_time = now;
        }

        dt = (now - last_frame_time) / 1000.0f;
        last_frame_time = now;
    }

    void dbg_print_frames() {
        static Uint32 log_timer = 0;
        Uint32 now = SDL_GetTicks();
        if (now - log_timer >= 1000) {
            SDL_Log("PHYS updates/sec: %llu", update_count);
            SDL_Log("RENDER frames/sec: %llu  (shown FPS: %.1f)", draw_count, fps);
            SDL_Log("Ratio draw/update: %.1f", (float)draw_count / update_count);
            update_count = 0;
            draw_count = 0;
            log_timer = now;
        }
    }
};

} // namespace lvichki
