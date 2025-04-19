#ifndef BOOLEQUATION_H
#define BOOLEQUATION_H

#include "boolinterval.h"
#include "BranchingStrategy.h"
#include <memory>

class BoolEquation
{
public:
    BoolInterval **cnf;//множество интервалов
    BoolInterval *root;//Корень уравнения
    int cnfSize; // Размер КНФ
    int count; //количество дизъюнкций
    BBV mask; //маска для столбцов
    std::shared_ptr<BranchingStrategy> branchingStrategy; // Стратегия ветвления
    
    // Обновленный конструктор принимает стратегию
    BoolEquation(BoolInterval **cnf, BoolInterval *root, int cnfSize, int count, BBV mask, 
                std::shared_ptr<BranchingStrategy> strategy);
    BoolEquation(BoolEquation &equation);
    int CheckRules();
    bool Rule1Row1(BoolInterval *interval);
    bool Rule2RowNull(BoolInterval *interval);
    void Rule3ColNull(BBV vector);
    bool Rule4Col0(BBV vector);
    bool Rule5Col1(BBV vector);
    void Simplify(int ixCol, char value);
    
    // Метод для выбора индекса ветвления с использованием текущей стратегии
    int ChooseBranchingIndex();
    
    // Метод для изменения стратегии ветвления
    void SetBranchingStrategy(std::shared_ptr<BranchingStrategy> strategy);
};

#endif // BOOLEQUATION_H
