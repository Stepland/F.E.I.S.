"""Run this after you've compiled FEIS"""

import argparse
import datetime
import shutil
import subprocess
from pathlib import Path

git_version = subprocess.check_output(["git", "describe", "--tags"]).strip().decode("utf-8")

parser = argparse.ArgumentParser()
parser.add_argument("--release-version", default=git_version)
parser.add_argument("--timestamp", action="store_true")
parser.add_argument("--build-dir", type=Path, default=Path("build"))
args = parser.parse_args()

release_folder = Path(f"FEIS-{args.release_version}")
if release_folder.exists():
    shutil.rmtree(release_folder)
release_folder.mkdir()
feis_exe = args.build_dir / "FEIS.exe"
shutil.copy(feis_exe, release_folder)
shutil.copytree("assets", release_folder / "assets", dirs_exist_ok=True)

subprocess.run([
    "python",
    "utils/copy_dependencies.py",
    "-d", release_folder,
    "-f", release_folder / "FEIS.exe"
])

archive_name = release_folder
if args.timestamp:
    timestamp = datetime.datetime.utcnow().strftime("%Y%m%dT%H%M%S")
    archive_name = f"{release_folder}+{timestamp}"

shutil.make_archive(
    archive_name,
    "zip",
    ".",
    release_folder
)
