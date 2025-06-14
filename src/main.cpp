#include "app.h"


int main(int argc, char *argv[]) {
    setvbuf(stdout, nullptr, _IOFBF, 1024 * 1024);
    App::create(argc, argv);
    return 0;
}
