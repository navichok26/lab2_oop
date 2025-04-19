#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstring>
#include <stack>
#include <algorithm>
#include <chrono>
#include <memory>
#include "NodeBoolTree.h"
#include "boolinterval.h"
#include "boolequation.h"
#include "BBV.h"
#include <Allocator.h>


Allocator equationAllocator(sizeof(BoolEquation), 1000, NULL, "EquationAllocator");  // Для BoolEquation
Allocator nodeAllocator(sizeof(NodeBoolTree), 2000, NULL, "NodeAllocator");          // Для NodeBoolTree
Allocator intervalAllocator(sizeof(BoolInterval), 500, NULL, "IntervalAllocator");   // Для BoolInterval
Allocator bbvAllocator(sizeof(BBV), 1000, NULL, "BBVAllocator");                     // Для BBV

// Функция для удаления символов переноса строки
std::string trimNewlines(const std::string& str) {
    std::string result = str;
    result.erase(std::remove(result.begin(), result.end(), '\r'), result.end());
    result.erase(std::remove(result.begin(), result.end(), '\n'), result.end());
    return result;
}

// Функция для удаления лидирующих и замыкающих пробелов
std::string trim(const std::string& str) {
    auto start = str.begin();
    while (start != str.end() && std::isspace(*start)) {
        start++;
    }

    auto end = str.end();
    do {
        end--;
    } while (std::distance(start, end) > 0 && std::isspace(*end));

    return std::string(start, end + 1);
}

#ifdef USE_CUSTOM_ALLOCATOR
BoolEquation* allocateEquation(BoolInterval** cnf, BoolInterval* root, int cnfSize, int count, BBV mask, std::shared_ptr<BranchingStrategy> strategy) {
    void* memory = equationAllocator.Allocate(sizeof(BoolEquation));
    return new(memory) BoolEquation(cnf, root, cnfSize, count, mask, strategy);
}

BoolEquation* allocateEquationCopy(BoolEquation& equation) {
    void* memory = equationAllocator.Allocate(sizeof(BoolEquation));
    return new(memory) BoolEquation(equation);
}

NodeBoolTree* allocateNode(BoolEquation* eq) {
    void* memory = nodeAllocator.Allocate(sizeof(NodeBoolTree));
    return new(memory) NodeBoolTree(eq);
}

BoolInterval* allocateInterval(BBV& vec, BBV& dnc) {
    void* memory = intervalAllocator.Allocate(sizeof(BoolInterval));
    return new(memory) BoolInterval(vec, dnc);
}

BoolInterval* allocateInterval(const char* str) {
    void* memory = intervalAllocator.Allocate(sizeof(BoolInterval));
    return new(memory) BoolInterval(str);
}

BBV* allocateBBV(const char* str) {
    void* memory = bbvAllocator.Allocate(sizeof(BBV));
    return new(memory) BBV(str);
}

void deallocateNode(NodeBoolTree* node) {
    if (node) {
        node->~NodeBoolTree();
        nodeAllocator.Deallocate(node);
    }
}

void deallocateEquation(BoolEquation* eq) {
    if (eq) {
        eq->~BoolEquation();
        equationAllocator.Deallocate(eq);
    }
}

void deallocateInterval(BoolInterval* interval) {
    if (interval) {
        interval->~BoolInterval();
        intervalAllocator.Deallocate(interval);
    }
}

void deallocateBBV(BBV* bbv) {
    if (bbv) {
        bbv->~BBV();
        bbvAllocator.Deallocate(bbv);
    }
}
#endif

