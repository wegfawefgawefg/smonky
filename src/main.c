/*
    make it draw by center not top corner of rect

    convert squares to surface
        - necessary for rotation anyways

    scale
        - bigger as it gets older
    rotate
    texture
*/


#include <stdio.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <cglm/cglm.h>

static const int SCALE = 100;
static const int WINDOW_WIDTH = 16 * SCALE;
static const int WINDOW_HEIGHT = 9 * SCALE;

void draw_text(
        SDL_Renderer *renderer, 
        TTF_Font *font, 
        char *text,
        SDL_Color color, 
        int x, int y) {
    SDL_Surface *text_surface = TTF_RenderText_Solid(font, text, color);
    SDL_Texture *text_texture = SDL_CreateTextureFromSurface(renderer, text_surface);
    SDL_Rect text_rect = {x, y, text_surface->w, text_surface->h};
    SDL_RenderCopy(renderer, text_texture, NULL, &text_rect);
    SDL_FreeSurface(text_surface);
    SDL_DestroyTexture(text_texture);
}

struct dimensions {
    int width, height;
};

void load_image(
        SDL_Renderer *renderer, 
        SDL_Texture **texture, 
        char *path) {
    SDL_Surface *surface = SDL_LoadBMP(path);
    *texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);
}

const float rand_range(float min, float max) {
    return min + (max - min) * (rand() % RAND_MAX / (float)RAND_MAX);
}

const vec2 emitter = {
    WINDOW_WIDTH / 4,
    WINDOW_HEIGHT / 2,
};
struct smoke {
    vec2 pos;
    vec2 vel;
    float angle;
    float angle_vel;
    float size;
    float size_vel;
    float life_span;
    float life;
};
static const int NUM_SMOKES = 1024;

static const float SMOKE_SPEED = 1000.0f;
static const float SMOKE_LIFE = 5.0f;
struct smoke new_smoke(vec2 dir)
{
    // randomize speed
    float speed = rand_range(SMOKE_SPEED / 2.0f, SMOKE_SPEED);

    // calculate velocity
    vec2 vel;

    //  // randomize direction
    vec2 vel_rand_component;
    vel_rand_component[0] = rand_range(-SMOKE_SPEED, SMOKE_SPEED);
    vel_rand_component[1] = rand_range(-SMOKE_SPEED, SMOKE_SPEED);
    glm_vec2_normalize(vel_rand_component);
    glm_vec2_scale(vel_rand_component, 1.0/4.0, vel_rand_component);
    glm_vec2_add(vel, vel_rand_component, vel);

    glm_vec2_add(dir, vel, vel);
    glm_vec2_scale_as(vel, speed, vel);

    struct smoke new_smoke;
    new_smoke.pos[0] = emitter[0];
    new_smoke.pos[1] = emitter[1];
    new_smoke.vel[0] = vel[0];
    new_smoke.vel[1] = vel[1];
    new_smoke.angle = rand_range(0.0f, 2.0f * M_PI);
    new_smoke.angle_vel = rand_range(-360, 360);
    new_smoke.size = rand_range(0.0f, 0.5f);
    new_smoke.size_vel = rand_range(0, 0.5f);
    new_smoke.life_span = rand_range(SMOKE_LIFE / 2.0f, SMOKE_LIFE);
    new_smoke.life = new_smoke.life_span;
    return new_smoke;
}

