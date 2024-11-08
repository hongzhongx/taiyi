# 测试系统

单元测试的项目，可以通过`make chain_test`来构建。然后就得到一个可执行程序`./tests/chain_test`，用这个程序可以运行所有的单元测试用例。

测试用例被分配到如下几个分类测试中：
```
basic_tests          // Tests of "basic" functionality
block_tests          // Tests of the block chain
operation_tests      // Unit Tests of Taiyi operations
operation_time_tests // Tests of Taiyi operations that include a time based component (ex. qi withdrawals)
serialization_tests  // Tests related of serialization
contract_tests       // Tests related of Smart Game Script
nfa_tests            // Tests related of NFA
```

# 代码覆盖测试

如果你从来没有玩过这个，你需要先执行`brew install lcov`安装lcov。

```
cmake -D BUILD_TAIYI_TESTNET=ON -D ENABLE_COVERAGE_TESTING=true -D CMAKE_BUILD_TYPE=Debug .
make
lcov --capture --initial --directory . --output-file base.info --no-external
tests/chain_test
lcov --capture --directory . --output-file test.info --no-external
lcov --add-tracefile base.info --add-tracefile test.info --output-file total.info
lcov -o interesting.info -r total.info tests/\*
mkdir -p lcov
genhtml interesting.info --output-directory lcov --prefix `pwd`
```

Now open `lcov/index.html` in a browser
