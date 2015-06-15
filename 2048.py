# #!/usr/bin/env python
# -*- coding: utf-8 -*-

import sys, copy, random, math
from futures.thread import ThreadPoolExecutor

try:
    import tty, termios
except ImportError:
    # Probably Windows.
    try:
        import msvcrt
    except ImportError:
        # FIXME what to do on other platforms?
        # Just give up here.
        raise ImportError('getch not available')
    else:
        getch = msvcrt.getch
else:
    def getch():
        """getch() -> key character

        Read a single keypress from stdin and return the resulting character.
        Nothing is echoed to the console. This call will block if a keypress
        is not already available, but will not wait for Enter to be pressed.

        If the pressed key was a modifier key, nothing will be detected; if
        it were a special function key, it may return the first character of
        of an escape sequence, leaving additional characters in the buffer.
        """
        fd = sys.stdin.fileno()
        old_settings = termios.tcgetattr(fd)
        try:
            tty.setraw(fd)
            ch = sys.stdin.read(1)
        finally:
            termios.tcsetattr(fd, termios.TCSADRAIN, old_settings)
        return ch

GRID_SIZE = 4

def cell(i, j):
    return GRID_SIZE * j + i

#heuristic helper
PREFS = [1.1 + float(i) * 0.8 / (GRID_SIZE * GRID_SIZE) for i in range(GRID_SIZE * GRID_SIZE)]
#for i in range(0, GRID_SIZE, 2):
#    PREFS[i * GRID_SIZE: (i+1) * GRID_SIZE] = list(reversed(PREFS[i * GRID_SIZE: (i+1) * GRID_SIZE]))
for i in range(0, GRID_SIZE, 2):
    avg = sum(PREFS[i * GRID_SIZE: (i+1) * GRID_SIZE]) / GRID_SIZE
    for j in range(GRID_SIZE):
        PREFS[cell(i, j)] += 2 * avg
        PREFS[cell(i, j)] /= 3.0

PREFS = [float(16.1 ** i) / 10000000.0 for i in range(GRID_SIZE * GRID_SIZE)]
for i in range(0, GRID_SIZE, 2):
    PREFS[i * GRID_SIZE: (i+1) * GRID_SIZE] = list(reversed(PREFS[i * GRID_SIZE: (i+1) * GRID_SIZE]))
print PREFS

ORDER = range(GRID_SIZE * GRID_SIZE)
for i in range(0, GRID_SIZE, 2):
    ORDER[i * GRID_SIZE: (i+1) * GRID_SIZE] = list(reversed(ORDER[i * GRID_SIZE: (i+1) * GRID_SIZE]))
print ORDER

def mavg(seq):
    if len(seq) == 0:
        return -1
    return sum(seq) / len(seq)


