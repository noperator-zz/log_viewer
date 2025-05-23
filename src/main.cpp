#define SDL_MAIN_HANDLED
#include <chrono>
#include <fstream>
#include <iostream>
#include <SDL.h>
#include <SDL_ttf.h>
#include <string.h>
#include <vector>

using namespace std::chrono;

static constexpr ssize_t X_SCROLL_MULT = 50;
static constexpr ssize_t LINE_SCROLL_MULT = 5;

static int WINDOW_W = 1200;
static int WINDOW_H = 600;

SDL_Window* window {};
SDL_Renderer* renderer {};
// SDL_Texture* texture {};
static inline TTF_Font *font {};
std::vector<SDL_Texture*> lines {};
bool done {};
ssize_t x_offset {};
ssize_t line_offset {};
bool ctrl_held {};
bool shift_held {};

bool init() {
	if (SDL_Init(SDL_INIT_VIDEO) != 0) {
		return false;
	}

	window = SDL_CreateWindow("Log Viewer", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINDOW_W, WINDOW_H, SDL_WINDOW_SHOWN);

	if (window == nullptr) {
		return false;
	}

	SDL_SetWindowResizable(window, SDL_TRUE);
	SDL_StartTextInput();

	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

	if (renderer == nullptr) {
		return false;
	}

	SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

	// texture = SDL_CreateTexture(renderer,
	// 	SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING,
	// 	WINDOW_W, WINDOW_H);

	auto ret = TTF_Init();
	if (ret != 0) {
		std::cerr << "TTF_Init failed: " << TTF_GetError() << std::endl;
		return false;
	}
#ifdef WIN32
	font = TTF_OpenFont("C:/Windows/Fonts/consola.ttf", 16);
#else
	font = TTF_OpenFont("/usr/share/fonts/truetype/ubuntu/Ubuntu-R.ttf", 16);
#endif
	if (!font) {
		std::cerr << "Failed to load font: " << TTF_GetError() << std::endl;
		return false;
	}
	return true;
}

void quit() {
	// SDL_DestroyTexture(texture);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();
}


void handle_events()
{
	SDL_Event event;

	while (SDL_PollEvent(&event)) {

		switch (event.type) {
			case SDL_QUIT: {
				done = true;
				break;
			}

			case SDL_WINDOWEVENT: {
				switch (event.window.event) {
					case SDL_WINDOWEVENT_RESIZED:
						WINDOW_H = event.window.data2;
						WINDOW_W = event.window.data1;
						break;
				}
				break;
			}

			case SDL_MOUSEWHEEL: {
				if (ctrl_held && shift_held) {
				} else if (shift_held) {
					x_offset = std::max(0LL, x_offset - event.wheel.y * X_SCROLL_MULT);
				} else if (ctrl_held) {
				} else {
					line_offset = std::max(0LL, line_offset - event.wheel.y * LINE_SCROLL_MULT);
				}
				break;
			}

			case SDL_MOUSEMOTION: {
				// world_mouse.x = ToWorldX(event.motion.x);
				// world_mouse.y = ToWorldY(event.motion.y);
				break;
			}

			case SDL_MOUSEBUTTONDOWN: {
				switch (event.button.button) {
					case SDL_BUTTON_LEFT:
						// mouse_left_held = true;
						break;
				}
				break;
			}

			case SDL_MOUSEBUTTONUP: {
				switch (event.button.button) {
					case SDL_BUTTON_LEFT:
						// mouse_left_held = false;
						break;
					case SDL_BUTTON_RIGHT:
						break;
				}
				break;
			}

			case SDL_KEYDOWN: {
				switch (event.key.keysym.sym) {
					case SDLK_LCTRL:
					case SDLK_RCTRL:
						ctrl_held = true;
						break;
					case SDLK_LSHIFT:
					case SDLK_RSHIFT:
						shift_held = true;
						break;
				}
				break;
			}

			case SDL_KEYUP: {
//				bool handled = true;
				switch (event.key.keysym.sym) {
					case SDLK_LCTRL:
					case SDLK_RCTRL:
						ctrl_held = false;
						break;
					case SDLK_LSHIFT:
					case SDLK_RSHIFT:
						shift_held = false;
						break;

					default: {
						break;
					}
				}

				break;
			}

			case SDL_TEXTINPUT: {
				// command += event.text.text;
				break;
			}

			default: {
				break;
			}
		}
	}
}

SDL_Texture* render_text(const char* text, SDL_Color color, TTF_Font* font) {
	SDL_Surface* surface = TTF_RenderUTF8_Blended(font, text, color);
	if (!surface) {
		// std::cerr << "Failed to create surface: " << TTF_GetError() << std::endl;
		return nullptr;
	}

	SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
	SDL_FreeSurface(surface);
	return texture;
}

void render() {
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
	SDL_RenderClear(renderer);

	int y = 0;
	for (size_t i = line_offset; i < lines.size(); ++i) {
		int tex_w, tex_h;
		SDL_QueryTexture(lines[i], nullptr, nullptr, &tex_w, &tex_h);
		SDL_Rect dst = {(int)-x_offset, y, tex_w, tex_h};
		SDL_RenderCopy(renderer, lines[i], nullptr, &dst);
		y += tex_h;
		if (y > WINDOW_H) break;
	}

	SDL_RenderPresent(renderer);
}

bool load(const char* filename) {
	auto mybuffer = std::make_unique<char[]>(1024 * 1024);
	std::ifstream file;
	file.rdbuf()->pubsetbuf(mybuffer.get(), 1024 * 1024);
	file.open(filename, std::ios::in | std::ios::binary);
	if (!file.is_open()) {
		std::cerr << "Failed to open file: " << filename << ": " << strerror(errno) << std::endl;
		return false;
	}

	std::string line {};
	char c {};
	while (file.get(c)) {
		if (c == '\n') {
			c = 0;
			SDL_Texture* tex = nullptr;
			// tex = render_text(line.c_str(), {255, 255, 255, 255}, font);
			// if (tex) {
				lines.push_back(tex);
				if (lines.size() % 10000 == 0) {
					std::cerr << "Loaded " << lines.size() << " lines" << std::endl;
				}
			// }
			line.clear();
		}
		else if (c == '\r') {
			continue;
		} else {
			line += c;
		}
	}
	return true;
}

int main(int argc, char *argv[]) {
	init();

	if (argc < 2) {
		std::cerr << "Usage: " << argv[0] << " <log file>" << std::endl;
		return 1;
	}

	// load log file
    auto start = high_resolution_clock::now();
	if (!load(argv[1])) {
		std::cerr << "Failed to load log file" << std::endl;
		return 1;
	}
	auto duration = duration_cast<milliseconds>(high_resolution_clock::now() - start);
	std::cerr << "Loaded " << lines.size() << " lines in " << duration.count() << "ms" << std::endl;

	auto last_stat = high_resolution_clock::now();

	size_t frames {};
	microseconds frame_time {};
    while (!done) {
    	auto start = high_resolution_clock::now();
    	render();
    	frame_time += duration_cast<microseconds>(high_resolution_clock::now() - start);
    	frames++;

    	auto diff = start - last_stat;
    	if (duration_cast<milliseconds>(diff).count() >= 1000) {
    		microseconds per_frame = duration_cast<microseconds>(frame_time) / frames;
			std::cerr << "FPS: " << frames << ", " << per_frame.count() << "us/frame" << std::endl;
			frames = 0;
    		frame_time = {};
			last_stat += diff;
		}

    	handle_events();
        SDL_Delay(10);
    }

	quit();
    return 0;
}
