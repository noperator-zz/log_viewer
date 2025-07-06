#include "app_window.h"


int main(int argc, char *argv[]) {
    int err {};
    setvbuf(stdout, nullptr, _IOFBF, 1024 * 1024);

    auto win = AppWindow::create(argc, argv, err);
    if (err) {
        fprintf(stderr, "Failed to create AppWindow: %d\n", err);
        return err;
    }
    return win->run();
}
