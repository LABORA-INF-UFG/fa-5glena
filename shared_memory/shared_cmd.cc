// improved_shared_cmd.cpp
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

constexpr const char* SHM_CMD_NAME = "/shared_cmd";
constexpr size_t CMD_SIZE = 256;  // 256 bytes for the command string

int main() {
    // --- Create or Open the shared memory for the command ---
    int shm_cmd_fd = shm_open(SHM_CMD_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_cmd_fd < 0) {
        std::cerr << "Error opening shared memory " << SHM_CMD_NAME 
                  << ": " << strerror(errno) << std::endl;
        return EXIT_FAILURE;
    }
    if (ftruncate(shm_cmd_fd, CMD_SIZE) == -1) {
        std::cerr << "Error setting size for " << SHM_CMD_NAME 
                  << ": " << strerror(errno) << std::endl;
        close(shm_cmd_fd);
        return EXIT_FAILURE;
    }
    
    // --- Map the shared memory segment ---
    void* cmd_ptr = mmap(nullptr, CMD_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_cmd_fd, 0);
    if (cmd_ptr == MAP_FAILED) {
        std::cerr << "Error mapping " << SHM_CMD_NAME 
                  << ": " << strerror(errno) << std::endl;
        close(shm_cmd_fd);
        return EXIT_FAILURE;
    }
    
    // --- Prepare and Write the Command ---
    std::vector<char> cmd_vector(CMD_SIZE, '\0');
    std::string init_cmd = "netend";  // Initial command
    std::copy(init_cmd.begin(), init_cmd.end(), cmd_vector.begin());
    
    // Write the command vector to shared memory.
    std::memcpy(cmd_ptr, cmd_vector.data(), CMD_SIZE);
    //std::cout << "C++ (cmd): Written command: \"" << cmd_vector.data() << "\"" << std::endl;
    
    // --- Wait for the other process to update the command ---
    //std::cout << "C++ (cmd): Waiting for update (press Enter to read back data and clean up)..." << std::endl;
    //std::cin.get();
  
    sleep(3);
/*    
    // --- Read back the command into a new vector ---
    std::vector<char> cmd_vector_read(CMD_SIZE, '\0');
    std::memcpy(cmd_vector_read.data(), cmd_ptr, CMD_SIZE);
    std::cout << "C++ (cmd): Read command: \"" << cmd_vector_read.data() << "\"" << std::endl;
*/ 
/* 
    // --- Clean Up ---
    munmap(cmd_ptr, CMD_SIZE);
    close(shm_cmd_fd);
    if (shm_unlink(SHM_CMD_NAME) == -1)
        std::cerr << "Error unlinking " << SHM_CMD_NAME 
                  << ": " << strerror(errno) << std::endl;
*/
    return EXIT_SUCCESS;
}

