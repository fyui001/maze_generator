#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define HEIGHT 11
#define WIDTH 21
#define ROAD 0
#define WALL 1

int map[HEIGHT][WIDTH];

struct {
    int y, x;
} dir[] = {
    {1, 0}, /* DOWN */
    {-1, 0}, /* UP */
    {0, 1}, /* RIGHT */
    {0, -1} /* LEFT */
};

/*
* 迷路を壁で埋める
*/
void maze_init(void)
{
    int x, y;
    for (y = 0; y < HEIGHT; y++) {
        for (x = 0; x < WIDTH; x++) {
            map[y][x] = WALL;
        }
    }
}

/*
* 掘り始める座標を吐き出す
* dim は辺の長さ全体（3 以上の奇数）。[1, dim - 2] の奇数を一様に返す
*/
int gen_rand_odd(int dim)
{
    return 1 + 2 * (rand() % ((dim - 1) / 2));
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
* 入口と出口を開ける
*/
void open_entrance_exit(void)
{
    map[0][1] = ROAD;
    map[HEIGHT - 1][WIDTH - 2] = ROAD;
}

/*
* 迷路の書き出し
*/
void print(void)
{
    int x, y;
    for (y = 0; y < HEIGHT; y++ ) {
        for (x = 0; x < WIDTH; x++) {
            fputs((map[y][x] == WALL) ? "██" : "  ", stdout);
        }
        fputc('\n', stdout);
    }
}

int main(void)
{
    struct timespec ts;
    int x, y;

    /* 同一秒内の連続実行でも異なる迷路になるようナノ秒も混ぜる */
    if (timespec_get(&ts, TIME_UTC) == 0) {
        fprintf(stderr, "timespec_get に失敗しました\n");
        return 1;
    }
    srand((unsigned int)ts.tv_sec ^ (unsigned int)ts.tv_nsec);

    x = gen_rand_odd(WIDTH);
    y = gen_rand_odd(HEIGHT);
    maze_init();
    map[y][x] = ROAD;
    make_maze(y, x);
    open_entrance_exit();
    print();
    return 0;
}
