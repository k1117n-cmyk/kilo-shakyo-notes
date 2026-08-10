# kilo.c 学習用解説

このファイルは、`kilo.c` を読むための学習ガイドです。

`kilo.c` は、ターミナル上で動く小さなテキストエディタです。1つのCファイルの中に、端末制御、画面描画、キー入力、テキスト編集、ファイル保存、検索、シンタックスハイライトがまとまっています。

## 1. まず全体像をつかむ

このプログラムの中心は、次の流れです。

```c
int main(int argc, char *argv[]) {
  enableRawMode();
  initEditor();
  if (argc >= 2) {
    editorOpen(argv[1]);
  }

  editorSetStatusMessage(
    "HELP: Ctrl-S = save | Ctrl-Q = quit | Ctrl-F = find");

  while (1) {
    editorRefreshScreen();
    editorProcessKeypress();
  }

  return 0;
}
```

大きく見ると、処理は次の順番です。

1. 端末を raw mode にする
2. エディタの状態を初期化する
3. ファイル名が指定されていれば読み込む
4. 画面を描画する
5. キー入力を1つ読む
6. 入力に応じて状態を更新する
7. 4から繰り返す

このエディタは、画面の一部だけを書き換えるのではなく、キー入力のたびに画面全体を再描画します。小さなエディタなので、この方式でも十分に成立します。

## 2. ファイルの構成

`kilo.c` は、おおよそ次の順番で書かれています。

```text
includes
defines
data
filetypes
prototypes
terminal
syntax highlighting
row operations
editor operations
file i/o
find
append buffer
output
input
init
main
```

読み始めるときは、上から順番にすべて理解しようとするより、まず `main()`、`initEditor()`、`editorRefreshScreen()`、`editorProcessKeypress()` を追う方が理解しやすいです。

## 3. 重要なデータ構造

### editorConfig

エディタ全体の状態は、グローバル変数 `E` にまとめられています。

```c
struct editorConfig E;
```

定義は `struct editorConfig` です。

```c
struct editorConfig {
  int cx, cy;
  int rx;
  int rowoff;
  int coloff;
  int screenrows;
  int screencols;
  int numrows;
  erow *row;
  int dirty;
  char *filename;
  char statusmsg[80];
  time_t statusmsg_time;
  struct editorSyntax *syntax;
  struct termios orig_termios;
};
```

主なメンバーは次の通りです。

| メンバー | 意味 |
|---|---|
| `cx`, `cy` | カーソル位置。`cx` は行内の文字位置、`cy` は行番号 |
| `rx` | 表示上のカーソル位置。タブ展開を考慮した列位置 |
| `rowoff` | 画面上部に表示しているファイル行 |
| `coloff` | 画面左端に表示している列 |
| `screenrows`, `screencols` | ターミナルの表示サイズ |
| `numrows` | ファイル内の行数 |
| `row` | 行データの配列 |
| `dirty` | 未保存の変更があるか |
| `filename` | 開いているファイル名 |
| `statusmsg` | 画面下部に表示するメッセージ |
| `syntax` | 現在のシンタックスハイライト設定 |
| `orig_termios` | raw mode にする前の端末設定 |

このプログラムでは、ほとんどの関数が `E` を読み書きします。つまり、`E` がエディタの現在状態そのものです。

### erow

1行分のテキストは `erow` で表現されます。

```c
typedef struct erow {
  int idx;
  int size;
  int rsize;
  char *chars;
  char *render;
  unsigned char *hl;
  int hl_open_comment;
} erow;
```

| メンバー | 意味 |
|---|---|
| `idx` | ファイル内での行番号 |
| `size` | `chars` の長さ |
| `rsize` | `render` の長さ |
| `chars` | 実際の行文字列 |
| `render` | 画面表示用の行文字列 |
| `hl` | 各文字のハイライト種別 |
| `hl_open_comment` | この行の終端で複数行コメントが継続しているか |

`chars` と `render` が分かれている点が重要です。

`chars` はファイルに保存される実データです。一方で `render` は画面表示用です。たとえばタブ文字 `\t` は、ファイル上では1文字ですが、画面上では複数の空白として表示されます。そのため、表示用の文字列を別に持っています。

## 4. 端末制御

