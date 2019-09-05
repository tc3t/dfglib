# TableCsv optimizations


This document describes steps taken to improve the results of TableCsv benchmark. Summary of the effects:

| Compiler      | Time (2019-06-23)  | Time (2019-07-20) | Time improvement | Speed Improvement | Source of biggest  improvement(s) |
| -------------  | -----    | -----    | ------          | ----- | ----- |
| MSVC2012 | 2.52 s | 0.85 s | 1.67 s | 200 % | std::deque -> std::vector (0.7 s) |
| MSVC2019.1 | 1.78  s | 0.77 s | 1.01 s | 130 % | std::deque -> std::vector (0.3 s) and std::vector -> custom vector (0.3 s) |
| MinGW 4.8.0 | 1.19 s | 0.83 s | 0.36 s | 40 % | std::vector -> custom vector (0.3 s) |

# Introduction

When dfgQtTableEditor seemed effectively ready for version 1.0.0, the benchmark that measured the performance of the underlying data structure used in the editor, looked peculiar. It measures the time taken to read and store a 112 MB csv file consisting of 4 million lines and 7 columns with content "abc" in every cell. These were the figures 2019-06-23 commit [41a6cc4](https://github.com/tc3t/dfglib/tree/41a6cc4740114ec5917ec77c3421553527a12760):

| Compiler      | Time  |
| -------------  | -----    |
| MSVC2012 | 2.52 s |
| MSVC2019.1 | 1.78  s |
| MinGW 4.8.0 | 1.19 s |

The differences are massive: over x2 between MinGW 4.8.0 and MSVC2012 and even the newest MSVC is about 50 % slower than MinGW.

What causes this? To a rather good approximation, the answer seems to be simple: differences in standard library implementations.

# Optimizations

The optimizations were done in steps and after each change, the benchmark was re-run to see the effect. Summary of the figures gathered from [TableCsv_optimization.csv](https://github.com/tc3t/dfglib/blob/master/misc/TableCsv_optimization.csv):

| # | Change      | Time (MSVC2012) | Time (MSVC2019.1) | Time (MinGW4.8.0) | Details |
| -------------  | -----    | -----    | ------          | ----- | ----- |
| 1 | Baseline | 2.52 s | 1.78 s | 1.19 s | Commit [41a6cc4](https://github.com/tc3t/dfglib/tree/41a6cc4740114ec5917ec77c3421553527a12760) |
| 2 | deque -> vector | 1.80 s (-0.72) | 1.47 s (-0.31) | 1.18 s (~0) | deque -> vector in CharBufferContainer, commit [3ae78ba](https://github.com/tc3t/dfglib/commit/3ae78bae7784097574ceeb608cc497077bf91b33) |
| 3 | Growth factor for row container | 1.65 s (-0.15) | 1.30 s (-0.17) | 1.18 s (~0) | In MinGW vector growth factor was 2, in MSVC 1.5. Implemented growth factor 2 for MSVC manually, commit [d458743](https://github.com/tc3t/dfglib/commit/d458743fa5059e4378c739919269f25024bf06e6) |
| 4 | Custom char storage | 1.51 s (-0.14) | 1.03 s (-0.27) | 0.91 s (-0.27) | Changed char storage from std::vector to custom implementation, commit [e77cc1a](https://github.com/tc3t/dfglib/commit/e77cc1ad65a038915332fdff86d8b514de92ab6a) |
| 5 | std::map -> std::vector | 1.30 s (-0.21) | 0.95 s (-0.08) | 0.86 s (-0.05) | Changed char storage container from std::map to std::vector, commit [563903e](https://github.com/tc3t/dfglib/commit/563903e34ede0f96936f7c8e3aeb4d41536684da) |
| 6 | memcpy -> loop | 1.02 s (-0.28) | 0.90 s (-0.05) | 0.85 s (~0) | In MSVC, converted memcpy(), that was copying 3 characters, to loop, commits [6563a4e](https://github.com/tc3t/dfglib/commit/6563a4e47c9ce0048c27a7ef89e3c1d959a95abd) and [69f2ef3](https://github.com/tc3t/dfglib/commit/69f2ef39f815ccd24f5226455ca43033d954d11f) |
| 7 | Micro-optimizations  | 0.98 s (-0.04) | 0.87 s (-0.03) | 0.87 s (+0.02) | Micro-optimizations to TableSz, commit [7078578](https://github.com/tc3t/dfglib/commit/70785785383de84d17a7f46afcc80910af569c7b) |
| 8 | FileMemoryMapped | 0.93 s (-0.05) | 0.82 s (-0.05) | 0.82 s (-0.05) | Use of FileMemoryMapped in TableCsv, commit [69a52d8](https://github.com/tc3t/dfglib/commit/69a52d8e781461976ca5c498c0684c6d1d04a8d9) |
| 9 | MSVC inlining | 0.85 s (-0.08) | 0.77 s (-0.05) | 0.83 s (~0) | Added forceinlines on MSVC, commit [56e6d05](https://github.com/tc3t/dfglib/commit/56e6d056aae54292ab844714b2ffffd2a39c8543) |

After optimizations the figures look pretty similar and also the best time was reduced significantly. Some remarks for each step:

## 2 (deque -> vector)

Discovery: profiler/guess

In the beginning char storages were in std::deque\<std::vector\<char\>\> and in this change it was converted to std::vector\<std::vector\<char\>>. This brought significant improvement particularly in MSVC2012, but there was no difference in MinGW. So what's the difference in the implementations?

### But first, what is deque?
Quoting [cppreference.com](https://en.cppreference.com/w/cpp/container/deque)
>std::deque (double-ended queue) is an indexed sequence container that allows fast insertion and deletion at both its beginning and its end. In addition, insertion and deletion at either end of a deque never invalidates pointers or references to the rest of the elements.

>As opposed to std::vector, the elements of a deque are not stored contiguously: typical implementations use a sequence of individually allocated fixed-size arrays, with additional bookkeeping, which means indexed access to deque must perform two pointer dereferences, compared to vector's indexed access which performs only one.

The description mentions term "fixed-size arrays". This rises a question that what is the size? The answer is that it is implementation defined and can't be controlled by the user. And what may be particularly surprising is that e.g. in MSVC in the case of deque holding vectors, the array size is 1 and even if storing doubles, the size would be only 2; this makes deque in locality aspect more or less as bad as std::list. For more discussions on implementation of deque, see e.g. [1](https://bugs.chromium.org/p/chromium/issues/detail?id=674287), [2](https://stackoverflow.com/questions/13989835/dealing-with-deque-block-size-causing-performance-issues), [3](https://stackoverflow.com/questions/8305492/about-dequets-extra-indirection), [4](https://stackoverflow.com/questions/57031917/why-doesnt-stddeque-allow-specifying-the-bucket-size), [5](https://stackoverflow.com/questions/6292332/what-really-is-a-deque-in-stl)). In MinGW (that uses libstdc++), the array size was probably 512 bytes translating to roughly 40 vector objects per array.

### Does the array size difference explain the performance figures?

The matter was not investigated thoroughly enough to determine the exact reason. There were also some indications that the deque's operator[] might be causing the performance problem, but if it was related e.g. to the double indirection, one would expect to see the same effect in MinGW.

Regardless of the reason the conclusion is clear: deque was wrong data structure to begin with and in general, deque seems quite questionable container for many uses because the actual implementation varies wildly and thus there's no good guarantees what the user actual gets for example from locality and memory overhead aspects.

## 3 (Growth factor for row container)

Discovery: profiler/manual code analysis

[Growth factor](https://en.wikipedia.org/wiki/Dynamic_array#Growth_factor) is implementation defined and in this particular case factor 2 seemed to give better results, so using it for now. The whole data structure for row storage would need revamping; vector is not optimal for the use case.

## 4 (Custom char storage)

Discovery: guess/profiler

This was almost the only but still rather significant real optimization that wasn't workarounding standard library implementation differences. This change effectively replaced char content storage from std::vector to a custom dynamic array. With std::vector content was appended with ```insert(end(), content_begin, content_end)``` and with custom implementation it was effectively a ```memcpy()``` and size increment. Why would vector insert be slower than custom memcpy() to end?

Let's compare the insert codes; the call sites were:

```C++
// With new implementation using custom storage
currentBuffer.append_unchecked(toCharPtr_raw(sv.begin()), toCharPtr_raw(sv.end()));
currentBuffer.append_unchecked('\0');

// Old with std::vector
currentBuffer.insert(currentBuffer.end(), toCharPtr_raw(sv.begin()), toCharPtr_raw(sv.end()));
currentBuffer.push_back('\0');
```

The append_unchecked() functions are quite simple (simplified a little for readability):
```C++
void append_unchecked(const char* pBegin, const char* pEnd)
{
    const size_t nCount = pEnd - pBegin;
    memcpy(&m_spStorage[m_nSize], pBegin, nCount); // m_spStorage is std::unique_ptr<char[]>
    m_nSize += nCount;
}

void append_unchecked(const char c)
{
    m_spStorage[m_nSize++] = c;
}
```

How do these compare to insert() and push_back()?

In VC2019.1, the internal function that does insertion is over 100 lines long and while this obviously includes debug-only code, code in non-active branches and stuff that optimizer will optimize out, it's seem clear that it does a lot more than corresponding append_unchecked(); also it appeared that at least in debug build there were two calls to memmove(). push_back() is not as long, but still relatively long compared to the one liner append_unchecked(). In MinGW 4.8.0, insert() implementation seemed to be also in the order of 100 lines.

While it's not exactly clear where the performance benefit comes from, given the effects of micro-optimizations in step 7, it would sound plausible that the aforementioned difference could be enough to explain the figures.

But why vector was only reserved instead of resized after which one could have done direct operations that append_unchecked() does? If it was resized, a separate counter for real size would have been needed and also I'm not aware of any method to resize a vector without initializing the elements (discussed e.g. in [this Stack Overflow question](https://stackoverflow.com/questions/15219984/using-vectorchar-as-a-buffer-without-initializing-it-on-resize/)). Not sure if it would cause a notable overhead, but in this use case element initialization would be a redundant overhead, so leaving it out at least frees from the need to worry about it.

## 5 (std::map -> std::vector)

Discovery: guess/profiler

Not a big effect, but nothing surprising to see better performance when not using std::map.

## 6 (memcpy() -> loop, in MSVC only)

Discovery: profiler

This was somewhat questionable change as memcpy() should obviously offer better performance when size is large. The correct solution might have been to use the [intrinsic version of memcpy()](https://docs.microsoft.com/en-us/cpp/preprocessor/intrinsic), but for unknown reason couldn't get it working.

## 7 (micro-optimizations)

Discovery: guess/profiler

The behaviour for MinGW - possibly a slightly worse performance - was unexpected. While the difference is small enough to question if it's real to begin with, the benchmark was run quite many times reducing the possibility of measurement error.

## 8 (FileMemoryMapped)

Discovery: guess

It was quite expected that FileMemoryMapped might improve performance as fileToVector() is just a quick convenience function that hasn't been optimized for performance.

## 9 (MSVC inlining)

Discovery: guess/profiler

Using forced inlining in MSVC as it seems to improve performance.

# Further optimizations

Big bottle neck in read performance is the row-to-content mapping, which is currently a ```std::vector<std::pair<Index_T, const Char_T*>>```; for example in MSVC2019.1 benchmark time 0.77 s reduces by about 0.1 s if doing reserve to row count before reading.

Ways to optimize could include:
* Better data structure, e.g. a list of arrays similar to deque, but having more reasonable array sizes for this use case.
* Row count hints: read functions could provide row count hints so that row storage could be preallocated.

# Notes and conclusions

* Benchmarks measure time from serial runs; "cold start" performance was out of scope for this investigation.
* The results are done in sequence and it is well possible that some effects would be different if the changes were done in different order.
* Data structures matter: improvements were mostly due to changes in data structures
* Details matter: although std::vector and the custom storage in part 4 are both sort of dynamic arrays, the implementation details have significant effect on the result.
* std::deque is rather questionable data structure: relevant details are implementation defined making the data structure badly portable: yes, the code compiles with different implementations, but is the resource usage or performance characteristics portable? And how often one likes to have tiny one or two elements arrays for thousands of elements or use thousands of bytes even if there's only one element ([1](https://bugs.chromium.org/p/chromium/issues/detail?id=674287))?  
* There are major differences in the standard library implementations, which in this case caused a big difference in performance. Thus while using standard library is often convenient and good choice, the downside is that one may lose control to performance and resource usage characteristics.
