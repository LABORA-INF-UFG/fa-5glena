//#include "/home/ufg/anaconda3/envs/fa_5glena/myproject/5G-LENA/shared_memory_writer.h"
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <vector>
#include <string>
#include <cstring>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <filesystem>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>

namespace py = pybind11;

struct SharedMemory {
    size_t size;            // Number of elements in the delivery_ratios vector
    size_t string_count;    // Number of strings in the command vector
    double data[];          // Flexible array member to hold the data dynamically
    // Strings will be stored immediately after the delivery_ratios data
};

// Function to read delivery ratios from the CSV file
std::vector<double> read_csv(const std::string& file_path) {
    std::vector<double> delivery_ratios;
    std::ifstream file(file_path);

    if (!file.is_open()) {
        throw std::runtime_error("Error opening CSV file.");
    }

    std::string line;
    std::getline(file, line);  // Ignore the header line

    while (std::getline(file, line)) {
        std::istringstream stream(line);
        std::string ueid, col2, col3, deliveryratio;

        std::getline(stream, ueid, ',');            // Column 1: ueid
        std::getline(stream, col2, ',');            // Column 2: Ignored
        std::getline(stream, col3, ',');            // Column 3: Ignored
        std::getline(stream, deliveryratio, ',');   // Column 4: deliveryratio

        delivery_ratios.push_back(std::stod(deliveryratio));
    }

    return delivery_ratios;
}

void create_shared_memory(const std::vector<std::string>& commands) {
    const char *shm_name = "/my_shared_memory";

    // Unlink any previous shared memory object to ensure a fresh start
    shm_unlink(shm_name);

    // Get the absolute path dynamically
    std::string csv_path = std::filesystem::absolute("/home/ufg/anaconda3/envs/fa_5glena/myproject/5G-LENA/randTx_stats.csv").string();
    std::vector<double> delivery_ratios = read_csv(csv_path);

    // Calculate the total size of shared memory
    size_t data_size = delivery_ratios.size();
    size_t string_count = commands.size();

    // Calculate memory required for storing strings
    size_t string_data_size = 0;
    for (const auto &cmd : commands) {
        string_data_size += cmd.size() + 1; // Add 1 for null-terminator
    }

    // Total size of shared memory
    size_t shm_size = sizeof(SharedMemory) + 
                      data_size * sizeof(double) + 
                      sizeof(size_t) * string_count + 
                      string_data_size;

    // Create new shared memory object
    int shm_fd = shm_open(shm_name, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        throw std::runtime_error("Error creating shared memory.");
    }

    // Set the size of the shared memory object
    if (ftruncate(shm_fd, shm_size) == -1) {
        close(shm_fd);
        throw std::runtime_error("Error setting the size of shared memory.");
    }

    // Map the shared memory into the process address space
    SharedMemory *shm_ptr = static_cast<SharedMemory*>(mmap(NULL, shm_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0));
    if (shm_ptr == MAP_FAILED) {
        close(shm_fd);
        throw std::runtime_error("Error mapping shared memory.");
    }

    // Write the size of the vectors to shared memory
    shm_ptr->size = data_size;
    shm_ptr->string_count = string_count;

    // Write the delivery ratios to shared memory
    std::memcpy(shm_ptr->data, delivery_ratios.data(), data_size * sizeof(double));

    // Write the strings to shared memory after the delivery ratios
    size_t *string_sizes = reinterpret_cast<size_t *>(shm_ptr->data + data_size);
    char *string_data = reinterpret_cast<char *>(string_sizes + string_count);

    size_t offset = 0;
    for (size_t i = 0; i < commands.size(); ++i) {
        string_sizes[i] = commands[i].size() + 1; // Include null-terminator
        std::memcpy(string_data + offset, commands[i].c_str(), string_sizes[i]);
        offset += string_sizes[i];
    }

    // Synchronize changes to shared memory
    msync(shm_ptr, shm_size, MS_SYNC);

    // Unmap and close the shared memory
    munmap(shm_ptr, shm_size);
    close(shm_fd);
}




std::pair<std::vector<double>, std::vector<std::string>> read_shared_memory() {
    const char *shm_name = "/my_shared_memory";

    // Open the shared memory object
    int shm_fd = shm_open(shm_name, O_RDWR, 0666);
    if (shm_fd == -1) {
        throw std::runtime_error("Error opening shared memory.");
    }

    // Map the size fields first to determine the total size of the shared memory
    SharedMemory temp;
    if (read(shm_fd, &temp, sizeof(size_t) * 2) == -1) {
        close(shm_fd);
        throw std::runtime_error("Error reading size from shared memory.");
    }

    size_t data_size = temp.size;
    size_t string_count = temp.string_count;

    // Calculate the total size of shared memory
    size_t shm_size = sizeof(SharedMemory) + 
                      data_size * sizeof(double) + 
                      string_count * sizeof(size_t);

    // Map the entire shared memory
    SharedMemory *shm_ptr = static_cast<SharedMemory*>(mmap(NULL, shm_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0));
    if (shm_ptr == MAP_FAILED) {
        close(shm_fd);
        throw std::runtime_error("Error mapping shared memory.");
    }

    // Read the delivery ratios
    std::vector<double> delivery_ratios(shm_ptr->data, shm_ptr->data + data_size);

    // Read the strings
    size_t *string_sizes = reinterpret_cast<size_t *>(shm_ptr->data + data_size);
    char *string_data = reinterpret_cast<char *>(string_sizes + string_count);

    std::vector<std::string> commands;
    size_t offset = 0;
    for (size_t i = 0; i < string_count; ++i) {
        commands.emplace_back(string_data + offset, string_sizes[i] - 1); // Exclude null-terminator
        offset += string_sizes[i];
    }

    // Unmap and close the shared memory
    munmap(shm_ptr, shm_size);
    close(shm_fd);

    return {delivery_ratios, commands};
}

PYBIND11_MODULE(shared_memory_writer, m) {
    m.def("create_shared_memory", &create_shared_memory, "Create and write data to shared memory", py::arg("commands"));
    m.def("read_shared_memory", &read_shared_memory, "Read data from shared memory");
}

