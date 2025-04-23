import random
import subprocess
import time

keys = "hjklyubn,<> "
while True:
    proc = subprocess.Popen("./assignment1_09", stdin=subprocess.PIPE)
    key_count = 0
    while proc.poll() is None:
        key = random.choice(keys).encode()
        proc.stdin.write(key)
        proc.stdin.flush()
        time.sleep(0.01)
        key_count += 1
        if key_count >= 1000:
            proc.terminate()
            proc = subprocess.Popen("./assignment1_09", stdin=subprocess.PIPE)
            key_count = 0
    if proc.returncode != 0:
        print(f"Program exited with code {proc.returncode}")
        break
