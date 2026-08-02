
//puyo_sim
//低速だが表示が完成済
//塗り替え数が広い

/*
試験塗り替え（人力で１８８点）
13121232
25241231
21545231
45512452
44132442
35545511

10001001
01100111
01011101
11011011
11110011
01111011

00000000
10001100
11001010
01000111
01101011
00110110

11111111

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <emscripten.h>
////#include <omp.h>




#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/*
u = -0.42x + 0.05y + 12.72
v = 0.05x - 0.42y + 12.72を参考に
*/



#define CONV_NUM1 12
#define CONV_NUM2 12
#define TIME 300
#define SUKOA 170



// --- プロトタイプ宣言 ---

int bouhatsukakuninn(signed char board[48]);
void show_status(signed char color[48], signed char plus[48], signed char bonus[48], signed char n_color[8], signed char n_plus[8]);
int score_keisan(signed char b_color[48], signed char b_plus[48], signed char b_bonus[48], signed char n_color[8], signed char n_plus[8], int target_idx);
void print_simple_board(signed char color[48]);
int check_composite_bouhatsu_1(signed char original[48], signed char swapped[48]);
int check_composite_bouhatsu_2(signed char original[48], signed char swapped[48]);
void print_new_color1_map(signed char original[48], signed char current[48]);
void print_new_color2_map(signed char original[48], signed char current[48]);
int input_row(signed char *array, int row_idx, const char *label);
void shuffle(signed char *array, int n);
unsigned int xorshift32(unsigned int *state);
unsigned int xor_rand(unsigned int *state, unsigned int max);
void shuffle_r(signed char *array, int n, unsigned int *seed);
void run_puyo_analysis(char* combined_data);

// --- ランキング保存用の構造体 ---
typedef struct {
    int score;
    int tap;
    signed char color[48]; 
} Ranking;




// --- メイン関数 ---
int main(void) {
return 0;
}









// --- 入力用関数 ---
int input_row(signed char *array, int row_idx, const char *label) {
    char buf[256];
    printf("%s 行%d: ", label, row_idx + 1);
    if (scanf("%s", buf) != 1) return 0;
    if (buf[0] == 'x') return 1;

    for (int i = 0; i < 8 && buf[i] != '\0'; i++) {
        if (buf[i] >= '0' && buf[i] <= '9') {
            array[row_idx * 8 + i] = buf[i] - '0';
        }
    }
    return 0;
}

// --- 3分割表示関数 ---
void show_status(signed char color[48], signed char plus[48], signed char bonus[48], signed char n_color[8], signed char n_plus[8]) {
    printf("\n==========================================\n");
    printf("        【 盤面データ確認 】\n");
    printf("==========================================\n");

    // NEXTの表示
    printf("NEXT + : ");
    for(int i=0; i<8; i++) printf("%s ", n_plus[i] ? "+" : ".");
    printf("\nNEXT色 : ");
    for(int i=0; i<8; i++) printf("%d ", n_color[i]);
    printf("\n------------------------------------------\n");

    // 1. 色盤面
    printf("\n[1] 色ぷよ (Color 0-5)\n");
    for (int i = 0; i < 48; i++) {
        printf("%d ", color[i]);
        if ((i + 1) % 8 == 0) printf("\n");
    }

    // 2. プラス盤面
    printf("\n[2] プラスぷよ (+) \n");
    for (int i = 0; i < 48; i++) {
        printf("%s ", plus[i] ? "+" : ".");
        if ((i + 1) % 8 == 0) printf("\n");
    }

    // 3. ボーナス盤面
    printf("\n[3] ボーナスエリア (B)\n");
    for (int i = 0; i < 48; i++) {
        printf("%s ", bonus[i] ? "B" : ".");
        if ((i + 1) % 8 == 0) printf("\n");
    }
    printf("==========================================\n");
}

