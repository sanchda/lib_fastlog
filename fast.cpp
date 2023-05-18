#include <iostream>
#include <thread>
#include <vector>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <x86intrin.h>

const char* filename = "test.txt";
const int bufferSize = 1024;  // Size of the buffer (number of integers)
volatile int appendLocation = 0;  // Location to append the next integer
int lock = 0;  // Spinlock

// The global struct shared among all threads
struct GlobalStruct {
    int fd;  // File descriptor
    void* addr;  // Mapped address
    size_t size;  // Current size of the file
} gs;

void appendInts(const std::vector<int>& nums) {
    for (int num : nums) {
        while (true) {
            // If we need to extend the file
            if (appendLocation >= gs.size / sizeof(int)) {
                // Acquire lock
                while (__atomic_test_and_set(&lock, __ATOMIC_SEQ_CST)) {}

                // Double the size of the file
                gs.size *= 2;
                if (ftruncate(gs.fd, gs.size) == -1) {
                    std::cerr << "Failed to extend file.\n";
                    return;
                }

                // Remap the file into memory
                munmap(gs.addr, gs.size / 2);
                gs.addr = mmap(NULL, gs.size, PROT_WRITE, MAP_SHARED, gs.fd, 0);
                if (gs.addr == MAP_FAILED) {
                    std::cerr << "Failed to map file.\n";
                    return;
                }

                // Release lock
                __atomic_clear(&lock, __ATOMIC_SEQ_CST);
            }

            // If the file was extended by another thread
            if (appendLocation >= gs.size / sizeof(int)) {
                continue;
            }

            // Append the integer
            int* p = (int*)((char*)gs.addr + appendLocation * sizeof(int));
            *p = num;
            ++appendLocation;
            break;
        }
    }
}

int main() {
    const int num_threads = 10;

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
