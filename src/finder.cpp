#include "finder.h"

#include "search.h"
#include "util.h"

using namespace std::chrono;

static int handler (unsigned int id, unsigned long long from, unsigned long long to, unsigned int flags, void *context) {
	printf("Match found: id=%u, from=%llu, to=%llu, flags=%u, context=%p\n", id, from, to, flags, context);
	return 0; // Continue matching
}

Finder::Finder(size_t num_workers, std::mutex &mtx)
	: workers_(num_workers), mtx_(mtx) {
}

Finder::~Finder() {
	workers_.close();
}


void Finder::add_data(const uint8_t *data, size_t len) {

}

void Finder::submit(const void* ctx, std::string_view pattern, int flags) {

}

void Finder::remove(const void* ctx) {
	results_.erase(ctx);
}


// std::thread Finder::start(const Event &quit) {
// 	// return std::thread(&Finder::worker, this, std::ref(quit));
// }



// void Finder::worker(const Event &quit) {
// 	while (!quit.wait(100ms)) {
// 	}
// }
