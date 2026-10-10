# Sorting algorithm experiment (1,000,000 doubles × 10 datasets)

## Algorithms
- Randomized-pivot QuickSort
- HeapSort
- Bottom-up MergeSort
- C++ Standard Library `std::sort`

## Build
```bash
g++ -O2 -std=c++17 -DNDEBUG src/main.cpp -o sorting_benchmark
```

## Generate the data
```bash
./sorting_benchmark generate data
```
A fixed seed (`20261010`) makes generation reproducible. Dataset 01 is ascending, dataset 02 descending, and datasets 03–10 are random. Each binary file contains 1,000,000 IEEE-754 double values.

## Run the benchmark
```bash
./sorting_benchmark benchmark data results/timings.csv
```
Each algorithm sorts its own copy of the same dataset. Copying/loading is outside the timed region. Times use `std::chrono::steady_clock`, and sorted output is verified.

## Repository contents
See `src/` for source code, `data/` for datasets, `results/` for CSV timing results and charts, and `report/` for the PDF report.

Benchmark results depend on hardware, compiler, OS load, and compiler flags. Record the machine and compiler details when reproducing the experiment.