int main(int argc, char **argv)
{
    SDL_Window *window;
    SDL_Renderer *renderer;

    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 16);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 16);
    SDL_Init(SDL_INIT_EVERYTHING);
    TTF_Init();

    window = SDL_CreateWindow("smonky", 
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 
        WINDOW_WIDTH, WINDOW_HEIGHT,
        SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
    // SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN);
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_ShowCursor(SDL_DISABLE);

    // Load font
    TTF_Font *font = TTF_OpenFont("./assets/FreeSans.ttf", 24);
    if (font == NULL) {
        printf("Failed to load font: %s\n", TTF_GetError());
        return 1;
    }

    // load smoke
    SDL_Texture *smoke_texture = IMG_LoadTexture(renderer, "./assets/smoke.png");
    int width, height;
    SDL_QueryTexture(smoke_texture, NULL, NULL, &width, &height);
    const int smoke_scale = 4;
    const struct dimensions smoke_shape = {
        64 * smoke_scale, 
        64 * smoke_scale,
    };

    struct smoke smokes[NUM_SMOKES];
    for(int i = 0; i < NUM_SMOKES; i++)
    {
        vec2 dir;
        dir[0] = 0;
        dir[1] = 0;
        struct smoke a_smoke = new_smoke(dir);
        a_smoke.life = 0.0;
        smokes[i] = a_smoke;
    }

    // setup for delta time
    Uint64 frame_time_now = SDL_GetPerformanceCounter();
    Uint64 frame_time_last = 0;
    double dt = 0;

    SDL_Event event;
    while (1) {
        // update delta time
        frame_time_last = frame_time_now;
        frame_time_now = SDL_GetPerformanceCounter();
        dt = (double)((frame_time_now - frame_time_last) / (double)SDL_GetPerformanceFrequency() );
        const double fps = 1.0 / dt;   

        // events
        SDL_PollEvent(&event);
        if (event.type == SDL_QUIT) {
            break;
        }

        // keys
        const Uint8 *keys = SDL_GetKeyboardState(NULL);
        if (keys[SDL_SCANCODE_Q]) {
            break;
        }

        // clear screen
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        // draw emitter
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderDrawPoint(renderer, emitter[0], emitter[1]);

        // draw mouse location
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        int x, y;
        SDL_GetMouseState(&x, &y);
        SDL_Rect mouse_rect = {x, y, 8, 8};
        SDL_RenderFillRect(renderer, &mouse_rect);

        // draw line from emitter to mouse
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderDrawLine(renderer, emitter[0], emitter[1], x, y);

        vec2 to_mouse;
        to_mouse[0] = x - emitter[0];
        to_mouse[1] = y - emitter[1];
        glm_vec2_normalize(to_mouse);

        // update smokes
        for(int i = 0; i < NUM_SMOKES; i++) {
            // multiply velocity by delta time
            vec2 dtvel;
            glm_vec2_scale(smokes[i].vel, dt, dtvel);
            // add that to position
            vec2 new_pos;
            glm_vec2_add(smokes[i].pos, dtvel, new_pos);
            smokes[i].pos[0] = new_pos[0];
            smokes[i].pos[1] = new_pos[1];

            // smoke antigravity
            smokes[i].pos[1] += -20.0 * dt;

            smokes[i].angle = fmod(smokes[i].angle + smokes[i].angle_vel * dt, 360.0);
            smokes[i].size = fmax(0.0, smokes[i].size + smokes[i].size_vel * dt);

            // shrink velocity
            glm_vec2_scale(smokes[i].vel, 0.98, smokes[i].vel);
            smokes[i].angle_vel *= 0.98;

            // age out
            smokes[i].life -= dt;
            if (smokes[i].life < 0) {
                smokes[i] = new_smoke(to_mouse);
            }

            // bounds check
            if ((smokes[i].pos[1] < 0)  ||
                (smokes[i].pos[1] > WINDOW_HEIGHT)  ||
                (smokes[i].pos[0] < 0)  ||
                (smokes[i].pos[0] > WINDOW_WIDTH)) 
            {
                smokes[i] = new_smoke(to_mouse);
            }

            // draw smoke
            //  // col
            const struct smoke s = smokes[i];
            float a = s.life / s.life_span;
            a = a * 0.5;
            a = a * a;
            a = a * 255;
            int alpha = (int)a;
            // SDL_SetRenderDrawColor(renderer, 255, 255, 255, col);    

            //  // pos
            vec2 scale = {
                smoke_shape.width * smokes[i].size,
                smoke_shape.height * smokes[i].size
            };
            const SDL_Rect rect = {
                (int)smokes[i].pos[0] - (scale[0] / 2),
                (int)smokes[i].pos[1] - (scale[1] / 2),
                (int)scale[0],
                (int)scale[1]
            };
            // SDL_RenderCopy(renderer, smoke_texture, NULL, &rect);
            SDL_SetTextureAlphaMod(smoke_texture, alpha);
            const float angle = smokes[i].angle;
            SDL_RenderCopyEx(renderer, smoke_texture, NULL, &rect, angle, NULL, SDL_FLIP_NONE);
            // SDL_RenderFillRect(renderer, &rect);
        }




        // draw fps
        const SDL_Color color = {255, 255, 255, 255};
             
        char fps_str[32];
        sprintf(fps_str, "%f", fps);
        draw_text(renderer, font, fps_str, color, 0, 0);




        SDL_RenderPresent(renderer);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();

    return 0;

}
