#include <cassert>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>
#include <map>
#include <chrono>

#include "font.h"
#include "File.h"
// #include "shader.h"
#include "parser.h"
#include "util.h"
#include "file_view.h"
#include "text_shader.h"

using namespace std::chrono;

int main(int argc, char *argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <file>\n";
        return -1;
    }

    Timeit init_timeit("Init");
    int screenWidth = 2560, screenHeight = 800;

    if (glfwInit() == 0) {
        std::cerr << "Failed to initialize GLFW\n";
        return -1;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow* window = glfwCreateWindow(screenWidth, screenHeight, "Text Hello", nullptr, nullptr);
    glfwMakeContextCurrent(window);

    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW\n";
        return -1;
    }
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    if (Font::init()) {
        std::cerr << "Failed to initialize FreeType\n";
        return -1;
    }

    init_timeit.~Timeit();

    Font font{48,
        "C:/Windows/Fonts/Consola.ttf",
        "C:/Windows/Fonts/Consolab.ttf",
        "C:/Windows/Fonts/Consolai.ttf",
        "C:/Windows/Fonts/Consolaz.ttf"
    };

    {
        Timeit load_timeit("Font Load");
        if (font.load()) {
            std::cerr << "Failed to load font\n";
            return -1;
        }
    }

    FileView file_view(argv[1]);
    {
        Timeit file_open_timeit("File Open");
        if (file_view.open() != 0) {
            std::cerr << "Failed to open file\n";
            return -1;
        }
    }

    // Orthographic projection matrix (0,0 top-left, width,height bottom-right)


    TextShader text_shader {font};
    if (text_shader.setup() != 0) {
        std::cerr << "Failed to compile shader\n";
        return -1;
    }
    text_shader.set_viewport(0, 0, screenWidth, screenHeight);

    // glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
    // glDrawArrays(GL_TRIANGLES, 0, glyph_count * 6);
    // glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );


    while (!glfwWindowShouldClose(window)) {
        Timeit frame("Frame");
        glClear(GL_COLOR_BUFFER_BIT);

        file_view.draw(text_shader);
        glfwSwapBuffers(window);
        frame.stop();
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}
