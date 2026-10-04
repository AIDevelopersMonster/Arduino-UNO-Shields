# Venn Diagram Lesson + Quiz V2

Second variant of the MAR2406 Venn demonstration. The original
`Venn_Diagram_Touch_Learning` example is preserved unchanged.

## Why V2 exists

The first variant allowed touching the diagram during LEARN and reported
`A ONLY`, `A AND B`, `B ONLY`, etc. That mixed two different teaching tasks:
learning the set operation and classifying an arbitrary touched point.

V2 separates them.

## LEARN

The learner does not need to touch the circles.

`NEXT` advances through seven lessons:

1. SET A
2. SET B
3. A AND B
4. A OR B
5. A - B
6. B - A
7. A XOR B

Each page shades the relevant region and gives a short explanation.

## QUIZ

The diagram is shown without a shaded answer.

There are exactly **10 questions**. Each question asks for one region:

- A ONLY
- A AND B
- B ONLY

One touch gives one answer. The program then advances to the next question.
At the end it shows `SCORE n/10`, the wrong-answer count, and buttons for
`AGAIN` or `LEARN`.

## Build

```powershell
arduino-cli compile --fqbn arduino:avr:uno .\shields\MAR2406-2.4-TFT\examples\Venn_Diagram_Lesson_Quiz_v2
```

## Upload

```powershell
arduino-cli upload -p COM4 --fqbn arduino:avr:uno .\shields\MAR2406-2.4-TFT\examples\Venn_Diagram_Lesson_Quiz_v2
```
