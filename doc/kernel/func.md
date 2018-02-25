GenericFunction
===============

概要
---
汎用関数ポインタ
所謂C言語の関数ポインタだとC++のメンバ関数等を扱いづらいため、その代替品と考えれば良い。

GenericFunction
---------------
汎用関数ポインタの基底クラス
単体で関数ポインタとして用いる事はない。
「関数ポインタを引数として受け取る関数」において用いるのが主な用途になるだろう。

```
// 引数が関数ポインタ
void exec(uptr<GenericFunction<>> f) {
  f->Execute();
}

uptr<Function<void *>> f1 = make_uptr(new Function<void *>([](void *) {}, nullptr));
exec(f1);

uptr<Function<int>> f2 = make_uptr(new Function<int>([](int) {}, 0));
exec(f2);

uptr<ClassFunction<Hoge, void *>> f3 = make_uptr(new ClassFunction<Hoge, void *>(this, &Hoge::Func, nullptr));
exec(f3);
```

データの受け渡し
---------------
GenericFunctionから派生する関数ポインタ（相当）クラスには、「関数ポインタクラス宣言時のデータ」を「関数実行時に渡す」事ができる。
分かりにくいので、例を示そう。

```
extern void exec(uptr<GenericFunction<>> f);

void func(int i) {
  gtty->Printf("%d", i);
}

int i;
// func()の関数ポインタクラスを作成
// func()に対して、「クラスを作成したこの瞬間のデータ」であるiの値を渡したい。
uptr<Function<int>> f = make_uptr(new Function<int>(func, i));
exec(f);


//========================================================
extern void exec(uptr<GenericFunction<>> f) {
  f->Execute();
}

```

このコードの意図は、func()の中で変数xの値を表示させたい、という物である。
しかしながら、変数xの値を知っているのはfunc()の関数ポインタクラスを作成した、その瞬間であり、関数ポインタを実行するexec()関数はxの値を知らないため、iの値をfunc()に渡す事ができない。

１つの解決策としては、exec()にiの値を渡すインターフェースを追加し、f->Execute(i)とする事である。しかし、exec()に渡される関数ポインタとして、もしかするとint*を表示する関数へのポインタがあるかもしれない。この時、exec()にint*の引数を追加するのはあまりにも馬鹿馬鹿しい。

そこで、関数ポインタの中に渡したいデータをくるむ必要がある。
そしてそれが本節で解説した機能である。

注意点として、関数ポインタの引数と混同しない事。また、両者が混在するケースの取扱については次節で解説する。

関数ポインタの引数
-----------------
関数ポインタ実行時に引数を与えたい事もある。
この場合、以下のようにすれば良い。

```
extern void exec(uptr<GenericFunction<char>> f);

void func(int i, char c) {
  gtty->Printf("%d %c", i, c);
}

int i = 1;
uptr<Function<int, char>> f = make_uptr(new Function<int, char>(func, i));
exec(f);


//========================================================
extern void exec(uptr<GenericFunction<char>> f) {
  char c = 'A';
  f->Execute(c);
}

```

引数の変更は以下の通りである、
* GenericFunction<>はGenericFunction<char>へ
* Function<int>はFunction<int, char>へ
* func(int)はfunc(int, char)へ

func()には関数ポインタ作成時に与える引数と、GenericFuntion::Execute()時の両方の引数が渡っている事に注意して欲しい。

Function/Function2/Function3
----------------------------
「関数ポインタ作成時に与える引数」の個数によって用いる関数が異なる。
いずれも、GenericFuntion::Execute()時に与える引数の個数は可変である。
４個以上の引数を与えたい場合は、構造体を用いる事。

ClassFunction
-------------------------------------------
メソッドに対する関数ポインタを作成する事ができる。
メソッドはオブジェクトとセットで呼び出される物なので、関数ポインタ生成時にオブジェクトも渡さなければいけない事に注意。
ClassFunction2/ClassFunction3は引数の個数が異なるバージョン。
