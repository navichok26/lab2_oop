#include "boolequation.h"
#include <vector>
#include <algorithm>
#include <ostream>
#include <string>

BoolEquation::BoolEquation(BoolInterval **cnf, BoolInterval *root, int cnfSize, int count, BBV mask,
                          std::shared_ptr<BranchingStrategy> strategy)
{
    this->cnf = new BoolInterval*[cnfSize];

    for (int i = 0; i < cnfSize; i++) {
        this->cnf[i] = cnf[i];
    }

    this->root = root;
    this->cnfSize = cnfSize;
    this->count = count;
    this->mask = mask;
    
    // Просто устанавливаем переданную стратегию
    this->branchingStrategy = strategy;
}

BoolEquation::BoolEquation(BoolEquation &equation)
{
    this->cnf = new BoolInterval*[equation.cnfSize];

    for (int i = 0; i < equation.cnfSize; i++) {
        this->cnf[i] = equation.cnf[i];
    }

    this->root = new BoolInterval(equation.root->vec, equation.root->dnc);
    this->cnfSize = equation.cnfSize;
    this->count = equation.count;
    this->mask = equation.mask;
    
    // Копируем стратегию в зависимости от её типа
    if (auto colStrategy = dynamic_cast<ColumnBranchingStrategy*>(equation.branchingStrategy.get())) {
        this->branchingStrategy = std::make_shared<ColumnBranchingStrategy>(*colStrategy);
    } else if (auto rowStrategy = dynamic_cast<RowBranchingStrategy*>(equation.branchingStrategy.get())) {
        this->branchingStrategy = std::make_shared<RowBranchingStrategy>(*rowStrategy);
    }
    // Здесь можно добавить поддержку других типов стратегий при необходимости
}

// Проверка правил
int BoolEquation::CheckRules()
{
	BBV rez, rez1, rez0;
	bool rezInit = false;

	for (int i = 0; i < cnfSize; i++) {
		BoolInterval *interval = cnf[i];

		//		std::cout << string(*interval) << " <---> " << i << endl;

		if (interval != nullptr) {
			if (Rule2RowNull(interval)) {
				return 0;
			}

			if (count == 1) {
				if (Rule4Col0(interval->vec ^ interval->dnc)) {
					return 1;
				}

				if (Rule5Col1(interval->vec)) {
					return 1;
				}
			}

			if (Rule1Row1(interval)) {
				for (int k = 0; k < interval->length(); k++) {
					if (mask[k] != 1) {
						char value = interval->getValue(k);

						if (value != '-') {
							if (value == '0') {
								//Simplify(k, '1');
								Simplify(k, '0');
								break;
							} else {
								//Simplify(k, '0');
								Simplify(k, '1');
								break;
							}
						}
					}
				}

				return 1;
			}

			if (!rezInit) {
				// cout << interval->vec;
				rez0 = interval->vec ^ interval->dnc;
				rez1 = interval->vec;
				rez  = interval->dnc;
				rezInit = true;
			} else {
				rez = rez & interval->dnc;

				//cout << rez0;
				BBV temprez = interval->vec ^ interval->dnc;
				// cout << temprez;
				rez0 = rez0 | temprez;
				// cout << rez0;

				// cout << rez1;
				rez1 = rez1 & interval->vec;
				// cout << rez1;
			}
		}
	}

	//       cout << "Vector dlya ---- " << rez << "\n";
	//       cout << "Vector dlya 1 " << rez1 << "\n";
	//       cout << "Vector dlya 0 " << rez0 << "\n";
	Rule3ColNull(rez);

	if (Rule4Col0(rez0)) {
		return 1;
	}

	if (Rule5Col1(rez1)) {
		return 1;
	}


	return 2;
}

// Строка пустая
bool BoolEquation::Rule2RowNull(BoolInterval *interval)
{
	int counter = 0;

	for (int i = 0; i < mask.getSize(); i++) {
		if (mask[i] != 1) {
			if (interval->getValue(i) != '-') {
				counter++;
				break;
			}
		}
	}

	if (counter > 0) {
		return false;
	}

	return true;
}
//Строка содержит одну переменную
bool BoolEquation::Rule1Row1(BoolInterval *interval)
{
	int counter = 0;

	for (int i = 0; i < mask.getSize(); i++) {
		if (mask[i] != 1) {
			if (interval->getValue(i) != '-') {
				counter++;
			}
		}
	}

	if (counter == 1) {
		return true;
	}

	return false;
}

void BoolEquation::Rule3ColNull(BBV vector)
{

	//cout << "Vector "<< vector;
	//cout << "Mask " << mask;
	for (int i = 0; i < vector.getSize(); i++) {
		if (vector[i] == 1 && mask[i] != 1) {
			mask.Set1(i);
		}
	}

}

bool BoolEquation::Rule4Col0(BBV vector)
{
	for (int i = 0; i < vector.getSize(); i++) {
		if (vector[i] == 0 && mask[i] != 1) {
			//			Simplify(i, '1');
			Simplify(i, '0');
			return true;
		}
	}

	return false;
}

bool BoolEquation::Rule5Col1(BBV vector)
{
	for (int i = 0; i < vector.getSize(); i++) {
		if (vector[i] == 1 && mask[i] != 1) {
			//			Simplify(i, '0');
			Simplify(i, '1');
			return true;
		}
	}

	return false;
}

void BoolEquation::Simplify(int ixCol, char value)
{
	for (int i = 0; i < cnfSize; i++) {
		BoolInterval *interval = cnf[i];

		if (interval != nullptr) {
			char val = interval->getValue(ixCol);

			//if (val != value && val != '-') {
			if (val == value && val != '-') {
				cnf[i] = nullptr;
				count--;
			}
		}
	}

	root->setValue(value, ixCol);
	mask.Set1(ixCol);
}

int BoolEquation::ChooseBranchingIndex()
{
    // Используем выбранную стратегию для определения индекса ветвления
    return branchingStrategy->chooseBranchingIndex(cnf, cnfSize, mask);
}

void BoolEquation::SetBranchingStrategy(std::shared_ptr<BranchingStrategy> strategy)
{
    this->branchingStrategy = strategy;
}
