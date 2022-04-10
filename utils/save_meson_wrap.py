"""implement the missing --save option of meson subprojects packagefiles
for wrap-git packages"""

from argparse import ArgumentParser
from path import Path

parser = ArgumentParser()
parser.add_argument("wrap", type=Path)
args = parser.parse_args()

subprojects = Path("subprojects")
if not subprojects.exists():
    print(f"{subprojects} folder doesn't exist, are you in the right directory ?")

subproject = subprojects / args.wrap
patch = subprojects / "packagefiles" / args.wrap
if not patch.exists():
    print(f"subproject folder doesn't exist ({subproject}), is the name correct ?")

for absolute in subproject.walkfiles("*meson*"):
    relative = absolute.relpath(subproject)
    (patch / relative).parent.makedirs_p()
    absolute.copy(patch / relative)

