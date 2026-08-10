# Kilo チュートリアル写経メモ

![Language: C](https://img.shields.io/badge/Language-C-blue)
![Topic: Terminal](https://img.shields.io/badge/Topic-Terminal-lightgrey)
![Purpose: Learning](https://img.shields.io/badge/Purpose-Learning-green)

このリポジトリは、snaptoken 氏の [Build Your Own Text Editor](https://viewsourcecode.org/snaptoken/kilo/index.html) を読みながら写経した `kilo.c` と、その学習メモをまとめたものです。

目的は学習です。元チュートリアルの公式翻訳や再配布ではありません。ここに置いているメモは、同じチュートリアルを進める日本語読者が、つまずきやすい点を確認したり、自分の理解と見比べたりするための補足資料です。

## チュートリアルについて

[Build Your Own Text Editor](https://viewsourcecode.org/snaptoken/kilo/index.html) は、[antirez 氏の kilo](https://github.com/antirez/kilo) をもとに、小さなターミナル用テキストエディタを C で段階的に作っていくチュートリアルです。

このチュートリアルでは、特に次のような内容を学べます。

- `termios` を使った Raw mode
- ターミナルからのキー入力の直接読み取り
- ANSI エスケープシーケンス
- 画面の再描画処理
- 動的な行バッファ
- ファイルの読み込みと保存
- 検索
- シンタックスハイライト

## ファイル構成

- `kilo.c`: チュートリアルを進めながら写経したテキストエディタ本体
- `makefile`: `kilo.c` をビルドするためのルール
- `kilo_learning_notes.md`: 日本語の学習メモ、補足説明、つまずきやすい点の整理
- `kilo_explained.md`: 完成版 `kilo.c` を読むための学習ガイド

主に読む対象は `kilo.c`、`kilo_learning_notes.md`、`kilo_explained.md` です。ビルド済みの実行ファイルや個人的な実験用ファイルは Git の追跡対象から外しています。

## ビルド方法

このプロジェクトは C コンパイラと `make` を使います。

```sh
make
```

実行ファイル `kilo` が生成されます。

ファイルを開く場合は次のように実行します。

```sh
./kilo kilo.c
```

生成された実行ファイルを削除する場合は、次のコマンドを使います。

```sh
make clean
```

現在のビルドコマンドは次の通りです。

```sh
$(CC) kilo.c -o kilo -Wall -Wextra -pedantic -std=c99
```

C の学習中に警告を確認できるよう、警告オプションを意図的に有効にしています。

## 学習メモ

主な補足ドキュメントは [kilo_learning_notes.md](./kilo_learning_notes.md) です。

このメモでは、次のような内容を扱っています。

- 元記事の step ごとの差分形式の読み方
- 削除された行を見落としやすい理由
- C における `void func()` と `void func(void)` の違い
- 余分な `\x1b[H` を残したことで起きたカーソル位置のバグ
- エスケープシーケンス処理で見落としやすい `O` と `0` の違い
- 完成版 `kilo.c` の大まかな構造

## つまずきやすかった点

チュートリアルを進める中で、他の学習者にも参考になりそうな問題にいくつか遭遇しました。

たとえば、カーソルが左上に固定されたまま動かない問題がありました。直接の原因は、`editorRefreshScreen()` に古いカーソル位置指定のエスケープシーケンスが残っていたことです。コードはコンパイルできましたが、実行時の挙動が正しくありませんでした。

また、エスケープシーケンスの解析で、英大文字の `O` と数字の `0` を見間違えたこともありました。このようなタイプミスはコンパイルエラーにならないことがありますが、特定のキーが正しく動かない原因になります。

これらの内容は [kilo_learning_notes.md](./kilo_learning_notes.md) にもう少し詳しくまとめています。

## 公開記事

このリポジトリで学んだ内容を、UNIX Cafe のブログ記事にもまとめました。

- [Build Your Own Text Editor を写経して kilo を学んだ](https://pc-fan.net/kilo-shakyo-learning-notes/)

## 参考元

このリポジトリは、次の資料をもとに学習した内容です。

- チュートリアル: snaptoken 氏の [Build Your Own Text Editor](https://viewsourcecode.org/snaptoken/kilo/index.html)
- 元になったエディタ: [antirez's kilo](https://github.com/antirez/kilo)
- チュートリアル付録: [Appendices](https://viewsourcecode.org/snaptoken/kilo/08.appendices.html)

各資料のライセンスについては、元のチュートリアルやソースリポジトリを参照してください。このリポジトリのメモは、個人的な学習メモと補足説明を目的としています。
