import difflib
import os
import subprocess
import sys

def compare(testpath: str, snappath: str) -> None:
    if not os.path.isfile(snappath):
        print("snapshot does not exist:", snappath)
        return
    with open("temp", "w+") as tempfile:
        subprocess.run(["./build/flood", testpath], stdout=tempfile)
    with open("temp", "r") as tempfile, open(snappath, "r") as snapshot:
        try:
            diff = difflib.unified_diff(snapshot.readlines(), tempfile.readlines(), n=0)
            next(diff)
            next(diff)
            next(diff)
            print(testpath)
            for line in diff:
                if line.startswith("-"):
                    print(f"\033[31m{line}\033[0m",end="")
                elif line.startswith("+"):
                    print(f"\033[32m{line}\033[0m",end="")
                else:
                    print(line,end="")
        except StopIteration:
            pass

def upgrade(testpath: str, snappath: str) -> None:
    snapdir = snappath[:snappath.rfind("/")]
    os.makedirs(snapdir, exist_ok=True)
    with open(snappath, "w") as snapshot:
        subprocess.run(["./build/flood", testpath], stdout=snapshot)

def run() -> None:
    try:
        argv = sys.argv
        if len(argv) != 2 or (argv[1] != "--compare" and argv[1] != "--upgrade"):
            print("usage: python3 test.py [--compare | --upgrade]") 
            return
        for dirpath, _, filenames in os.walk("test/features"):
            for name in filenames:
                testpath = os.path.join(dirpath, name)
                snappath = testpath.replace("test", "snapshot", 1).replace(".fl", ".out")
                if argv[1] == "--compare":
                    compare(testpath, snappath)
                else: 
                    upgrade(testpath, snappath)
    finally:
        if os.path.isfile("temp"):
            os.remove("temp")

run()