// --- 暴発確認関数 (BFS) ---
int bouhatsukakuninn(signed char board[48]) {
    signed char visited[48] = {0}, queue[48];
    int head, tail;
    for (int i = 0; i < 48; i++) {
        if (board[i] >= 1 && board[i] <= 5 && !visited[i]) {
            int color = board[i];
            int count = 0;
            head = tail = 0;
            queue[tail++] = i;
            visited[i] = 1;
            count++;
            while (head < tail) {
                int curr = queue[head++];
                int r = curr / 8, c = curr % 8;
                int dr[] = {-1, 1, 0, 0}, dc[] = {0, 0, -1, 1};
                for (int d = 0; d < 4; d++) {
                    int nr = r + dr[d], nc = c + dc[d];
                    int next = nr * 8 + nc;
                    if (nr >= 0 && nr < 6 && nc >= 0 && nc < 8 &&
                        board[next] == color && !visited[next]) {
                        visited[next] = 1;
                        queue[tail++] = next;
                        count++;
                        if (count >= 4) return -1;
                    }
                }
            }
        }
    }
    return 0;
}





#include <stdio.h>
#include <string.h>

// --- ヘルパー：重力処理 ---
// -1（空）を詰め、0〜5のぷよを下に移動させる
void apply_gravity(signed char color[48], signed char plus[48]) {
    for (int c = 0; c < 8; c++) {
        int write_r = 5; 
        for (int r = 5; r >= 0; r--) {
            int curr = r * 8 + c;
            if (color[curr] != -1) { // 空(-1)以外なら下に落とす
                int target = write_r * 8 + c;
                if (curr != target) {
                    color[target] = color[curr];
                    plus[target] = plus[curr];
                    color[curr] = -1;
                    plus[curr] = 0;
                }
                write_r--;
            }
        }
    }
}

// --- ヘルパー：連結と巻き込み判定 ---
// 戻り値: 連鎖が発生したか (1:発生, 0:なし)
int find_connections(signed char color[48], signed char to_delete[48]) {
    signed char visited[48] = {0};
    int found_chain = 0;
    memset(to_delete, 0, sizeof(signed char) * 48);

    // 1. 色ぷよ(1-5)の4連結を探す
    for (int i = 0; i < 48; i++) {
        if (color[i] >= 1 && color[i] <= 5 && !visited[i]) {
            signed char group[48], g_size = 0;
            signed char queue[48];
			int head = 0, tail = 0;
            int target_color = color[i];

            queue[tail++] = i;
            visited[i] = 1;
            while (head < tail) {
                int curr = queue[head++];
                group[g_size++] = curr;
                int r = curr / 8, c = curr % 8;
                int dr[] = {-1, 1, 0, 0}, dc[] = {0, 0, -1, 1};
                for (int d = 0; d < 4; d++) {
                    int nr = r + dr[d], nc = c + dc[d];
                    int next = nr * 8 + nc;
                    if (nr >= 0 && nr < 6 && nc >= 0 && nc < 8 &&
                        color[next] == target_color && !visited[next]) {
                        visited[next] = 1;
                        queue[tail++] = next;
                    }
                }
            }
            if (g_size >= 4) {
                found_chain = 1;
                for (int j = 0; j < g_size; j++) to_delete[group[j]] = 1;
            }
        }
    }

    // 2. 消える色ぷよの隣にあるハートぷよ(0)を巻き込む
    if (found_chain) {
        for (int i = 0; i < 48; i++) {
            if (to_delete[i] && color[i] != 0) { // 消える色ぷよの周囲を確認
                int r = i / 8, c = i % 8;
                int dr[] = {-1, 1, 0, 0}, dc[] = {0, 0, -1, 1};
                for (int d = 0; d < 4; d++) {
                    int nr = r + dr[d], nc = c + dc[d];
                    int next = nr * 8 + nc;
                    if (nr >= 0 && nr < 6 && nc >= 0 && nc < 8 && color[next] == 0) {
                        to_delete[next] = 1; // ハートも削除対象にする
                    }
                }
            }
        }
    }
    return found_chain;
}

