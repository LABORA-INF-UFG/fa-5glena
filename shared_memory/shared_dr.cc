#include <iostream>
#include <vector>
#include <string>
#include <cstring>
#include <algorithm>  // for std::copy
#include <sys/mman.h>
#include <sys/stat.h>   // For mode constants
#include <fcntl.h>      // For O_* constants
#include <unistd.h>
#include <cerrno>

constexpr const char* SHM_DR_NAME = "/shared_dr";  // Shared memory name
constexpr size_t DR_COUNT = 20;                    // Number of elements in delivery_ratios
constexpr size_t DR_SIZE = DR_COUNT * sizeof(double);  // Size for 20 double values

int main() {
    // --- Create or Open the shared memory for the delivery_ratios ---
    int shm_dr_fd = shm_open(SHM_DR_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_dr_fd < 0) {
        std::cerr << "Error opening shared memory " << SHM_DR_NAME 
                  << ": " << strerror(errno) << std::endl;
        return EXIT_FAILURE;
    }

    if (ftruncate(shm_dr_fd, DR_SIZE) == -1) {
        std::cerr << "Error setting size for " << SHM_DR_NAME 
                  << ": " << strerror(errno) << std::endl;
        close(shm_dr_fd);
        return EXIT_FAILURE;
    }

    // --- Map the shared memory segment ---
    void* dr_ptr = mmap(nullptr, DR_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_dr_fd, 0);
    if (dr_ptr == MAP_FAILED) {
        std::cerr << "Error mapping " << SHM_DR_NAME 
                  << ": " << strerror(errno) << std::endl;
        close(shm_dr_fd);
        return EXIT_FAILURE;
    }

    // --- Initialize the shared memory with zeros (empty memory) ---
    std::vector<double> delivery_ratios(DR_COUNT, 0.0);  // Initialize a vector of 20 doubles to 0.0
    std::memcpy(dr_ptr, delivery_ratios.data(), DR_SIZE);  // Write the zeroed vector to shared memory
    //std::cout << "C++: Initialized shared memory with 20 zeros for delivery_ratios." << std::endl;

    // --- Wait for the other process to update the shared memory ---
    //std::cout << "C++: Waiting for updates to the delivery_ratios..." << std::endl;
    //std::cin.get();

    sleep(3);
/*
    // --- Read back the delivery_ratios from shared memory ---
    std::vector<double> delivery_ratios_read(DR_COUNT, 0.0);
    std::memcpy(delivery_ratios_read.data(), dr_ptr, DR_SIZE);
    std::cout << "C++: Read delivery_ratios from shared memory:" << std::endl;
    for (size_t i = 0; i < DR_COUNT; ++i) {
        std::cout << "delivery_ratios[" << i << "] = " << delivery_ratios_read[i] << std::endl;
    }
 */
/*
    // --- Clean Up ---
    munmap(dr_ptr, DR_SIZE);
    close(shm_dr_fd);
    if (shm_unlink(SHM_DR_NAME) == -1)
        std::cerr << "Error unlinking " << SHM_DR_NAME 
                  << ": " << strerror(errno) << std::endl;
*/
    return EXIT_SUCCESS;
}

