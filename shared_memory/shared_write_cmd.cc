#include <iostream>
#include <cstring>
#include <sys/mman.h>
#include <sys/stat.h>   // For mode constants
#include <fcntl.h>      // For O_* constants
#include <unistd.h>
#include <cerrno>

// Shared memory name and size constants.
constexpr const char* SHM_CMD_NAME = "/shared_cmd";  // Name of the shared memory segment
constexpr size_t CMD_SIZE = 256;  // Maximum size for the command string

int main() {
    // --- Open the shared memory for the command string ---
    int shm_cmd_fd = shm_open(SHM_CMD_NAME, O_RDWR, 0666);
    if (shm_cmd_fd < 0) {
        std::cerr << "Error opening shared memory " << SHM_CMD_NAME 
                  << ": " << strerror(errno) << std::endl;
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

    // --- Prepare the command string to be written ---
    std::string command = "netend";  // The command to be written
    std::memset(cmd_ptr, 0, CMD_SIZE);  // Clear memory before writing

    // Write the command to the shared memory
    std::memcpy(cmd_ptr, command.c_str(), command.size());
    //std::cout << "C++: Written command '" << command << "' to shared memory." << std::endl;

    // --- Wait for the other process to update the shared memory ---
    //std::cout << "C++: Waiting for updates (press Enter to read back data and clean up)..." << std::endl;
    
    sleep(3);  // Simulate wait time

/*
    // --- Read back the command from shared memory ---
    char read_command[CMD_SIZE];
    std::memset(read_command, 0, CMD_SIZE);  // Clear memory before reading

    std::memcpy(read_command, cmd_ptr, CMD_SIZE);  // Read the data
    std::cout << "C++: Read command from shared memory: '" << read_command << "'" << std::endl;
*/
    // --- Clean Up ---
    munmap(cmd_ptr, CMD_SIZE);
    close(shm_cmd_fd);

    return EXIT_SUCCESS;
}

