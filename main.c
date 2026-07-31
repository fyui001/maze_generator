#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define ROAD 0
#define WALL 1
#define PATH 2
/* parent_dir 用のセンチネル。到達方向（0〜3）と衝突しない値を選ぶ */
#define START 4
#define UNVISITED 0xFF
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
* parent_dir には親からの到達方向（dir[] の添字）が残る。START = 入口、UNVISITED = 未訪問
*/
int solve(int *queue, unsigned char *parent_dir)
{
    size_t head = 0, tail = 0;

    memset(parent_dir, UNVISITED, (size_t)height * (size_t)width * sizeof(*parent_dir));

    queue[tail++] = entrance_index();
    parent_dir[entrance_index()] = START;

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
            if (parent_dir[nidx] != UNVISITED) {
                continue;
            }
            parent_dir[nidx] = (unsigned char)d;
            queue[tail++] = nidx;
        }
    }

    return 0;
}

/*
* 出口から parent_dir を遡って経路長を返す。mark_path が真なら経路を PATH で塗る
* solve が 1 を返した後にのみ呼べる
*/
long path_length(const unsigned char *parent_dir, int mark_path)
{
    long len = 1;
    int cur = exit_index();

    if (mark_path) {
        map[cur] = PATH;
    }
    /* UNVISITED で止めることで dir[] の範囲外読みを構造的に防ぐ */
    while (parent_dir[cur] < START) {
        cur -= dir[parent_dir[cur]].y * width + dir[parent_dir[cur]].x;
        if (mark_path) {
            map[cur] = PATH;
        }
        len++;
    }
    return len;
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
* 多バイト値をリトルエンディアンで 1 バイトずつ書く
* 構造体パディングとホストエンディアンへの依存を避けるため struct の fwrite は使わない
*/
void write_le(FILE *fp, unsigned long v, int n)
{
    int i;
    for (i = 0; i < n; i++) {
        fputc((int)((v >> (8 * i)) & 0xFFUL), fp);
    }
}

/*
* BMP の 1 行のバイト数。行は 4 バイト境界に切り上げる
* 上限を引き上げたときに静かに溢れないよう long で計算する
*/
long bmp_row_bytes(int bitcount)
{
    return ((long)bitcount * width + 31L) / 32L * 4L;
}

/* 添字が map の値と一致するので変換がいらない。要素は RGBQUAD の {B, G, R} */
static const unsigned char bmp_palette[3][3] = {
    {0xFF, 0xFF, 0xFF}, /* ROAD = 白 */
    {0x00, 0x00, 0x00}, /* WALL = 黒 */
    {0x00, 0x00, 0xFF} /* PATH = 赤 */
};

/*
* BITMAPFILEHEADER + BITMAPINFOHEADER + パレットを書く
* biHeight は正値。したがってピクセルデータはボトムアップになる
*/
void write_bmp_header(FILE *fp, int bitcount, int colors)
{
    long image_size = bmp_row_bytes(bitcount) * height;
    long off_bits = 14L + 40L + 4L * colors;
    int i;

    fputc('B', fp);
    fputc('M', fp);
    write_le(fp, (unsigned long)(off_bits + image_size), 4);
    write_le(fp, 0, 2);
    write_le(fp, 0, 2);
    write_le(fp, (unsigned long)off_bits, 4);
    write_le(fp, 40, 4);
    write_le(fp, (unsigned long)width, 4);
    write_le(fp, (unsigned long)height, 4);
    write_le(fp, 1, 2);
    write_le(fp, (unsigned long)bitcount, 2);
    write_le(fp, 0, 4);
    write_le(fp, (unsigned long)image_size, 4);
    write_le(fp, 0, 4);
    write_le(fp, 0, 4);
    write_le(fp, (unsigned long)colors, 4);
    write_le(fp, 0, 4);

    for (i = 0; i < colors; i++) {
        write_le(fp, bmp_palette[i][0], 1);
        write_le(fp, bmp_palette[i][1], 1);
        write_le(fp, bmp_palette[i][2], 1);
        write_le(fp, 0, 1);
    }
}

/*
* 迷路を 1bit 白黒 2 色の BMP に書き出す。行バッファ 1 本だけを使い回す
* 道以外を黒とするので、経路を塗った後に呼ぶ順序ミスが壁色の線として見える
*/
int write_bmp_1bit(const char *path)
{
    long row_bytes = bmp_row_bytes(1);
    unsigned char *row;
    FILE *fp;
    int file_row, x, failed;

    row = malloc((size_t)row_bytes);
    if (row == NULL) {
        fprintf(stderr, "画像行バッファの確保に失敗しました\n");
        return 1;
    }
    fp = fopen(path, "wb");
    if (fp == NULL) {
        fprintf(stderr, "画像ファイルのオープンに失敗しました: %s: %s\n", path, strerror(errno));
        free(row);
        return 1;
    }

    write_bmp_header(fp, 1, 2);
    for (file_row = 0; file_row < height; file_row++) {
        int y = height - 1 - file_row;
        /* 余りビットと行パディングを 0 のまま残す */
        memset(row, 0, (size_t)row_bytes);
        for (x = 0; x < width; x++) {
            if (AT(y, x) != ROAD) {
                row[x / 8] |= (unsigned char)(0x80u >> (x % 8));
            }
        }
        if (fwrite(row, 1, (size_t)row_bytes, fp) != (size_t)row_bytes) {
            break;
        }
    }
    free(row);

    /* 小さい画像は stdio のバッファに収まるため fclose の flush でしか失敗が分からない */
    failed = (file_row < height) || ferror(fp);
    if (fclose(fp) != 0) {
        failed = 1;
    }
    if (failed) {
        fprintf(stderr, "画像の書き出しに失敗しました: %s\n", path);
        remove(path);
        return 1;
    }
    return 0;
}

/*
* 迷路を 4bit パレット 3 色の BMP に書き出す。上位ニブルが左のピクセル
*/
int write_bmp_4bit(const char *path)
{
    long row_bytes = bmp_row_bytes(4);
    unsigned char *row;
    FILE *fp;
    int file_row, x, failed;

    row = malloc((size_t)row_bytes);
    if (row == NULL) {
        fprintf(stderr, "画像行バッファの確保に失敗しました\n");
        return 1;
    }
    fp = fopen(path, "wb");
    if (fp == NULL) {
        fprintf(stderr, "画像ファイルのオープンに失敗しました: %s: %s\n", path, strerror(errno));
        free(row);
        return 1;
    }

    write_bmp_header(fp, 4, 3);
    for (file_row = 0; file_row < height; file_row++) {
        int y = height - 1 - file_row;
        memset(row, 0, (size_t)row_bytes);
        for (x = 0; x < width; x++) {
            unsigned char v = AT(y, x);
            row[x / 2] |= (unsigned char)((x % 2 == 0) ? (v << 4) : v);
        }
        if (fwrite(row, 1, (size_t)row_bytes, fp) != (size_t)row_bytes) {
            break;
        }
    }
    free(row);

    failed = (file_row < height) || ferror(fp);
    if (fclose(fp) != 0) {
        failed = 1;
    }
    if (failed) {
        fprintf(stderr, "画像の書き出しに失敗しました: %s\n", path);
        remove(path);
        return 1;
    }
    return 0;
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
    Frame *stack = NULL;
    int *queue = NULL;
    unsigned char *parent_dir = NULL;
    int x, y;
    int reached;
    long path_len;
    double gen_ms, solve_ms;
    /* 既定は失敗。成功経路を最後まで通ったときだけ 0 に落とす */
    int status = 1;

    if (argc >= 4) {
        fprintf(stderr, "使用法: %s [DIM | HEIGHT WIDTH]\n", argv[0]);
        goto cleanup;
    }
    if (argc == 2) {
        if (parse_dim(argv[1], &height) != 0) {
            goto cleanup;
        }
        width = height;
    } else if (argc == 3) {
        if (parse_dim(argv[1], &height) != 0 || parse_dim(argv[2], &width) != 0) {
            goto cleanup;
        }
    }

    /* 同一秒内の連続実行でも異なる迷路になるようナノ秒も混ぜる */
    if (timespec_get(&ts, TIME_UTC) == 0) {
        fprintf(stderr, "timespec_get に失敗しました\n");
        goto cleanup;
    }
    srand((unsigned int)ts.tv_sec ^ (unsigned int)ts.tv_nsec);

    map = malloc((size_t)height * (size_t)width * sizeof(*map));
    if (map == NULL) {
        fprintf(stderr, "迷路用メモリの確保に失敗しました\n");
        goto cleanup;
    }

    /* 開始セルを含め各論理セルは高々 1 回しか push されないため、push 総数は論理セル総数を超えない */
    stack = malloc((size_t)((height - 1) / 2) * (size_t)((width - 1) / 2) * sizeof(*stack));
    if (stack == NULL) {
        fprintf(stderr, "穴掘り用スタックの確保に失敗しました\n");
        goto cleanup;
    }

    if (timespec_get(&t0, TIME_UTC) == 0) {
        fprintf(stderr, "timespec_get に失敗しました\n");
        goto cleanup;
    }
    x = gen_rand_odd(width);
    y = gen_rand_odd(height);
    maze_init();
    AT(y, x) = ROAD;
    make_maze(y, x, stack);
    open_entrance_exit();
    if (timespec_get(&t1, TIME_UTC) == 0) {
        fprintf(stderr, "timespec_get に失敗しました\n");
        goto cleanup;
    }
    gen_ms = elapsed_ms(&t0, &t1);
    /* stack はここが最後の参照。求解用メモリを確保する前に返す */
    free(stack);
    stack = NULL;

    queue = malloc((size_t)height * (size_t)width * sizeof(*queue));
    if (queue == NULL) {
        fprintf(stderr, "経路探索用キューの確保に失敗しました\n");
        goto cleanup;
    }
    parent_dir = malloc((size_t)height * (size_t)width * sizeof(*parent_dir));
    if (parent_dir == NULL) {
        fprintf(stderr, "経路復元用メモリの確保に失敗しました\n");
        goto cleanup;
    }

    if (timespec_get(&t0, TIME_UTC) == 0) {
        fprintf(stderr, "timespec_get に失敗しました\n");
        goto cleanup;
    }
    reached = solve(queue, parent_dir);
    if (reached == 0) {
        fprintf(stderr, "入口から出口へ到達できませんでした\n");
        goto cleanup;
    }
    path_len = path_length(parent_dir, 0);
    if (timespec_get(&t1, TIME_UTC) == 0) {
        fprintf(stderr, "timespec_get に失敗しました\n");
        goto cleanup;
    }
    solve_ms = elapsed_ms(&t0, &t1);

    fprintf(stderr, "生成: %.3fms / 求解: %.3fms / 経路: %ld マス\n", gen_ms, solve_ms, path_len);

    print();

    if (fflush(stdout) != 0 || ferror(stdout)) {
        fprintf(stderr, "迷路の書き出しに失敗しました\n");
        goto cleanup;
    }

    status = 0;

cleanup:
    free(parent_dir);
    free(queue);
    free(stack);
    free(map);
    return status;
}