class Grid:
    def __init__(self):
        self.reset()

    def copy(self, source):
        self.numfree = source.numfree
        self.grid = source.grid[:]


    def reset(self):
        self.grid = [0 for i in range(GRID_SIZE * GRID_SIZE)]
        self.numfree = GRID_SIZE * GRID_SIZE
        self.moved = True

    def start(self):
        self.reset()
        self.spawn()
        self.spawn()

    def freecell(self):
        if self.numfree <= 0:
            return -1
        i = random.randint(1, self.numfree)
        for j in range(len(self.grid)):
            if self.grid[j] == 0:
                i -= 1
                if i == 0:
                    return j

    def spawn(self):
        value = 2
        if random.random() >= 0.9:
            value = 4
        self.grid[self.freecell()] = value
        self.numfree -= 1

    def canmove(self):
        if self.numfree > 0:
            return True
        for i in range(GRID_SIZE):
            for j in range(GRID_SIZE):
                if i < GRID_SIZE - 1 and self.grid[cell(i, j)] == self.grid[cell(i+1, j)]:
                    return True
                if j < GRID_SIZE - 1 and self.grid[cell(i, j)] == self.grid[cell(i, j+1)]:
                    return True
        return False

    def value(self):
        r = 70000.0
        r += sum([self.grid[i] * PREFS[i] for i in range(len(PREFS))])
        r += 128 * self.numfree
        if self.grid[cell(0, 2)] > self.grid[cell(0, 3)]:
            r -= 50000
        if self.grid[cell(3, 1)] > self.grid[cell(3, 2)]:
            r -= 20000
        return r

    def value2(self):
        r = 100 * self.numfree
        for i in range(GRID_SIZE):
            #column
            m, midx = -1, -1
            for j in range(GRID_SIZE):
                if self.grid[cell(i, j)] > m:
                    m, midx = self.grid[cell(i, j)], j
            if midx in [0, GRID_SIZE - 1] and m > 0:
                r += 100
            #row
            m, midx = -1, -1
            for j in range(GRID_SIZE):
                if self.grid[cell(j, i)] > m:
                    m, midx = self.grid[cell(i, j)], j
            if midx in [0, GRID_SIZE - 1] and m > 0:
                r += 100
        return r

    def value3(self):
        r = 50 * self.numfree
        for j in range(GRID_SIZE-1):
            for i in range(GRID_SIZE):
                if j%2 == 0:
                    if self.grid[cell(i,j)] >= self.grid[cell(i, j+1)]:
                        r += 10 * j
                else:
                    if self.grid[cell(i,j)] <= self.grid[cell(i, j+1)]:
                        r += 50 * (j-1)
        for i in range(GRID_SIZE):
            for j in range(GRID_SIZE-1):
                if self.grid[cell(i, j)] > 0 and self.grid[cell(i, j)] < self.grid[cell(i, j+1)]:
                    r += 20 * i
        return r

    def value4(self):
        r = self.grid[15]
        c = 1
        for i in range(1, 16):
            if self.grid[ORDER[15-i]] > 0 and self.grid[ORDER[15-i]] <= self.grid[ORDER[16-i]]:
                c += 1
                r *= c
            else:
                break
        r += 1000 * self.numfree
        return r


    def expect(self, depth, max_depth):
        if depth >= max_depth:
            return self.value()
        s = 0
        n = 0
        self.numfree -= 1
        for i in range(len(self.grid)):
            if self.grid[i] == 0:
                n += 10
                self.grid[i] = 2
                s += 9 * self.imax(depth+1, max_depth)
                self.grid[i] = 4
                s += self.imax(depth+1, max_depth)
                self.grid[i] = 0
        self.numfree += 1
        return s / n

    def mini(self, depth, max_depth):
        if depth >= max_depth:
            return self.value4()
        m = 0
        self.numfree -= 1
        for i in range(len(self.grid)):
            if self.grid[i] == 0:
                self.grid[i] = 2
                s = self.imax(depth+1, max_depth)
                if s < m:
                    m = s
                self.grid[i] = 4
                s = self.imax(depth+1, max_depth)
                if s < m:
                    m = s
                self.grid[i] = 0
        self.numfree += 1
        return m

    def imax(self, depth = 0, max_depth = 5):
        if depth >= max_depth:
            return self.value()
        moves = ["down", "right", "left", "up"]
        bestv = -1
        bestmove = ""
        a = Grid()
        for move in moves:
            a.copy(self)
            exec("a.move_"+move+"()")
            if a.moved:
                v = a.expect(depth+1, max_depth)
                if v > bestv:
                    bestv = v
                    bestmove = move
        if depth == 0:
            return bestmove
        return bestv


    def advise(self):
        return self.imax()

    def move_up(self):
        self.moved = False
        merged = []
        for i in range(GRID_SIZE):
            for j in range(1, GRID_SIZE):
                if self.grid[cell(i, j)] > 0:
                    a = j-1
                    while a > 0 and self.grid[cell(i, a)] == 0:
                        a -= 1
                    if self.grid[cell(i, a)] == 0:
                        #we're at the top
                        self.grid[cell(i, a)] = self.grid[cell(i, j)]
                        self.grid[cell(i, j)] = 0
                        self.moved = True
                    elif self.grid[cell(i, a)] == self.grid[cell(i, j)] and not cell(i, a) in merged:
                        self.grid[cell(i, a)] *= 2
                        self.grid[cell(i, j)] = 0
                        merged += [cell(i, a)]
                        self.numfree += 1
                        self.moved = True
                    elif a < j-1:
                        self.grid[cell(i, a+1)] = self.grid[cell(i, j)]
                        self.grid[cell(i, j)] = 0
                        self.moved = True
        return self.moved

    def up(self):
        if self.move_up():
            self.spawn()

    def move_down(self):
        self.moved = False
        merged = []
        for i in range(GRID_SIZE):
            for j in range(GRID_SIZE - 2, -1, -1):
                if self.grid[cell(i, j)] > 0:
                    a = j+1
                    while a < GRID_SIZE - 1 and self.grid[cell(i, a)] == 0:
                        a += 1
                    if self.grid[cell(i, a)] == 0:
                        #we're at the bottom
                        self.grid[cell(i, a)] = self.grid[cell(i, j)]
                        self.grid[cell(i, j)] = 0
                        self.moved = True
                    elif self.grid[cell(i, a)] == self.grid[cell(i, j)] and not cell(i, a) in merged:
                        self.grid[cell(i, a)] *= 2
                        self.grid[cell(i, j)] = 0
                        merged += [cell(i, a)]
                        self.numfree += 1
                        self.moved = True
                    elif a > j + 1:
                        self.grid[cell(i, a-1)] = self.grid[cell(i, j)]
                        self.grid[cell(i, j)] = 0
                        self.moved = True
        return self.moved

    def down(self):
        if self.move_down():
            self.spawn()

    def move_left(self):
        self.moved = False
        merged = []
        for j in range(GRID_SIZE):
            for i in range(1, GRID_SIZE):
                if self.grid[cell(i, j)] > 0:
                    a = i - 1
                    while a > 0 and self.grid[cell(a, j)] == 0:
                        a -= 1
                    if self.grid[cell(a, j)] == 0:
                        #we're at the top
                        self.grid[cell(a, j)] = self.grid[cell(i, j)]
                        self.grid[cell(i, j)] = 0
                        self.moved = True
                    elif self.grid[cell(a, j)] == self.grid[cell(i, j)] and not cell(a, j) in merged:
                        self.grid[cell(a, j)] *= 2
                        self.grid[cell(i, j)] = 0
                        merged += [cell(a, j)]
                        self.numfree += 1
                        self.moved = True
                    elif a < i - 1:
                        self.grid[cell(a + 1, j)] = self.grid[cell(i, j)]
                        self.grid[cell(i, j)] = 0
                        self.moved = True
        return self.moved

    def left(self):
        if self.move_left():
            self.spawn()

    def move_right(self):
        self.moved = False
        merged = []
        for j in range(GRID_SIZE):
            for i in range(GRID_SIZE - 2, -1, -1):
                if self.grid[cell(i, j)] > 0:
                    a = i + 1
                    while a < GRID_SIZE - 1 and self.grid[cell(a, j)] == 0:
                        a += 1
                    if self.grid[cell(a, j)] == 0:
                        #we're at the top
                        self.grid[cell(a, j)] = self.grid[cell(i, j)]
                        self.grid[cell(i, j)] = 0
                        self.moved = True
                    elif self.grid[cell(a, j)] == self.grid[cell(i, j)] and not cell(a, j) in merged:
                        self.grid[cell(a, j)] *= 2
                        self.grid[cell(i, j)] = 0
                        merged += [cell(a, j)]
                        self.numfree += 1
                        self.moved = True
                    elif a > i + 1:
                        self.grid[cell(a - 1, j)] = self.grid[cell(i, j)]
                        self.grid[cell(i, j)] = 0
                        self.moved = True
        return self.moved

    def right(self):
        if self.move_right():
            self.spawn()


    def __str__(self):
        f = "{0: <"+str(GRID_SIZE)+"} "
        s = ''
        for j in range(GRID_SIZE):
            for i in range(GRID_SIZE):
                s += f.format(self.grid[cell(i, j)])
            s += '\n'
        #s += str(self.numfree)
        return s

