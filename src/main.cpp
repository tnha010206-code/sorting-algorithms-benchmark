#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <random>
#include <string>
#include <vector>

using namespace std;
using Clock = chrono::steady_clock;
using Vec = vector<double>;
const size_t N = 1'000'000;

void saveBinary(const string& path, const Vec& a) {
    ofstream out(path, ios::binary);
    if (!out) throw runtime_error("Cannot write " + path);
    out.write(reinterpret_cast<const char*>(a.data()), static_cast<streamsize>(a.size() * sizeof(double)));
    if (!out) throw runtime_error("Failed writing " + path);
}
Vec loadBinary(const string& path) {
    ifstream in(path, ios::binary | ios::ate);
    if (!in) throw runtime_error("Cannot read " + path);
    auto bytes = in.tellg();
    if (bytes < 0 || bytes % static_cast<streamoff>(sizeof(double)) != 0) throw runtime_error("Invalid binary dataset: " + path);
    Vec a(static_cast<size_t>(bytes / sizeof(double)));
    in.seekg(0);
    in.read(reinterpret_cast<char*>(a.data()), bytes);
    if (!in) throw runtime_error("Failed reading " + path);
    return a;
}

void generateData(const string& dir) {
    filesystem::create_directories(dir);
    mt19937_64 rng(20261010);
    uniform_real_distribution<double> dist(-1'000'000.0, 1'000'000.0);
    for (int d = 1; d <= 10; ++d) {
        Vec a(N);
        for (double& x : a) x = dist(rng);
        if (d == 1) sort(a.begin(), a.end());
        else if (d == 2) { sort(a.begin(), a.end()); reverse(a.begin(), a.end()); }
        string path = dir + "/dataset_" + (d < 10 ? "0" : "") + to_string(d) + ".bin";
        saveBinary(path, a);
        cerr << "Generated " << path << " (" << a.size() << " doubles)\n";
    }
}

// Randomized-pivot QuickSort; recurse on the smaller partition to limit stack depth.
void quickSortRange(Vec& a, ptrdiff_t lo, ptrdiff_t hi, mt19937_64& rng) {
    while (lo < hi) {
        uniform_int_distribution<ptrdiff_t> pick(lo, hi);
        double pivot = a[static_cast<size_t>(pick(rng))];
        ptrdiff_t i = lo, j = hi;
        while (i <= j) {
            while (a[static_cast<size_t>(i)] < pivot) ++i;
            while (a[static_cast<size_t>(j)] > pivot) --j;
            if (i <= j) { swap(a[static_cast<size_t>(i)], a[static_cast<size_t>(j)]); ++i; --j; }
        }
        if (j - lo < hi - i) {
            if (lo < j) quickSortRange(a, lo, j, rng);
            lo = i;
        } else {
            if (i < hi) quickSortRange(a, i, hi, rng);
            hi = j;
        }
    }
}
void quickSort(Vec& a) { mt19937_64 rng(0x51A7E123ULL); if (!a.empty()) quickSortRange(a, 0, static_cast<ptrdiff_t>(a.size()) - 1, rng); }

void siftDown(Vec& a, size_t root, size_t n) {
    while (root * 2 + 1 < n) {
        size_t child = root * 2 + 1;
        if (child + 1 < n && a[child] < a[child + 1]) ++child;
        if (!(a[root] < a[child])) return;
        swap(a[root], a[child]); root = child;
    }
}
void heapSort(Vec& a) {
    size_t n = a.size();
    for (size_t start = n / 2; start > 0; --start) siftDown(a, start - 1, n);
    for (size_t end = n; end > 1; --end) { swap(a[0], a[end - 1]); siftDown(a, 0, end - 1); }
}
void mergeSort(Vec& a) {
    const size_t n = a.size();
    Vec tmp(n);
    for (size_t width = 1; width < n; width *= 2) {
        for (size_t left = 0; left < n; left += 2 * width) {
            size_t mid = min(left + width, n), right = min(left + 2 * width, n);
            size_t i = left, j = mid, k = left;
            while (i < mid && j < right) tmp[k++] = (a[i] <= a[j]) ? a[i++] : a[j++];
            while (i < mid) tmp[k++] = a[i++];
            while (j < right) tmp[k++] = a[j++];
            copy(tmp.begin() + static_cast<ptrdiff_t>(left), tmp.begin() + static_cast<ptrdiff_t>(right), a.begin() + static_cast<ptrdiff_t>(left));
        }
        if (width > n / 2) break;
    }
}

int main(int argc, char** argv) {
    if (argc < 2) {
        cerr << "Usage: sorting_benchmark <generate|benchmark> [data_dir] [results_csv]\n";
        return 1;
    }
    string mode = argv[1], dataDir = argc >= 3 ? argv[2] : "data", resultPath = argc >= 4 ? argv[3] : "results/timings.csv";
    try {
        if (mode == "generate") { generateData(dataDir); return 0; }
        if (mode != "benchmark") throw runtime_error("Unknown mode: " + mode);
        filesystem::create_directories(filesystem::path(resultPath).parent_path().empty() ? "." : filesystem::path(resultPath).parent_path());
        ofstream out(resultPath);
        if (!out) throw runtime_error("Cannot write result CSV");
        out << "dataset,n,quicksort_ms,heapsort_ms,mergesort_ms,std_sort_ms\n" << fixed << setprecision(3);
        for (int d = 1; d <= 10; ++d) {
            string path = dataDir + "/dataset_" + (d < 10 ? "0" : "") + to_string(d) + ".bin";
            Vec original = loadBinary(path);
            if (original.size() != N) throw runtime_error("Expected 1,000,000 numbers in " + path);
            double times[4];
            for (int alg = 0; alg < 4; ++alg) {
                Vec a = original;
                auto start = Clock::now();
                if (alg == 0) quickSort(a);
                else if (alg == 1) heapSort(a);
                else if (alg == 2) mergeSort(a);
                else sort(a.begin(), a.end());
                auto stop = Clock::now();
                times[alg] = chrono::duration<double, milli>(stop - start).count();
                if (!is_sorted(a.begin(), a.end())) throw runtime_error("Sort verification failed on dataset " + to_string(d));
            }
            out << d << ',' << original.size();
            for (double t : times) out << ',' << t;
            out << '\n'; out.flush();
            cout << "Dataset " << d << ": QuickSort=" << times[0] << " ms, HeapSort=" << times[1]
                 << " ms, MergeSort=" << times[2] << " ms, std::sort=" << times[3] << " ms\n";
        }
        cerr << "Saved timings to " << resultPath << "\n";
    } catch (const exception& e) { cerr << "ERROR: " << e.what() << '\n'; return 2; }
}
