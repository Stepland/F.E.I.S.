import argparse
import re
import subprocess
from dataclasses import dataclass

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


parser = argparse.ArgumentParser()
parser.add_argument("version", type=parse_version)
args = parser.parse_args()

subprocess.check_call(["meson", "rewrite", "kwargs", "set", "project", "/", "version", str(args.version)])
subprocess.check_call(["git", "add", "meson.build"])
subprocess.check_call(["git", "commit", "-m", f"bump to v{args.version}"])
subprocess.check_call(["git", "tag", f"v{args.version}"])