// --- メイン：スコア計算本体 ---
int score_keisan(signed char b_color[48], signed char b_plus[48], signed char b_bonus[48], 
                 signed char n_color[8], signed char n_plus[8], int target_idx) {
    
    // 作業用コピー
    signed char color[48], plus[48], bonus[48];
    memcpy(color, b_color, sizeof(signed char)*48);
    memcpy(plus, b_plus, sizeof(signed char)*48);
    memcpy(bonus, b_bonus, sizeof(signed char)*48);

    int total_score = 0;

    // 1. なぞり消し
    color[target_idx] = -1;
    plus[target_idx] = 0;
    apply_gravity(color, plus);

    // 2. 連鎖ループ
    while (1) {
        signed char to_delete[48] = {0};
        if (!find_connections(color, to_delete)) break;

        // スコア加算と消去
        for (int i = 0; i < 48; i++) {
            if (to_delete[i]) {
                int p = plus[i];
                int b = bonus[i];
                // 加点ルール：1, 2, 3, 6
                if (!p && !b) total_score += 1;
                else if (!p && b) total_score += 3;
                else if (p && !b) total_score += 2;
                else if (p && b) total_score += 6;

                color[i] = -1; // 空にする
                plus[i] = 0;
            }
        }
        apply_gravity(color, plus);
    }

    // 3. ネクスト落下（各列1個ずつ）
    for (int c = 0; c < 8; c++) {
        for (int r = 0; r < 6; r++) {
            int idx = r * 8 + c;
            if (color[idx] == -1) { // 一番上の空きマスに補充
                color[idx] = n_color[c];
                plus[idx] = n_plus[c];
                break;
            }
        }
    }
    apply_gravity(color, plus);

    // 4. ネクスト後の最終チェック（1回だけ連鎖判定）
    signed char final_delete[48] = {0};
    if (find_connections(color, final_delete)) {
        for (int i = 0; i < 48; i++) {
            if (final_delete[i]) {
                int p = plus[i];
                int b = bonus[i];
                if (!p && !b) total_score += 1;
                else if (!p && b) total_score += 3;
                else if (p && !b) total_score += 2;
                else if (p && b) total_score += 6;
            }
        }
    }

    return total_score;
}



// --- 盤面表示用ヘルパー関数 ---
void print_simple_board(signed char color[48]) {
    for (int i = 0; i < 48; i++) {
        printf("%d ", color[i]);
        if ((i + 1) % 8 == 0) printf("\n");
    }
}

// --- ヘルパー：配列をシャッフルする（ランダムな位置選び用） ---
void shuffle(signed char *array, int n) {
    for (int i = n - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        int tmp = array[i];
        array[i] = array[j];
        array[j] = tmp;
    }
}

int check_composite_bouhatsu_1(signed char original[48], signed char swapped[48]) {
    signed char composite[48];

    // 1. 合成マップの作成
    for (int i = 0; i < 48; i++) {
        if (original[i] == 1 || swapped[i] == 1) {
            composite[i] = 1; // どちらかが1なら1
        } else {
            composite[i] = 0; // それ以外は0
        }
    }

    // 2. 作成した合成マップを既存の暴発確認関数に投げる
    // 既存の関数は「1〜5の色が4つ以上繋がると-1を返す」ので、
    // 1しか存在しない盤面を渡せば、1の繋がりだけをチェックしてくれます。
    return bouhatsukakuninn(composite);
}

int check_composite_bouhatsu_2(signed char original[48], signed char swapped[48]) {
    signed char composite[48];

    // 1. 合成マップの作成
    for (int i = 0; i < 48; i++) {
        if (original[i] == 2 || swapped[i] == 2) {
            composite[i] = 2; // どちらかが2なら2
        } else {
            composite[i] = 0; // それ以外は0
        }
    }

    // 2. 作成した合成マップを既存の暴発確認関数に投げる
    // 既存の関数は「1〜5の色が4つ以上繋がると-1を返す」ので、
    // 1しか存在しない盤面を渡せば、1の繋がりだけをチェックしてくれます。
    return bouhatsukakuninn(composite);
}

// --- 新しく「1」に塗り替えた場所だけを表示する関数 ---
void print_new_color1_map(signed char original[48], signed char current[48]) {
    printf("[ 新規塗り替え：色1 ]\n");
    for (int i = 0; i < 48; i++) {
        // 「今は1」かつ「元は1ではない」場所だけを 1 と表示
        if (current[i] == 1 && original[i] != 1) {
            printf("1 ");
        } else {
            printf("0 ");
        }
        if ((i + 1) % 8 == 0) printf("\n");
    }
}

