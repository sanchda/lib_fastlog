#include <atomic>
#include <cstdlib>
#include <string>
#include <chrono>
#include <iostream>
#include <thread>
#include <vector>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <x86intrin.h>

const char* filename = "test.txt";
const int bufferSize = 1024;  // Size of the buffer (number of integers)
std::atomic<int> appendLocation(0);
std::atomic<size_t> num_locs(1);
std::atomic_flag lock = ATOMIC_FLAG_INIT; // spinlock
auto start_time = std::chrono::steady_clock::now();

// The global struct shared among all threads
struct GlobalStruct {
  int fd;  // File descriptor
  void* addr;  // Mapped address
  size_t size;  // Current size of the file
} gs;

void take_lock() {
  while (__atomic_test_and_set(&lock, __ATOMIC_SEQ_CST)) {}
}

void release_lock() {
  lock.clear(std::memory_order_release);
}

void append_value(int &num, int &idx) {
  take_lock();
  int* p = (int*)((char*)gs.addr + idx * sizeof(int));
  __atomic_store_n(p, num, __ATOMIC_SEQ_CST);
  appendLocation.fetch_add(1);
  release_lock();
}

void appendInts(const std::vector<int>& nums) {
  while (true) {
    for (int num : nums) {
      // If we need to extend the file
      int loc_snapshot;
      if ((loc_snapshot = appendLocation.fetch_add(1)) >= gs.size / sizeof(int)) {
        // Acquire lock
        take_lock();
        munmap(gs.addr, gs.size);
        gs.size += bufferSize;
        if (ftruncate(gs.fd, gs.size) == -1) {
          std::cerr << "Failed to extend file.\n";
          return;
        }
        gs.addr = mmap(NULL, gs.size, PROT_WRITE, MAP_SHARED, gs.fd, 0);
        if (gs.addr == MAP_FAILED) {
          std::cerr << "Failed to map file.\n";
          return;
        }
        release_lock();
      }

      append_value(num, loc_snapshot);
      auto locs = num_locs.fetch_add(1);
      if (!(locs % 1000000)) {
        using namespace std::chrono;
        auto seconds = duration<double>(steady_clock::now() - start_time).count();
        std::cout << locs / seconds << std::endl;
      }
    }
  }
}

int main(int argc, char **argv) {
    int num_threads = 10;

    if (argc > 1)
      num_threads = std::atoi(argv[1]);

    // Open the file
    gs.fd = open(filename, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    if (gs.fd == -1) {
        std::cerr << "Failed to open file.\n";
        return 1;
    }

    // Initial size of the file
    gs.size = bufferSize * sizeof(int);

    // Extend the file to the initial size
    if (ftruncate(gs.fd, gs.size) == -1) {
        std::cerr << "Failed to extend file.\n";
        return 1;
    }

    // Map the file into memory
    gs.addr = mmap(NULL, gs.size, PROT_WRITE, MAP_SHARED, gs.fd, 0);
    if (gs.addr == MAP_FAILED) {
        std::cerr << "Failed to map file.\n";
        return 1;
    }

    // Create the vector of integers
    std::vector<int> nums(bufferSize);
    for (int i = 0; i < bufferSize; ++i) {
        nums[i] = i;
    }

    // Create and start threads
    std::vector<std::thread> threads;
    for (int i = 0; i < num_threads; ++i) {
        threads.push_back(std::thread(appendInts, nums));
    }

    // Join threads
    for (std::thread& thread : threads) {
        thread.join();
    }

    munmap(gs.addr, gs.size);
    close(gs.fd);

    return 0;
}
