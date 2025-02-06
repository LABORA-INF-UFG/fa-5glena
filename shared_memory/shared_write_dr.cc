#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <fstream>
#include <stdexcept>
#include <cstring>
#include <sys/mman.h>
#include <sys/stat.h>   // For mode constants and fstat
#include <fcntl.h>      // For O_* constants
#include <unistd.h>
#include <cerrno>

// Shared memory name and size constants.
constexpr const char* SHM_DR_NAME = "/shared_dr";
constexpr size_t DR_COUNT = 20;
constexpr size_t DR_SIZE = DR_COUNT * sizeof(double);  // 20 double

// Function to read delivery ratios from a CSV file.
std::vector<double> read_csv(const std::string& file_path) {
    std::vector<double> delivery_ratios;
    std::ifstream file(file_path);

    if (!file.is_open()) {
        throw std::runtime_error("Error opening CSV file: " + file_path);
    }

    std::string line;
    std::getline(file, line);  // Ignore the header line

    while (std::getline(file, line)) {
        std::istringstream stream(line);
        std::string ueid, col2, col3, deliveryratio;

        std::getline(stream, ueid, ',');          // Column 1: ueid
        std::getline(stream, col2, ',');            // Column 2: Ignored
        std::getline(stream, col3, ',');            // Column 3: Ignored
        std::getline(stream, deliveryratio, ',');   // Column 4: deliveryratio

        delivery_ratios.push_back(std::stod(deliveryratio));
    }

    return delivery_ratios;
}

int main() {
    // Open the shared memory for the double array
    int shm_dr_fd = shm_open(SHM_DR_NAME, O_RDWR, 0666);
    if (shm_dr_fd < 0) {
        std::cerr << "Error opening shared memory " << SHM_DR_NAME 
                  << ": " << strerror(errno) << std::endl;
        return EXIT_FAILURE;
    }
    
    // Verify that the shared memory has the expected size.
    struct stat shm_stat;
    if (fstat(shm_dr_fd, &shm_stat) == -1) {
        std::cerr << "Error getting size of shared memory " << SHM_DR_NAME 
                  << ": " << strerror(errno) << std::endl;
        close(shm_dr_fd);
        return EXIT_FAILURE;
    }
    if (static_cast<size_t>(shm_stat.st_size) != DR_SIZE) {
        std::cerr << "Error: Shared memory size (" << shm_stat.st_size 
                  << ") does not match expected size (" << DR_SIZE << ")." << std::endl;
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
    
    // --- Read delivery ratios from the CSV file ---
    std::string csv_file_path = "/home/ufg/anaconda3/envs/fa_5glena/myproject/sim_results/network/net_client_stats.csv";  
    std::vector<double> csv_values;
    try {
        csv_values = read_csv(csv_file_path);
    } catch (const std::exception& ex) {
        std::cerr << "CSV error: " << ex.what() << std::endl;
        munmap(dr_ptr, DR_SIZE);
        close(shm_dr_fd);
        return EXIT_FAILURE;
    }
    
    if (csv_values.size() < DR_COUNT) {
        std::cerr << "Error: CSV file must contain at least " << DR_COUNT << " delivery ratios." << std::endl;
        munmap(dr_ptr, DR_SIZE);
        close(shm_dr_fd);
        return EXIT_FAILURE;
    }
    
    // Convert the CSV values to double and update the shared memory.
    std::vector<double> dr_vector(DR_COUNT);
    for (size_t i = 0; i < DR_COUNT; ++i) {
        dr_vector[i] = static_cast<double>(csv_values[i]);
    }
    
    // Copy the updated vector into the already initialized shared memory.
    std::memcpy(dr_ptr, dr_vector.data(), DR_SIZE);
/*    std::cout << "C++ (dr): Updated dr_vector from CSV: [";
    for (size_t i = 0; i < DR_COUNT; ++i) {
        std::cout << dr_vector[i] << (i < DR_COUNT - 1 ? ", " : "");
    }
    std::cout << "]" << std::endl;
*/    
    // Wait a few seconds to allow other processes to access the updated values.
    sleep(3);
    
    // --- Clean Up ---
    munmap(dr_ptr, DR_SIZE);
    close(shm_dr_fd);
    return EXIT_SUCCESS;
}

