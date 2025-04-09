#include <iostream>
#include <vector>
#include <string>
#include <cstring>
#include <algorithm>
#include <chrono>
#include <functional>
#include <random>
#include <Allocator.h>
#include <cassert>

struct TestBlock {
    int id;
    char data[1020];
};

Allocator stressTestAllocator(1024, 10, NULL, "StressTestAllocator");

static void out_of_memory()
{
    std::cout << "Hello world" << std::endl;
    assert(0);
}

void runAllocatorStressTest() {
    std::set_new_handler(out_of_memory);
    std::cout << "Запуск стресс-теста аллокатора...\n";
    
    const int NUM_BLOCKS = 1000000;
    const int BLOCK_SIZE = sizeof(TestBlock);
    const int MB_TO_ALLOCATE = NUM_BLOCKS * BLOCK_SIZE / (1024*1024);
    
    std::cout << "Попытка выделить ~" << MB_TO_ALLOCATE << " MB памяти через аллокатор\n";
    
    std::vector<TestBlock*> blocks;
    int successfulAllocations = 0;
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    try {
        for (int i = 0; i < NUM_BLOCKS; i++) {
            void* memory = stressTestAllocator.Allocate(BLOCK_SIZE);
            if (!memory) {
                std::cout << "Не удалось выделить блок #" << i << std::endl;
                break;
            }
            
            TestBlock* block = new(memory) TestBlock();
            block->id = i;
            
            for (int j = 0; j < 1020; j++) {
                block->data[j] = static_cast<char>(i % 256);
            }
            
            blocks.push_back(block);
            successfulAllocations++;
            
            if (i % 10000 == 0) {
                std::cout << "Выделено блоков: " << i << " (" 
                          << (i * BLOCK_SIZE / (1024*1024)) << " MB)\n";
            }
        }
    } catch (const std::exception& e) {
        std::cout << "Исключение при выделении памяти: " << e.what() << std::endl;
    }
    
    auto allocation_end_time = std::chrono::high_resolution_clock::now();
    auto allocation_duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        allocation_end_time - start_time).count();
    
    std::cout << "Успешно выделено блоков: " << successfulAllocations << " из " << NUM_BLOCKS << std::endl;
    std::cout << "Объем выделенной памяти: " << (successfulAllocations * BLOCK_SIZE / (1024*1024)) << " MB\n";
    std::cout << "Время выделения: " << allocation_duration << " мс\n";
    
    const int NUM_CHECKS = std::min(100, successfulAllocations);
    int errors = 0;
    
    for (int i = 0; i < NUM_CHECKS; i++) {
        int index = rand() % successfulAllocations;
        TestBlock* block = blocks[index];
        
        if (block->id != index) {
            std::cout << "Ошибка в ID блока #" << index << ": ожидалось " << index 
                        << ", получено " << block->id << std::endl;
            errors++;
            continue;
        }
        
        for (int j = 0; j < 10; j++) {  // Проверяем только первые 10 байт для скорости
            if (block->data[j] != static_cast<char>(index % 256)) {
                std::cout << "Ошибка в данных блока #" << index << " на позиции " << j << std::endl;
                errors++;
                break;
            }
        }
    }
    
    std::cout << "Проверено блоков: " << NUM_CHECKS << ", найдено ошибок: " << errors << std::endl;
    
    std::cout << "Освобождение памяти...\n";
    
    auto deallocation_start_time = std::chrono::high_resolution_clock::now();
    
    for (TestBlock* block : blocks) {
        block->~TestBlock();
        stressTestAllocator.Deallocate(block);
    }
    
    auto deallocation_end_time = std::chrono::high_resolution_clock::now();
    auto deallocation_duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        deallocation_end_time - deallocation_start_time).count();
    
    std::cout << "Время освобождения: " << deallocation_duration << " мс\n";
    
    std::cout << "\n===== Статистика аллокатора после стресс-теста =====\n";
    std::cout << "Размер блока: " << stressTestAllocator.GetBlockSize() << " байт\n";
    std::cout << "Всего блоков: " << stressTestAllocator.GetBlockCount() << "\n";
    std::cout << "Блоков в использовании: " << stressTestAllocator.GetBlocksInUse() << "\n";
    std::cout << "Аллокаций: " << stressTestAllocator.GetAllocations() << "\n";
    std::cout << "Деаллокаций: " << stressTestAllocator.GetDeallocations() << "\n";
}

int main(int argc, char *argv[])
{
    std::cout << "=== ПРОГРАММА СТРЕСС-ТЕСТИРОВАНИЯ АЛЛОКАТОРА ===\n\n";
    runAllocatorStressTest();
    return 0;
}