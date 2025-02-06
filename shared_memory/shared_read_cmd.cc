#include <iostream>
#include <cstring>
#include <sys/mman.h>
#include <sys/stat.h>   // For mode constants
#include <fcntl.h>      // For O_* constants
#include <unistd.h>
#include <cerrno>

// Shared memory name and size constants.
constexpr const char* SHM_CMD_NAME = "/shared_cmd";  // Name of the shared memory segment
constexpr size_t CMD_SIZE = 256;  // Maximum size for the command string (can be adjusted)

std::string read_shared_memory_command() {
    // --- Open the existing shared memory for the command string ---
    int shm_cmd_fd = shm_open(SHM_CMD_NAME, O_RDWR, 0666);  // No O_CREAT, as the memory already exists
    if (shm_cmd_fd < 0) {
        std::cerr << "Error opening shared memory " << SHM_CMD_NAME 
                  << ": " << strerror(errno) << std::endl;
        return "";
    }

    // --- Map the shared memory segment ---
    void* cmd_ptr = mmap(nullptr, CMD_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_cmd_fd, 0);
    if (cmd_ptr == MAP_FAILED) {
        std::cerr << "Error mapping " << SHM_CMD_NAME 
                  << ": " << strerror(errno) << std::endl;
        close(shm_cmd_fd);
        return "";
    }

    // --- Read back the command from shared memory ---
    char read_command[CMD_SIZE];
    std::memset(read_command, 0, CMD_SIZE);  // Clear memory before reading
    std::memcpy(read_command, cmd_ptr, CMD_SIZE);  // Read the data

    // --- Clean Up ---
    munmap(cmd_ptr, CMD_SIZE);
    close(shm_cmd_fd);

    return std::string(read_command);  // Return the command as a string
}

int main() {
    // --- Read back the command from shared memory ---
    std::string read_command = read_shared_memory_command();

    // Check if we got a valid command back
    if (!read_command.empty()) {
        if(read_command != "netrun"){
            std::cerr << "Server not running. Exiting..." << std::endl;
            exit(1);
        }    
        //std::cout << "C++: Read command from shared memory: '" << read_command << "'" << std::endl;
        return 0;  // Return success status with the read command value (print it too)
    } else {
        std::cerr << "C++: Failed to read valid command from shared memory." << std::endl;
        return 1;  // Return error status
    }
}

