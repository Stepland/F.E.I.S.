import argparse
import re
import subprocess
from dataclasses import dataclass
from pathlib import Path

@dataclass
class Version:
    major: int
    minor: int
    patch: int
    extra: str

    def __str__(self) -> str:
        return f"{self.major}.{self.minor}.{self.patch}{self.extra}"

def parse_version(text: str) -> Version:
    if (match := re.fullmatch("(\d+)\.(\d+).(\d+)(.*)", text)):
        return Version(
            major=int(match.group(1)),
            minor=int(match.group(2)),
            patch=int(match.group(3)),
            extra=match.group(4)
        )
    else:
        raise ValueError(f"Couldn't parse {text} as a version number")

def rewrite_debian_control_file():
    path = Path(__file__).parents[1] / "debian_packaging/f.e.i.s-control"
    with path.open(mode="r+") as f:
        debian_control_lines = f.readlines()
        for i in range(len(debian_control_lines)):
            line = debian_control_lines[i]
            if line.startswith("Version:"):
                debian_control_lines[i] = f"Version: {args.version}\n"
        
        f.truncate(0)
        f.seek(0)
        f.writelines(debian_control_lines)


parser = argparse.ArgumentParser()
parser.add_argument("version", type=parse_version)
parser.add_argument("--dry-run", action="store_true")
args = parser.parse_args()

rewrite_debian_control_file()
subprocess.check_call(["meson", "rewrite", "kwargs", "set", "project", "/", "version", str(args.version)])
if not args.dry_run:
    subprocess.check_call(["git", "add", "meson.build", "debian_packaging/f.e.i.s-control"])
    subprocess.check_call(["git", "commit", "-m", f"bump to v{args.version}"])
    subprocess.check_call(["git", "tag", f"v{args.version}"])