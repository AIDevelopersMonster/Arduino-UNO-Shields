# Arkanoid / Breakout Touch

Touch-controlled ARKANOID / BREAKOUT demonstrator for the verified Arduino UNO + MAR2406 2.4-inch TFT shield.

## Features

- 5 x 8 colored brick field;
- animated ball;
- three lives;
- score counter;
- brick counter;
- continuous touch-controlled paddle;
- paddle impact point changes the ball direction;
- win and game-over screens;
- touch restart;
- no microSD required.

The lower part of the display acts as a direct touch controller: drag a finger left or right and the paddle follows it.

## Verified hardware target

```text
LCD: ILI9341
Orientation: ROT1 / 320x240
Touch: XP=D6 XM=A2 YP=A1 YM=D7
Calibration: LEFT=153 RIGHT=930 TOP=962 BOTTOM=168
```

## Build

```powershell
arduino-cli compile --fqbn arduino:avr:uno .\shields\MAR2406-2.4-TFT\examples\Arkanoid_Breakout_Touch
```

## Upload

```powershell
arduino-cli upload -p COM4 --fqbn arduino:avr:uno .\shields\MAR2406-2.4-TFT\examples\Arkanoid_Breakout_Touch
```

## Test checklist

1. Touch the title screen to start.
2. Drag a finger across the lower part of the screen.
3. Confirm the paddle follows continuously.
4. Confirm the ball bounces from side/top walls.
5. Confirm brick hits remove exactly one brick and add 10 points.
6. Confirm paddle edge hits change the horizontal ball direction.
7. Miss the ball and confirm the lives counter decreases.
8. Clear all bricks and confirm the YOU WIN screen.
9. Lose all three lives and confirm GAME OVER.
10. Touch either result screen and confirm a new game starts.
