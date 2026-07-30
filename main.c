#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define ROAD 0
#define WALL 1
#define AT(y, x) map[(y) * (width) + (x)]

int height = 11;
int width = 21;
unsigned char *map = NULL;

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
    for (y = 0; y < height; y++) {
        for (x = 0; x < width; x++) {
            AT(y, x) = WALL;
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

typedef struct {
    int y;
    int x;
    unsigned char d;
    unsigned char ds;
} Frame;

/*
* 掘った先を最上段に積んだまま親の d に触れないことで、再帰版が子から戻って同じ方向を
* 再評価する挙動を LIFO でそのまま再現する
*/
void make_maze(int y, int x, Frame *stack)
{
    size_t len = 0;

    stack[len].y = y;
    stack[len].x = x;
    stack[len].d = (unsigned char)(rand() % 4);
    stack[len].ds = stack[len].d;
    len++;

    /* 掘り進める方向を決める */
    while (len > 0) {
        Frame *top = &stack[len - 1];
        /* 2つ先の座標を記憶する */
        int py = top->y + dir[top->d].y * 2;
        int px = top->x + dir[top->d].x * 2;

        if (px < 0 || px >= width || py < 0 || py >= height || AT(py, px) != WALL) {
            top->d++;
            if (top->d == 4) {
                top->d = 0;
            }
            if (top->d == top->ds) {
                len--;
            }
            continue;
        }
        AT(top->y + dir[top->d].y, top->x + dir[top->d].x) = ROAD;
        AT(py, px) = ROAD;

        stack[len].y = py;
        stack[len].x = px;
        stack[len].d = (unsigned char)(rand() % 4);
        stack[len].ds = stack[len].d;
        len++;
    }
}

/*
* 入口と出口を開ける
*/
void open_entrance_exit(void)
{
    AT(0, 1) = ROAD;
    AT(height - 1, width - 2) = ROAD;
}

/*
* 迷路の書き出し
*/
void print(void)
{
    int x, y;
    for (y = 0; y < height; y++ ) {
        for (x = 0; x < width; x++) {
            fputs((AT(y, x) == WALL) ? "██" : "  ", stdout);
        }
        fputc('\n', stdout);
    }
}

/*
* 寸法文字列をパースする。3〜711 の奇数のみ受け付け、long のまま判定して桁あふれも弾く
*/
static int parse_dim(const char *s, int *out)
{
    char *endptr;
    long v;

    errno = 0;
    v = strtol(s, &endptr, 10);
    if (endptr == s || *endptr != '\0') {
        fprintf(stderr, "寸法が整数ではありません: %s\n", s);
        return 1;
    }
    if (v < 3 || v > 711 || v % 2 != 1) {
        fprintf(stderr, "寸法は 3〜711 の奇数で指定してください: %s\n", s);
        return 1;
    }
    *out = (int)v;
    return 0;
}

int main(int argc, char *argv[])
{
    struct timespec ts;
    Frame *stack;
    int x, y;

    if (argc >= 4) {
        fprintf(stderr, "使用法: %s [DIM | HEIGHT WIDTH]\n", argv[0]);
        return 1;
    }
    if (argc == 2) {
        if (parse_dim(argv[1], &height) != 0) {
            return 1;
        }
        width = height;
    } else if (argc == 3) {
        if (parse_dim(argv[1], &height) != 0 || parse_dim(argv[2], &width) != 0) {
            return 1;
        }
    }

    /* 同一秒内の連続実行でも異なる迷路になるようナノ秒も混ぜる */
    if (timespec_get(&ts, TIME_UTC) == 0) {
        fprintf(stderr, "timespec_get に失敗しました\n");
        return 1;
    }
    srand((unsigned int)ts.tv_sec ^ (unsigned int)ts.tv_nsec);

    map = malloc((size_t)height * (size_t)width * sizeof(*map));
    if (map == NULL) {
        fprintf(stderr, "迷路用メモリの確保に失敗しました\n");
        return 1;
    }

    /* 各 push は WALL だった論理セルを 1 つ ROAD 化するため、push 総数は論理セル総数を超えない */
    stack = malloc((size_t)((height - 1) / 2) * (size_t)((width - 1) / 2) * sizeof(*stack));
    if (stack == NULL) {
        fprintf(stderr, "穴掘り用スタックの確保に失敗しました\n");
        free(map);
        return 1;
    }

    x = gen_rand_odd(width);
    y = gen_rand_odd(height);
    maze_init();
    AT(y, x) = ROAD;
    make_maze(y, x, stack);
    open_entrance_exit();
    print();
    free(stack);
    free(map);
    return 0;
}
