#include <SDL.h>
#include <SDL_ttf.h>
#include <emscripten.h>
#include <stdio.h>

void main_loop();
void physics_update(float dt);

#define _assert(n) if (!(n)) { \
	SDL_Log("Failure at %s:%d | SDL error:%s | TTF error:%s", __FILE__, __LINE__, SDL_GetError(), TTF_GetError());\
	SDL_ClearError(); \
}

#define _assert0(n) if ((n)) { \
	SDL_Log("Failure at %s:%d | SDL error:%s | TTF error:%s", __FILE__, __LINE__, SDL_GetError(), TTF_GetError());\
	SDL_ClearError(); \
}

SDL_Window* window;
SDL_Renderer* renderer;
TTF_Font* font;

float displacement_over_time[1200];
int current_time = 0;

int SDL_CreateWindowAndRenderer(
        int width, int height, Uint32 window_flags,
        SDL_Window **window, SDL_Renderer **renderer);

struct Motion {
	float y = 1.0f;
	float yp = 0.0f;
	float ypp = 0.0f;
};

struct spring_state_type {
	float mass { 5.0f };
	float spring_constant { 10.0f };
	float damping_constant { 5.0f };
	float unstretched_length { 1.0f };
	Motion motion;
} spring_state;

SDL_Color white_colour = { 255, 255, 255, 255};
SDL_Color black_colour = { 0, 0, 0, 255};

int main(int argc, char** argv)
{
	SDL_Log("Test");

	SDL_Init(SDL_INIT_EVERYTHING);
	TTF_Init();

	_assert(window = SDL_CreateWindow("Spring Simulation (c) Joel Kruger", 0, 0, 1280, 720, 0));
	_assert(renderer = SDL_CreateRenderer(window, 0, 0));

	_assert(font = TTF_OpenFont("resources/Monospace.ttf", 20));

	emscripten_set_main_loop(main_loop, 60, 1);
}

void main_loop()
{
	SDL_Event event;
	while (SDL_PollEvent(&event)) {
		switch (event.type) {
			case SDL_KEYDOWN:
				switch (event.key.keysym.sym) {
					case SDL_SCANCODE_M:
					 {
						double new_mass = EM_ASM_DOUBLE({
							return window.prompt("Mass (m):", "");
						});
						if (new_mass > 0.0)
							spring_state.mass = (float)new_mass;
						break;
					}
					case SDL_SCANCODE_S: case SDL_SCANCODE_K:
					{
						double new_spring_constant = EM_ASM_DOUBLE({
							return window.prompt("Spring constant (k):", "");
						});
						if (new_spring_constant > 0.0)
							spring_state.spring_constant = (float)new_spring_constant;
						break;
					}
					case SDL_SCANCODE_D: case SDL_SCANCODE_B:
					{
						double new_damping_constant = EM_ASM_DOUBLE({
							return window.prompt("Damping constant (B):", "");
						});
						if (new_damping_constant > 0.0)
							spring_state.damping_constant = (float)new_damping_constant;
						break;
					}
					case SDL_SCANCODE_RETURN: case SDL_SCANCODE_RETURN2:
					{
						current_time = 0;
						spring_state.motion = Motion();
						break;
					}
					case SDL_SCANCODE_R:
						current_time = 0;
						spring_state = spring_state_type();
						break;
					default:
						;
				}
		}
	}
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
	SDL_RenderClear(renderer);

	//sprintf(text_buffer, "Mass: %f kg\nSpring constant %f N/M\nUnstretched length %f M", \
		spring_state.mass, spring_state.spring_constant, spring_state.unstretched_length);
	
	char text_buf[128];

	auto write_string = [](const char* text, int y) {
		SDL_Surface* text_surface;
		SDL_Texture* text_texture;
		SDL_Rect rect = {0, y, 0, 0};
		
		_assert(text_surface = TTF_RenderText_Shaded(font, text, white_colour, black_colour));
		_assert(text_texture = SDL_CreateTextureFromSurface(renderer, text_surface));

		SDL_QueryTexture(text_texture, NULL, NULL, &rect.w, &rect.h);
	
		_assert0(SDL_RenderCopy(renderer, text_texture, NULL, &rect));

		SDL_FreeSurface(text_surface);
	};

	sprintf(text_buf, "Mass (m): %f kg", \
		spring_state.mass);
	write_string(text_buf, 0);
	
	sprintf(text_buf, "Spring constant (k): %f N/M", \
		spring_state.spring_constant);
	write_string(text_buf, 24);

	sprintf(text_buf, "Damping constant (B): %f kg/s", \
		spring_state.damping_constant);
	write_string(text_buf, 48);

	sprintf(text_buf, "y = %f", \
		spring_state.motion.y);
	write_string(text_buf, 96);

	sprintf(text_buf, "dy/dt = %f", \
		spring_state.motion.yp);
	write_string(text_buf, 120);

	sprintf(text_buf, "d\xB2y/dt\xB2 = %f", \
		spring_state.motion.ypp);
	write_string(text_buf, 144);
  
	for (int i = 0; i < 10; i++)
		physics_update(1.0f / 600.0f);
	
	if (current_time < 1200)
		current_time++;

	SDL_Rect rect = {620, (int)(spring_state.motion.y * 200.0f + 300.0f), 40, 40};
	SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
	SDL_RenderDrawRect(renderer, &rect);
	SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);
	SDL_RenderDrawLine(renderer, 640, 0, 640, (int)(spring_state.motion.y * 200.0f + 300.0f));

	SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
	SDL_RenderDrawLine(renderer, 40, 360, 40, 720);
	SDL_RenderDrawLine(renderer, 40, 540, 1240, 540);

	SDL_SetRenderDrawColor(renderer, 75, 255, 75, 255);
	for (int i = 0; i < current_time; i++) {
		SDL_RenderDrawPoint(renderer, 40 + i, (int)(540.0f + displacement_over_time[i] * 100.0f));
	}
}

void physics_update(float dt)
{
	float ypp = (- spring_state.spring_constant * spring_state.motion.y - spring_state.damping_constant * spring_state.motion.yp) / spring_state.mass;
	float yp = (- spring_state.spring_constant * spring_state.motion.y - spring_state.mass * spring_state.motion.ypp) / spring_state.damping_constant;
	
	spring_state.motion.y += yp * dt;
	spring_state.motion.yp += ypp * dt;

	spring_state.motion.ypp = (- spring_state.spring_constant * spring_state.motion.y - spring_state.damping_constant * spring_state.motion.yp) / spring_state.mass;

	if (current_time < 1200)
		displacement_over_time[current_time] = spring_state.motion.y;
}