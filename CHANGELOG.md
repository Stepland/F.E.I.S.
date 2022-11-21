# v2.0.0
*(+ 2.0.0-rc.1)*
## 🥝🍇🍓🍊🍏 New Stuff 🍏🍊🍓🍇🥝
- BPM Changes
- memon 1.0 + 0.3 + 0.2 support
- MP3 support

## 🍒 Small improvements 🍒
- Linear View
    - new settings
        - lane order
        - color notes according to quantization
        - cursor height
- Frendlier error message when the UI font is not found in the assets folder
- Playback position is kept instead of being reset to zero when you change charts or reload the audio file
- New menu `Settings > Editor`, allows setting
    - snap value
    - note collision zone size
- History now takes into account when you :
    - Rename a chart (you change its difficulty)
    - Re-rate a chart
    - Add and remove charts
    - Change the timing (either BPMs or offset)
- New Keyboard Shortcuts help dialog

## 🚧 Changes 🚧
- Force using the asset folder next to the executable
- Sound files in the assets have been renamed
- The long note currently being created (by holding right click) is dropped when the mouse stops hovering over the playfield
- The history window changed looks and now displays "current" and "saved" next to the correct history item
- Saving with `File > Save` or `Ctrl+S` now forces saving to isk even if no changes were made
- File Properties Dialog has been reconstructed from scratch

## 🗿 Bugfixes 🗿
- Playfield
    - Fixed the unwanted long note dummy appearing when right clicking outside the playfield *then* hovering over a button, (Fixes #10)
- Linear View
    - Fixed jerky motion when scrolling down (that one was *hard* to track down)
- Density Graph
    - Shows up for charts without music !
    - Updates more often, notably when :
        - cutting/pasting/deleting notes with the keyboard shortcuts or the menu item
        - the audio file changes or is unloaded (because of an invalid path for example) (or more generally when the end point of the chart changes without the notes changing)
        - BPMs are inserted

# v1.1.0
## 🍓 New Stuff 🍓
- Use the mouse wheel to move back and forth in time
- The Edit menu now has actual items inside, like Cut / Copy / Paste etc ...

## 🗿 Bugfixes 🗿
- Long notes textures would show incorrectly on some GPUs, not anymore !
- Long notes would disappear too early if a negative offset was set, not anymore !
- F.E.I.S would crash when trying to put notes too early in the chart, not anymore !
- Density graph would not reload when changing chart, not anymore !

## 🍒 Small improvements 🍒
- The Timeline (big thingy with the note density graph on the right side) now looks PIXEL-PERFECT
- The Playfield now cannot be scrolled, it used to be possible but I did not notice until now
- F.E.I.S would not let you put notes after what it decided was the end of the chart, now it always gives you one extra editable measure after the last note

# v1.0.1
*(+ v1.0.0)*
## 🍏🍓🍉🍎🥝 New Stuff 🥝🍎🍉🍓🍏
- More Edition Controls
    - Stepmania-like `Tab` selection
    - Cut / Copy / Paste / Delete
    - Undo / Redo
        - undoing/redoing an action that toggled notes scrolls to the first toggled note
- Linear View
    - Display the notes in a VSRG fashion
    - Shows the collision zones and the long notes durations as well
- Long Notes
    - F.E.I.S can now display long notes properly !
    - You can also edit long notes, see the wiki for instructions
- Density Graph
    - The Timeline on the side now displays a density graph very similar to the one you can see on the upper portion of the screen while playing a song in jubeat
- Notification System
    - Very similar to ArrowVortex, display a queue of notifications in the top-left corner
- Note Collision help
    - any note that's to close to another will be highlighted in red in both the playfield and the density graph on the side
- Pitch Control
    - Pressing `Shift+Left`/`Shift+Right` slows-down/speed-up the playback, up to 200 and down to 10%
- Chord-specific clap sound
    - Similar to what's done in jubeat analyser, pressing `Shift+F4` toggles the Clap+Chord sound

# v0.1.2
## Fixed
- Fixed a bug that would incorrectly load an empty marker instead of the one selected in the menu

# v0.1.1
## Fixed
- Crash when Using the "Close" Menu Item
## Changed
- Markers now default to loading the first valid folder in the assets

# v0.1.0
Initial release