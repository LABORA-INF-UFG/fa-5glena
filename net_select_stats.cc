#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include <cmath>
#include <iomanip>

struct ClientStats {
    double mean_rxpackets = 0;
    double std_rxpackets = 0;
    double avg_deliveryratio = 0;
    int count = 0;
    std::vector<int> rxpackets_values;
};

void calculate_and_save_statistics(const std::string& input_file, const std::string& output_file) {
    // Step 1: Read input CSV file
    std::ifstream infile(input_file);
    if (!infile.is_open()) {
        std::cerr << "Error opening input file!" << std::endl;
        return;
    }

    std::map<int, ClientStats> client_stats; // Map to store stats for each ueid

    std::string line;
    std::getline(infile, line); // Skip the header

    // Step 2: Process each row in the CSV file
    while (std::getline(infile, line)) {
        std::stringstream ss(line);
        std::string ueid_str, txpackets_str, rxpackets_str, packetLoss_str, deliveryratio_str, throughput_str, latency_str, meanjitter_str, txenergy_str, distance_str, txpower_str;
        
        // Reading values from CSV
        std::getline(ss, ueid_str, ',');
        std::getline(ss, txpackets_str, ',');
        std::getline(ss, rxpackets_str, ',');
        std::getline(ss, packetLoss_str, ',');
        std::getline(ss, deliveryratio_str, ',');
        std::getline(ss, throughput_str, ',');
        std::getline(ss, latency_str, ',');
        std::getline(ss, meanjitter_str, ',');
        std::getline(ss, txenergy_str, ',');
        std::getline(ss, distance_str, ',');
        std::getline(ss, txpower_str, ',');

        // Convert strings to appropriate types
        int ueid = std::stoi(ueid_str);
        int rxpackets = std::stoi(rxpackets_str);
        double deliveryratio = std::stod(deliveryratio_str);

        // Update the statistics for the client
        ClientStats& stats = client_stats[ueid];
        stats.rxpackets_values.push_back(rxpackets);
        stats.avg_deliveryratio += deliveryratio;
        stats.count++;
    }

    infile.close();

    // Step 3: Calculate mean and standard deviation for each client
    std::ofstream outfile(output_file);
    if (!outfile.is_open()) {
        std::cerr << "Error opening output file!" << std::endl;
        return;
    }

    // Write headers to the output CSV file
    outfile << "ueid,mean,std,deliveryratio" << std::endl;

    for (auto& [ueid, stats] : client_stats) {
        // Calculate mean for rxpackets
        double mean_rxpackets = 0;
        for (int rxpacket : stats.rxpackets_values) {
            mean_rxpackets += rxpacket;
        }
        mean_rxpackets /= stats.count;

        // Calculate std for rxpackets
        double variance = 0;
        for (int rxpacket : stats.rxpackets_values) {
            variance += std::pow(rxpacket - mean_rxpackets, 2);
        }
        stats.std_rxpackets = std::sqrt(variance / stats.count);

        // Calculate average delivery ratio
        stats.avg_deliveryratio /= stats.count;

        // Write the calculated values to the output CSV file with 4 decimal places
        outfile << ueid << ","
                << std::fixed << std::setprecision(4) << mean_rxpackets << ","
                << std::fixed << std::setprecision(4) << stats.std_rxpackets << ","
                << std::fixed << std::setprecision(4) << stats.avg_deliveryratio << std::endl;
    }

    outfile.close();
    std::cout << "Statistics for rxpackets and deliveryratio per ueid saved to " << output_file << std::endl;
}

int main() {
    // Define input and output file paths
    std::string input_file = "/home/ufg/anaconda3/envs/fa_5glena/myproject/sim_results/network/net_sorted_res.csv"; // Input CSV file path
    std::string output_file = "/home/ufg/anaconda3/envs/fa_5glena/myproject/sim_results/network/net_client_stats.csv"; // Output file path

    // Ensure the output directory exists
    std::ofstream test(output_file);
    test.close();

    // Step 4: Calculate and save the statistics
    calculate_and_save_statistics(input_file, output_file);

    return 0;
}

