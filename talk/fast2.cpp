#include <atomic>
#include <cstdlib>
#include <string>
#include <chrono>
#include <iostream>
#include <thread>
#include <vector>
#include <fcntl.h>
#include <unistd.h>

const char* filename = "test.txt";
std::atomic<size_t> num_locs(1);
auto start_time = std::chrono::steady_clock::now();

void appendInts(const std::vector<int>& nums, int fd) {
  while (true) {
    for (int num : nums) {
      write(fd, &num, sizeof(int));
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
    int fd = open(filename, O_RDWR | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR);
    if (fd == -1) {
        std::cerr << "Failed to open file.\n";
        return 1;
    }

    // Create the vector of integers
    std::vector<int> nums(1024);
    for (int i = 0; i < 1024; ++i) {
        nums[i] = i;
    }

    // Create and start threads
    std::vector<std::thread> threads;
    for (int i = 0; i < num_threads; ++i) {
        threads.push_back(std::thread(appendInts, nums, fd));
    }

    // Join threads
    for (std::thread& thread : threads) {
        thread.join();
    }

    close(fd);

    return 0;
}
