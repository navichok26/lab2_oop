#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstring>
#include <stack>
#include "NodeBoolTree.h"
#include "boolinterval.h"
#include "boolequation.h"
#include "BBV.h"
#include <Allocator.h>

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

int main(int argc, char *argv[])
{
    std::vector<std::string> full_file_list;
    std::string filepath;
    
    // Настройка стратегии ветвления (по умолчанию - по столбцам)
    BranchingStrategy strategy = COLUMN_BRANCHING;
    
    // Разбор аргументов командной строки
    if (argc > 1) {
        filepath = argv[1];
        
        if (argc > 2) {
            std::string strategyArg = argv[2];
            if (strategyArg == "row") {
                strategy = ROW_BRANCHING;
                std::cout << "Используется стратегия ветвления по строкам\n";
            } else {
                strategy = COLUMN_BRANCHING;
                std::cout << "Используется стратегия ветвления по столбцам\n";
            }
        }
    } else {
        // Hardcode input если нет аргументов
        filepath = "../data/Sat_ex11_3.pla";
    }
    
    std::ifstream file(filepath);

    // Считываем весь файл
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
            CNF[i] = new BoolInterval(strv.c_str());
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

        BBV vec(rootvec.c_str());
        BBV dnc(rootdnc.c_str());

        // Создаем пустой корень уравнения
        BoolInterval *root = new BoolInterval(vec, dnc);

        // Передаем стратегию в конструктор
        BoolEquation *boolequation = new BoolEquation(CNF, root, cnfSize, cnfSize, vec, strategy);

        // Алгоритм поиска корня. Работаем всегда с верхушкой стека.
        // Шаг 1. Правила выполняются? Нет - Ветвление Шаг 5. Да - Упрощаем Шаг 2.
        // Шаг 2. Строки закончились? Нет - Шаг1, Да - Корень найден? Да - Успех КОНЕЦ, Нет - Шаг 3.
        // Шаг 3. Кол-во узлов в стеке > 1? Нет - Корня нет КОНЕЦ, Да - Шаг 4.
        // Шаг 4. Текущий узел выталкиваем из стека, попадаем в новый узел. У нового узла lt rt отличны от NULL? Нет - Шаг 1. Да - Шаг 3.
        // Шаг 5. Выбор компоненты ветвления, создание двух новых узлов, добавление их в стек сначала с 1 потом с 0. Шаг 1.

        bool rootIsFinded = false;
        std::stack<NodeBoolTree *> BoolTree;
        NodeBoolTree *startNode = new NodeBoolTree(boolequation);
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

                            BoolEquation *Equation0 = new BoolEquation(*currentEquation);
                            BoolEquation *Equation1 = new BoolEquation(*currentEquation);

                            Equation0->Simplify(indexBranching, '0');
                            Equation1->Simplify(indexBranching, '1');

                            NodeBoolTree *Node0 = new NodeBoolTree(Equation0);
                            NodeBoolTree *Node1 = new NodeBoolTree(Equation1);

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

        if (rootIsFinded) {
            std::cout << "Root is:\n ";
            BoolInterval *finded_root = BoolTree.top()->eq->root;
            std::cout << std::string(*finded_root);
        } else {
            std::cout << "Root is not exists!";
        }

    } else {
        std::cout << "File does not exists.\n";
    }

    return 0;
}