// --- 新しく「2」に塗り替えた場所だけを表示する関数 ---
void print_new_color2_map(signed char original[48], signed char current[48]) {
    printf("[ 新規塗り替え：色2 ]\n");
    for (int i = 0; i < 48; i++) {
        // 「今は2」かつ「元は2ではない」場所だけを 2 と表示
        if (current[i] == 2 && original[i] != 2) {
            printf("2 ");
        } else {
            printf("0 ");
        }
        if ((i + 1) % 8 == 0) printf("\n");
    }
}


// 32bit版 Xorshift: 爆速でそれなりの精度の乱数を作ります
unsigned int xorshift32(unsigned int *state) {
    unsigned int x = *state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    return *state = x;
}

// 範囲指定の乱数 (0 〜 max-1) を返す補助関数
unsigned int xor_rand(unsigned int *state, unsigned int max) {
    if (max == 0) return 0;
    return xorshift32(state) % max;
}

// 引数に seed を追加。rand() % (i+1) を xor_rand に置き換え
void shuffle_r(signed char *array, int n, unsigned int *seed) {
    for (int i = n - 1; i > 0; i--) {
        int j = xor_rand(seed, i + 1); 
        int tmp = array[i];
        array[i] = array[j];
        array[j] = tmp;
    }
}

// --- WebAssembly 連携用ブリッジ関数 ---