### raw mode とは

通常のターミナルでは、ユーザーが文字を入力しても、Enterを押すまでプログラムには渡されません。また、Ctrl-C などもターミナル側で特別に処理されます。

エディタでは、矢印キーやCtrlキーをすぐに処理したいので、端末を raw mode に変更します。

関係する関数は次の3つです。

```c
void enableRawMode(void);
void disableRawMode(void);
int editorReadKey(void);
```

### enableRawMode()

`enableRawMode()` は、現在の端末設定を `E.orig_termios` に保存し、いくつかのフラグを無効化します。

```c
raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
```

代表的な意味は次の通りです。

| フラグ | 無効化する意味 |
|---|---|
| `ECHO` | 入力文字を自動表示しない |
| `ICANON` | Enterまで入力をためない |
| `ISIG` | Ctrl-C などをシグナルとして扱わない |
| `IEXTEN` | Ctrl-V などの拡張入力処理を無効化 |

終了時には `atexit(disableRawMode)` によって、元の端末設定へ戻します。

### editorReadKey()

`editorReadKey()` は、キー入力を1つ読み取ります。

通常の文字ならそのまま返します。矢印キーやPageUpなどは、複数バイトのエスケープシーケンスとして届くため、`ARROW_UP` などの独自の値に変換して返します。

```c
enum editorKey {
  BACKSPACE = 127,
  ARROW_LEFT = 1000,
  ARROW_RIGHT,
  ARROW_UP,
  ARROW_DOWN,
  DEL_KEY,
  HOME_KEY,
  END_KEY,
  PAGE_UP,
  PAGE_DOWN
};
```

`1000` から始めているのは、通常のASCII文字と値が重ならないようにするためです。

## 5. 画面サイズの取得

画面サイズは `getWindowSize()` で取得します。

```c
int getWindowSize(int *rows, int *cols);
```

まず `ioctl()` を使ってターミナルサイズを取得します。失敗した場合は、カーソルを右下方向へ大きく動かし、その後 `getCursorPosition()` で現在位置を問い合わせるフォールバック処理を使います。

```c
write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12)
```

これはANSIエスケープシーケンスです。ターミナルに対して「カーソルを右へ999、下へ999動かす」という命令を送っています。

## 6. 行データの操作

行操作は `row operations` セクションにまとまっています。

主な関数は次の通りです。

| 関数 | 役割 |
|---|---|
| `editorInsertRow()` | 行を追加する |
| `editorDelRow()` | 行を削除する |
| `editorRowInsertChar()` | 行内に1文字追加する |
| `editorRowDelChar()` | 行内の1文字を削除する |
| `editorRowAppendString()` | 行末に文字列を追加する |
| `editorUpdateRow()` | 表示用文字列とハイライトを更新する |
| `editorFreeRow()` | 行が持つメモリを解放する |

### editorInsertRow()

`editorInsertRow()` は、`E.row` という行配列を `realloc()` で広げ、必要なら `memmove()` で既存の行を後ろへずらします。

```c
E.row = realloc(E.row, sizeof(erow) * (E.numrows + 1));
memmove(&E.row[at + 1], &E.row[at], sizeof(erow) * (E.numrows - at));
```

配列の途中に要素を挿入するため、後ろの要素を1つずつずらす必要があります。ここで `memmove()` を使っているのは、コピー元とコピー先の領域が重なる可能性があるためです。

### editorUpdateRow()

`editorUpdateRow()` は、`chars` から `render` を作り直します。

```c
if (row->chars[j] == '\t') {
  row->render[idx++] = ' ';
  while (idx % KILO_TAB_STOP != 0)
    row->render[idx++] = ' ';
}
```

タブ文字を、表示上のタブ幅に合わせて空白へ展開しています。

その後、`editorUpdateSyntax(row)` を呼び、ハイライト情報も更新します。

## 7. カーソル位置とタブ

このコードには、似た名前の2種類の列位置があります。

| 値 | 意味 |
|---|---|
| `cx` | `chars` 上の文字位置 |
| `rx` | `render` 上の表示位置 |

タブがない行では、`cx` と `rx` はほぼ同じです。しかし、タブがあるとずれます。

変換には次の関数を使います。

