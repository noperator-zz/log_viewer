#include "app.h"

int main(int argc, char *argv[]) {
    App app {};
    app.start();

    if (argc > 1) {
        for (int i = 1; i < argc; i++) {
            app.add_file(argv[i]);
        }
    }

    app.run();
    return 0;
}
