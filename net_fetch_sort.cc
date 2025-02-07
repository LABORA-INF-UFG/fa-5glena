#include <iostream>
#include <filesystem>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <map>
#include <iomanip>

namespace fs = std::filesystem;

// Função para verificar se uma string é numérica
bool is_number(const std::string& s) {
    return !s.empty() && std::all_of(s.begin(), s.end(), ::isdigit);
}

// Estrutura para armazenar os dados da rede, incluindo todas as colunas
struct NetworkData {
    std::map<std::string, std::string> columns;  // Armazena todas as colunas como chave-valor
};

// Função para dividir uma linha CSV em tokens
std::vector<std::string> split_csv_line(const std::string& line) {
    std::vector<std::string> tokens;
    std::stringstream ss(line);
    std::string token;
    
    while (std::getline(ss, token, ',')) {
        tokens.push_back(token);
    }
    
    return tokens;
}

// Função para ler arquivos CSV e capturar todos os dados e cabeçalhos
std::vector<NetworkData> read_csv(const std::string& file_path, std::vector<std::string>& headers) {
    std::vector<NetworkData> data;
    std::ifstream file(file_path);
    std::string line;

    if (!file.is_open()) {
        std::cerr << "Erro ao abrir o arquivo: " << file_path << "\n";
        return data;
    }

    // Ler o cabeçalho apenas uma vez
    if (headers.empty()) {
        std::getline(file, line);
        headers = split_csv_line(line);
    } else {
        std::getline(file, line);  // Ignorar o cabeçalho se já foi lido
    }

    // Ler as linhas dos dados
    while (std::getline(file, line)) {
        std::vector<std::string> tokens = split_csv_line(line);
        if (tokens.size() != headers.size()) continue;  // Ignorar linhas incompletas

        NetworkData row;
        for (size_t i = 0; i < headers.size(); ++i) {
            row.columns[headers[i]] = tokens[i];
        }
        data.push_back(row);
    }

    file.close();
    return data;
}

// Função para escrever o CSV final com todas as colunas
void write_csv(const std::string& output_path, const std::vector<std::string>& headers, const std::vector<NetworkData>& data) {
    std::ofstream out_file(output_path);
    if (!out_file.is_open()) {
        std::cerr << "Erro ao abrir o arquivo para escrita: " << output_path << "\n";
        return;
    }

    // Escrever o cabeçalho
    for (size_t i = 0; i < headers.size(); ++i) {
        out_file << headers[i];
        if (i != headers.size() - 1) out_file << ",";
    }
    out_file << "\n";

    // Escrever os dados
    for (const auto& row : data) {
        for (size_t i = 0; i < headers.size(); ++i) {
            auto it = row.columns.find(headers[i]);
            if (it != row.columns.end()) {
                out_file << it->second;
            }
            if (i != headers.size() - 1) out_file << ",";
        }
        out_file << "\n";
    }

    out_file.close();
}

int main() {
    std::string base_dir = "/home/ufg/anaconda3/envs/fa_5glena/myproject/ns-3-dev/simulation_results/";
    std::string output_dir = "/home/ufg/anaconda3/envs/fa_5glena/myproject/sim_results/network";
    std::string output_file_name = "net_sorted_res.csv";

    // Criar o diretório de saída se não existir
    fs::create_directories(output_dir);

    std::vector<std::string> input_files;
    std::vector<std::string> missing_files;

    // Detectar diretórios numerados
    for (const auto& entry : fs::directory_iterator(base_dir)) {
        if (entry.is_directory() && is_number(entry.path().filename().string())) {
            std::string round_dir = entry.path().filename().string();
            std::string file_path = base_dir + round_dir + "/" + round_dir + ".csv";

            if (fs::exists(file_path)) {
                input_files.push_back(file_path);
            } else {
                missing_files.push_back(file_path);
            }
        }
    }

    std::cout << "\nTotal de arquivos de rede encontrados: " << input_files.size() << "\n";

    if (!missing_files.empty()) {
        std::cout << "\nArquivos ausentes:\n";
        for (const auto& file : missing_files) {
            std::cout << file << "\n";
        }
    }

    if (input_files.empty()) {
        std::cerr << "\nErro: Nenhum arquivo de entrada válido encontrado.\n";
        return 1;
    }

    // Ler e concatenar os dados dos arquivos CSV
    std::vector<NetworkData> combined_data;
    std::vector<std::string> headers;

    for (const auto& file : input_files) {
        std::vector<NetworkData> file_data = read_csv(file, headers);
        combined_data.insert(combined_data.end(), file_data.begin(), file_data.end());
    }

    // Ordenar os dados pela coluna 'ueid'
    std::sort(combined_data.begin(), combined_data.end(),
              [](const NetworkData& a, const NetworkData& b) { 
                  return std::stoi(a.columns.at("ueid")) < std::stoi(b.columns.at("ueid")); 
              });

    // Salvar o resultado final em um arquivo CSV
    std::string output_file_path = output_dir + "/" + output_file_name;
    write_csv(output_file_path, headers, combined_data);

    std::cout << "\nSucesso: Dados de rede limpos e ordenados salvos em " << output_file_path << ".\n";

    return 0;
}

