#include <cstdio>
#include <stdlib.h>
#include <string.h>

int main() {
	for (int i = 0; i < 64; i++) {
		auto p = malloc(1024 * 1024 * 1024);
		memset(p, 1, 1024 * 1024 * 1024);
		printf("Allocated %d GB\n", i + 1);
	}
}
