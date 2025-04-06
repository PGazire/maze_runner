#include <iostream>
#include <fstream>
#include <vector>
#include <stack>
#include <thread>
#include <chrono>
#include <mutex>
// Representação do labirinto
using Maze = std::vector<std::vector<char>>;

std::mutex maze_mutex;
std::mutex exit_mutex;
bool exit_found = false;

// Estrutura para representar uma posição no labirinto
struct Position {
    int row;
    int col;
};

// Variáveis globais
Maze maze;
int num_rows;
int num_cols;
std::stack<Position> valid_positions;

// Função para carregar o labirinto de um arquivo
Position load_maze(const std::string& file_name) {
    // TODO: Implemente esta função seguindo estes passos:
    // 1. Abra o arquivo especificado por file_name usando std::ifstream
    // 2. Leia o número de linhas e colunas do labirinto
    // 3. Redimensione a matriz 'maze' de acordo (use maze.resize())
    // 4. Leia o conteúdo do labirinto do arquivo, caractere por caractere
    // 5. Encontre e retorne a posição inicial ('e')
    // 6. Trate possíveis erros (arquivo não encontrado, formato inválido, etc.)
    // 7. Feche o arquivo após a leitura
    // 1. Abre o arquivo
    std::ifstream file(file_name);
    if (!file.is_open()) {
        std::cerr << "Erro ao abrir o arquivo: " << file_name << std::endl;
        return {-1, -1};
    }

    //Lê o número de linhas e colunas
    file >> num_rows >> num_cols;

    if (num_rows <= 0 || num_cols <= 0) {
        std::cerr << "Tamanho inválido para o labirinto." << std::endl;
        return {-1, -1};
    }

    // Ajusta o tamanho do labirinto
    maze.resize(num_rows, std::vector<char>(num_cols));

    // Variável para armazenar a posição de entrada
    Position start_pos{-1, -1};

    // Descarta o resto da linha após ler num_rows e num_cols
    file.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    //Lê o conteúdo do arquivo linha a linha
    for (int i = 0; i < num_rows; i++) {
        std::string line;
        std::getline(file, line);

        //Se a linha for menor que num_cols, o arquivo pode estar incorreto
        if (static_cast<int>(line.size()) < num_cols) {
            std::cerr << "Linha do arquivo com caracteres insuficientes: "
                      << line << std::endl;
            return {-1, -1};
        }

        // Copia cada caractere para a matriz
        for (int j = 0; j < num_cols; j++) {
            maze[i][j] = line[j];
            //Se encontrar 'e', guarda como posição inicial
            if (line[j] == 'e') {
                start_pos = {i, j};
            }
        }
    }

    file.close();
    return start_pos;  
}

// Função para imprimir o labirinto
void print_maze() {
    // TODO: Implemente esta função
    // 1. Percorra a matriz 'maze' usando um loop aninhado
    // 2. Imprima cada caractere usando std::cout
    // 3. Adicione uma quebra de linha (std::cout << '\n') ao final de cada linha do labirinto
    // Percorre a matriz e imprime caractere a caractere
    for (int i = 0; i < num_rows; i++) {
        for (int j = 0; j < num_cols; j++) {
            std::cout << maze[i][j];
        }
        std::cout << '\n';
    }
    std::cout << std::endl;
}


// Função para verificar se uma posição é válida
bool is_valid_position(int row, int col) {
    // TODO: Implemente esta função
    // 1. Verifique se a posição está dentro dos limites do labirinto
    //    (row >= 0 && row < num_rows && col >= 0 && col < num_cols)
    // 2. Verifique se a posição é um caminho válido (maze[row][col] == 'x')
    // 3. Retorne true se ambas as condições forem verdadeiras, false caso contrário
    //Verifica se a posição está dentro dos limites
    if (row < 0 || row >= num_rows || col < 0 || col >= num_cols) {
        return false;
    }
    //Verifica se é um caminho válido ('x') ou a saída ('s')
    if (maze[row][col] == 'x' || maze[row][col] == 's') {
        return true;
    }
    return false;
}


// Função principal para navegar pelo labirinto
bool walk(Position pos) {
    {
        std::lock_guard<std::mutex> lock(exit_mutex);
        if (exit_found) return true;
    }

    // Marca posição atual como 'o'
    {
        std::lock_guard<std::mutex> lock(maze_mutex);
        if (maze[pos.row][pos.col] == 's') {
            exit_found = true;
            return true;
        }
        if (maze[pos.row][pos.col] != 'e') {
            maze[pos.row][pos.col] = 'o';
        }
    }

    print_maze();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    {
        std::lock_guard<std::mutex> lock(maze_mutex);
        if (maze[pos.row][pos.col] != 'e') {
            maze[pos.row][pos.col] = '.';
        }
    }

    std::vector<Position> next_positions;

    // Cima
    if (is_valid_position(pos.row - 1, pos.col)) {
        next_positions.push_back({pos.row - 1, pos.col});
    }
    // Baixo
    if (is_valid_position(pos.row + 1, pos.col)) {
        next_positions.push_back({pos.row + 1, pos.col});
    }
    // Esquerda
    if (is_valid_position(pos.row, pos.col - 1)) {
        next_positions.push_back({pos.row, pos.col - 1});
    }
    // Direita
    if (is_valid_position(pos.row, pos.col + 1)) {
        next_positions.push_back({pos.row, pos.col + 1});
    }

    std::vector<std::thread> threads;

    for (size_t i = 1; i < next_positions.size(); ++i) {
        threads.emplace_back(walk, next_positions[i]);
    }

    bool found = false;
    if (!next_positions.empty()) {
        found = walk(next_positions[0]);
    }

    for (auto& t : threads) {
        if (t.joinable()) {
            t.join();
        }
    }

    return found;
}
int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Uso: " << argv[0] << " <arquivo_do_labirinto>" << std::endl;
        return 1;
    }

    Position initial_pos = load_maze(argv[1]);
    if (initial_pos.row == -1 || initial_pos.col == -1) {
        std::cerr << "Posição inicial não encontrada ou erro ao carregar o labirinto." << std::endl;
        return 1;
    }

    // Inicia a exploração em uma thread separada
    std::thread main_thread(walk, initial_pos);
    main_thread.join();

    // Usa a variável global exit_found
    if (exit_found) {
        std::cout << "Saída encontrada!" << std::endl;
    } else {
        std::cout << "Não foi possível encontrar a saída." << std::endl;
    }

    return 0;
}


// Nota sobre o uso de std::this_thread::sleep_for:
// 
// A função std::this_thread::sleep_for é parte da biblioteca <thread> do C++11 e posteriores.
// Ela permite que você pause a execução do thread atual por um período especificado.
// 
// Para usar std::this_thread::sleep_for, você precisa:
// 1. Incluir as bibliotecas <thread> e <chrono>
// 2. Usar o namespace std::chrono para as unidades de tempo
// 
// Exemplo de uso:
// std::this_thread::sleep_for(std::chrono::milliseconds(50));
// 
// Isso pausará a execução por 50 milissegundos.
// 
// Você pode ajustar o tempo de pausa conforme necessário para uma melhor visualização
// do processo de exploração do labirinto.
