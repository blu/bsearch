Description
-----------

This project benchmarks several implementations of binary search over a dense array of PODs:

* bsearch_standard - vanilla binary search
* bsearch_binned   - binary search with binning info prepended to the array -- saves first few iterations of the binary search
* bsearch_breadth  - breadth-first (tree-like) layout of the array
* bsearch_veb      - [Van Emde Boas](https://en.wikipedia.org/wiki/Van_Emde_Boas_tree), recursive version
* bsearch_veb_iter - Van Emde Boas, iterative version

How to Build
------------

The build script recognizes these options:

```
$ ./build_test_bsearch.sh 
usage: ./build_test_bsearch.sh { gcc | clang | debug }
```

* gcc   - use the system-default g++ compiler to build a release binary
* clang - use the system-default clang++ compiler to build a release binary
* debug - use the system-default clang++ compiler to build a debug binary

How to benchmark
----------------

The benchmark tool recognizes these options:
```
$ ./test_bsearch --help
usage: ./test_bsearch [space_size <unsigned>] [alt <unsigned>] [<sample_size>]
        alt 0: standard binary search (default)
        alt 1: binned binary search
        alt 2: breadth-first layout binary search
        alt 3: Van Emde Boas (VEB) layout binary search, recursive version
        alt 4: VEB layout binary search, iterative version
        alt 5: standard linear search
        alt 6: binned linear search
```

The POD of the search space is hardcoded to `float` -- to build for another POD change `searchitem_t` in test_bsearch.cpp. The default search-set size (`sample_size`) is set to 10M; the default search-space size (`space_size`) is 2K. To benchmark the performance of standard binary search over a search-space size of 2^24 and a search-set size of 10M do:

```
$ ./test_bsearch space_size $(echo "2^24" | bc) alt 0
```

Warning: don't run any of the linear searches (`alt` 5 & 6) on large seach spaces unless you have unlimited machine time and patience.

Results
-------

Averaged searches/second from 10M random binary searches in a 64MB dataset -- see graph below

| CPU (single thread only)                                                   | bsearch_standard | bsearch_binned   | bsearch_breadth  | bsearch_veb      | bsearch_veb_iter |
| -------------------------------------------------------------------------- | ---------------- | ---------------- | ---------------- | ---------------- | ---------------- |
| Intel E3-1270v2 1.6GHz, DDR3-1600 x 128-bit = 25.6GB/s, g++-4.9.4          |  1,233,214.83816 |  1,314,079.77670 |  2,067,069.50045 |  3,686,770.34838 |  3,754,412.82091 |
| Intel E3-1270v2 1.6GHz, DDR3-1600 x 128-bit = 25.6GB/s, clang++-3.6.2      |  1,182,672.68827 |  1,307,087.44580 |  2,066,906.89736 |  3,682,097.72430 |  3,431,576.57391 |
| Mediatek MT8173C 2.1GHz, LPDDR3-1600 x 64-bit = 12.8GB/s, g++-8.1.0, A64   |    490,095.37049 |    526,391.19130 |    744,651.85599 |  2,035,683.46409 |  2,031,427.43252 |
| Mediatek MT8173C 2.1GHz, LPDDR3-1600 x 64-bit = 12,8GB/s, clang++-5, A64   |    505,498.85542 |    526,628.48955 |    724,330.70865 |  2,051,006.51255 |  2,052,157.44440 |
| Rockchip RK3399 2.0GHz, LPDDR3-1600 x 64-bit = 12.8GB/s, g++-8.1.0, A64    |    462,759.62345 |    478,642.74222 |    738,867.48099 |  2,038,538.98643 |  2,046,879.84171 |
| Rockchip RK3399 2.0GHz, LPDDR3-1600 x 64-bit = 12.8GB/s, clang++-5, A64    |    450,501.15446 |    552,472.60090 |    729,130.91045 |  2,069,238.54899 |  2,064,635.95656 |
| Amlogic S922X 1.8GHz, LPDDR4-2640 x 32-bit = 10.56GB/s, clang++-6, A64     |    304,724.91082 |    396,877.43037 |    759,732.27935 |  1,977,061.21213 |  1,982,474.13447 |
| Marvell A8040 1.3GHz, DDR4-1600 x 64-bit = 12.8GB/s, g++-8.1.0, A64        |    295,051.20016 |    371,600.31788 |    681,138.89748 |  1,787,888.14059 |  1,795,045.00659 |
| Marvell A8040 1.3GHz, DDR4-1600 x 64-bit = 12.8GB/s, clang++-5, A64        |    299,796.12389 |    385,103.14619 |    662,799.74976 |  1,775,231.37958 |  1,776,729.11277 |
| Marvell A8040 1.6GHz, DDR4-2100 x 64-bit = 16.8GB/s, g++-8.1.0, A64        |    357,402.94131 |    457,186.94522 |    835,043.38528 |  2,164,437.69986 |  2,174,108.44857 |
| Marvell A8040 1.6GHz, DDR4-2100 x 64-bit = 16.8GB/s, clang++-5, A64        |    362,563.01363 |    468,497.01981 |    815,416.15608 |  2,151,162.92427 |  2,190,350.23157 |
| Marvell A8040 2.0GHz, DDR4-2400 x 64-bit = 19.2GB/s, g++-8.1.0, A64        |    398,350.45181 |    511,816.67916 |    938,053.73429 |  2,469,142.11860 |  2,487,483.38162 |
| Marvell A8040 2.0GHz, DDR4-2400 x 64-bit = 19.2GB/s, clang++-5, A64        |    402,650.96764 |    524,488.21526 |    909,785.29362 |  2,553,528.05088 |  2,543,346.58651 |
| AWS Graviton 2.29GHz, DDR4-2400 x 64-bit = 19.2GB/s, g++-8.3.0, A64        |    687,075.86894 |    760,547.82880 |  1,205,666.85495 |  3,089,063.10634 |  3,087,658.99837 |
| AWS Graviton 2.29GHz, DDR4-2400 x 64-bit = 19.2GB/s, clang++-6, A64        |    613,374.18554 |    729,346.71797 |  1,195,205.53597 |  3,120,434.72988 |  3,119,255.48163 |
| Tegra 210 1.428GHz, LPDDR4-3200 x 64-bit = 25.6GB/s, g++-8.2.0, A64        |    327,417.96741 |    433,628.34148 |    818,782.11485 |  2,072,251.75682 |  2,050,263.25087 |
| Tegra 210 1.428GHz, LPDDR4-3200 x 64-bit = 25.6GB/s, clang++-6, A64        |    323,665.85688 |    434,564.48425 |    824,377.91673 |  1,924,823.51200 |  1,953,149.83628 |

![10M average searches/second](images/bsearch_graph_000.png "average searchs/s")
