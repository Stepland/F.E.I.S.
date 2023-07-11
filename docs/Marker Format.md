# The marker format(s)

Markers are stored in `assets/textures/markers/`

one directory = one marker

let's call that directory `folder/`

`folder/preview.png` will be used as preview if present

## Old Format

| Animation | File names |
|-|-|
| *(Approach)* | `ma00.png` → `ma15.png` |
| PERFECT | `h400.png` → `h415.png` |
| GREAT | `h300.png` → `h315.png` |
| GOOD | `h200.png` → `h215.png` |
| POOR | `h100.png` → `h115.png` |
| MISS | `ma16.png` → `ma23.png` |

Markers in the old format run at 30 fps.

## Jujube format

`folder/marker.json` has to exist. It has this structure :

```json
{
    "name": "jubeat analyser",
    "size": 150,
    "fps": 30,
    "approach": {
        "sprite_sheet": "approach.png",
        "count": 16,
        "columns": 4,
        "rows": 4
    },
    "miss": {
        "sprite_sheet": "miss.png",
        "count": 9,
        "columns": 3,
        "rows": 3
    },
    "poor": {
        "sprite_sheet": "poor.png",
        "count": 9,
        "columns": 5,
        "rows": 2
    },
    "good": {
        "sprite_sheet": "good.png",
        "count": 9,
        "columns": 5,
        "rows": 2
    },
    "great": {
        "sprite_sheet": "great.png",
        "count": 9,
        "columns": 5,
        "rows": 2
    },
    "perfect": {
        "sprite_sheet": "perfect.png",
        "count": 9,
        "columns": 5,
        "rows": 2
    }
}
```

| Key | Meaning |
|-|-|
| `size` | side length of each frame, in pixels |
| `fps` | number of frames per seconds of this marker |
| `*.count` | how many frames are used in a given sprite sheet |

`*.sprite_sheet` can be either an absolute or a relative path.
Relative is prefered. Relative paths are relative to the `marker.json` file.

Sprites in a sheet are ordrered left to right, top to bottom :

```none
1 2 3
4 5 6
7 8 9
```

`count` can be <= `columns` * `rows`. It means the last few frames in
the sheet are unused. For instance in a 3 * 3 sprite sheet with `count` = 7 :

```none
1 2 3  
4 5 6
7 _ _
```