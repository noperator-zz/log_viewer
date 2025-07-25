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

int Font::render(FT_Face face, size_t style_idx) {
	FT_Error err;
	int x = 0;

	int max_descent = FT_MulFix(abs(face->descender), face->size->metrics.y_scale) >> 6;

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
		glTexSubImage2D(GL_TEXTURE_2D, 0,
			x + g->bitmap_left,
			style_idx * size.y + (size.y - g->bitmap_top - max_descent),
			g->bitmap.width,
			g->bitmap.rows,
            GL_RED, GL_UNSIGNED_BYTE, g->bitmap.buffer);

		x += size.x;
	}

	return 0;
}

int Font::load() {
	FT_Error err;

	bool tex_created = false;
	size_t style_idx = 0;
	for (auto & path : paths) {
		if (!path) {
			path = paths[0];
		}

		FT_Face face {};
		if (FT_New_Face(ft, path, 0, &face) != 0) {
			return -1;
		}

		if (FT_Set_Pixel_Sizes(face, 0, size.y) != 0) {
			FT_Done_Face(face);
			return -2;
		}

		if (!tex_created) {
			tex_created = true;
			// TODO
			size.x = face->size->metrics.max_advance >> 6;
			size.y = face->size->metrics.height >> 6;

			glGenTextures(1, &tex_atlas_);
			glBindTexture(GL_TEXTURE_2D, tex_atlas_);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, num_glyphs * size.x, num_faces * size.y, 0, GL_RED, GL_UNSIGNED_BYTE, nullptr);
			GLubyte zero = 0.0f;
			glClearTexImage(tex_atlas_, 0, GL_RED, GL_UNSIGNED_BYTE, &zero);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
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