int main(int argc, char *argv[])
{
    std::vector<std::string> full_file_list;
    std::string filepath;
    
    // Настройка стратегии ветвления (по умолчанию - по столбцам)
    std::shared_ptr<BranchingStrategy> strategy = std::make_shared<ColumnBranchingStrategy>();
    
    // Разбор аргументов командной строки
    if (argc > 1) {
        filepath = argv[1];
        
        if (argc > 2) {
            std::string strategyArg = argv[2];
            if (strategyArg == "row") {
                strategy = std::make_shared<RowBranchingStrategy>();
                std::cout << "Используется стратегия ветвления по строкам\n";
            } else {
                strategy = std::make_shared<ColumnBranchingStrategy>();
                std::cout << "Используется стратегия ветвления по столбцам\n";
            }
        }
    } else {
        filepath = "../data/Sat_ex11_3.pla";
    }
    
    #ifdef USE_CUSTOM_ALLOCATOR
    std::cout << "Используются пользовательские аллокаторы для разных типов классов\n";
    #else
    std::cout << "Используется стандартный аллокатор\n";
    #endif
    
    std::ifstream file(filepath);

    auto start_time = std::chrono::high_resolution_clock::now();

    if (file.is_open()) {
        std::string line;
        while (std::getline(file, line)) {
            full_file_list.push_back(trimNewlines(line));
        }
        file.close();

        int cnfSize = full_file_list.size();
        BoolInterval **CNF = new BoolInterval*[cnfSize];
        int rangInterval = -1; // error

        if (cnfSize) {
            rangInterval = trim(full_file_list[0]).length();
        }

        for (int i = 0; i < cnfSize; i++) { // Заполняем массив
            std::string strv = trim(full_file_list[i]);
            #ifdef USE_CUSTOM_ALLOCATOR
            CNF[i] = allocateInterval(strv.c_str());
            #else
            CNF[i] = new BoolInterval(strv.c_str());
            #endif
        }

        std::string rootvec = "";
        std::string rootdnc = "";

        // Строим интервал в котором все компоненты принимают значение '-',
        // который представляет собой корень уравнения, пока пустой.
        // В процессе поиска корня, компоненты интервала будут заменены на конкретные значения.

        for (int i = 0; i < rangInterval; i++) {
            rootvec += "0";
            rootdnc += "1";
        }

        #ifdef USE_CUSTOM_ALLOCATOR
        BBV* vec_ptr = allocateBBV(rootvec.c_str());
        BBV* dnc_ptr = allocateBBV(rootdnc.c_str());
        BBV& vec = *vec_ptr;
        BBV& dnc = *dnc_ptr;
        
        // Создаем пустой корень уравнения
        BoolInterval *root = allocateInterval(vec, dnc);
        #else
        BBV vec(rootvec.c_str());
        BBV dnc(rootdnc.c_str());
        
        // Создаем пустой корень уравнения
        BoolInterval *root = new BoolInterval(vec, dnc);
        #endif

        // Создаем уравнение и начальный узел в зависимости от типа аллокации
        BoolEquation *boolequation;
        NodeBoolTree *startNode;
        
        #ifdef USE_CUSTOM_ALLOCATOR
        // Используем специализированные аллокаторы
        boolequation = allocateEquation(CNF, root, cnfSize, cnfSize, vec, strategy);
        startNode = allocateNode(boolequation);
        #else
        // Используем стандартный new
        boolequation = new BoolEquation(CNF, root, cnfSize, cnfSize, vec, strategy);
        startNode = new NodeBoolTree(boolequation);
        #endif

        // Алгоритм поиска корня. Работаем всегда с верхушкой стека.
        bool rootIsFinded = false;
        std::stack<NodeBoolTree *> BoolTree;
        BoolTree.push(startNode);

        do {
            NodeBoolTree *currentNode(BoolTree.top());

            if (currentNode->lt == nullptr && currentNode->rt == nullptr) { 
                // Если вернулись в обработанный узел
                BoolEquation *currentEquation = currentNode->eq;
                bool flag = true;

                // Цикл для упрощения по правилам.
                while (flag) {
                    int a = currentEquation->CheckRules(); // Проверка выполнения правил

                    switch (a) {
                        case 0: { // Корня нет.
                            BoolTree.pop();
                            flag = false;
                            break;
                        }

                        case 1: { // Правило выполнилось, корень найден или продолжаем упрощать.
                            if (currentEquation->count == 0 ||
                                    currentEquation->mask.getWeight() ==
                                    currentEquation->mask.getSize()) { // Если кончились строки или столбцы, корень найден.
                                flag = false;
                                rootIsFinded = true; // Полагаем, что корень найден, выполняем проверку корня

                                for (int i = 0; i < cnfSize; i++) {
                                    if (!CNF[i]->isEqualComponent(*currentEquation->root)) {
                                        rootIsFinded = false; // Корень не найден. Продолжаем искать дальше.
                                        BoolTree.pop();
                                        break;
                                    }
                                }
                            }
                            break;
                        }

                        case 2: { // Правила не выполнились, ветвление
                            // Теперь используем общий метод для выбора индекса ветвления
                            int indexBranching = currentEquation->ChooseBranchingIndex();

                            // Создаем новые уравнения и узлы в зависимости от типа аллокации
                            BoolEquation *Equation0, *Equation1;
                            NodeBoolTree *Node0, *Node1;
                            
                            #ifdef USE_CUSTOM_ALLOCATOR
                            Equation0 = allocateEquationCopy(*currentEquation);
                            Equation1 = allocateEquationCopy(*currentEquation);
                            
                            Equation0->Simplify(indexBranching, '0');
                            Equation1->Simplify(indexBranching, '1');
                            
                            Node0 = allocateNode(Equation0);
                            Node1 = allocateNode(Equation1);
                            #else
                            Equation0 = new BoolEquation(*currentEquation);
                            Equation1 = new BoolEquation(*currentEquation);
                            
                            Equation0->Simplify(indexBranching, '0');
                            Equation1->Simplify(indexBranching, '1');
                            
                            Node0 = new NodeBoolTree(Equation0);
                            Node1 = new NodeBoolTree(Equation1);
                            #endif

                            currentNode->lt = Node0;
                            currentNode->rt = Node1;

                            BoolTree.push(Node1);
                            BoolTree.push(Node0);

                            flag = false;
                            break;
                        }
                    }
                }
            } else {
                BoolTree.pop();
            }

        } while (BoolTree.size() > 1 && !rootIsFinded);

        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count();

        if (rootIsFinded) {
            std::cout << "Root is:\n ";
            BoolInterval *finded_root = BoolTree.top()->eq->root;
            std::cout << std::string(*finded_root) << std::endl;
        } else {
            std::cout << "Root is not exists!" << std::endl;
        }

        std::cout << "Время выполнения: " << duration << " мкс" << std::endl;

        #ifdef USE_CUSTOM_ALLOCATOR
        std::cout << "\n===== Статистика аллокаторов =====\n";
        std::cout << "Аллокатор для BoolEquation:\n";
        std::cout << "  Размер блока: " << equationAllocator.GetBlockSize() << " байт\n";
        std::cout << "  Всего блоков: " << equationAllocator.GetBlockCount() << "\n";
        std::cout << "  Блоков в использовании: " << equationAllocator.GetBlocksInUse() << "\n";
        std::cout << "  Аллокаций: " << equationAllocator.GetAllocations() << "\n";
        std::cout << "  Деаллокаций: " << equationAllocator.GetDeallocations() << "\n";
        
        std::cout << "\nАллокатор для NodeBoolTree:\n";
        std::cout << "  Размер блока: " << nodeAllocator.GetBlockSize() << " байт\n";
        std::cout << "  Всего блоков: " << nodeAllocator.GetBlockCount() << "\n";
        std::cout << "  Блоков в использовании: " << nodeAllocator.GetBlocksInUse() << "\n";
        std::cout << "  Аллокаций: " << nodeAllocator.GetAllocations() << "\n";
        std::cout << "  Деаллокаций: " << nodeAllocator.GetDeallocations() << "\n";
        #endif

        #ifdef USE_CUSTOM_ALLOCATOR
        while (!BoolTree.empty()) {
            NodeBoolTree* node = BoolTree.top();
            BoolTree.pop();
            
            if (node->eq) {
                deallocateEquation(node->eq);
            }
            
            deallocateNode(node);
        }
        
        for (int i = 0; i < cnfSize; i++) {
            deallocateInterval(CNF[i]);
        }
        
        deallocateInterval(root);
        deallocateBBV(vec_ptr);
        deallocateBBV(dnc_ptr);
        #else
        while (!BoolTree.empty()) {
            NodeBoolTree* node = BoolTree.top();
            BoolTree.pop();
            delete node->eq;
            delete node;
        }
        
        for (int i = 0; i < cnfSize; i++) {
            delete CNF[i];
        }
        delete root;
        #endif
        
        delete[] CNF;

    } else {
        std::cout << "File does not exists.\n";
    }

    return 0;
}