#include "finder.h"

#include <cassert>

#include "search.h"
#include "util.h"

using namespace std::chrono;

Finder::~Finder() {
	stop();
}

void Finder::stop() {
	std::lock_guard lock(mtx_);
	jobs_.clear();
}

std::unique_ptr<Finder::Job> && Finder::Job::create(std::string_view pattern, int flags, int &error) {
	hs_compile_error_t *compile_err;
	hs_error_t err;

	hs_database_t *db;
	hs_scratch_t *scratch;
	hs_stream_t *stream;

	// err = hs_compile_multi(expressions.data(), flags.data(), ids.data(),
	//                        expressions.size(), mode, nullptr, &db, &compileErr);

	auto flag = HS_FLAG_DOTALL | HS_FLAG_CASELESS | HS_FLAG_MULTILINE | HS_FLAG_SOM_LEFTMOST;
	err = hs_compile(pattern.data(), flag, HS_MODE_STREAM | HS_MODE_SOM_HORIZON_LARGE, NULL, &db, &compile_err);
	if (err != HS_SUCCESS) {
		fprintf(stderr, "ERROR: Unable to compile pattern \"%s\": %s\n",
				pattern.data(), compile_err->message);
		hs_free_compile_error(compile_err);
		error = -1;
		return nullptr;
	}

	if (hs_alloc_scratch(db, &scratch) != HS_SUCCESS) {
		fprintf(stderr, "ERROR: Unable to allocate scratch space. Exiting.\n");
		hs_free_database(db);
		error = -2;
		return nullptr;
	}

	if (hs_open_stream(db, 0, &stream) != HS_SUCCESS) {
		fprintf(stderr, "ERROR: Unable to open stream. Exiting.\n");
		hs_free_scratch(scratch);
		hs_free_database(db);
		error = -3;
		return nullptr;
	}

	error = 0;
	// return std::make_unique<Job>(pattern, flags, db, scratch, stream);
	return std::unique_ptr<Job>(new Job(pattern, flags, db, scratch, stream));
}

Finder::Job::Job(std::string_view pattern, int flags, hs_database_t *db_, hs_scratch_t *scratch_, hs_stream_t *stream_)
	: thread_(&worker, this), pattern_(pattern), flags_(flags), db_(db_), scratch_(scratch_), stream_(stream_) {
}

Finder::Job::~Job() {
	assert(stream_ && scratch_ && db_);

	{
		Timeit t("Finder::Job::~Job()");
		quit();
	}

	hs_close_stream(stream_, nullptr, nullptr, nullptr);
	hs_free_scratch(scratch_);
	hs_free_database(db_);
}

void Finder::Job::quit()  {
	quit_ = true;
	cv_.notify_one();

	if (thread_.joinable()) {
		thread_.join();
	}
}

int Finder::Job::event_handler(unsigned int id, unsigned long long from, unsigned long long to, unsigned int flags, void *context) {
	return static_cast<Job *>(context)->event_handler(id, from, to, flags);
}

int Finder::Job::event_handler(
unsigned int id, unsigned long long from, unsigned long long to, unsigned int flags) {
	printf("Match found: id=%u, from=%llu, to=%llu, flags=%u, context=%p\n", id, from, to, flags, this);
	chunk_results_.emplace_back(from, to);
	if (quit_) {
		return 1; // Stop matching
	}
	return 0; // Continue matching
}

void Finder::Job::worker() {
	static constexpr size_t CHUNK_SIZE = 1024 * 1024;
	QueueItem item;

	while (true) {
		{
			std::unique_lock lock(mtx_);
			cv_.wait(lock, [this] { return quit_ || !queue_.empty(); });

			if (quit_) {
				status_ = Status::kQUIT;
			}

			item = std::move(queue_.front());
			queue_.pop();
		}

		assert(item.length >= stream_pos_);
		item.data += stream_pos_;
		item.length -= stream_pos_;

		hs_error_t err = HS_SUCCESS;
	    for (size_t offset = 0; offset < item.length; offset += CHUNK_SIZE) {
	        size_t chunk_size = std::min(item.length - offset, CHUNK_SIZE);
    		err = hs_scan_stream(stream_, (const char*)item.data + offset, chunk_size, 0, scratch_, event_handler, this);
			if (err != HS_SUCCESS) {
				break;
	        }
		    if (quit_) {
		    	// Will be handled above
			    break;
		    }
	    	stream_pos_ += chunk_size;
		    {
	        	std::lock_guard lock(mtx_);
			    results_.insert(results_.end(), chunk_results_.begin(), chunk_results_.end());
		    }
	    	chunk_results_.clear();
	    }

		if (err == HS_SUCCESS) {

		} else if (err == HS_SCAN_TERMINATED) {
			// Will be handled above
			assert(quit_);
		} else {
			status_ = Status::kERROR;
			fprintf(stderr, "ERROR: Unable to scan input buffer. Exiting.\n");
			break;
		}
	}
}

std::mutex &Finder::Job::mutex() {
	return mtx_;
}

void Finder::Job::update_data(const uint8_t *data, size_t len) {
	{
		std::lock_guard lock(mtx_);
		queue_.emplace(data, len);
	}
	cv_.notify_one();
}

Finder::Job::Status Finder::Job::status() const {
	return status_;
}


std::mutex &Finder::mutex() {
	return mtx_;
}

const std::unordered_map<const void*, std::unique_ptr<Finder::Job>> & Finder::jobs() const {
	return jobs_;
}

void Finder::update_data(const uint8_t *data, size_t len) {
	std::lock_guard lock(mtx_);
	latest_data_ = data;
	latest_length_ = len;

	for (const auto &[ctx, job] : jobs_) {
		job->update_data(data, len);
	}
}

int Finder::submit(const void* ctx, std::string_view pattern, int flags) {
	std::lock_guard lock(mtx_);
	int err;

	if (jobs_.contains(ctx)) {
		// if (pattern != jobs_[ctx]->pattern()) {
			jobs_.erase(ctx);
		// }
	}

	if (!jobs_.contains(ctx)) {
		auto job = Job::create(pattern, flags, err);
		if (err) {
			fprintf(stderr, "ERROR: Unable to create job for pattern \"%s\".\n", pattern.data());
			return err;
		}
		assert(job);

		jobs_.emplace(ctx, std::move(job));
	}

	if (latest_data_) {
		jobs_[ctx]->update_data(latest_data_, latest_length_);
	}
	return 0;
}

void Finder::remove(const void* ctx) {
	std::lock_guard lock(mtx_);
	assert(jobs_.contains(ctx));
	// TODO This will block until the next match is found. Probably milliseconds, but it's indeterminate.
	jobs_.erase(ctx);
}
