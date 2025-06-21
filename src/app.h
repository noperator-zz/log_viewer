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

class App : public Widget {
	static inline std::unique_ptr<App> app_ {};

	std::unique_ptr<Font> font_ {};
	std::unique_ptr<Window> window_ {};
	glm::ivec2 fb_size_ {};
	std::vector<std::unique_ptr<FileView>> file_views_;

	App();

	int start();
	int add_file(const char *path);
	void draw() override;
	int run();

	void file_worker();

	void on_resize() override;
	bool on_scroll(glm::ivec2 offset, Window::KeyMods mods) override;
	bool on_drop(int path_count, const char* paths[]) override;

	FileView& active_file_view();

public:
	~App();
	static void create(int argc, char *argv[]);
};