EMSCRIPTEN_KEEPALIVE
void run_puyo_analysis(char* combined_data) {
    // 既存のグローバル変数やメインで使っていた変数へ流し込むための準備
	signed char board_color[48] = {0}, board_plus[48] = {0}, board_bonus[48] = {0};
    signed char next_color[8] = {0}, next_plus[8] = {0};
    int random_mode = 0;
	int time_limit = 300;
    int CN1=0,CN2=0,count=0;
	char* token = strtok(combined_data, ",");
    srand((unsigned int)time(NULL));
	int hanni1 = 0,hanni = 0;

    while (token != NULL && count < 24) {
        // --- [0-5] 盤面の色 (盤面行 1-6) ---
        if (count >= 0 && count <= 5) {
            for (int i = 0; i < 8 && token[i] != '\0'; i++) {
                board_color[(count * 8) + i] = token[i] - '0';
            }
        }
        // --- [6-11] プラス状態 (+行 1-6) ---
        else if (count >= 6 && count <= 11) {
            int row = count - 6;
            for (int i = 0; i < 8 && token[i] != '\0'; i++) {
                // 1 または '+' が入力されたらプラス1とする
                board_plus[(row * 8) + i] = (token[i] == '1' || token[i] == '+') ? 1 : 0;
            }
        }
        // --- [12-17] ボーナスエリア (BE行 1-6) ---
        else if (count >= 12 && count <= 17) {
            int row = count - 12;
            for (int i = 0; i < 8 && token[i] != '\0'; i++) {
                // 1 または 'B' が入力されたらボーナス1とする
                board_bonus[(row * 8) + i] = (token[i] == '1' || token[i] == 'B') ? 1 : 0;
            }
        }
        // --- [18] ネクストの色 (N盤面) ---
        else if (count == 18) {
            for (int i = 0; i < 8 && token[i] != '\0'; i++) {
                next_color[i] = token[i] - '0';
            }
        }
        // --- [19] ネクストのプラス (N+) ---
        else if (count == 19) {
            for (int i = 0; i < 8 && token[i] != '\0'; i++) {
                next_plus[i] = (token[i] == '1' || token[i] == '+') ? 1 : 0;
            }
        }
        // --- [20] 塗り替え1の数 (塗 1) ---
        else if (count == 20) {
            CN1 = atoi(token);
        }
        // --- [21] 塗り替え2の数 (塗 2) ---
        else if (count == 21) {
            CN2 = atoi(token);
        }
        // --- [22] 実行時間（思考時間の制限など） ---
        else if (count == 22) {
            time_limit = atoi(token);
        }
		else if (count == 23) {
			hanni1 = atoi(token);
			hanni = (hanni1 * hanni1 + hanni1) / 2;
		}

        token = strtok(NULL, ",");
        count++;
    }
	
	
	if (bouhatsukakuninn(board_color) == -1) {
        printf("\n[!] 警告: この盤面はなぞる前に4つ以上繋がって消えてしまいます。\n");////////////////
    }
    // --- ここで探索ロジックを実行 ---
    int off1[] = {
    0, 
    1, 0, 
    1, 2, 0, 
    2, 1, 3, 0, 
    2, 3, 1, 4, 0, 
    3, 2, 4, 1, 5, 0, 
    3, 4, 2, 5, 1, 6, 0, 
    4, 3, 5, 2, 6, 1, 7, 0, 
    4, 5, 3, 6, 2, 7, 1, 8, 0, 
    5, 4, 6, 3, 7, 2, 8, 1, 9, 0, 
    5, 6, 4, 7, 3, 8, 2, 9, 1, 10, 0, 
    6, 5, 7, 4, 8, 3, 9, 2, 10, 1, 11, 0, 
    6, 7, 5, 8, 4, 9, 3, 10, 2, 11, 1, 12, 0, 
    7, 6, 8, 5, 9, 4, 10, 3, 11, 2, 12, 1, 13, 0, 
    7, 8, 6, 9, 5, 10, 4, 11, 3, 12, 2, 13, 1, 14, 0, 
    8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15, 0, 
    8, 9, 7, 10, 6, 11, 5, 12, 4, 13, 3, 14, 2, 15, 1, 16, 0
};

int off2[] = {
    0, 
    0, 1, 
    1, 0, 2, 
    1, 2, 0, 3, 
    2, 1, 3, 0, 4, 
    2, 3, 1, 4, 0, 5, 
    3, 2, 4, 1, 5, 0, 6, 
    3, 4, 2, 5, 1, 6, 0, 7, 
    4, 3, 5, 2, 6, 1, 7, 0, 8, 
    4, 5, 3, 6, 2, 7, 1, 8, 0, 9, 
    5, 4, 6, 3, 7, 2, 8, 1, 9, 0, 10, 
    5, 6, 4, 7, 3, 8, 2, 9, 1, 10, 0, 11, 
    6, 5, 7, 4, 8, 3, 9, 2, 10, 1, 11, 0, 12, 
    6, 7, 5, 8, 4, 9, 3, 10, 2, 11, 1, 12, 0, 13, 
    7, 6, 8, 5, 9, 4, 10, 3, 11, 2, 12, 1, 13, 0, 14, 
    7, 8, 6, 9, 5, 10, 4, 11, 3, 12, 2, 13, 1, 14, 0, 15, 
    8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15, 0, 16
};
if (hanni < 1) hanni = 1;
if (hanni > 153) hanni = 153;
Ranking top10_all[10];
for(int i=0; i<10; i++) top10_all[i].score = -1; 


long tried_count = 0;
long valid_count = 0;
long over_100_count = 0;	
	
Ranking top10[10];
for(int i=0; i<10; i++) top10[i].score = -1; 

time_t start_time = time(NULL);

	// 20スレッド使用（2スレッド余裕を持たせる）(論理プロセッサ数ー２を目安に)
////    omp_set_num_threads(20);

////#pragma omp parallel
{
	int tid=1;//atodekesu
////	int tid = omp_get_thread_num();
    // 1. 各自の道具（カウンター、種、作業盤面）を準備
    unsigned int local_counter = 0;
	long local_tried = 0; // 個別カウント用
    unsigned int my_seed = emscripten_get_now() * 1000.0;
	if (my_seed == 0) my_seed = 1;
	int local_CN1 = (tid < 20) ? (CN1 - off1[tid]) : CN1;
    int local_CN2 = (tid < 20) ? (CN2 - off2[tid]) : CN2;
	
	if (local_CN1 < 0) local_CN1 = CN1;
    if (local_CN2 < 0) local_CN2 = CN2;
	tid=0;
while (1) { 
local_counter++;
local_tried++;
        if ((local_counter & 16383) == 0) {
			tid++;
			if(tid==hanni){
				tid=0;
			}
			 local_CN1 = (tid < hanni) ? (CN1 - off1[tid]) : CN1;
			 local_CN2 = (tid < hanni) ? (CN2 - off2[tid]) : CN2;
			 if (local_CN1 < 0) local_CN1 = CN1;
			 if (local_CN2 < 0) local_CN2 = CN2;
            if (difftime(time(NULL), start_time) >= time_limit) break;
        }

    signed char work_color[48];
    signed char work_plus[48];
    memcpy(work_color, board_color, sizeof(signed char) * 48);
    memcpy(work_plus, board_plus, sizeof(signed char) * 48);

    // 1. 塗り替え（12個の「1」）
    signed char candidates1[48];
	int	c1_size = 0;
    for (int i = 0; i < 48; i++) if (work_color[i] != 1) candidates1[c1_size++] = i;
    shuffle_r(candidates1, c1_size, &my_seed);
    int converted_to_1[local_CN1];
    for (int i = 0; i < local_CN1 && i < c1_size; i++) {
        work_color[candidates1[i]] = 1;
        converted_to_1[i] = candidates1[i];
    }

    // 2. 塗り替え（12個の「2」）
    signed char candidates2[48];
	int c2_size = 0;
    for (int i = 0; i < 48; i++) {
    if (board_color[i] != 2 && (work_color[i] != 1 || board_color[i] == 1)) {
        candidates2[c2_size++] = (signed char)i;
    }
}
    shuffle_r(candidates2, c2_size, &my_seed);
    for (int i = 0; i < local_CN2 && i < c2_size; i++) work_color[candidates2[i]] = 2;

    // 3. 暴発チェック
    if (bouhatsukakuninn(work_color) == -1) continue;
////	#pragma omp atomic
    valid_count++;

    // 4. スコア計算
    int best_s = -1;
    int best_t = -1;
    for (int t = 8; t < 48; t++) {
        int s = score_keisan(work_color, work_plus, board_bonus, next_color, next_plus, t);
        if (s > best_s) { best_s = s; best_t = t; }
    }

    // 6. ランキング更新
    if (best_s > top10[9].score && (check_composite_bouhatsu_1(board_color, work_color)== 0 || check_composite_bouhatsu_2(board_color, work_color)== 0)) {
////        #pragma omp critical(ranking_update)
		{
			if (best_s > top10[9].score) {
		int pos = 9;
        while (pos > 0 && best_s > top10[pos-1].score) {
            top10[pos] = top10[pos-1];
            pos--;
        }
        top10[pos].score = best_s;
        top10[pos].tap = best_t;
        memcpy(top10[pos].color, work_color, sizeof(signed char) * 48);
		if (best_s > top10_all[9].score) {
			int pos = 9;
			while (pos > 0 && best_s > top10_all[pos-1].score) {
				top10_all[pos] = top10_all[pos-1];
				pos--;
			}
            top10_all[pos].score = best_s;
            top10_all[pos].tap = best_t;
            memcpy(top10_all[pos].color, work_color, sizeof(signed char) * 48);
        }
    }
		}
	}
}
////#pragma omp atomic
    tried_count += local_tried;
}
////// --- [結果送信処理] ---
    if (top10[0].score != -1) {
        int priority = (check_composite_bouhatsu_1(board_color, top10[0].color) == 0) ? 1 : 2;
        int tap = top10[0].tap;

// Workerにある「reportResultToMain」を呼ぶ
        EM_ASM({
            const score = $0;
            const tap = $1;
            const priority = $2;
            const colorPtr = $3;
            const originalColorPtr = $4;
			const plusPtr = $5;
            const bonusPtr = $6;

            const colorArray = [];
            const originalColorArray = [];
			const plusArray = [];
            const bonusArray = [];
            for (let i = 0; i < 48; i++) {
                // 🚀 Module. をとって HEAP8 直接参照にする
                colorArray.push(HEAP8[colorPtr + i]);
                originalColorArray.push(HEAP8[originalColorPtr + i]);
				plusArray.push(HEAP8[plusPtr + i]);  
                bonusArray.push(HEAP8[bonusPtr + i]);
            }

            // reportResultToMain を呼び出す
            reportResultToMain(score, tap, colorArray, originalColorArray, priority, plusArray, bonusArray);
        }, top10[0].score, tap, priority, top10[0].color, board_color, board_plus, board_bonus);

        // 探索パターン数の送信
        EM_ASM({
            if (typeof reportPatterns === 'function') {
                reportPatterns($0);
            }
        }, (int)tried_count);

    } else {
        EM_ASM({
            if (typeof reportNoResult === 'function') {
                reportNoResult();
            }
        });
    }
}