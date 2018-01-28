一般
===

カーネルコード全てに共通する話とか

staticな変数の初期化
-------------------
普通のc++プログラムと違って「初期化はされない」ので、placement new等を用いて主導で初期化する事

ラムダ式
-------
c++11から導入されたラムダ式が利用可能。
但し、libc依存なコードを生成しないようにするため、一般的な関数以上の事を行ってはいけない。
具体的には、変数のキャプチャ等。キャプチャしたくなったら、普通の関数を書く時もそうするように、引数で渡す。

ラムダ式は一般的にstd::functionで取り扱うが、一般的な関数の範疇を超えなければ関数ポインタとして扱う事もできる。
このため、GenericFunctionに対して渡す事も可能。

[スレッド](thread.md)と組み合わせた例
```
  auto thread = ThreadCtrl::GetCtrl(
                    cpu_ctrl->RetainCpuIdForPurpose(CpuPurpose::kLowPriority))
                    .AllocNewThread(Thread::StackState::kIndependent);
  auto t_op = thread->CreateOperator();
  t_op.SetFunc(make_uptr(new Function<int i>(
      [](int i) { /* hello world */ },
      12345)));
  t_op.Schedule();
```

Functionの引数として、ラムダ式を取っている。
これは以下の関数と同期である。
```
static void func(int i) {
}
```

ラムダ式とはいえ、これ以上の機能を期待しない事。関数内部で関数を書くための物、と思っておけば良い。