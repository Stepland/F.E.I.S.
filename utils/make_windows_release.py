"""Run this after you've compiled FEIS"""

import argparse
import shutil
import subprocess
from pathlib import Path

parser = argparse.ArgumentParser()
parser.add_argument("release_name")
parser.add_argument("--build-dir", type=Path, default=Path("build"))
args = parser.parse_args()

release_folder = Path(args.release_name)
release_folder.mkdir(exist_ok=True)
feis_exe = args.build_dir / "FEIS.exe"
shutil.copy(feis_exe, release_folder)
shutil.copytree("assets", release_folder)

subprocess.run([
    "python",
    "utils/copy_dependencies.py",
    "-d", release_folder,
    "-f", release_folder / "FEIS.exe"
])

shutil.make_archive(args.release_name, "zip", ".", release_folder)
