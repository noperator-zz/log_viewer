#pragma once

#include <cstdint>
#include <ft2build.h>
#include <GL/glew.h>

#include FT_FREETYPE_H

class Font {
public:
	static constexpr size_t num_glyphs = 256;
	static constexpr size_t num_faces = 4;

	const size_t size;
private:
	const char *paths[4];
	GLuint tex_atlas_ {};
	GLuint tex_bearing_ {};

	int render(FT_Face face, size_t style_idx) const;

	static FT_Library ft;
public:
	static int init();

	Font(int size,
		const char *path_reg,
		const char *path_bold,
		const char *path_italic,
		const char *path_bold_italic
	);
	int load();

	GLuint tex_atlas() const {
		return tex_atlas_;
	}
	GLuint tex_bearing() const {
		return tex_bearing_;
	}
};
