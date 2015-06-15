#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <sys/time.h>

typedef unsigned long long grid_t;
typedef unsigned short row_t;
typedef unsigned short col_t;
typedef unsigned char cell_t;

#define MAX(a, b)  ((a) > (b) ? (a) : (b))

//#define NEW_HEURISTIC

#ifdef NEW_HEURISTIC
    #define double int
#endif

//#define DEBUG
//#define REPLAY
//#define inline
#define QUICKSTART
#define TIMINGS

static grid_t cell_masks[16];
static row_t *translations_down;
static row_t *translations_up;

#define REVERSED(a, b, c, d) d, c, b, a

#ifndef NEW_HEURISTIC
    static double *heuristics_1;
    static double *heuristics_2;
    static double *heuristics_3;
    static double *heuristics_4;
    static const double heurisitc_prefs[] = /*{0.0053144099999999994, 0.0006561, 8.099999999999999e-05, 1e-05,
        0.04304672099999999, 0.34867844009999993, 2.824295364809999, 22.876792454960995,
        98477.09021836107, 12157.665459056923, 1500.9463529699906, 185.30201888518405,
        797664.4307687246, 6461081.88922667, 52334763.30273603, 423911582.75216174};*/
        {0.0004173281000000001, 2.5921000000000005e-05, 1.61e-06, 1e-07,
        REVERSED(0.006718982410000003, 0.10817561680100003, 1.741627430496101, 28.040201630987227),
        1884016.2153145657, 117019.64070276804, 7268.3006647682005, 451.44724625889444,
        30332661.066564508, 488355843.1716886, 7862529075.064187, 126586718108.53343};
#else
    static double *heuristics_ext;
#endif


static cell_t inline get_cell(const grid_t g, const int i) {
  #ifdef DEBUG
    if (i >= 0 && i < 16) {
  #endif
        return (g & cell_masks[i]) >> 4 * i;
  #ifdef DEBUG
    }
    printf("Error, trying to access cell %d\n", i);
    exit(1);
  #endif
}

static cell_t inline get_cell_rc(const grid_t g, const int row, const int col) {
    return get_cell(g, 4 * row + col);
}

static grid_t inline set_cell(const grid_t g, const int i, const cell_t v) {
  #ifdef DEBUG
    if (i >= 0 && i < 16 && v <= 0xF) {
  #endif
        return (g & (~cell_masks[i])) | ((grid_t) v << (4*i));
  #ifdef DEBUG
    }
    printf("Error, setting %d at %d\n", (int) v, i);
    exit(1);
  #endif
}

static grid_t inline set_cell_rc(const grid_t g, const int row, const int col, const cell_t v) {
    return set_cell(g, 4 * row + col, v);
}

static row_t inline get_row(const grid_t g, const int i) {
  #ifdef DEBUG
    if (i >= 0 && i < 4) {
  #endif
        row_t r = (g >> (16 * i)) & 0xFFFF;
        return r;
  #ifdef DEBUG
    }
    printf("Error, getting row %d\n", i);
    exit(1);
  #endif
}

static grid_t inline set_row(grid_t g, const int i, const row_t row) {
  #ifdef DEBUG
    if (i >= 0 && i < 4) {
  #endif
        g &= ~((grid_t)0xFFFF << (16 * i));
        g |= (((grid_t) row) << 16 * i);
        return g;
  #ifdef DEBUG
    }
    printf("Error, setting row %d\n", i);
    exit(1);
  #endif
}

static col_t inline get_col(grid_t g, int i) {
  #ifdef DEBUG
    if (i >= 0 && i < 4) {
  #endif
        col_t r = 0;
        for (int j = 0; j < 4; j ++) {
            r |= ((col_t) get_cell_rc(g, j, i)) << (4 * j);
        }
        return r;
  #ifdef DEBUG
    }
    printf("Error, getting col %d\n", i);
    exit(1);
  #endif
}

static grid_t inline set_col(grid_t g, int i, col_t col) {
  #ifdef DEBUG
    if (i >= 0 && i < 4) {
  #endif
        for (int j = 0; j < 4; j ++) {
            g = set_cell_rc(g, j, i, (col & (0xF << 4 * j)) >> 4 * j);
        }
        return g;
  #ifdef DEBUG
    }
    printf("Error, setting row %d\n", i);
    exit(1);
  #endif
}

static grid_t move_up(grid_t g) {
    for (int i = 0; i < 4; i ++) {
        g = set_col(g, i, translations_up[get_col(g, i)]);
    }
    return g;
}

static grid_t move_down(grid_t g) {
    for (int i = 0; i < 4; i ++) {
        g = set_col(g, i, translations_down[get_col(g, i)]);
    }
    return g;
}

