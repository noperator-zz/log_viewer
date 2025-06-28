#pragma once

#include <iostream>
#include <streambuf>
#include <string>
#include <mutex>
#include <unordered_map>
#include <thread>

class LogStreamBuf : public std::streambuf {
public:
    LogStreamBuf(void (&log)(const char *data, size_t length))
	: log_(log) {}

protected:
    int overflow(int c) override {
        if (c != EOF) {
			std::lock_guard lock(mutex_);
            char ch = static_cast<char>(c);
			if (ch == '\n') {
				unsafe_flush();
			} else {
				buffer_().push_back(ch);
			}
        }
        return c;
    }

    std::streamsize xsputn(const char* s, std::streamsize n) override {
        for (std::streamsize i = 0; i < n; ++i) {
            overflow(s[i]);
        }
        return n;
    }

    int sync() override {
		std::lock_guard lock(mutex_);
		unsafe_flush();
        return 0;
    }

private:
	void unsafe_flush() {
		if (!buffer_().empty()) {
			log_(buffer_().c_str(), buffer_().size());
			buffer_().clear();
		}
	}

	std::string & buffer_() {
		return buffer_map_[std::this_thread::get_id()];
	}

	void (&log_)(const char *data, size_t length);
	std::mutex mutex_;
	std::unordered_map<std::thread::id, std::string> buffer_map_;
};

class LogStream : public std::ostream {
public:
    LogStream(void (&log)(const char *data, size_t length)) : std::ostream(&stream_buf_), stream_buf_(log) {}

private:
    LogStreamBuf stream_buf_;
};

extern LogStream logger;
