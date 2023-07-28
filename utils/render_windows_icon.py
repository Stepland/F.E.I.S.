import argparse
from io import BytesIO
from pathlib import Path

import cairosvg
from PIL import Image

parser = argparse.ArgumentParser(description="Render the given .svg file to a windows .ico file")
parser.add_argument("svg", type=Path)

args = parser.parse_args()

png_data = BytesIO(cairosvg.svg2png(url=str(args.svg), parent_height=256, parent_width=256))
image = Image.open(png_data, formats=["PNG"])
image.save(args.svg.with_suffix(".ico"), format="ICO")