def autoplay2(grid):
    while grid.canmove():
        r = grid.advise()
        print r, grid.value(), grid.grid
        eval("g."+r+"()")
        print grid

def autoplay(grid):
    while grid.canmove():
        while True:
            grid.up()
            #print(grid)
            m = grid.moved
            grid.left()
            #print(grid)
            if not m and not grid.moved:
                break
        while True:
            grid.up()
            #print(grid)
            m = grid.moved
            grid.right()
            #print(grid)
            if not m and not grid.moved:
                break
        print grid


if __name__ == "__main__":
    arrows = {'A': 'z', 'B': 's', 'C': 'd', 'D': 'q'}
    g = Grid()
    g.start()
    while True:
        print(g)
        if not g.canmove():
            print("Lost!")
            break
        i = getch()
        #arrows
        if i == '\x1b': #escape sequence
            getch() #ignore next
            i = arrows[getch()]
        if i == "a":
            autoplay2(g)
        elif i == "n":
            #new game
            g.start()
        elif i == "z":
            g.up()
        elif i =="s":
            g.down()
        elif i == "q":
            g.left()
        elif i == "d":
            g.right()
        elif i == "h":
            #hint
            r = g.advise()
            print r
            eval("g."+r+"()")
        elif i in ["x", '\x03']:
            print "Quitting."
            break
        else:
            pass


    # g.grid = [2048, 65536, 131072, 524288, 1048576, 262144, 65536, 2048,
    #             1024, 4096, 16384,32768,131072,32768,8192, 512,
    #             256,  2048, 4096, 8192, 16384, 4096, 1024, 256,
    #             64,   256,  512,  1024, 2048, 512,  256,  64,
    #             8,    16,   32,   4,    64,   256,  64,   4,
    #             4,    8,    16,   32,   4,    2,    4,    2,
    #             0,    0,    0,    0,    0,    0,    0,    0,
    #             0,    0,    0,    0,    0,    0,    0,    0]
    # g.numfree = 16

