#pragma once

#include <memory>
#include <vector>
#include <thread>
#include "file_view.h"
#include "../shaders/text_shader.h"
#include "gp_shader.h"
#include "widget.h"
#include "window.h"

struct GLFWwindow;

// TODO Need another class to hold the window (or be a subclass of WIndow, so it can track FPS during window_refresh_cb)
//  instead of App being both the root widget and the window manager.
class App : public Widget {
	static inline std::unique_ptr<App> app_ {};

	std::unique_ptr<Font> font_ {};
	std::unique_ptr<Window> window_ {};
	glm::ivec2 fb_size_ {};
	std::vector<std::unique_ptr<FileView>> file_views_;

	App();

	int start();
	int add_file(const char *path);
	int run();

	void file_worker();

	void on_resize() override;
	bool on_key(int key, int scancode, int action, Window::KeyMods mods) override;
	bool on_scroll(glm::ivec2 offset, Window::KeyMods mods) override;
	bool on_drop(int path_count, const char* paths[]) override;

	FileView& active_file_view();

	void update() override;
	void draw() override;

public:
	~App();
	static void create(int argc, char *argv[]);
};
