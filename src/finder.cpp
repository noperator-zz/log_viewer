#include <cassert>

#include "finder.h"
#include <hs/hs.h>
#include "util.h"
#include "event.h"

using namespace std::chrono;

Finder::Finder(Dataset &dataset) : dataset_(dataset) {}

Finder::~Finder() {
	stop();
}

void Finder::stop() {
	std::lock_guard lock(jobs_mtx_);
	jobs_.clear();
}

std::unique_ptr<Finder::Job> Finder::Job::create(Dataset &dataset, std::string_view pattern, int flags, int &error) {
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
	return std::unique_ptr<Job>(new Job(dataset, pattern, flags, db, scratch, stream));
}

Finder::Job::Job(Dataset &dataset, std::string_view pattern, int flags,
	hs_database_t *db_, hs_scratch_t *scratch_, hs_stream_t *stream_)
	: thread_(&worker, this), dataset_(dataset), pattern_(pattern), flags_(flags), db_(db_)
	, scratch_(scratch_), stream_(stream_) {
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
	static constexpr size_t CHUNK_SIZE = 0xFFFFFFFF;
	// static constexpr size_t CHUNK_SIZE = 1ULL * 1024 * 1024;

	while (true) {
		// {
		// 	std::shared_lock lock(dataset_.mtx);
		// 	update_cv_.wait(lock, [this]{;
		// 		return quit_.test() || (dataset_.data != nullptr && dataset_.length > stream_pos_);
		// 	});
		// }

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
			// if (dataset_.data == nullptr) {
			// 	continue;
			// }

			assert(user.length() >= stream_pos_);

			const uint8_t *data = user.data() + stream_pos_;
			const size_t length = user.length() - stream_pos_;

			hs_error_t err = HS_SUCCESS;
		    for (size_t offset = 0; offset < length; offset += CHUNK_SIZE) {
		        size_t chunk_size = std::min(length - offset, CHUNK_SIZE);
	    		chunk_results_.clear();
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
				FIXME use callback instead
		    	send_event();
	    		chunk_results_.clear();

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

std::mutex &Finder::Job::result_mtx() {
	return result_mtx_;
}

Finder::Job::Status Finder::Job::status() const {
	return status_;
}


std::mutex &Finder::mutex() {
	return jobs_mtx_;
}

const std::unordered_map<const void*, std::unique_ptr<Finder::Job>> & Finder::jobs() const {
	return jobs_;
}


int Finder::submit(const void* ctx, std::string_view pattern, int flags) {
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
		// if (pattern != jobs_[ctx]->pattern()) {
			jobs_.erase(ctx);
		// }
	}

	if (!jobs_.contains(ctx)) {
		std::cout << "Create" << std::endl;
		auto job = Job::create(dataset_, pattern, flags, err);
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

void Finder::remove(const void* ctx) {
	std::lock_guard lock(jobs_mtx_);
	assert(jobs_.contains(ctx));
	// TODO This will block until the next match is found. Probably milliseconds, but it's indeterminate.
	jobs_.erase(ctx);
}
