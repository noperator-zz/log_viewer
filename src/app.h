#pragma once

#include <memory>
#include <vector>
#include "file_view.h"

struct GLFWwindow;

class App {
	std::unique_ptr<Font> font_ {};
	std::unique_ptr<TextShader> text_shader_ {};
	GLFWwindow *window {};
	int screenWidth = 2500, screenHeight = 800;

	std::vector<std::unique_ptr<FileView>> file_views_;

public:
	App() = default;

	int start();
	int add_file(const char *path);
	int run();
};
