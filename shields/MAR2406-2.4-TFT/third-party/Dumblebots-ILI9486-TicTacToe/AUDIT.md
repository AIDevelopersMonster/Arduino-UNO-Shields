# Compatibility audit — ILI9486 Tic-Tac-Toe vs MAR2406

## Verdict

**Reference value: HIGH**

**Direct compatibility with MAR2406: NO**

The upstream project proves that Arduino UNO R3 can run a TFT/touch Tic-Tac-Toe game in which the microcontroller itself acts as the opponent. However, it targets a different display controller, geometry and touch wiring.

## Required changes before a MAR2406 port

- replace ILI9486 initialization with the verified ILI9341 initialization;
- redesign the 320 x 480 portrait coordinates for 320 x 240 landscape;
- replace touch pins with XP=D6, XM=A2, YP=A1, YM=D7;
- use our certified touch transform/calibration;
- add proper press/release debouncing as used by LAB-03;
- replace or intentionally keep the random opponent;
- if persistent history is required, integrate with the existing TEST-08 SD log rather than the upstream game;
- provide a replay/new-game path instead of halting forever at endgame.

## Opponent logic

Upstream computer move, conceptually:

```text
repeat:
    x = random cell column
    y = random cell row
until selected cell is empty
play there
```

This is computationally trivial for ATmega328P. A stronger opponent can therefore be added without any concern about CPU speed. Our real constraint is program Flash, because the current TEST-08 already occupies 95% of UNO program storage.

A compact rule-based opponent is likely the best first upgrade for our project:

1. take a winning move;
2. block an immediate human win;
3. take center;
4. take an available corner;
5. take an available side.

A full minimax search is also computationally feasible on UNO, but must be evaluated against the remaining Flash budget.
