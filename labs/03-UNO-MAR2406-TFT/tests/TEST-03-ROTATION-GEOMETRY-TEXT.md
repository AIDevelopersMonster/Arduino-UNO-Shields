# TEST-03 — rotation, geometry and text

## Purpose

Move from the TEST-02 pixel-write proof to a usable display coordinate system.

TEST-03 will:

1. exercise all four ILI9341 rotations;
2. show the logical width and height for each rotation;
3. draw a border and corner markers;
4. draw lines and rectangles;
5. render a small built-in bitmap font;
6. identify the landscape orientation that matches the physical shield position used in the lab.

## Bench decision

The photographed TEST-02 result shows that MADCTL `0x48` is a portrait logical frame rotated relative to the board as normally viewed.

The candidate landscape settings are:

- rotation 1: MADCTL `0x28`, 320 x 240;
- rotation 3: MADCTL `0xE8`, 320 x 240.

TEST-03 will display both explicitly rather than assuming which one should become the HMI default.

## PASS criteria

Record PASS if:

- all four rotations occupy the full screen;
- borders reach the expected edges;
- corner markers stay in the correct logical corners;
- text is readable;
- horizontal and vertical primitives are not swapped or truncated;
- one 320 x 240 landscape rotation is selected as the project default.

Touch coordinates are **not** part of TEST-03. Touch calibration begins only after the display coordinate system is fixed.
