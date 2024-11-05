## Benchmark 1
### Matrix 1: 9x9
### Matrix 2: 9x9
### Single thread: 0ms
|             |Row-by-row|Column-by-column|Kth element|Tasks|Pool size|
|-------------|----------|----------------|-----------|-----|---------|
|Threads      |2ms       |1ms             |1ms        |4    |         |
|             |3ms       |3ms             |3ms        |8    |         |
|             |7ms       |6ms             |6ms        |16   |         |
|-------------|----------|----------------|-----------|-----|---------|
|Thread pool  |1ms       |1ms             |1ms        |4    |4        |
|             |3ms       |3ms             |3ms        |4    |8        |
|             |1ms       |1ms             |1ms        |8    |4        |
|             |3ms       |2ms             |2ms        |8    |8        |
|             | 6ms      | 6ms            | 6ms       |8    |16       |
|             |7ms       |7ms             |7ms       |16   |16       |

## Benchmark 2
### Matrix 1: 50x60
### Matrix 2: 60x70
### Single thread: 1ms

|             |Row-by-row|Column-by-column|Kth element|Tasks|Pool size|
|-------------|----------|----------------|-----------|-----|---------|
|Threads      | 3ms      | 1ms            | 1ms       |4    |         |
|             | 3ms      | 3ms            | 3ms       |8    |         |
|             | 6ms      | 6ms            | 6ms       |16   |         |
|-------------|----------|----------------|-----------|-----|---------|
|Thread pool  | 1ms      | 1ms            | 1ms       |4    |4        |
|             | 1ms      | 1ms            | 3ms       |4    |8        |
|             | 2ms      | 1ms            | 1ms       |8    |4        |
|             | 3ms      | 3ms            | 3ms       |8    |8        |
|             | 3ms      | 3ms            | 3ms       |8    |16       |
|             | 6ms      | 6ms            | 6ms       |16   |16       |

## Benchmark 3
### Matrix 1: 256x256
### Matrix 2: 256x256
### Single thread: 81ms

|             |Row-by-row|Column-by-column|Kth element|Tasks|Pool size|
|-------------|----------|----------------|-----------|-----|---------|
|Threads      |28ms      |26ms            |25ms       |4    |         |
|             |20ms      |21ms            |20ms       |8    |         |
|             |20ms      |19ms            |20ms       |16   |         |
|-------------|----------|----------------|-----------|-----|---------|
|Thread pool  |33ms      |25ms            |30ms       |4    |4        |
|             |26ms      |26ms            |27ms       |4    |8        |
|             |26ms      |24ms            |25ms       |8    |4        |
|             |20ms      |20ms            |21ms       |8    |8        |
|             |25ms      |27ms            |25ms       |8    |16       |
|             |18ms      |18ms            |19ms       |16   |16       |

## Benchmark 4
### Matrix 1: 1024x2048
### Matrix 2: 2048x1024
### Single thread: 12286ms

|             |Row-by-row|Column-by-column|Kth element|Tasks|Pool size|
|-------------|----------|----------------|-----------|-----|---------|
|Threads      |3135ms    |3119ms          |3603ms     |4    |         |
|             |1957ms    |1766ms          |2575ms     |8    |         |
|             |1955ms    |1899ms          |2358ms     |16   |         |
|-------------|----------|----------------|-----------|-----|---------|
|Thread pool  |3237ms    |3036ms          |3501ms     |4    |4        |
|             |3247ms    |3240ms          |3950ms     |4    |8        |
|             |4261ms    |3220ms          |3720ms     |8    |4        |
|             |1870ms    |2402ms          |2315ms     |8    |8        |
|             |1979ms    |2287ms          |2651ms     |8    |16       |
|             |2009ms    |1982ms          |3260ms     |16   |16       |