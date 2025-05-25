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
    FT_Library ft;
    FT_Face face;

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
    if (FT_Init_FreeType(&ft) != 0) {
        std::cerr << "Failed to initialize FreeType\n";
        return -1;
    }
    if (FT_New_Face(ft, "C:/Windows/Fonts/Consola.ttf", 0, &face) != 0) {
        std::cerr << "Failed to load font\n";
        return -1;
    }
    if (FT_Set_Pixel_Sizes(face, 0, 48) != 0) {
        std::cerr << "Failed to set font size\n";
        return -1;
    }


    static constexpr int glyph_width = 32;
    static constexpr int glyph_height = 48;
    static constexpr int atlas_cols = 127;
    static constexpr int atlas_width = glyph_width * atlas_cols;

    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, atlas_width, glyph_height, 0, GL_RED, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    Glyph glyphs[atlas_cols] {};
    int penX = 0;

    for (char c = 0; c < atlas_cols; ++c) {
        if (FT_Load_Char(face, c, FT_LOAD_RENDER)) continue;
        FT_GlyphSlot g = face->glyph;

        printf("%c - X: %3d, W: %2d, H: %2d, Bx: %2d, By: %2d, Ax: %2d\n",
            c, penX,
            g->bitmap.width, g->bitmap.rows,
            g->bitmap_left, g->bitmap_top,
            g->advance.x >> 6);

        glTexSubImage2D(GL_TEXTURE_2D, 0, penX, 0,
            g->bitmap.width, g->bitmap.rows,
            GL_RED, GL_UNSIGNED_BYTE, g->bitmap.buffer);

        glyphs[c] = {
            g->bitmap.width, g->bitmap.rows,
            g->bitmap_left, g->bitmap_top,
            g->advance.x >> 6,
            (float)penX / atlas_width,
            0,
            (float)(penX + g->bitmap.width) / atlas_width,
            (float)(0 + g->bitmap.rows) / glyph_height,
        };

        // penX += g->bitmap.width;
        penX += glyph_width;
    }

    // std::vector<Vertex> verts;
    // float x = 20, y = 100;
    // const char* msg = "Hello, world!";
    //
    // for (const char* p = msg; *p; ++p) {
    //     Glyph& g = glyphs[*p];
    //     float xpos = x + g.bearingX;
    //     float ypos = y - g.bearingY;
    //     float w = g.width;
    //     float h = g.height;
    //
    //     float fg[4] = {1, 1, 1, 1}, bg[4] = {0, 0, 0, 1};
    //     verts.push_back({xpos, ypos, g.u1, g.v1, fg[0], fg[1], fg[2], fg[3], bg[0], bg[1], bg[2], bg[3]});
    //     verts.push_back({xpos + w, ypos, g.u2, g.v1, fg[0], fg[1], fg[2], fg[3], bg[0], bg[1], bg[2], bg[3]});
    //     verts.push_back({xpos + w, ypos + h, g.u2, g.v2, fg[0], fg[1], fg[2], fg[3], bg[0], bg[1], bg[2], bg[3]});
    //     verts.push_back({xpos, ypos + h, g.u1, g.v2, fg[0], fg[1], fg[2], fg[3], bg[0], bg[1], bg[2], bg[3]});
    //     x += g.advanceX;
    // }
    //
    // std::vector<GLuint> indices;
    // for (int i = 0; i < verts.size(); i += 4) {
    //     indices.push_back(i); indices.push_back(i+1); indices.push_back(i+2);
    //     indices.push_back(i); indices.push_back(i+2); indices.push_back(i+3);
    //     // indices.push_back(i+2); indices.push_back(i+1); indices.push_back(i);
    //     // indices.push_back(i+3); indices.push_back(i+2); indices.push_back(i);
    // }

    // verts = {
    //     {-1, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 1},
    //     {1, 0, 1, 0, 1, 1, 1, 1, 0, 0, 0, 1},
    //     {1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1},
    //     {-1, 1, 0, 1, 1, 1, 1, 1, 0, 0, 0, 1},
    // };
    // indices = {0, 1, 2, 0, 2, 3};

    // const char* msg = "Hello, GPU Text!";
    struct CharData {
        uint8_t y;
    };
    const char msg[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    CharData datas[sizeof(msg)] = {
        {1}, {2},
    };

    // GLuint vao, vbo, ebo;
    // glGenVertexArrays(1, &vao);
    // glGenBuffers(1, &vbo);
    // glGenBuffers(1, &ebo);
    //
    // glBindVertexArray(vao);
    // glBindBuffer(GL_ARRAY_BUFFER, vbo);
    // glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(Vertex), verts.data(), GL_STATIC_DRAW);
    // glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    // glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), indices.data(), GL_STATIC_DRAW);
    //
    // glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, x));
    // glEnableVertexAttribArray(0);
    // glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, u));
    // glEnableVertexAttribArray(1);
    // glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, fg));
    // glEnableVertexAttribArray(2);
    // glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, bg));
    // glEnableVertexAttribArray(3);

    GLuint vao, vbos[2];
    glGenVertexArrays(1, &vao);
    glGenBuffers(2, vbos);
    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbos[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(msg), msg, GL_DYNAMIC_DRAW);
    glVertexAttribIPointer(0, 1, GL_UNSIGNED_INT, 1, (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribDivisor(0, 1);

    glBindBuffer(GL_ARRAY_BUFFER, vbos[1]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(msg) * sizeof(CharData), datas, GL_DYNAMIC_DRAW);
    glVertexAttribIPointer(1, 1, GL_UNSIGNED_INT, sizeof(CharData), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribDivisor(1, 1);

    // GLuint program = create_program(vertex_glsl, fragment_glsl);
    // glUseProgram(program);
    // glUniform1i(glGetUniformLocation(program, "atlas"), 0);

    GLuint program = create_program(vertex_glsl, fragment_glsl);
    glUseProgram(program);
    glUniform1i(glGetUniformLocation(program, "atlas"), 0);

    // Set static uniforms
    glUniform1i(glGetUniformLocation(program, "glyph_width"), glyph_width);
    glUniform1i(glGetUniformLocation(program, "glyph_height"), glyph_height);
    glUniform1i(glGetUniformLocation(program, "atlas_cols"), atlas_cols);
    glUniform2f(glGetUniformLocation(program, "origin"), 0, 0);

    // Orthographic projection matrix (0,0 top-left, width,height bottom-right)
    float w = (float)screenWidth, h = (float)screenHeight;
    float ortho[16] = {
        2.0f / w,  0.0f,       0.0f, 0.0f,
        0.0f,     -2.0f / h,   0.0f, 0.0f,
        0.0f,      0.0f,      -1.0f, 0.0f,
       -1.0f,      1.0f,       0.0f, 1.0f
    };

    GLint proj_loc = glGetUniformLocation(program, "u_proj");
    glUniformMatrix4fv(proj_loc, 1, GL_FALSE, ortho);


    while (!glfwWindowShouldClose(window)) {
        glClear(GL_COLOR_BUFFER_BIT);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, tex);
        glBindVertexArray(vao);

        // glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
        // glDrawArrays(GL_TRIANGLES, 0, glyph_count * 6);
        // glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
        glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, sizeof(msg));
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}
