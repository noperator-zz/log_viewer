#pragma once

#include <memory>
#include <vector>
#include "file_view.h"
#include "text_shader.h"
#include "widget.h"
#include "window.h"

struct GLFWwindow;
class AppWindow;

// TODO Need another class to hold the window (or be a subclass of WIndow, so it can track FPS during window_refresh_cb)
//  instead of App being both the root widget and the window manager.
class App : public Widget {
	friend class AppWindow;

	std::vector<std::unique_ptr<FileView>> file_views_ {};
	std::unique_ptr<Font> font_ {};

	App(AppWindow &window);

	[[nodiscard]] int start(int argc, char *argv[]);
	[[nodiscard]] int add_file(const char *path);
	[[nodiscard]] int run();

	void file_worker();

	void on_resize() override;
	bool on_key(int key, int scancode, int action, Window::KeyMods mods) override;
	bool on_scroll(glm::ivec2 offset, Window::KeyMods mods) override;
	bool on_drop(int path_count, const char* paths[]) override;

	FileView& active_file_view();

	void update() override;
	void draw() override;

	App(const App &) = delete;
	App &operator=(const App &) = delete;
	App(App &&) = delete;
	App &operator=(App &&) = delete;

public:
	~App();
};
