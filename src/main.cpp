#include "app_window.h"


void write_file(FILE *h) {
    static constexpr int CDELAY = 10;
    static constexpr int SDELAY = 500;
    const char s[] = "Hello, world! This is a long line that wil be emitted slowly over the course of several seconds. Wow, look. Here are some more characters to make this line even longer. Can you believe how long this line is?\n";
    while (1) {
        for (int i = 0; i < 100; i++) {
            for (auto c : s) {
                fputc(c, h);
                std::this_thread::sleep_for(std::chrono::milliseconds(CDELAY));
            }
        }
        fputs("single line match\n", h);
        std::this_thread::sleep_for(std::chrono::milliseconds(SDELAY));
        fputs("pad\n", h);
        std::this_thread::sleep_for(std::chrono::milliseconds(SDELAY));
        fputs("multi\nline match\n", h);
        std::this_thread::sleep_for(std::chrono::milliseconds(SDELAY));
        fputs("pad\n", h);
        std::this_thread::sleep_for(std::chrono::milliseconds(SDELAY));
    }
}

int main(int argc, char *argv[]) {
    int err {};
    setvbuf(stdout, nullptr, _IOFBF, 1024 * 1024);

    auto h = fopen("C:/development/log_viewer_win/test_set/tailer.log", "w");
    std::thread t1(write_file, h);

    auto win = AppWindow::create(argc, argv, err);
    if (err) {
        fprintf(stderr, "Failed to create AppWindow: %d\n", err);
        return err;
    }
    return win->run();
}
