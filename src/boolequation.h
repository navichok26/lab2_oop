#ifndef BOOLEQUATION_H
#define BOOLEQUATION_H

#include "boolinterval.h"

// Перечисление для стратегий ветвления
enum BranchingStrategy {
    COLUMN_BRANCHING,    // Ветвление по столбцам (оригинальная стратегия)
    ROW_BRANCHING,       // Ветвление по строкам
};

class BoolEquation
{
public:
    BoolInterval **cnf;//множество интервалов
    BoolInterval *root;//Корень уравнения
    int cnfSize; // Размер КНФ
    int count; //количество дизъюнкций
    BBV mask; //маска для столбцов
    BranchingStrategy branchingStrategy; // Выбранная стратегия ветвления
    
    BoolEquation(BoolInterval **cnf, BoolInterval *root, int cnfSize, int count, BBV mask, BranchingStrategy strategy = COLUMN_BRANCHING);
    BoolEquation(BoolEquation &equation);
    int CheckRules();
    bool Rule1Row1(BoolInterval *interval);
    bool Rule2RowNull(BoolInterval *interval);
    void Rule3ColNull(BBV vector);
    bool Rule4Col0(BBV vector);
    bool Rule5Col1(BBV vector);
    void Simplify(int ixCol, char value);
    int ChooseColForBranching(); // Выбор столбца для ветвления
    int ChooseRowForBranching(); // Выбор строки для ветвления
    int ChooseBranchingIndex(); // Обобщенный метод выбора с учетом стратегии
    void SetBranchingStrategy(BranchingStrategy strategy); // Установка стратегии
};

#endif // BOOLEQUATION_H
