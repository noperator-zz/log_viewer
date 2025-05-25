#include <cassert>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>
#include <map>
#include <chrono>

#include "fragment.glsl.h"
#include "vertex.glsl.h"

#include "font.h"
#include "File.h"
#include "shader.h"
#include "parser.h"

using namespace std::chrono;

struct Vertex {
    float x, y;
    float u, v;
    float fg[4];
    float bg[4];
};

struct Timeit {
    const char* name;
    time_point<high_resolution_clock> start;
    time_point<high_resolution_clock> end {};
    Timeit(const char *name) : name(name), start(high_resolution_clock::now()) {}

    void stop() {
        end = high_resolution_clock::now();
    }

    ~Timeit() {
        if (end == time_point<high_resolution_clock>()) {
            end = high_resolution_clock::now();
        }
        static constexpr const char* units[] {"ns", "us", "ms", "s"};
        auto duration = duration_cast<nanoseconds>(end - start).count();
        size_t unit_idx = 0;
        while (duration > 10000 && unit_idx < 3) {
            duration /= 1000;
            unit_idx++;
        }
        std::cout << "Timeit [" << name << "]: " << duration << " " << units[unit_idx] << "\n";
    }
};

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

    File file(argv[1]);
    {
        Timeit file_open_timeit("File Open");
        if (file.open() != 0) {
            std::cerr << "Failed to open file\n";
            return -1;
        }
        if (file.mmap() != 0) {
            std::cerr << "Failed to mmap file\n";
            return -1;
        }
    }

    std::vector<size_t> line_starts;

    {
        size_t num_threads = 1;
        size_t total_size = file.size();
        std::cout << "Parsing " << total_size / 1024 / 1024 << " MiB\n";
        Timeit parse_timeit("Parse");
        line_starts = find_newlines(file.data(), total_size, num_threads);
        parse_timeit.stop();
        std::cout << "Found " << line_starts.size() << " newlines\n";
    }

    struct __attribute__((packed)) CharStyle {
        uint8_t bold : 1;
        uint8_t italic : 1;
        uint8_t underline : 1;
        uint8_t strikethrough : 1;
    };

    GLuint vao, vbos[2];
    glGenVertexArrays(1, &vao);
    glGenBuffers(2, vbos);
    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbos[0]);
    glVertexAttribIPointer(0, 1, GL_UNSIGNED_INT, 1, (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribDivisor(0, 1);

    glBindBuffer(GL_ARRAY_BUFFER, vbos[1]);
    glVertexAttribIPointer(1, 1, GL_UNSIGNED_INT, sizeof(CharStyle), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribDivisor(1, 1);

    float scroll_offset[2] = {0.0f, 0.0f};

    Shader<vertex_glsl, fragment_glsl> shader {};
    if (shader.compile() != 0) {
        std::cerr << "Failed to compile shader\n";
        return -1;
    }

    set_uniform(1i, shader, "atlas", 0);
    set_uniform(1i, shader, "bearing_table", 1);

    // Set static uniforms
    set_uniform(1i, shader, "glyph_width", font.size);
    set_uniform(1i, shader, "glyph_height", font.size);
    set_uniform(1i, shader, "atlas_cols", font.num_glyphs);
    set_uniform(1i, shader, "line_height", font.size);

    // Orthographic projection matrix (0,0 top-left, width,height bottom-right)
    float w = (float)screenWidth, h = (float)screenHeight;
    float ortho[16] = {
        2.0f / w,  0.0f,       0.0f, 0.0f,
        0.0f,     -2.0f / h,   0.0f, 0.0f,
        0.0f,      0.0f,      -1.0f, 0.0f,
       -1.0f,      1.0f,       0.0f, 1.0f
    };

    set_uniform(Matrix4fv, shader, "u_proj", 1, GL_FALSE, ortho);

    set_uniform(2fv, shader, "scroll_offset", 2, scroll_offset);

    glBindBuffer(GL_ARRAY_BUFFER, vbos[0]);
    glBufferData(GL_ARRAY_BUFFER, std::min(file.size(), 1024 * 1024ULL), file.data(), GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, vbos[1]);
    glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, font.tex_atlas());
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, font.tex_bearing());

    glBindVertexArray(vao);

    // glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
    // glDrawArrays(GL_TRIANGLES, 0, glyph_count * 6);
    // glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );

    while (!glfwWindowShouldClose(window)) {
        Timeit frame("Frame");
        glClear(GL_COLOR_BUFFER_BIT);

        Timeit draw("Draw");
        size_t line_offset = 0;
        size_t visible_lines = 10;
        for (size_t line_idx = line_offset; line_idx < line_offset + visible_lines; line_idx++) {
            auto line = line_starts[line_idx];
            auto next_line = line_starts[line_idx+1];
            set_uniform(1i, shader, "line_idx", line_idx);
            glDrawArraysInstancedBaseInstance(GL_TRIANGLE_STRIP, 0, 4, next_line - line, line);
        }
        draw.stop();
        glfwSwapBuffers(window);
        frame.stop();
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}