```c
int editorRowCxToRx(erow *row, int cx);
int editorRowRxToCx(erow *row, int rx);
```

検索時には `render` 上で一致箇所を探しますが、カーソル位置は `cx` で管理しているため、`editorRowRxToCx()` で戻す必要があります。

## 8. 編集操作

エディタとしての編集操作は `editor operations` セクションにあります。

| 関数 | 役割 |
|---|---|
| `editorInsertChar()` | 現在位置に1文字入力する |
| `editorInsertNewline()` | 改行を挿入する |
| `editorDelChar()` | カーソル左の文字を削除する |

### editorInsertChar()

```c
void editorInsertChar(int c) {
  if (E.cy == E.numrows) {
    editorInsertRow(E.numrows, "", 0);
  }
  editorRowInsertChar(&E.row[E.cy], E.cx, c);
  E.cx++;
}
```

カーソルがファイル末尾の空行にいる場合は、まず新しい行を作ります。その後、現在行に文字を挿入し、カーソルを右へ進めます。

### editorInsertNewline()

改行には2パターンあります。

1. 行頭でEnterを押した場合
2. 行の途中でEnterを押した場合

行頭なら空行を現在行の前に挿入します。行の途中なら、現在行をカーソル位置で分割し、後半を新しい行に移します。

### editorDelChar()

削除にも2パターンあります。

1. 行の途中なら、カーソル左の1文字を消す
2. 行頭なら、前の行と現在行を結合する

行頭でBackspaceを押したときに前の行と結合する処理は、テキストエディタらしい挙動を実現する重要な部分です。

## 9. ファイル読み書き

### editorOpen()

`editorOpen()` は、ファイルを1行ずつ読み込み、各行を `editorInsertRow()` でエディタの内部データに追加します。

```c
while ((linelen = getline(&line, &linecap, fp)) != -1) {
  while (linelen > 0 && (line[linelen - 1] == '\n' ||
      line[linelen - 1] == '\r'))
    linelen--;
  editorInsertRow(E.numrows, line, linelen);
}
```

ファイルから読んだ行末の `\n` や `\r` は取り除いています。内部では、各行は改行文字を含まない形で保持されます。

### editorRowsToString()

保存時には、内部の行配列を1つの文字列に戻します。

```c
char *editorRowsToString(int *buflen);
```

各行の `chars` をコピーし、その後ろに `\n` を付けます。つまり、読み込み時に取り除いた改行を保存時に戻しています。

### editorSave()

`editorSave()` は、`editorRowsToString()` で作った文字列をファイルに書き込みます。

```c
int fd = open(E.filename, O_RDWR | O_CREAT, 0644);
```

その後、`ftruncate(fd, len)` でファイルサイズを保存内容に合わせ、`write()` で書き込みます。

保存に成功すると、`E.dirty = 0` になります。

## 10. dirty フラグ

`E.dirty` は、未保存の変更があるかどうかを表します。

行や文字を変更する関数では、最後に `E.dirty++` しています。

```c
E.dirty++;
```

終了時には、未保存の変更がある場合に警告します。

```c
if (E.dirty && quit_times > 0) {
  editorSetStatusMessage("WARNING!!! File has unsaved changes. "
    "Press Ctrl-Q %d more times to quit.", quit_times);
  quit_times--;
  return;
}
```

`KILO_QUIT_TIMES` が3なので、未保存状態では `Ctrl-Q` を複数回押さないと終了しません。

## 11. 画面描画

画面描画の中心は `editorRefreshScreen()` です。

```c
void editorRefreshScreen(void) {
  editorScroll();

  struct abuf ab = ABUF_INIT;

  abAppend(&ab, "\x1b[?25l", 6);
  abAppend(&ab, "\x1b[H", 3);

  editorDrawRows(&ab);
  editorDrawStatusBar(&ab);
  editorDrawMessageBar(&ab);

  char buf[32];
  snprintf(buf, sizeof(buf), "\x1b[%d;%dH",
    (E.cy - E.rowoff) + 1, (E.rx - E.coloff) + 1);
  abAppend(&ab, buf, strlen(buf));

  abAppend(&ab, "\x1b[?25h", 6);

  write(STDOUT_FILENO, ab.b, ab.len);
  abFree(&ab);
}
```

