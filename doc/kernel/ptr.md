uptr/sptr
=========

概要
---
スマートポインタ
uptrとsptrはそれぞれ一般的なunique_ptrとshared_ptrである。

スマートポインタの基礎
---------------------
そもそもスマートポインタとは、C/C++の危険な生ポインタ操作を隠蔽して、メモリ安全なプログラムを書くための一般的な概念である。
生ポインタの場合、delete済みのポインタへアクセスする事ができる。（結果、一見不可解なバグや、セグフォを起こす）スマートポインタはメモリの動的確保されたメモリの管理し、不正なメモリアクセスを行わない（或いは、直前に止める）ようにしたり、deleteを自動化してメモリリークを防ぐための仕組みである。

uptr
----
uptrはメモリを単一のuptrからしか参照できないようにするポインタ。
uptrが破壊される時、メモリ領域もまた破壊される。

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

uptrが破壊されると、自動的にメモリが解放される。
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
uptrと異なり、動的確保されたメモリ領域を複数のsptrインスタンスによって管理できる。
sptrは内部に参照カウンタを持っており、メモリ領域の解放は、参照カウンタが０になった時に実行される。（つまり、最後のsptrインスタンスが破壊された時に解放される）

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

使い分け
-------
sptrの方が使い勝手が良いように思えるが、sptrでないと構造上実装不可能である場合以外は、基本的にuptrを使う事。
sptrは内部でnewを呼び出すため、コストが高い。
また、sptrはメモリリークを完全に防ぐ事はできない。（循環参照が発生している場合等は参照カウンタが０にならない）

uptr/sptr共通操作
-------
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

TODO
----
wptrの話
