#pragma once
#include <glm/vec2.hpp>

class Resizable {
protected:
	glm::ivec2 pos_ {};
	glm::ivec2 size_ {};

public:
	virtual ~Resizable() = default;

	glm::ivec2 pos() const {
		return pos_;
	}

	glm::ivec2 size() const {
		return size_;
	}

	virtual std::tuple<int, int, int, int> resize(glm::ivec2 pos, glm::ivec2 size) {
		pos_ = pos;
		size_ = size;
		return {pos.x, pos.y, size.x, size.y};
	}

	virtual void apply() {}
};
