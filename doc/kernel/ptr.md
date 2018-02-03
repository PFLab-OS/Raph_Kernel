uptr/sptr/wptr
==============

概要
---
スマートポインタ
uptrとsptr、wptrはそれぞれ一般的なunique_ptrとshared_ptr、weak_ptrである。

スマートポインタの基礎
---------------------
そもそもスマートポインタとは、C/C++の危険な生ポインタ操作を隠蔽して、メモリ安全なプログラムを書くための一般的な概念である。
生ポインタの場合、delete済みのポインタへアクセスする事ができる。（結果、一見不可解なバグや、セグフォを起こす）スマートポインタは動的確保されたオブジェクトを管理し、不正なメモリアクセスを行わない（或いは、直前に止める）ようにしたり、deleteを自動化してメモリリークを防ぐための仕組みである。

uptr
----
uptrはオブジェクトを単一のuptrからしか参照できないようにするポインタ。
uptrが破壊される時、オブジェクトもまた破壊される。

uptrを初期化していない場合（nullptrの時）は、エラーとなる。
```
uptr<int> p;
int i = *p; // エラー
```

uptrを初期化した場合は参照可能
```
class Test {
public:
  void Func() {}
};

uptr<Test> p(new Test);
p->Func(); // OK
```

uptrをmoveした際、古いuptrはnullptrとなる。
```
uptr<int> p1(new int(3));
uptr<int> p2;
p2 = p1; // p1はnullptrとなる
*p1; // エラー
*p2; // OK
p1 = p2; // p2がnullptrとなる
*p1; // OK
*p2; // エラー
```

uptrが破壊されると、自動的にオブジェクトが解放される。
```
class Test {
};

do {
  uptr<Test> p(new Test);
  // hogehoge
} while(0);
// pが破壊され、Testのdestructorが呼び出される
```

破壊される前にmoveされている場合は、この限りではない。
```
class Test {
};

do {
  uptr<Test> p1;
  do {
    uptr<Test> p2(new Test);
    p1 = p2;
  } while(0);
  // p2が破壊されるが、何も起こらない。
} while(0);
// p1が破壊され、Testのdestructorが呼び出される
```

関数の引数として渡す事もできる。関数の引数として渡した後は、そのuptrを使い続ける事はできない。
```
class Test {
};

void func(uptr<Test> p) {
  // return後にTestのdestructorが呼び出される
}

uptr<Test> p;
func(p);
*p; // エラー！
```

関数からのreturn後も使い続けたい場合は以下のようにすれば良い。
```
class Test {
};

uptr<Test> func(uptr<Test> p) {
  return p;
}

uptr<Test> p;
p = func(p);
*p; // OK!
```

sptr
----
uptrと異なり、動的確保されたオブジェクトを複数のsptrインスタンスによって管理できる。
sptrは内部に参照カウンタを持っており、オブジェクトの解放は、参照カウンタが０になった時に実行される。（つまり、最後のsptrインスタンスが破壊された時に解放される）

sptrも初期化していない場合（nullptrの時）は、エラーとなる。
```
sptr<int> p;
int i = *p; // エラー
```

```
class Test {
};

uptr<Test> p1(new Test);
uptr<Test> p2;
p2 = p1;
*p2; // OK
*p1; // OK
```

```
class Test {
};

do {
  uptr<Test> p1(new Test);
  do {
    uptr<Test> p2;
    p2 = p1;
  } while(0);
  // p2が破壊されるが、p1が参照しているため、destructorは呼ばれない。
  *p1; // OK
} while(0);
// p1が破壊され、destructorが呼ばれる。
```

GetCnt()で参照カウントを取得できる。
```
uptr<int> p1(new int(0));
uptr<int> p2;
p1.GetCnt(); // 1
p2 = p1;
p1.GetCnt(); // 2
do {
  uptr<int> p3;
  p3 = p1;
  p1.GetCnt(); // 3
} while(0);
p1.GetCnt(); // 2
```

wptr
----
sptrは使い方を誤るとメモリリークの原因になる（循環参照が発生している場合等は参照カウンタが０にならず、解放されない）

簡単な例を示す。
```
struct Test {
  sptr<Test> t;
};

do {
  sptr<Test> p(new Test);
  p.GetCnt(); // 1
  p.t = p;
  p.GetCnt(); // 2
} while(0);
// pは破壊されるが、参照カウンタは1のままなので解放されるメモリリークする。
```

こういった場合は、wptrを用いる事で解決できる。
wptrはsptrで管理しているオブジェクトを参照する事ができる。（逆に、sptrで管理されていないオブジェクトをwptrが管理する事はできない。wptrはsptrとセットで使う事になる）
オブジェクトの解放は、そのオブジェクトに対するsptrが0になると実行されるというのは前述の通りだが、ここで大事なのはwptrによるオブジェクトへの参照があっても問答無用で解放される、という事である。
そのため、wptr経由でオブジェクトを読もうとするとクラッシュする可能性がある。（クラッシュしないようにするのはプログラマの責任）しかし、適切に使えば循環参照によるメモリリークを回避できる。

先程の例は以下のようにすれば良い。

```
struct Test {
  wptr<Test> t;
};

wptr<Test> wp;
do {
  sptr<Test> p(new Test);
  p.GetCnt(); // 1
  p.t = make_wptr(p);
  p.GetCnt(); // 1
  wp = p.t;
} while(0);
// pは破壊され、オブジェクトも解放される。
*wp; // エラー
```
* IsObjReleased()
オブジェクトが解放されたかどうかを返す。

使い分け
-------
sptrの方が使い勝手が良いように思えるが、sptrでないと構造上実装不可能である場合以外は、基本的にuptrを使う事。
sptrは内部でnewを呼び出すため、コストが高い。
また前述の通り、sptrはメモリリークを完全に防ぐ事はできない。sptrを使う場合は、循環参照を回避するために適切にwptrを使う事。
適切なwptrの使用は難しいので、それと比較すると最初からuptrで実装した方が楽かもしれない。

uptr/sptr/wptr共通操作
---------------------
* IsNull()
nullptrかどうかチェックできる。
```
uptr<int> p;
p.IsNull(); // true
```

* GetRawPtr()
生ポインタを取得できる。
```
int buf[10];
uptr<int[]> p(new int[10]);
memcpy(p.GetRawPtr(), buf, 10);
```

