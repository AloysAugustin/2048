# 2048
A 2048 AI

### Usage

Compile the C version with `gcc -o 2048 -O3 2048.c`

Both versions can be played with ZQSD (change the keys if you are not on an AZERTY keyboard), 
A triggers the AI, H displays the move that the AI would choose and X quits the game.

The AI usually reaches 4096 or 8192 with the default settings. The AI could probably be improved
by tweaking the constants in `heuristic_prefs`. If you find some constants that work better let me know ;-)

### Code details

The branch `adaptive-depth` has a small speed improvement for the C version, that should not change the end result.

The C version is storing a game state in a single 64 bits long. This is possible because, in this game, the value of the cells are
always powers of 2. By storing their logarithm in base 2, we can store the value of a cell on 4 bits, so the whole grid fits in 16*4=64 bits.

A few defines allow to change the behaviour of the C version:
* NEW_HEURISTIC enables an attemps to find a better heuristic.
* DEBUG enables a few checks but should not be needed unless you modify the core of the code
* REPLAY allows to understand the choices of the AI for a given grid, filled in the `replay()` function. This is useful to debug the AI.
* QUICKSTART allows to accelerate the start of the game by just playing down and right alternatively a few times.
