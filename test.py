import argparse
import filecmp
import subprocess
import tempfile

from pathlib import Path    

def to_snap_path(test_path: Path) -> Path:
    return Path("snapshots") / test_path.relative_to("tests").with_suffix(".out")

def clean_all() -> None:
    snap_paths_keep = set([to_snap_path(path) for path in Path("tests").glob("**/*") if path.is_file()])
    snap_paths_all = set([path for path in Path("snapshots").glob("**/*") if path.is_file()])
    snap_paths_clean = snap_paths_all - snap_paths_keep
    for snap_path in snap_paths_clean:  
        print(f"\033[32mcleaned: {snap_path}\033[0m")  
        snap_path.unlink()

def diff(test_path: Path) -> None:
    snap_path = to_snap_path(test_path)
    if not snap_path.is_file():
        print(f"\033[31msnapshot `{snap_path}` does not exist.\033[0m")
        return
    with tempfile.NamedTemporaryFile("w+") as tmp, open(snap_path, "r") as snapshot:
        subprocess.run(["./build/flood", test_path], stdout=tmp)
        tmp.flush()
        tmp.seek(0)
        if filecmp.cmp(tmp.name, snap_path):
            print(f"\033[32m{test_path}\033[0m")
        else:
            print(f"\033[31m{test_path}\033[0m")
            print("old:")
            print(snapshot.read(),end="")
            print("new:")
            print(tmp.read(),end="")

def upgrade(test_path: Path) -> None:
    snap_path = to_snap_path(test_path)
    snap_path.parent.mkdir(parents=True, exist_ok=True)
    with open(snap_path, "w") as snapshot:
        subprocess.run(["./build/flood", test_path], stdout=snapshot)
        print(f"\033[32mupgraded: {snap_path}\033[0m")

def leak_check(test_path: Path) -> None:
    with tempfile.NamedTemporaryFile("w+") as tmp:
        exit_code = subprocess.run(
            ["valgrind", "--leak-check=full", "--error-exitcode=3", "./build/flood", test_path], 
            stderr=tmp,
            stdout=subprocess.DEVNULL,
        ).returncode
        tmp.flush()
        tmp.seek(0)
        if exit_code != 3:
            print(f"\033[32m{test_path}\033[0m")
        else:
            print(f"\033[31m{test_path}\033[0m")
            print(tmp.read(),end="")

def main() -> None:
    parser = argparse.ArgumentParser()
    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument("--diff", action="store_true")
    group.add_argument("--upgrade", action="store_true")
    group.add_argument("--clean", action="store_true")
    group.add_argument("--leak-check", action="store_true")
    args = parser.parse_args()

    Path("tests").mkdir(exist_ok=True)
    Path("snapshots").mkdir(exist_ok=True)

    if args.clean:
        clean_all()
        return

    test_dirs = [
        # "language/comptime_error", 
        "language/runtime_error", 
        "language/runtime",
        "misc"
    ]

    for dir in test_dirs:
        dir_path = Path("tests") / Path(dir)
        if not dir_path.is_dir():
            print(f"`{dir_path}` does not exist or is not a directory.")
            continue
        for test_path in dir_path.glob("**/*"):
            if not test_path.is_file():
                continue
            if args.diff:
                diff(test_path)
            elif args.upgrade:       
                upgrade(test_path) 
            else:
                leak_check(test_path)

if __name__ == "__main__":
    main()