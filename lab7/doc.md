## Benchmark 1
### 5000 x 5000
|                        |ST O(n<sup>2</sup>)|MT O(n<sup>2</sup>)|ST Karatsuba|MT Karatsuba|MPI O(n<sup>2</sup>)|MPI Karatsuba|
|------------------------|-------------------|-------------------|------------|------------|--------------------|-------------|
|4 threads/processes     |95ms      |84ms|23ms|23ms|55ms|10ms|
|16 threads/processes     |95ms|33ms|23ms|5ms|32ms|10ms|


## Benchmark 2
### 10000 x 10000
|                        |ST O(n<sup>2</sup>)|MT O(n<sup>2</sup>)|ST Karatsuba|MT Karatsuba|MPI O(n<sup>2</sup>)|MPI Karatsuba|
|------------------------|-------------------|-------------------|------------|------------|--------------------|-------------|
|4 threads/processes     |388ms|345ms|77ms|22ms|226ms|28ms|
|16 threads/processes     |384ms|135ms|72ms|15ms|132ms|27ms|

## Benchmark 3
### 50000 x 50000
|                        |ST O(n<sup>2</sup>)|MT O(n<sup>2</sup>)|ST Karatsuba|MT Karatsuba|MPI O(n<sup>2</sup>)|MPI Karatsuba|
|------------------------|-------------------|-------------------|------------|------------|--------------------|-------------|
|4 threads/processes     |9698ms|8738ms|889ms|277ms|5487ms|618ms|
|16 threads/processes     |9665ms|3259ms|881ms|150ms|2665ms|614ms|