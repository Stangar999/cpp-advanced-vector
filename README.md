# cpp-advanced-vector
## Вектор

## Цель проекта
Обучение.
* Forwarding reference
* размещающий оператор new
* void* operator new(std::size_t size)
* void operator delete(void* ptr) noexcept
* функции std::uninitialized_*
* variadic templates
* выражение свёртки или fold expression
* методы вектора

## Описание проекта
Вектор, который при изменении capacity выделяет сырую память, а при инициализации элемента в сырой памяти использует размещающий оператор new 

## Cистемные требования
- С++17;

## Инструкция по развертыванию проекта
Cобрать, запустить и спользовать подобно std::vector