static grid_t move_left(grid_t g) {
    for (int i = 0; i < 4; i ++) {
        g = set_row(g, i, translations_up[get_row(g, i)]);
    }
    return g;
}

static grid_t move_right(grid_t g) {
    for (int i = 0; i < 4; i ++) {
        g = set_row(g, i, translations_down[get_row(g, i)]);
    }
    return g;
}

static unsigned int inline free_cells(const grid_t g) {
    unsigned int r = 0;
    for (int i = 0; i < 16; i ++) {
        if (get_cell(g, i) == 0) r ++;
    }
    return r;
}

static grid_t spawn(grid_t g) {
    unsigned int f = free_cells(g);
    if (f > 0) {
        int loc = (rand() % f) + 1;
        int v = rand() % 10 == 4 ? 2 : 1;
        int i = 0;
        while (loc > 0) {
            if (get_cell(g, i) == 0) {
                loc --;
            }
            i ++;
        }
        g = set_cell(g, i-1, v);
    }
    return g;
}

static grid_t up(grid_t g) {
    grid_t r = move_up(g);
    if (r != g)
        r = spawn(r);
    return r;
}

static grid_t down(grid_t g) {
    grid_t r = move_down(g);
    if (r != g)
        r = spawn(r);
    return r;
}

static grid_t left(grid_t g) {
    grid_t r = move_left(g);
    if (r != g)
        r = spawn(r);
    return r;
}

static grid_t right(grid_t g) {
    grid_t r = move_right(g);
    if (r != g)
        r = spawn(r);
    return r;
}

static unsigned int inline real_value(cell_t cell) {
    return cell ? 1 << cell : 0;
}

static col_t col_masks[] = {0xF, 0xF0, 0xF00, 0xF000};

static col_t init_translate_down(col_t c) {
    unsigned char merged = 0;
    for (int j = 2; j >= 0; j --) {
        if (c & col_masks[j]) {
            int a = j + 1;
            while (a < 3 && ((c & col_masks[a]) == 0))
                a ++;
            if ((c & col_masks[a]) == 0) {
                c |= (c & col_masks[j]) << (4 * (a - j));
                c &= ~col_masks[j];
            } else if (
                    ((c & col_masks[j]) == ((c & col_masks[a]) >> (4 * (a - j))))
                    && (((c & col_masks[a]) >> (4*a)) < 0xF)
                    && (!(merged & (1 << a) ))
                      ) {
                c += ((col_t)1) << (4*a);
                c &= ~col_masks[j];
                merged |= (1 << a);
            } else if (a > j + 1) {
                c |= (c & col_masks[j]) << (4 * (a - 1 - j));
                c &= ~col_masks[j];
            }
         }
    }
    return c;
}

static col_t init_translate_up(col_t c) {
    unsigned char merged = 0;
    for (int j = 1; j < 4; j ++) {
        if (c & col_masks[j]) {
            int a = j - 1;
            while (a > 0 && ((c & col_masks[a]) == 0))
                a --;
            if ((c & col_masks[a]) == 0) {
                c |= (c & col_masks[j]) >> (4 * (j - a));
                c &= ~col_masks[j];
            } else if (
                    (((c & col_masks[j]) >> 4 * (j - a)) == (c & col_masks[a]))
                    && (((c & col_masks[a]) >> (4*a)) < 0xF)
                    && (!(merged & (1 << a) ))
                      ) {
                c += ((col_t)1) << (4*a);
                c &= ~col_masks[j];
                merged |= (1 << a);
            } else if (a < j - 1) {
                c |= (c & col_masks[j]) >> (4 * (j - a - 1));
                c &= ~col_masks[j];
            }
         }
    }
    return c;
}


