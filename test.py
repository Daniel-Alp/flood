import tempfile
import difflib
import os
import subprocess
import sys

def diff(testpath: str, snappath: str) -> None:
    if not os.path.isfile(snappath):
        print(f"\033[31msnapshot does not exist: {snappath}\033[0m")
        return
    with tempfile.NamedTemporaryFile("w+") as tmp, open(snappath, "r") as snapshot:
        subprocess.run(["./build/flood", testpath], stdout=tmp)
        tmp.flush()
        tmp.seek(0)
        diff = list(difflib.unified_diff(snapshot.readlines(), tmp.readlines(), n=0))
        if not diff:
            print(f"\033[32m{testpath}\033[0m")
        else:
            print(f"\033[31m{testpath}\033[0m\n")
            for line in diff:
                if line.startswith("-"):
                    print(f"\033[31m{line}\033[0m", end="")
                elif line.startswith("+"):
                    print(f"\033[32m{line}\033[0m", end="")
                else:
                    print(line, end="")
            print()

def upgrade(testpath: str, snappath: str) -> None:
    snapdir = snappath[:snappath.rfind("/")]
    os.makedirs(snapdir, exist_ok=True)
    with open(snappath, "w") as snapshot:
        subprocess.run(["./build/flood", testpath], stdout=snapshot)
        print(f"\033[32mupgraded: {snappath}\033[0m")

def run() -> None:
    argv = sys.argv
    if len(argv) != 2 or (argv[1] != "--diff" and argv[1] != "--upgrade"):
        print("usage: python3 test.py [--diff | --upgrade]") 
        return
    for dirpath, _, filenames in os.walk("test/features"):
        for name in filenames:
            testpath = os.path.join(dirpath, name)
            snappath = testpath.replace("test", "snapshot", 1).replace(".fl", ".out")
            if argv[1] == "--diff": 
                diff(testpath, snappath)
            else:
                upgrade(testpath, snappath)

run()