import time

d = 0.1

path = r"C:/development/log_viewer_win/test_set/tailer.log"
path = r"E:\enourmous.log"

with open(path, "ab") as f:
    while 1:
        for i in range(10):
            f.write(f"{i}".encode())
            f.flush()
            time.sleep(d)

        f.write(b"\n")
        f.flush()
        time.sleep(d)

        print("Wrote line")