static void init() {
    srand(time(NULL));
    //system("/bin/stty raw");
    for(int i = 0; i < 16; i ++) {
        cell_masks[i] = (grid_t) 0xF << (4*i);
    }
    translations_down = (row_t *) malloc(0x10000 * sizeof(row_t));
    translations_up = (row_t *) malloc(0x10000 * sizeof(row_t));
    for (int i = 0; i <= 0xFFFF; i ++) {
  #ifdef DEBUG
        col_t test = init_translate_down((col_t)i);
        if (test < i)
            printf("Warning: down: %.4x gave %.4x\n", i, test);
        test = init_translate_up((col_t)i);
        if (test > i || (test == 0 && i > 0))
            printf("Warning: up: %.4x gave %.4x\n", i, test);
  #endif
        translations_down[i] = init_translate_down((col_t)i);
        translations_up[i] = init_translate_up((col_t)i);
    }
  #ifndef NEW_HEURISTIC
    heuristics_1 = (double *) malloc(0x10000 * sizeof(double));
    heuristics_2 = (double *) malloc(0x10000 * sizeof(double));
    heuristics_3 = (double *) malloc(0x10000 * sizeof(double));
    heuristics_4 = (double *) malloc(0x10000 * sizeof(double));
    for (int i = 0; i <= 0xFFFF; i ++) {
        double r = 0.0;
        for (int j = 0; j < 4 ; j ++) {
            r += heurisitc_prefs[j] * real_value((i & (0xF << (4 * j))) >> (4 * j));
        }
        heuristics_1[i] = r;
        r = 0.0;
        for (int j = 0; j < 4 ; j ++) {
            r += heurisitc_prefs[4+j] * real_value((i & (0xF << (4 * j))) >> (4 * j));
        }
        heuristics_2[i] = r;
        r = 0.0;
        for (int j = 0; j < 4 ; j ++) {
            r += heurisitc_prefs[8+j] * real_value((i & (0xF << (4 * j))) >> (4 * j));
        }
        heuristics_3[i] = r;
        r = 0.0;
        for (int j = 0; j < 4 ; j ++) {
            r += heurisitc_prefs[12+j] * real_value((i & (0xF << (4 * j))) >> (4 * j));
        }
        heuristics_4[i] = r;
    }
  #else
    heuristics_ext = (double *) malloc(0x10000 * sizeof(double));
    for (int i = 0; i <= 0xFFFF; i ++) {
        if ( ( (i & 0xF) >= ((i & 0xF0) >> 4) && (i & 0xF) >= ((i & 0xF00) >> 8) )
           ||( (i & 0xF000) >= ((i & 0xF0) << 8) && (i & 0xF000) >= ((i & 0xF00) << 4) )
           ) {
            heuristics_ext[i] = 10000;
        }
        for (int j = 0; j < 4; j ++) {
            if ((i & (0xF << (4*j))) == 0) {
                heuristics_ext[i] += 5000;
            }
        }

    }
  #endif
}


static void shutdown() {
    free(translations_up);
    free(translations_down);
  #ifndef NEW_HEURISTIC
    free(heuristics_1);
    free(heuristics_2);
    free(heuristics_3);
    free(heuristics_4);
  #else
    free(heuristics_ext);
  #endif
    //system ("/bin/stty cooked");
}

static void print(grid_t g) {
    for(int i = 0; i < 4; i ++) {
        for(int j = 0; j < 4; j ++) {
            printf(" %5d", real_value(get_cell_rc(g, i, j)));
        }
        printf("\n\r");
    }
    printf("\n\r");
}

#ifdef TIMINGS
static unsigned long grids_seen = 0;
#endif

#ifndef NEW_HEURISTIC
static inline double heuristic(grid_t g) {
    /*static double prefs[] = {0.0027 , 0.0009  , 0.0003  , 0.0001,
                             0.0081 , 0.0243  , 0.0729  , 0.2187,
                             17.7147, 5.9049  , 1.9683  , 0.6561,
                             53.1441, 159.4323, 478.2969, 1434.8907};
                             */
  #ifdef TIMINGS
    grids_seen ++;
  #endif
    return heuristics_1[get_row(g, 0)] + heuristics_2[get_row(g, 1)] +
           heuristics_3[get_row(g, 2)] + heuristics_4[get_row(g, 3)];


}
#else
static inline double heuristic(grid_t g) {
  #ifdef TIMINGS
    grids_seen ++;
  #endif
    return heuristics_ext[get_row(g, 0)] + heuristics_ext[get_row(g, 1)] +
           heuristics_ext[get_row(g, 2)] + heuristics_ext[get_row(g, 3)] +
           heuristics_ext[get_col(g, 0)] + heuristics_ext[get_col(g, 1)] +
           heuristics_ext[get_col(g, 2)] + heuristics_ext[get_col(g, 3)];


}
#endif

#define MAX_DEPTH 8

static double expect(grid_t, int);

enum move {UP, DOWN, LEFT, RIGHT, NONE};
static enum move best;

