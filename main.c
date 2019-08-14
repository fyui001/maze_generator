#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define WIDTH 51
#define HEIGHT 51
#define ROAD 0
#define WALL 1

int map[WIDTH][HEIGHT];

struct {
    int y, x;
} dir[] = {
    {1, 0}, /* UP */
    {-1, 0}, /* DOWN */
    {0, 1}, /* RIGHT */
    {0, -1} /* LEFT */
};

/*
* 迷路を壁で埋める
*/
void maze_init()
{
    int x, y;
    for (y = 0; y < WIDTH; y++) {
        for (x = 0; x < HEIGHT; x++) {
            map[y][x] = WALL;
        }
    }
}

/*
* 掘り始める座標を吐き出す
*/
int gen_rand_odd(int mod)
{
    int r = rand() % mod;
    if (r % 2 == 0) {
        r++;
    } else if (r > mod) {
        r -= 2;
    }
    return r;
}

void make_maze(int y, int x)
{
    int d = rand() % 4;
    int ds = d;

    /* 掘り進める方向を決める */
    while(1) {
        /* 2つ先の座標を記憶する */
        int py = y + dir[d].y * 2;
        int px = x + dir[d].x * 2;

        if ( px < 0 || px >= WIDTH || py < 0 || py >= HEIGHT || map[py][px] != WALL ) {
            d++;
            if (d == 4) {
                d = 0;
            }
            if (d == ds) {
                return;
            }
            continue;
        }
        map[y + dir[d].y][x + dir[d].x] = ROAD;
        map[py][px] = ROAD;
        make_maze(py, px);
    }
}

/*
* 迷路の書き出し
*/
void print()
{
    int x, y;
    map[0][1] = 0;
    map[HEIGHT - 1][WIDTH-2] = 0;
    for (y = 0; y < WIDTH; y++ ) {
        for (x = 0; x < HEIGHT; x++) {
            fprintf(stdout, "%s ", (map[y][x] == WALL) ? "■" : " ");
        }
        fprintf(stdout, "\n");
    }
}

int main()
{
    time_t t;
    int x = gen_rand_odd(WIDTH - 2);
    int y = gen_rand_odd(HEIGHT - 2);
    time(&t);
    srand(t);
    maze_init();
    make_maze(y, x);
    print();
    return 0;
}