処理の流れは次の通りです。

1. スクロール位置を更新する
2. カーソルを非表示にする
3. カーソルを画面左上へ移動する
4. 本文行を描画する
5. ステータスバーを描画する
6. メッセージバーを描画する
7. カーソルを正しい位置へ戻す
8. カーソルを表示する
9. まとめてターミナルへ書き込む

### append buffer

描画内容はすぐに `write()` せず、`abuf` にためてから一度に出力します。

```c
struct abuf {
  char *b;
  int len;
};
```

`abAppend()` で文字列を追加し、最後に `write(STDOUT_FILENO, ab.b, ab.len)` でまとめて出力します。

細かく何度も `write()` するより、画面のちらつきが少なくなり、処理もまとまりやすくなります。

## 12. スクロール

`editorScroll()` は、カーソルが画面外へ出ないように `rowoff` と `coloff` を調整します。

```c
if (E.cy < E.rowoff) {
  E.rowoff = E.cy;
}
if (E.cy >= E.rowoff + E.screenrows) {
  E.rowoff = E.cy - E.screenrows + 1;
}
```

縦方向では `cy` と `rowoff` を比較します。

横方向では、タブ展開後の表示位置である `rx` と `coloff` を比較します。

```c
if (E.rx < E.coloff) {
  E.coloff = E.rx;
}
if (E.rx >= E.coloff + E.screencols) {
  E.coloff = E.rx - E.screencols + 1;
}
```

この処理により、カーソルを移動すると画面も追従します。

## 13. キー入力の処理

キー入力の分岐は `editorProcessKeypress()` にまとまっています。

```c
void editorProcessKeypress(void) {
  static int quit_times = KILO_QUIT_TIMES;

  int c = editorReadKey();

  switch (c) {
    ...
  }
}
```

主なキー操作は次の通りです。

| キー | 処理 |
|---|---|
| `Ctrl-Q` | 終了 |
| `Ctrl-S` | 保存 |
| `Ctrl-F` | 検索 |
| Enter | 改行 |
| Backspace | 文字削除 |
| Delete | 右へ移動してから削除 |
| 矢印キー | カーソル移動 |
| PageUp/PageDown | ページ移動 |
| Home/End | 行頭・行末へ移動 |
| 通常文字 | 文字入力 |

ここで重要なのは、キー入力が直接画面を書き換えるのではなく、まず `E` や `erow` の状態を変更することです。画面は次のループで `editorRefreshScreen()` が再描画します。

## 14. 入力プロンプト

`editorPrompt()` は、保存ファイル名の入力や検索文字列の入力に使われる汎用プロンプトです。

```c
char *editorPrompt(char *prompt, void (*callback)(char *, int));
```

第2引数の `callback` が特徴です。検索機能では、ユーザーが1文字入力するたびに `editorFindCallback()` が呼ばれ、インクリメンタル検索を実現しています。

`callback` が不要な場合は `NULL` を渡します。

```c
E.filename = editorPrompt("Save as: %s (ESC to cancel)", NULL);
```

## 15. 検索機能

検索機能は次の2つが中心です。

```c
void editorFind(void);
void editorFindCallback(char *query, int key);
```

`editorFind()` は、検索開始前のカーソル位置とスクロール位置を保存してから、`editorPrompt()` を呼びます。

```c
int saved_cx = E.cx;
int saved_cy = E.cy;
int saved_coloff = E.coloff;
int saved_rowoff = E.rowoff;
```

ユーザーがESCで検索をキャンセルした場合は、保存しておいた位置へ戻します。

`editorFindCallback()` は、検索文字列が変わるたびに呼ばれます。`strstr()` で一致箇所を探し、見つかった行へカーソルを移動します。

```c
char *match = strstr(row->render, query);
```

一致箇所は `HL_MATCH` に変更され、検索結果として色付き表示されます。

## 16. シンタックスハイライト

このコードは、C/C++系ファイルの簡易的なシンタックスハイライトに対応しています。

### ハイライト設定

対象拡張子は次の配列で定義されています。

```c
char *C_HL_extensions[] = { ".c", ".h", ".cpp", NULL };
```

キーワードは次の配列です。

