# HW5: Capacitated Facility Location

## Что делает обертка

- `run.sh` компилирует `solution.cpp` в `a.out` и запускает `checker.py`.
- `checker.py` прогоняет все тесты из `config.json`, валидирует решение и считает баллы.
- `config.json` содержит список тестов и пороги на 3/5 баллов.


## Интерфейс запуска через run.sh

```bash
./run.sh [solution|genetics|aboba]
```

**Аргументы:**
- `solution` — компилирует и запускает `solution.cpp`
- `genetics` — компилирует и запускает `genetics.cpp`
- `aboba` — компилирует и запускает `aboba.cpp`

**Пример:**
```bash
./run.sh genetics
```

**Что делает скрипт:**
1. Компилирует выбранный cpp-файл (`solution.cpp`, `genetics.cpp` или `aboba.cpp`).
2. Запускает `checker.py` для проверки решения на всех тестах из `data/`.
3. Печатает summary-таблицу с колонками:
   - objective
   - points за тест
   - next threshold
   - gap до следующего порога

## Формат вывода решения, который проверяет checker

Три логические части:
1. Первая строка: значение objective.
2. Вторая строка: список выбранных facilities.
3. Третья и далее: назначения клиентов.

Индексы разрешены как `0..n-1`, так и `1..n` (checker нормализует оба варианта).
