#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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
* stack は ((height - 1) / 2) * ((width - 1) / 2) 要素以上を要求する（確保は呼び出し側の責務）
* 親の d を進めずに子を push することが、子から戻って同じ方向を再評価する再帰版の再現になる
*/
void make_maze(int y, int x, Frame *stack)
{
    size_t len = 0;

    stack[len].y = y;
    stack[len].x = x;
    stack[len].d = (unsigned char)(rand() % 4);
    stack[len].ds = stack[len].d;
    len++;

    while (len > 0) {
        Frame *top = &stack[len - 1];
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
* 入口（上端の x=1）の map 添字。y=0 なので添字は x と一致する
*/
int entrance_index(void)
{
    return 1;
}

/*
* 出口（下端の x=width-2）の map 添字
*/
int exit_index(void)
{
    return (height - 1) * width + (width - 2);
}

/*
* 入口と出口を開ける
*/
void open_entrance_exit(void)
{
    map[entrance_index()] = ROAD;
    map[exit_index()] = ROAD;
}

/*
* 入口から出口までを BFS で探索し、到達できたら 1 を返す
* queue・parent_dir は height * width 要素以上を要求する（確保は呼び出し側の責務）
* parent_dir には親からの到達方向（dir[] の添字）が残る。4 = 入口、0xFF = 未訪問
*/
int solve(int *queue, unsigned char *parent_dir)
{
    size_t head = 0, tail = 0;

    memset(parent_dir, 0xFF, (size_t)height * (size_t)width * sizeof(*parent_dir));

    queue[tail++] = entrance_index();
    parent_dir[entrance_index()] = 4;

    while (head < tail) {
        int idx = queue[head++];
        int y = idx / width;
        int x = idx % width;
        int d;

        if (idx == exit_index()) {
            return 1;
        }

        for (d = 0; d < 4; d++) {
            int ny = y + dir[d].y;
            int nx = x + dir[d].x;
            int nidx;

            if (nx < 0 || nx >= width || ny < 0 || ny >= height || AT(ny, nx) != ROAD) {
                continue;
            }
            nidx = ny * width + nx;
            if (parent_dir[nidx] != 0xFF) {
                continue;
            }
            parent_dir[nidx] = (unsigned char)d;
            queue[tail++] = nidx;
        }
    }

    return 0;
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

double elapsed_ms(const struct timespec *start, const struct timespec *end)
{
    return (double)(end->tv_sec - start->tv_sec) * 1000.0 + (double)(end->tv_nsec - start->tv_nsec) / 1000000.0;
}

/*
* 寸法文字列をパースする。3〜8191 の奇数のみ受け付け、long のまま判定して桁あふれも弾く
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
    if (v < 3 || v > 8191 || v % 2 != 1) {
        fprintf(stderr, "寸法は 3〜8191 の奇数で指定してください: %s\n", s);
        return 1;
    }
    *out = (int)v;
    return 0;
}

int main(int argc, char *argv[])
{
    struct timespec ts;
    struct timespec t0, t1;
    Frame *stack;
    int *queue;
    unsigned char *parent_dir;
    int x, y;
    int cur;
    int reached;
    long path_len;
    double gen_ms, solve_ms;
    int ok;

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

    /* 開始セルを含め各論理セルは高々 1 回しか push されないため、push 総数は論理セル総数を超えない */
    stack = malloc((size_t)((height - 1) / 2) * (size_t)((width - 1) / 2) * sizeof(*stack));
    if (stack == NULL) {
        fprintf(stderr, "穴掘り用スタックの確保に失敗しました\n");
        free(map);
        return 1;
    }

    if (timespec_get(&t0, TIME_UTC) == 0) {
        fprintf(stderr, "timespec_get に失敗しました\n");
        free(stack);
        free(map);
        return 1;
    }
    x = gen_rand_odd(width);
    y = gen_rand_odd(height);
    maze_init();
    AT(y, x) = ROAD;
    make_maze(y, x, stack);
    open_entrance_exit();
    if (timespec_get(&t1, TIME_UTC) == 0) {
        fprintf(stderr, "timespec_get に失敗しました\n");
        free(stack);
        free(map);
        return 1;
    }
    gen_ms = elapsed_ms(&t0, &t1);
    /* stack はここが最後の参照。求解用メモリを確保する前に返す */
    free(stack);

    queue = malloc((size_t)height * (size_t)width * sizeof(*queue));
    if (queue == NULL) {
        fprintf(stderr, "経路探索用キューの確保に失敗しました\n");
        free(map);
        return 1;
    }
    parent_dir = malloc((size_t)height * (size_t)width * sizeof(*parent_dir));
    if (parent_dir == NULL) {
        fprintf(stderr, "経路復元用メモリの確保に失敗しました\n");
        free(queue);
        free(map);
        return 1;
    }

    if (timespec_get(&t0, TIME_UTC) == 0) {
        fprintf(stderr, "timespec_get に失敗しました\n");
        free(parent_dir);
        free(queue);
        free(map);
        return 1;
    }
    reached = solve(queue, parent_dir);
    if (reached == 0) {
        fprintf(stderr, "入口から出口へ到達できませんでした\n");
        free(parent_dir);
        free(queue);
        free(map);
        return 1;
    }
    path_len = 1;
    cur = exit_index();
    while (parent_dir[cur] != 4) {
        cur -= dir[parent_dir[cur]].y * width + dir[parent_dir[cur]].x;
        path_len++;
    }
    if (timespec_get(&t1, TIME_UTC) == 0) {
        fprintf(stderr, "timespec_get に失敗しました\n");
        free(parent_dir);
        free(queue);
        free(map);
        return 1;
    }
    solve_ms = elapsed_ms(&t0, &t1);

    fprintf(stderr, "生成: %.3fms / 求解: %.3fms / 経路: %ld マス\n", gen_ms, solve_ms, path_len);

    print();

    ok = (fflush(stdout) == 0 && !ferror(stdout));
    if (!ok) {
        fprintf(stderr, "迷路の書き出しに失敗しました\n");
    }

    free(parent_dir);
    free(queue);
    free(map);
    return ok ? 0 : 1;
}