```c
char *C_HL_keywords[] = {
  "switch", "if", "while", "for", "break", "continue", "return", "else",
  "struct", "union", "typedef", "static", "enum", "class", "case",
  "int|", "long|", "double|", "float|", "char|", "unsigned|", "signed|",
  "void|", NULL
};
```

末尾に `|` があるキーワードは `HL_KEYWORD2` として扱われます。このコードでは、型名などを別の色で表示するために使われています。

### editorSelectSyntaxHighlight()

`editorSelectSyntaxHighlight()` は、ファイル名の拡張子を見て、どのハイライト設定を使うか決めます。

```c
char *ext = strrchr(E.filename, '.');
```

`.c` や `.h` などに一致すると、`E.syntax` にC用の設定を入れ、全行のハイライトを更新します。

### editorUpdateSyntax()

`editorUpdateSyntax()` は、1行分の `render` を見ながら、各文字にハイライト種別を付けます。

判定している主なものは次の通りです。

| 対象 | ハイライト種別 |
|---|---|
| 通常文字 | `HL_NORMAL` |
| `//` コメント | `HL_COMMENT` |
| `/* ... */` コメント | `HL_MLCOMMENT` |
| キーワード | `HL_KEYWORD1` |
| 型名など | `HL_KEYWORD2` |
| 文字列 | `HL_STRING` |
| 数字 | `HL_NUMBER` |
| 検索一致 | `HL_MATCH` |

複数行コメントでは、前の行からコメントが続いているかどうかを `hl_open_comment` で管理します。

```c
int in_comment = (row->idx > 0 && E.row[row->idx - 1].hl_open_comment);
```

この状態が変わった場合は、次の行のハイライトも再計算します。

```c
if (changed && row->idx + 1 < E.numrows)
  editorUpdateSyntax(&E.row[row->idx + 1]);
```

複数行コメントは後続行の色にも影響するためです。

## 17. 色の指定

`editorSyntaxToColor()` は、ハイライト種別をANSIカラーコードに変換します。

```c
int editorSyntaxToColor(int hl) {
  switch (hl) {
  case HL_COMMENT:
  case HL_MLCOMMENT:
    return 36;
  case HL_KEYWORD1:
    return 33;
  case HL_KEYWORD2:
    return 32;
  case HL_STRING:
    return 35;
  case HL_NUMBER:
    return 31;
  case HL_MATCH:
    return 34;
  default:
    return 37;
  }
}
```

これらはターミナルのANSIカラーコードです。

| コード | 色 |
|---|---|
| 31 | 赤 |
| 32 | 緑 |
| 33 | 黄色 |
| 34 | 青 |
| 35 | マゼンタ |
| 36 | シアン |
| 37 | 白 |

## 18. メモリ管理

このコードでは、行データや描画バッファを動的に確保しています。

よく出てくる関数は次の通りです。

| 関数 | 役割 |
|---|---|
| `malloc()` | 新しくメモリを確保する |
| `realloc()` | 既存のメモリ領域を拡張・縮小する |
| `free()` | 確保したメモリを解放する |
| `memcpy()` | メモリをコピーする |
| `memmove()` | 重なりがあり得るメモリを安全に移動する |
| `memset()` | メモリを特定の値で埋める |

特に重要なのは、`erow` が複数の動的メモリを持っていることです。

```c
char *chars;
char *render;
unsigned char *hl;
```

そのため、行を削除するときは `editorFreeRow()` でまとめて解放します。

```c
void editorFreeRow(erow *row) {
  free(row->render);
  free(row->chars);
  free(row->hl);
}
```

## 19. 学習時に注意したい点

### グローバル状態に依存している

このコードは、`struct editorConfig E` をグローバル変数として使っています。小さなプログラムでは読みやすいですが、大きなプログラムでは依存関係が見えにくくなることがあります。

学習時には、「この関数は `E` のどの値を読むか」「どの値を書き換えるか」を意識すると理解しやすいです。

### 描画と状態更新が分かれている

キー入力処理では、基本的に内部状態を変更します。画面表示は次の `editorRefreshScreen()` でまとめて行われます。

この構造は、イベントループ型プログラムの基本です。

### chars と render の違い

`chars` は実データ、`render` は表示用データです。

