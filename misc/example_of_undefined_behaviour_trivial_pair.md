# Joys of undefined behaviour

While making adjustments to make dfglib to work on Clang, one test failed and this is short analysis of the failure providing a concrete example of weird effects of undefined behaviour.

## Observation

Using code of commit [3f86237](https://github.com/tc3t/dfglib/commit/3f86237e4e0b9bb21a9eed4d294660b859b37ace) where TrivialPair tests passed in VC2010-2017 (both debug and release), MinGW 4.8, GCC 5.4 and Clang debug build, in Clang optimized build one part of the test failed:

```
auto t0 = T0();
auto t1 = T1();
TrivialPair<T0, T1> tp2(t0, t1);
TrivialPair<T0, T1> tp22(tp2.first, tp2.second);
EXPECT_EQ(tp2, tp22);
```
with
```
T0 = TrivialPair<int, int> and T1 = bool
```

Below is a simplified example that can be used to reproduce the problem:

```
template <class T0, class T1>
class PairClass
{
public:
    typedef T0 first_type;
    typedef T1 second_type;

    PairClass()
    {}

    PairClass(const T0& t0, const T1& t1) :
        first(t0),
        second(t1)
    {}

    bool operator==(const PairClass& other) const
    {
        return this->first == other.first && this->second == other.second;
    }

    T0 first;
    T1 second;
}; // Class PairClass

TEST(Test, UbTest)
{
    typedef PairClass<int, int> T0;
    typedef bool T1;

    const auto t0 = T0();
    const auto t1 = T1();
    const PairClass<T0, T1> tp2(t0, t1);
    const PairClass<T0, T1> tp22(tp2.first, tp2.second);
    EXPECT_EQ(tp2, tp22);
}
```

The EXPECT_EQ on last line fails with error message like:
```
Value of: tp22
  Actual: 12-byte object <00-00 00-00 7C-00 00-00 00-62 CD-B7>
Expected: tp2
  Which is: 12-byte object <48-84 2F-09 1A-51 32-08 00-89 2F-09>
```

How on earth this can be: tp22 is clearly constructed from tp2.first and tp2.second? Lets add some prints:
```
template <class T0, class T1>
std::ostream& operator<<(std::ostream& strm, const PairClass<T0, T1>& pa)
{
    strm << "(" << pa.first << ", " << pa.second << ")";
    return strm;
}

TEST(Test, UbTest)
{
    typedef PairClass<int, int> T0;
    typedef bool T1;

    const auto t0 = T0();
    const auto t1 = T1();
    const PairClass<T0, T1> tp2(t0, t1);
    const PairClass<T0, T1> tp22(tp2.first, tp2.second);
    std::cout << "tp2 = " << tp2 << '\n';
    std::cout << "tp22 = " << tp22 << '\n';
    EXPECT_EQ(tp2, tp22);
}
```
With this code output is something like the following:
```
tp2 = ((134821128, 134827937), 0)
tp22 = ((134821128, 0), 0)
dfglib/dfgTest/dfgTestCont.cpp:1425: Failure
Value of: tp22
  Actual: ((0, 124), 0)
Expected: tp2
  Which is: ((136192768, 134772890), 0)
```
So tp2 and tp22 are different and moreover both values seems to be different in EXPECT_EQ() than in the lines that precede it even though they are consts?

What if adding another set of prints:
```
TEST(Test, UbTest)
{
    typedef PairClass<int, int> T0;
    typedef bool T1;

    const auto t0 = T0();
    const auto t1 = T1();
    std::cout << "t0 = " << t0 << '\n'; // Added 
    std::cout << "t1 = " << t1 << '\n'; // Added
    const PairClass<T0, T1> tp2(t0, t1);
    const PairClass<T0, T1> tp22(tp2.first, tp2.second);
    std::cout << "tp2 = " << tp2 << '\n';
    std::cout << "tp22 = " << tp22 << '\n';
    EXPECT_EQ(tp2, tp22);
}
```

With this code test passes and prints out something like the following:
```
t0 = (134821432, 134828241)
t1 = 0
tp2 = ((0, 0), 0)
tp22 = ((0, 0), 0)
```

Still rather weird: it is as if t0 is not copied to tp2.

 Since there is no reason to believe that the Clang compiler would be completely broken, there's practically one explanation left:
something triggers undefined behaviour.

## Fix

In this case it's easy to see what the undefined behaviour might be: int values seem uninitialized so undefined behaviour seems to be triggered through the use of uninitialized scalar (http://en.cppreference.com/w/cpp/language/ub).

[Simple fix](https://github.com/tc3t/dfglib/commit/f40855a1a31ab7d1c0c41233aa931eee5a90a08a) to the problem is to value initialize members of PairClass, i.e. 
```
PairClass() :
        first(T0()),
        second(T1())
    {}
```

after which int values are zero and code no longer triggers undefined behaviour and the test passes.

## Takeaway

This example demonstrated a concrete effect that undefined behaviour can have:
* Undefined behaviour made reasoning about runtime behaviour from source code impossible, i.e. basic assumptions about the relation of, say, order of source code lines and their runtime execution order may no longer hold (more examples in http://en.cppreference.com/w/cpp/language/ub).
* Occurred only on one of tested 8 compilers and even there only in optimized build.

While it would be oversimplification to say that every UB will break the code at runtime in way beyond comprehension, as this case demonstrates, a seemingly tiny and subtle error can make the runtime behaviour seemingly insane. While it in some sense could be considered as a mitigating factor that this only occurred in one compiler and on certain optimization flags, that is the very reason why this is potentially so nasty a bug: may appear from nowhere in new compilers, in different compile flags or in newer versions of the compiler where the code works today.

Thus to avoid this kind of joyful bugs from appearing, understanding and avoiding UB's might be worthwhile for the quality of the software and for the well-being of software developers.