static double imax(grid_t g, int depth) {
    if (depth > MAX_DEPTH) return heuristic(g);
    double m = -1.0;
    double t;
    enum move b = NONE;
    grid_t test = move_up(g);
    if (test != g) {
        t = expect(test, depth+1);
  #ifdef REPLAY
        if (depth <= 0) {
            printf("%*s Up  : expect = %lf\n", 2*depth, "", t);
        }
  #endif
        if (t > m) {
            m = t;
            b = UP;
        }
    }
    test = move_down(g);
    if (test != g) {
        t = expect(test, depth+1);
  #ifdef REPLAY
        if (depth <= 0) {
            printf("%*s Down: expect = %lf\n", 2*depth, "", t);
        }
  #endif
        if (t > m) {
            m = t;
            b = DOWN    ;
        }
    }
    test = move_left(g);
    if (test != g) {
        t = expect(test, depth+1);
  #ifdef REPLAY
        if (depth <= 0) {
            printf("%*s Left: expect = %lf\n", 2*depth, "", t);
        }
  #endif
        if (t > m) {
            m = t;
            b = LEFT;
        }
    }
    test = move_right(g);
    if (test != g) {
        t = expect(test, depth+1);
  #ifdef REPLAY
        if (depth <= 0) {
            printf("%*s Right expect = %lf\n", 2*depth, "", t);
        }
  #endif
        if (t > m) {
            m = t;
            b = RIGHT;
        }
    }
    /*if (depth == 0)*/ best = b;
    return m;
}

#ifdef REPLAY
static void print_best() {
    switch(best) {
        case UP:
            printf("Best choice: up\n");
            break;
        case DOWN:
            printf("Best choice: down\n");
            break;
        case LEFT:
            printf("Best choice: left\n");
            break;
        case RIGHT:
            printf("Best choice: right\n");
            break;
        default:
            printf("No decision made\n");
            break;
    }
}
#endif


static double expect(grid_t g, int depth) {
    if (depth > MAX_DEPTH) return heuristic(g);
    double s = 0;
    int n = 0;
    for (int i = 0; i < 16; i ++) {
        if (get_cell(g, i) == 0) {
            n += 10;
            grid_t test = set_cell(g, i, 1);
            s += 9 * imax(test, depth + 1);
  #ifdef REPLAY
            if (depth == 1) {
                printf("Trying 2 at %d, result = %lf. ", i, imax(test, depth+1));
                print_best();
            }
  #endif
            test = set_cell(g, i, 2);
  #ifdef REPLAY
            if (depth == 1) {
                printf("Trying 4 at %d, result = %lf. ", i, imax(test, depth+1));
                print_best();
            }
  #endif
            s += imax(test, depth + 1);
        }
    }
    return s / n;
}

#ifdef REPLAY
static cell_t from_real(int val) {
    if (val == 0) return 0;
    cell_t r = 0;
    while (!(val & 0x1)) {
        val >>= 1;
        r ++;
    }
    return r;
}

static void replay() {
    int array[] = {  0   ,  0  ,   2 ,    0,
                     2  ,   0 ,    2,     0,
                     8  ,   0 ,    8,     4,
                    16  ,  64 ,   64,   512};
    grid_t g = 0;
    for (int i = 0; i < 16; i ++) {
        g = set_cell(g, i, from_real(array[i]));
        //print(g);
    }
    print(g);
    imax(g, 0);
    print_best();

}
#endif

static void autoplay(grid_t g) {
  #ifdef QUICKSTART
    for(int i = 0; i < 25; i ++) {
        g = down(g);
        g = right(g);
    }
    print(g);
  #endif
    do {
      #ifdef TIMINGS
        static struct timeval tvs, tve;
        grids_seen = 0;
        gettimeofday(&tvs, NULL);
      #endif
        imax(g, 0);
      #ifdef TIMINGS
        gettimeofday(&tve, NULL);
        int usecs = 1000000 * (tve.tv_sec - tvs.tv_sec) + (tve.tv_usec - tvs.tv_usec);
        printf("Timings: %9lu grids seen in %4d ms (%luM/s)\n", grids_seen, usecs / 1000, grids_seen / MAX(usecs, 1));
      #endif
        switch(best) {
            case UP:
                g = up(g);
                break;
            case DOWN:
                g = down(g);
                break;
            case LEFT:
                g = left(g);
                break;
            case RIGHT:
                g = right(g);
                break;
            case NONE:
                break;
        }
        print(g);
    } while (best != NONE);
}

static grid_t newgame() {
    grid_t g = 0;
    g = spawn(g);
    g = spawn(g);
    return g;
}


int main(void) {
  #ifdef DEBUG
    printf("Warning: compiled in debug, gonna be slow\n");
  #endif
    init();
  #ifdef REPLAY
    printf("Starting replay...\n");
    replay();
  #else
    grid_t g = newgame();
    print(g);
    while(1) {
        int c = getchar();
        if (c == 'z')
            g = up(g);
        if (c == 'q')
            g = left(g);
        if (c == 's')
            g = down(g);
        if (c == 'd')
            g = right(g);
        if (c == 'a')
            autoplay(g);
        if (c == 'h')
            imax(g, 0);
        if (c == 'r') {
            printf("Reset.\n");
            g = newgame();
        }
        if (c == 'x')
            break;
        printf("%u\n", free_cells(g));
        print(g);
    }
  #endif
    shutdown();
    return 0;
}

