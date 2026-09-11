# Linux camera and lighting

The battlefield now follows the window size. At 100% zoom, enlarging or
maximising the window shows more terrain at the original sprite size.
The sidebar, radar and cursor stay at their original size.

| Control                             | Action                               |
| ----------------------------------- | ------------------------------------ |
| Hold middle mouse and drag          | Grab the terrain and move the camera |
| Wheel over the battlefield          | Zoom around the pointer              |
| Click the percentage in the top bar | Reset to 100%                        |
| Click `LIGHT: OFF` / `LIGHT: ON`    | Toggle explosion and Tesla lighting  |
| Alt+Enter                           | Toggle desktop fullscreen            |

Zoom stops are 50%, 67%, 80%, 100%, 125%, 150% and 200%. Zoom uses crisp
nearest-neighbour sampling. The camera stops at map boundaries; maps smaller
than the view have black margins. Camera gestures are disabled during unit
selection and building placement, and over the sidebar. Losing focus releases
a middle drag. Original keyboard commands and edge scrolling remain available.

Lighting starts off. Enable it for brief warm explosion light and blue Tesla
flashes. It uses existing animations, respects shroud and palette fades, and
does not create units, alter damage or consume simulation randomness.

Menus, save/load dialogs and movies retain their classic layout and fit the
window. Returning to play restores the native camera. Zoom and lighting are
presentation preferences for the running process, not part of saved games.

Run normally from the repository root:

```sh
scripts/run_linux_dev.sh
```

For the original whole-screen scaling, including the original input mapping:

```sh
RA_CLASSIC_VIEW=1 scripts/run_linux_dev.sh --no-build
```

## Implementation notes

The original indexed renderer draws once into a retained atlas. The original
640-pixel-wide UI region and the battlefield region starting at `(640,16)`
are disjoint, so a world click cannot collide with a sidebar gadget. SDL
composes the battlefield, fixed-size UI and cursor separately. Native camera
state lives outside serialized game classes. Resizing rebinds the atlas at
frame boundaries, before drawing or locking a surface.

Linux requests accelerated SDL presentation with a software fallback. Rendering
uses drawable dimensions; input and UI use window coordinates for consistent
behaviour on displays with different pixel densities.

Regression checks are part of `tests/run_script_tests.sh`: camera layout,
input capture, compositor visibility masking, save compression round trips,
and LCW image decoding. The latter two cover legacy memory errors found while
checking the camera: LZO work buffers must use `LZO1X_MEM_COMPRESS` on 64-bit
hosts, and LCW runs must respect the destination capacity during in-place
image decoding.
