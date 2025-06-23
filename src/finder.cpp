#include "finder.h"

#include <cassert>

#include "search.h"
#include "util.h"

using namespace std::chrono;

void Finder::Job::abort(std::mutex &pool_mtx, std::condition_variable &pool_cv) {
	bool was_active {};
	{
		std::lock_guard lock(pool_mtx);
		if (state_ == State::Active) {
			was_active = true;
			state_ = State::Abort;
		}
	}
	pool_cv.notify_all();

	if (was_active) {
		std::unique_lock lock(pool_mtx);
		in_cv_.wait(lock, [&] {
			return state_ == State {};
		});
	}
}

Finder::Finder(size_t num_workers, std::mutex &mtx)
	: workers_(num_workers), mtx_(mtx) {
}

Finder::~Finder() {
	workers_.close();
}

Finder::Job::Job(std::string_view pattern, int flags)
	: thread_(&worker, this), pattern_(pattern), flags_(flags) {
}


int Finder::Job::event_handler(unsigned int id, unsigned long long from, unsigned long long to, unsigned int flags, void *context) {
	return static_cast<Job *>(context)->event_handler(id, from, to, flags);
}

int Finder::Job::event_handler(
unsigned int id, unsigned long long from, unsigned long long to, unsigned int flags) {
	printf("Match found: id=%u, from=%llu, to=%llu, flags=%u, context=%p\n", id, from, to, flags, this);
	chunk_results_.emplace_back(from, to);
	return 0; // Continue matching
}

void Finder::search(std::mutex &pool_mtx, std::condition_variable &pool_cv, bool &quit, const void *ctx,
	const uint8_t *data, size_t length, std::string_view pattern, int flags) {

	static constexpr size_t CHUNK_SIZE = 1024 * 1024;

	auto &job = *jobs_[ctx].get();
	job.abort(pool_mtx, pool_cv);

	assert(job.state_ == Job::State::New || job.state_ == Job::State::Idle);

	if (pattern.empty()) {
		jobs_.erase(ctx);
		return;
	}

	if (job.state_ == Job::State::New) {
		hs_compile_error_t *compile_err;

		// err = hs_compile_multi(expressions.data(), flags.data(), ids.data(),
		//                        expressions.size(), mode, nullptr, &db, &compileErr);

		auto flags = HS_FLAG_DOTALL | HS_FLAG_CASELESS | HS_FLAG_MULTILINE | HS_FLAG_SOM_LEFTMOST;
		if (hs_compile(pattern.data(), flags, HS_MODE_STREAM | HS_MODE_SOM_HORIZON_LARGE, NULL, &job.db_,
					   &compile_err) != HS_SUCCESS) {
			fprintf(stderr, "ERROR: Unable to compile pattern \"%s\": %s\n",
					pattern.data(), compile_err->message);
			hs_free_compile_error(compile_err);
			// TODO
			return;
		}

	    if (hs_alloc_scratch(job.db_, &job.scratch_) != HS_SUCCESS) {
	        fprintf(stderr, "ERROR: Unable to allocate scratch space. Exiting.\n");
	        hs_free_database(job.db_);
    		// TODO
	        return;
	    }

	    if (hs_open_stream(job.db_, 0, &job.stream_) != HS_SUCCESS) {
	        fprintf(stderr, "ERROR: Unable to open stream. Exiting.\n");
	        hs_free_scratch(job.scratch_);
	        hs_free_database(job.db_);
			// TODO
    		return;
	    }

		job.state_ = Job::State::Active;
	}

	hs_error_t err = HS_SUCCESS;
    for (size_t offset = 0; offset < length; offset += CHUNK_SIZE) {
        size_t chunk_size = std::min(length - offset, CHUNK_SIZE);
    	err = hs_scan_stream(job.stream_, (const char*)data + offset, chunk_size, 0, job.scratch_, Job::event_handler, &job);
		if (err != HS_SUCCESS) {
			break;
        }
    }

	if (err == HS_SUCCESS) {
		job.state_ = Job::State::Idle;
	// } else if (err == HS_SCAN_TERMINATED) {

	} else {
		fprintf(stderr, "ERROR: Unable to scan input buffer. Exiting.\n");
	}
    /* Scanning is complete, any matches have been handled, so now we just
     * clean up and exit.
     */
    hs_free_scratch(job.scratch_);
    hs_free_database(job.db_);
    return 0;


	if (state == abort) {
		staet = {};
		notify;
	}
}

void Finder::add_data(const uint8_t *data, size_t len) {

}

void Finder::submit(const void* ctx, std::string_view pattern, int flags) {
	workers_.push([&](auto &pool_mtx, auto &pool_cv, auto &quit) {
		search(pool_mtx, pool_cv, quit, ctx, pattern, flags);
	});
}

void Finder::remove(const void* ctx) {
	// empty string is a special case to indicate removal
	// NOTE erasure will happen in the job thread, so not immediately
	submit(ctx, "", 0);
}
