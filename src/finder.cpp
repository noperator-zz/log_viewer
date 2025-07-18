#include <cassert>

#include "finder.h"
#include <hs/hs.h>
#include "util.h"

using namespace std::chrono;

Finder::Finder(Dataset &dataset) : dataset_(dataset) {}

Finder::~Finder() {
	stop();
}

void Finder::stop() {
	std::lock_guard lock(jobs_mtx_);
	jobs_.clear();
}

std::unique_ptr<Finder::Job> Finder::Job::create(std::mutex &results_mtx, Dataset &dataset, std::function<void(void*, size_t)> &&on_result,
	void* ctx, std::string_view pattern, int flags, int &error) {
	Timeit t("Finder::Job::create()");
	hs_compile_error_t *compile_err;
	hs_error_t err;

	hs_database_t *db {};
	hs_scratch_t *scratch {};
	hs_stream_t *stream {};

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

	err = hs_alloc_scratch(db, &scratch);
	if (err != HS_SUCCESS) {
		fprintf(stderr, "ERROR: Unable to allocate scratch space. Exiting.\n");
		hs_free_database(db);
		error = -2;
		return nullptr;
	}

	err = hs_open_stream(db, 0, &stream);
	if (err != HS_SUCCESS) {
		fprintf(stderr, "ERROR: Unable to open stream. Exiting.\n");
		hs_free_scratch(scratch);
		hs_free_database(db);
		error = -3;
		return nullptr;
	}

	error = 0;
	return std::unique_ptr<Job>(new Job(results_mtx, dataset, std::move(on_result), ctx, pattern, flags, db, scratch, stream));
}

Finder::Job::Job(std::mutex &result_mtx, Dataset &dataset, std::function<void(void*, size_t)> &&on_result,
	void* ctx, std::string_view pattern, int flags, hs_database_t *db, hs_scratch_t *scratch, hs_stream_t *stream)
	: thread_(&worker, this), result_mtx_(result_mtx), dataset_(dataset), on_result_(std::move(on_result)), ctx_(ctx)
	, pattern_(pattern), flags_(flags), db_(db) , scratch_(scratch), stream_(stream) {
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
	std::cout << "Job quitting..." << std::endl;
	quit_.test_and_set();
	dataset_.notify();

	if (thread_.joinable()) {
		thread_.join();
	}
	std::cout << "Job quit." << std::endl;
}

int Finder::Job::event_handler(unsigned int id, unsigned long long from, unsigned long long to, unsigned int flags, void *context) {
	return static_cast<Job *>(context)->event_handler(id, from, to, flags);
}

int Finder::Job::event_handler(unsigned int id, unsigned long long from, unsigned long long to, unsigned int flags) {
	// printf("Match found: id=%u, from=%llu, to=%llu, flags=%u, context=%p\n", id, from, to, flags, this);
	// fflush(stdout);
	// std::this_thread::sleep_for(milliseconds(1));
	chunk_results_.emplace_back(from, to);
	if (dataset_.is_update_pending() || quit_.test()) {
		return 1; // Stop matching
	}
	return 0; // Continue matching
}

void Finder::Job::worker() {
	// NOTE Chunk size needs to be relatively small because it sets the latency of the quit event being handled
	static constexpr size_t CHUNK_SIZE = 1ULL * 1024 * 1024;

	while (true) {
		std::cout << "Job acquire..." << std::endl;
		{
			auto user {dataset_.wait([this](auto length) {
				return quit_.test() || length > stream_pos_;
			})};

			if (quit_.test()) {
				std::cout << "Quit event" << std::endl;
				status_ = Status::kQUIT;
				break;
			}

			Timeit t("Scan");
			std::cout << "Job acquired." << std::endl;
			assert(user.data());
			assert(user.length() > stream_pos_);

			const uint8_t *data = user.data() + stream_pos_;
			const size_t length = user.length() - stream_pos_;

			hs_error_t err = HS_SUCCESS;
		    for (size_t offset = 0; offset < length; offset += CHUNK_SIZE) {
		        size_t chunk_size = std::min(length - offset, CHUNK_SIZE);
	    		chunk_results_.resize_uninitialized(0);
		    	// std::cout << "Job scan... " << offset << " - " << (offset + chunk_size) << " / " << length << std::endl;
    			err = hs_scan_stream(stream_, (const char*)data + offset, chunk_size, 0, scratch_, event_handler, this);
		    	// std::this_thread::sleep_for(milliseconds(100));
		    	// std::cout << "Job scan = " << err << ". " << chunk_results_.size() << " results." << std::endl;

			    if (quit_.test()) {
		    		// Will be handled above
					std::cout << "Quit during scan" << std::endl;
			    }
				if (err != HS_SUCCESS) {
					break;
		        }

	    		stream_pos_ += chunk_size;

			    {
	        		std::lock_guard lock(result_mtx_);
				    results_.extend(chunk_results_);
		        	// std::swap(results_, chunk_results_);
			    }
		    	if (on_result_) {
		    		on_result_(ctx_, last_report_);
		    	}
		    	last_report_ = results_.size();
	    		chunk_results_.resize_uninitialized(0);

		    	// if (dataset_.update_pending_.test()) {
		    	// 	// Finder wants to update the dataset. Break out of the loop to release the lock earlier.
		    	// 	break;
		    	// }
		    }

			if (err == HS_SUCCESS) {

			} else if (err == HS_SCAN_TERMINATED) {
				// Will be handled above
			} else {
				status_ = Status::kERROR;
				fprintf(stderr, "ERROR: Unable to scan input buffer. Exiting.\n");
				break;
			}
			std::cout << "Job release...." << std::endl;
		}
		std::cout << "Job released." << std::endl;
	}
}

