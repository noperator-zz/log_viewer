#include "font.h"
#include <GL/glew.h>

FT_Library Font::ft;

int Font::init() {
	FT_Error err = FT_Init_FreeType(&ft);
	if (err) {
		return err;
	}
	return 0;
}

Font::Font(int size,
	const char *path_reg,
	const char *path_bold,
	const char *path_italic,
	const char *path_bold_italic)
		: size(size), paths{path_reg, path_bold, path_italic, path_bold_italic} {
}

int Font::render(FT_Face face, size_t style_idx) const {
	FT_Error err;
	int x = 0;
	for (int c = 0; c < num_glyphs; ++c) {
		err = FT_Load_Char(face, c, FT_LOAD_RENDER);
		if (err) {
			return err;
		}

		FT_GlyphSlot g = face->glyph;

		// printf("%c - X: %3d, W: %2d, H: %2d, Bx: %2d, By: %2d, Ax: %2d\n",
		// 	c, x,
		// 	g->bitmap.width, g->bitmap.rows,
		// 	g->bitmap_left, g->bitmap_top,
		// 	g->advance.x >> 6);

		glBindTexture(GL_TEXTURE_2D, tex_atlas_);
		glTexSubImage2D(GL_TEXTURE_2D, 0, x, style_idx * size,
			g->bitmap.width, g->bitmap.rows,
            GL_RED, GL_UNSIGNED_BYTE, g->bitmap.buffer);

		float bearing_top = size - g->bitmap_top;
		glBindTexture(GL_TEXTURE_2D, tex_bearing_);
		glTexSubImage2D(GL_TEXTURE_2D, 0, c, style_idx,1, 1,
			GL_RED, GL_FLOAT, &bearing_top);

		x += size;
	}

	return 0;
}

int Font::load() {
	FT_Error err;

	glGenTextures(1, &tex_atlas_);
	glBindTexture(GL_TEXTURE_2D, tex_atlas_);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, num_glyphs * size, num_faces * size, 0, GL_RED, GL_UNSIGNED_BYTE, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glGenTextures(1, &tex_bearing_);
	glBindTexture(GL_TEXTURE_2D, tex_bearing_);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, num_glyphs * 1, num_faces, 0, GL_RED, GL_FLOAT, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	size_t style_idx = 0;
	for (auto & path : paths) {
		if (!path) {
			path = paths[0];
		}

		FT_Face face {};
		if (FT_New_Face(ft, path, 0, &face) != 0) {
			return -1;
		}
		if (FT_Set_Pixel_Sizes(face, 0, size) != 0) {
			FT_Done_Face(face);
			return -2;
		}

		err = render(face, style_idx);
		FT_Done_Face(face);

		if (err) {
			return err;
		}
		style_idx++;
	}
	return 0;
}
