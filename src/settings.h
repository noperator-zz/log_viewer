#pragma once
#include "text_shader.h"


// TODO make these limits dynamic
static constexpr glm::ivec2 MAX_VISIBLE_CHARS {1024, 1024};
static constexpr size_t CONTENT_BUFFER_SIZE = MAX_VISIBLE_CHARS.y * MAX_VISIBLE_CHARS.x * sizeof(TextShader::CharStyle);
static constexpr size_t LINENUM_BUFFER_SIZE = MAX_VISIBLE_CHARS.y * 10 * sizeof(TextShader::CharStyle);

static_assert(CONTENT_BUFFER_SIZE < 128 * 1024 * 1024, "Content buffer size too large");
