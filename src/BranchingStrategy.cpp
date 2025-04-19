#include "BranchingStrategy.h"
#include <algorithm>
#include <vector>
#include <limits>

// Реализация стратегии ветвления по столбцам
int ColumnBranchingStrategy::chooseBranchingIndex(BoolInterval** cnf, int cnfSize, BBV& mask) const {
    std::vector<int> indexes;
    std::vector<int> values;
    bool rezInit = false;

    // Получаем все незамаскированные индексы
    for (int i = 0; i < mask.getSize(); i++) {
        if (mask[i] == 0) {
            indexes.push_back(i);
        }
    }

    // Если нет доступных столбцов, возвращаем -1
    if (indexes.empty()) {
        return -1;
    }

    // Подсчитываем количество "-" в каждом столбце
    for (int i = 0; i < cnfSize; i++) {
        BoolInterval* interval = cnf[i];

        if (interval != nullptr) {
            if (!rezInit) {
                for (size_t k = 0; k < indexes.size(); k++) {
                    if (interval->getValue(indexes.at(k)) == '-') {
                        values.push_back(1);
                    } else {
                        values.push_back(0);
                    }
                }

                rezInit = true;
            } else {
                for (size_t k = 0; k < indexes.size(); k++) {
                    if (interval->getValue(indexes.at(k)) == '-') {
                        values.at(k)++;
                    }
                }
            }
        }
    }

    // Выбираем столбец с минимальным числом "-"
    int minElementIndex = std::min_element(values.begin(), values.end()) - values.begin();
    return indexes.at(minElementIndex);
}

// Реализация стратегии ветвления по строкам
int RowBranchingStrategy::chooseBranchingIndex(BoolInterval** cnf, int cnfSize, BBV& mask) const {
    std::vector<int> nonEmptyRows;
    std::vector<int> rowWeights;

    // Собираем непустые строки (интервалы)
    for (int i = 0; i < cnfSize; i++) {
        if (cnf[i] != nullptr) {
            nonEmptyRows.push_back(i);
            
            // Вычисляем вес строки (количество незамаскированных переменных)
            int weight = 0;
            for (int j = 0; j < mask.getSize(); j++) {
                if (mask[j] == 0 && cnf[i]->getValue(j) != '-') {
                    weight++;
                }
            }
            rowWeights.push_back(weight);
        }
    }
    
    // Если нет строк, возвращаем -1 (ошибка)
    if (nonEmptyRows.empty()) {
        return -1;
    }
    
    // Выбираем строку с минимальным весом (но не нулевым)
    int minIndex = 0;
    int minWeight = std::numeric_limits<int>::max();
    
    for (size_t i = 0; i < rowWeights.size(); i++) {
        if (rowWeights[i] > 0 && rowWeights[i] < minWeight) {
            minWeight = rowWeights[i];
            minIndex = i;
        }
    }
    
    // Для выбранной строки ищем индекс переменной (столбец) для ветвления
    int rowIndex = nonEmptyRows[minIndex];
    
    // Выбираем первый незамаскированный столбец в этой строке
    for (int j = 0; j < mask.getSize(); j++) {
        if (mask[j] == 0 && cnf[rowIndex]->getValue(j) != '-') {
            return j;
        }
    }
    
    // Если не нашли подходящий столбец, делегируем выбор стратегии по столбцам
    return ColumnBranchingStrategy().chooseBranchingIndex(cnf, cnfSize, mask);
}