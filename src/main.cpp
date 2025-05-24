#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include <iostream>
#include <vector>
#include <map>

#include "fragment.glsl.h"
#include "vertex.glsl.h"

struct Glyph {
    int width, height;
    int bearingX, bearingY;
    int advanceX;
    float u1, v1, u2, v2;
};

struct Vertex {
    float x, y;
    float u, v;
    float fg[4];
    float bg[4];
};

GLuint compile_shader(GLenum type, const char* src) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);
    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char log[512];
        glGetShaderInfoLog(shader, 512, nullptr, log);
        std::cerr << "Shader compile error: " << log << "\n";
    }
    return shader;
}

GLuint create_program(const char* vs, const char* fs) {
    GLuint v = compile_shader(GL_VERTEX_SHADER, vs);
    GLuint f = compile_shader(GL_FRAGMENT_SHADER, fs);
    GLuint program = glCreateProgram();
    glAttachShader(program, v);
    glAttachShader(program, f);
    glLinkProgram(program);
    glDeleteShader(v);
    glDeleteShader(f);
    return program;
}

int main() {
    if (!glfwInit()) return -1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow* window = glfwCreateWindow(800, 600, "Text Hello", nullptr, nullptr);
    glfwMakeContextCurrent(window);
    glewInit();

    FT_Library ft;
    FT_Init_FreeType(&ft);
    FT_Face face;
    FT_New_Face(ft, "DejaVuSansMono.ttf", 0, &face);
    FT_Set_Pixel_Sizes(face, 0, 48);

    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, 512, 512, 0, GL_RED, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    std::map<char, Glyph> glyphs;
    int penX = 0, penY = 0, rowH = 0;

    for (char c = 32; c < 127; ++c) {
        if (FT_Load_Char(face, c, FT_LOAD_RENDER)) continue;
        FT_GlyphSlot g = face->glyph;

        if (penX + g->bitmap.width >= 512) {
            penX = 0;
            penY += rowH;
            rowH = 0;
        }

        glTexSubImage2D(GL_TEXTURE_2D, 0, penX, penY,
            g->bitmap.width, g->bitmap.rows,
            GL_RED, GL_UNSIGNED_BYTE, g->bitmap.buffer);

        glyphs[c] = {
            g->bitmap.width, g->bitmap.rows,
            g->bitmap_left, g->bitmap_top,
            g->advance.x >> 6,
            (float)penX / 512, (float)penY / 512,
            (float)(penX + g->bitmap.width) / 512,
            (float)(penY + g->bitmap.rows) / 512
        };

        penX += g->bitmap.width + 1;
        rowH = std::max(rowH, (int)g->bitmap.rows);
    }

    std::vector<Vertex> verts;
    float x = -0.95f, y = 0.0f;
    const char* msg = "Hello, GPU Text!";

    for (const char* p = msg; *p; ++p) {
        Glyph& g = glyphs[*p];
        float xpos = x + g.bearingX / 400.0f;
        float ypos = y - g.bearingY / 300.0f;
        float w = g.width / 400.0f;
        float h = g.height / 300.0f;

        float fg[4] = {1, 1, 1, 1}, bg[4] = {0, 0, 0, 1};
        verts.push_back({xpos, ypos, g.u1, g.v1, fg[0], fg[1], fg[2], fg[3], bg[0], bg[1], bg[2], bg[3]});
        verts.push_back({xpos + w, ypos, g.u2, g.v1, fg[0], fg[1], fg[2], fg[3], bg[0], bg[1], bg[2], bg[3]});
        verts.push_back({xpos + w, ypos + h, g.u2, g.v2, fg[0], fg[1], fg[2], fg[3], bg[0], bg[1], bg[2], bg[3]});
        verts.push_back({xpos, ypos + h, g.u1, g.v2, fg[0], fg[1], fg[2], fg[3], bg[0], bg[1], bg[2], bg[3]});
        x += g.advanceX / 400.0f;
    }

    std::vector<GLuint> indices;
    for (int i = 0; i < verts.size(); i += 4) {
        indices.push_back(i); indices.push_back(i+1); indices.push_back(i+2);
        indices.push_back(i); indices.push_back(i+2); indices.push_back(i+3);
    }

    GLuint vao, vbo, ebo;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(Vertex), verts.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), indices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, x));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, u));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, fg));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, bg));
    glEnableVertexAttribArray(3);


    GLuint program = create_program(vertex_glsl, fragment_glsl);
    glUseProgram(program);
    glUniform1i(glGetUniformLocation(program, "atlas"), 0);

    while (!glfwWindowShouldClose(window)) {
        glClear(GL_COLOR_BUFFER_BIT);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, tex);
        glBindVertexArray(vao);
        glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}
