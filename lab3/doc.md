## Benchmark 1
### Matrix 1: 9x9
### Matrix 2: 9x9
### Single thread: 1ms
|             |Row-by-row|Column-by-column|Kth element|Tasks|Pool size|
|-------------|----------|----------------|-----------|-----|---------|
|Threads      |3ms       |3ms             |3ms        |4    |         |
|             |6ms       |5ms             |5ms        |8    |         |
|             |11ms      |10ms            |10ms       |16   |         |
|-------------|----------|----------------|-----------|-----|---------|
|Thread pool  |3ms       |3ms             |3ms        |4    |4        |
|             |3ms       |3ms             |3ms        |4    |8        |
|             |5ms       |5ms             |6ms        |8    |4        |
|             |6ms       |6ms             |7ms        |8    |8        |
|             |11ms      |10ms            |14ms       |8    |16       |
|             |11ms      |12ms            |15ms       |16   |16       |

## Benchmark 2
### Matrix 1: 20x30
### Matrix 2: 30x30
### Single thread: 46ms

|             |Row-by-row|Column-by-column|Kth element|Tasks|Pool size|
|-------------|----------|----------------|-----------|-----|---------|
|Threads      |27ms      |27ms            |27ms       |4    |         |
|             |32ms      |34ms            |34ms       |8    |         |
|             |51ms      |53ms            |52ms       |16   |         |
|-------------|----------|----------------|-----------|-----|---------|
|Thread pool  |29ms      |29ms            |29ms       |4    |4        |
|             |29ms      |28ms            |30ms       |4    |8        |
|             |27ms      |26ms            |34ms       |8    |4        |
|             |35ms      |42ms            |36ms       |8    |8        |
|             |40ms      |48ms            |41ms       |8    |16       |
|             |59ms      |61ms            |62ms       |16   |16       |

## Benchmark 3
### Matrix 1: 100x100
### Matrix 2: 100x100
### Single thread: 17976ms

|             |Row-by-row|Column-by-column|Kth element|Tasks|Pool size|
|-------------|----------|----------------|-----------|-----|---------|
|Threads      |7426ms    |10743ms         |11329ms    |4    |         |
|             |4194ms    |5823ms          |5884ms     |8    |         |
|             |5930ms    |6530ms          |6593ms     |16   |         |
|-------------|----------|----------------|-----------|-----|---------|
|Thread pool  |10032ms   |10606ms         |8842ms     |4    |4        |
|             |7900ms    |10721ms         |9729ms     |4    |8        |
|             |25139ms   |28033ms         |27615ms    |8    |4        |
|             |6129ms    |6838ms          |6495ms     |8    |8        |
|             |5513ms    |6112ms          |6158ms     |8    |16       |
|             |6610ms    |6559ms          |6628ms     |16   |16       |