Finder::Job::Status Finder::Job::status() const {
	return status_;
}


int Finder::submit(void* ctx, std::function<void(void*, size_t)> &&on_result, std::string_view pattern, int flags) {
	// NOTE This is called by the main thread (during search input box event handling) and should not block.
	//  The only blocker here is erasing an existing job. Given how we have the quit flag set up, this will block until
	//  the next match is found or the chunk is processed, whichever comes first.
	//  For a complex regex (with no matches) and a large chunk size, this could take a while.
	// TODO benchmark this. How small of a chunk size can we use without losing performance?
	std::cout << "Submit: " << pattern << std::endl;
	std::lock_guard lock(jobs_mtx_);
	int err;

	if (jobs_.contains(ctx)) {
		std::cout << "Erase" << std::endl;
		jobs_.erase(ctx);
	}

	if (!jobs_.contains(ctx)) {
		std::cout << "Create" << std::endl;
		auto job = Job::create(results_mtx_, dataset_, std::move(on_result), ctx, pattern, flags, err);
		if (err) {
			fprintf(stderr, "ERROR: Unable to create job for pattern \"%s\".\n", pattern.data());
			return err;
		}
		assert(job);

		jobs_.emplace(ctx, std::move(job));
	}

	// Notify the job to start processing the dataset.
	dataset_.notify();

	return 0;
}

void Finder::remove(void* ctx) {
	std::lock_guard lock(jobs_mtx_);
	assert(jobs_.contains(ctx));
	// TODO This will block until the next match is found. Probably milliseconds, but it's indeterminate.
	jobs_.erase(ctx);
}

size_t Finder::find_prev_match(const dynarray<Job::Result> &results, size_t char_idx) {
	// search through results for the first match that starts before char_idx
	auto it = std::lower_bound(results.begin(), results.end(), char_idx);
	if (it == results.end()) {
		return results.size() - 1; // char_idx is after the last match, return the last match
	}
	if (char_idx > it->start) {
		return it - results.begin(); // char_idx is after the match, return this match
	}
	// char_idx is at or before the match, return the previous match
	if (it == results.begin()) {
		return results.size() - 1; // wrap around to the last match
	}
	it--; // move to the previous match
	return it - results.begin();
}

size_t Finder::find_next_match(const dynarray<Job::Result> &results, size_t char_idx) {
	// search through results for the first match that starts after char_idx
	auto it = std::upper_bound(results.begin(), results.end(), char_idx,
		[](size_t char_idx, const Job::Result &result) { return char_idx < result.start; });
	if (it == results.end()) {
		return 0; // wrap around to the first match
	}
	return it - results.begin();
}

size_t Finder::find_line_containing_SOM(const dynarray<size_t> &line_starts, const dynarray<Job::Result> &results, size_t match_idx) {
	auto char_pos = results[match_idx].start;

	auto line_it = std::lower_bound(line_starts.begin(), line_starts.end(), char_pos);
	assert(line_it != line_starts.end());
	if (*line_it > char_pos && line_it != line_starts.begin()) {
		line_it--;
	}
	return line_it - line_starts.begin();
}
