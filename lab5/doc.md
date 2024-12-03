## Algorithms
1. Singlethreaded O(n<sup>2</sup>) multiplication
    * For a polynomial p1 of degree n and another polynomial of degree m, compute the polynomial p3 = p1 * p2 as
        * `p3[i + j] = p3[i + j] + p1[i] * p2[j]`, for any i $\in$ [0, n], j $\in$ [0, m]
2. Multithreaded O(n<sup>2</sup>) multiplication
    * For a polynomial p1 of degree n and another polynomial of degree m, compute the polynomial p3 = p1 * p2 as
        * `p3[i + j] = p3[i + j] + p1[i] * p2[j]`, for any i $\in$ [0, n], j $\in$ [0, m]
    * Using a thread pool, each thread has to compute a chunk with indexes in a given range of size `s = max_degree * 1.5 / THREAD_POOL_SIZE`
    * Each thread will compute a chunk of the result polynomial as 
        * `p3[i + j] = p3[i + j] + p1[i] + p2[j]`, where `iteration * s <= i + j < (iteration + 1) * s`
3. Singlethreaded Karatsuba multiplication
    * Before beginning the algorithm, both polynomials will be padded with zeroes in order to have the same size (necessary for splits)
    * For polynomials of degree 64 or less, the standard O(n<sup>2</sup>) multiplication algorithm will be used
    * Otherwise, the algorithm works as follows:
        * Split the coefficients vectors of the polynomials in half (lower and upper)
        * Compute the following polynomials:
        ```
            z0 = karatsuba(low1, low2)
            z1 = karatsuba(high1, high2)
            z2 = karatsuba(l1h1, l2h2)
            z4 = (z2 - z1 - z0)
            where l1h1 is low1 + high1, l2h2 is low2 + high2
        ```
        * The result vector will be computed in the following manner:
        ```c++
            // z0 contributes to the lower half
            result[i] += z0[i];

            // z4 contributes to the middle of the result
            result[i + mid] += z4.coefficients[i];

            // z1 contributes to the upper half
            result[i + 2 * mid] += z1.coefficients[i];
        ```
4. Multithreaded Karatsuba multiplication
    * For the multithreaded version, the algorithm works in the same way, the only difference being the following:
        * For each split, compute z0 and z1 in a thread pool (will return a future for each), after which we compute z2 in the current thread
        * Once we obtain the results for z0 and z1, the current thread will compute and return the final result
        * If the thread pool is busy(all threads have work to do), z0 and z1 will be computed on the current thread instead

## Benchmark 1 (Polynomial 1 degree : 1000 --- Polynomial 2 degree : 1000)

### Singlethreaded O(n<sup>2</sup>) multiplication : 3ms
### Multithreaded O(n<sup>2</sup>) multiplication : 1ms
### Singlethreaded Karatsuba multiplication : 1ms
### Multithreaded Karatsuba multiplication : 0ms

## Benchmark 2 (Polynomial 1 degree : 10000 --- Polynomial 2 degree : 1000)

### Singlethreaded O(n<sup>2</sup>) multiplication : 37ms
### Multithreaded O(n<sup>2</sup>) multiplication : 11ms
### Singlethreaded Karatsuba multiplication : 72ms
### Multithreaded Karatsuba multiplication : 13ms

## Benchmark 3 (Polynomial 1 degree : 1000 --- Polynomial 2 degree : 10000)

### Singlethreaded O(n<sup>2</sup>) multiplication : 37ms
### Multithreaded O(n<sup>2</sup>) multiplication : 11ms
### Singlethreaded Karatsuba multiplication : 70ms
### Multithreaded Karatsuba multiplication : 16ms

## Benchmark 4 (Polynomial 1 degree : 99799 --- Polynomial 2 degree : 102000)

### Singlethreaded O(n<sup>2</sup>) multiplication : 38420ms
### Multithreaded O(n<sup>2</sup>) multiplication : 12704ms
### Singlethreaded Karatsuba multiplication : 2721ms
### Multithreaded Karatsuba multiplication : 653ms

## Benchmark 5 (Polynomial 1 degree : 100000 --- Polynomial 2 degree : 100000)

### Singlethreaded O(n<sup>2</sup>) multiplication : 38091ms
### Multithreaded O(n<sup>2</sup>) multiplication : 12705ms
### Singlethreaded Karatsuba multiplication : 2606ms
### Multithreaded Karatsuba multiplication : 626ms