タブのように、保存上の文字数と表示上の幅が違うものを扱うために、この2つを分けています。`cx` と `rx` の違いもここにつながります。

## 20. おすすめの読む順番

初めて読む場合は、次の順番がおすすめです。

1. `main()`
2. `initEditor()`
3. `editorRefreshScreen()`
4. `editorDrawRows()`
5. `editorProcessKeypress()`
6. `editorMoveCursor()`
7. `editorInsertChar()`
8. `editorInsertNewline()`
9. `editorDelChar()`
10. `editorInsertRow()` と `editorUpdateRow()`
11. `editorOpen()` と `editorSave()`
12. `editorFind()`
13. `editorUpdateSyntax()`
14. `enableRawMode()` と `editorReadKey()`

端末制御はC初心者には少し難しいため、最初から完全に理解しようとしなくても構いません。先に「エディタの状態がどう変わり、どう描画されるか」を追う方が全体像をつかみやすいです。

## 21. 関数一覧

| 関数 | 役割 |
|---|---|
| `die()` | エラー表示後に終了する |
| `disableRawMode()` | 端末設定を元に戻す |
| `enableRawMode()` | 端末を raw mode にする |
| `editorReadKey()` | キー入力を読む |
| `getCursorPosition()` | 現在のカーソル位置を取得する |
| `getWindowSize()` | ターミナルサイズを取得する |
| `is_separator()` | キーワード判定用の区切り文字か調べる |
| `editorUpdateSyntax()` | 1行分のハイライトを更新する |
| `editorSyntaxToColor()` | ハイライト種別を色に変換する |
| `editorSelectSyntaxHighlight()` | ファイル名からハイライト設定を選ぶ |
| `editorRowCxToRx()` | 実文字位置から表示位置へ変換する |
| `editorRowRxToCx()` | 表示位置から実文字位置へ変換する |
| `editorUpdateRow()` | 表示用文字列を更新する |
| `editorInsertRow()` | 行を挿入する |
| `editorFreeRow()` | 行のメモリを解放する |
| `editorDelRow()` | 行を削除する |
| `editorRowInsertChar()` | 行内に文字を挿入する |
| `editorRowAppendString()` | 行末へ文字列を追加する |
| `editorRowDelChar()` | 行内の文字を削除する |
| `editorInsertChar()` | エディタ上で文字を挿入する |
| `editorInsertNewline()` | エディタ上で改行する |
| `editorDelChar()` | エディタ上で文字を削除する |
| `editorRowsToString()` | 行配列を保存用文字列へ変換する |
| `editorOpen()` | ファイルを読み込む |
| `editorSave()` | ファイルを保存する |
| `editorFindCallback()` | 検索中の入力に応じて検索結果を更新する |
| `editorFind()` | 検索を開始する |
| `abAppend()` | 描画バッファへ文字列を追加する |
| `abFree()` | 描画バッファを解放する |
| `editorScroll()` | スクロール位置を更新する |
| `editorDrawRows()` | 本文行を描画する |
| `editorDrawStatusBar()` | ステータスバーを描画する |
| `editorDrawMessageBar()` | メッセージバーを描画する |
| `editorRefreshScreen()` | 画面全体を再描画する |
| `editorSetStatusMessage()` | ステータスメッセージを設定する |
| `editorPrompt()` | 入力プロンプトを表示する |
| `editorMoveCursor()` | カーソルを移動する |
| `editorProcessKeypress()` | キー入力に応じた処理を行う |
| `initEditor()` | エディタ状態を初期化する |
| `main()` | プログラムの入口 |

## 22. このコードから学べること

この `kilo.c` からは、次のような実践的なCプログラミングを学べます。

1. ターミナルの raw mode 制御
2. ANSIエスケープシーケンスによる画面描画
3. 動的配列の管理
4. 文字列バッファの伸長
5. 1行単位のテキストエディタ設計
6. ファイル読み書き
7. イベントループ
8. カーソル移動とスクロール
9. インクリメンタル検索
10. 簡易シンタックスハイライト

全体として、このコードは「小さいが実用的なCプログラム」を読む題材としてよくまとまっています。関数ごとの役割を追いながら、最終的に `main()` のループの中で全機能がどうつながるかを理解するのが目標です。
