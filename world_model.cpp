// world_model.cpp
// =====================================================================
// ДЕМОНСТРАЦИЯ WORLD MODEL НА БАЗЕ MOTOR SYNC
// 
// В этой симуляции:
// 1. Физика (Законы) - шарики летают и отскакивают от стен.
// 2. Внешний ввод (Сознание) - нажатие клавиш дает шарикам импульс.
// 3. Моторное измерение - синхронизирует всё это, гарантируя,
//    что законы физики не нарушаются, а ввод применяется строго 
//    между кадрами.
// =====================================================================

#ifdef _WIN32
#  ifndef NOMINMAX
#    define NOMINMAX
#  endif
#  include <windows.h>
#  include <conio.h> // Для _kbhit() и _getch() на Windows
#  pragma comment(lib, "winmm.lib")
#endif

#include "motor_sync.hpp"

#include <iostream>
#include <vector>
#include <cmath>
#include <iomanip>
#include <mutex>
#include <string>

// =====================================================================
// 1. ОПРЕДЕЛЕНИЕ МИРА (Поля / Данные)
// =====================================================================
const int WIDTH = 40;
const int HEIGHT = 15;

struct Ball {
    float x, y;
    float vx, vy;
    char symbol;
};

struct World {
    std::vector<Ball> balls;
    std::mutex mtx; // Защита данных при доступе из разных потоков
};

void init_world(World& world) {
    // Создаем 3 шарика с разными скоростями
    world.balls.push_back({ 5.0f, 5.0f,  0.5f,  0.3f, 'O' });
    world.balls.push_back({ 20.0f, 10.0f, -0.4f,  0.5f, '0' });
    world.balls.push_back({ 35.0f, 2.0f, -0.3f, -0.4f, '@' });
}

// =====================================================================
// 2. ФИЗИКА (Законы / Измерения-правила)
// =====================================================================
void physics_work(World& world) {
    // В фазе work мы читаем данные
    std::lock_guard<std::mutex> lock(world.mtx);
    for (auto& b : world.balls) {
        b.x += b.vx;
        b.y += b.vy;

        // Отскок от стен
        if (b.x <= 0 || b.x >= WIDTH - 1) { b.vx = -b.vx; b.x += b.vx; }
        if (b.y <= 0 || b.y >= HEIGHT - 1) { b.vy = -b.vy; b.y += b.vy; }
    }
}

void physics_commit(World& world) {
    // (Здесь нечего коммитить, физика уже обновила данные в work-фазе.
    // В более сложных системах здесь можно записывать результат в общую память)
}

// =====================================================================
// 3. ВНЕШНИЙ ВВОД (Сознание / Наблюдатель)
// =====================================================================
void input_work(World& world) {
    std::lock_guard<std::mutex> lock(world.mtx);

    // Проверяем, нажата ли клавиша (только для Windows)
#ifdef _WIN32
    if (_kbhit()) {
        char key = _getch();

        // Клавиша '1', '2', '3' дают пинок соответствующему шарику
        if (key >= '1' && key <= '3') {
            int idx = key - '1';
            if (idx < world.balls.size()) {
                auto& b = world.balls[idx];
                // Добавляем случайный импульс (пинок)
                b.vx += (rand() % 100 - 50) / 100.0f;
                b.vy += (rand() % 100 - 50) / 100.0f;
            }
        }
    }
#endif
}

void input_commit(World& world) {
    // (Аналогично physics_commit)
}

// =====================================================================
// 4. РЕНДЕРИНГ (Отображение текущего кадра)
// =====================================================================
void render_work(World& world) {
    std::lock_guard<std::mutex> lock(world.mtx);
    std::string frame = "";

    // Верхняя граница
    frame += "+" + std::string(WIDTH, '-') + "+\n";

    for (int y = 0; y < HEIGHT; ++y) {
        frame += "|";
        for (int x = 0; x < WIDTH; ++x) {
            char c = ' ';
            for (const auto& b : world.balls) {
                int bx = static_cast<int>(std::round(b.x));
                int by = static_cast<int>(std::round(b.y));
                if (bx == x && by == y) {
                    c = b.symbol;
                    break;
                }
            }
            frame += c;
        }
        frame += "|\n";
    }

    // Нижняя граница
    frame += "+" + std::string(WIDTH, '-') + "+\n";
    frame += "Нажмите 1, 2 или 3, чтобы дать шарику импульс. Q для выхода.\n";

    // Очищаем консоль и выводим кадр
#ifdef _WIN32
    system("cls"); // Очистка экрана (Windows)
#else
    system("clear"); // Очистка экрана (Linux/Mac)
#endif
    std::cout << frame;
    std::cout.flush();
}

void render_commit(World& world) {
    // (Вывод уже произошёл в work-фазе)
}

// =====================================================================
// 5. ГЛАВНЫЙ ЦИКЛ (Моторное измерение)
// =====================================================================
int main() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    timeBeginPeriod(1);
#endif

    World world;
    init_world(world);

    const int N_MEASUREMENTS = 3; // 1: Физика, 2: Ввод, 3: Рендер
    const std::uint64_t MAX_TICKS = 1000; // 1000 кадров (или пока не нажмут Q)

    // Частота обновления: 100 000 мкс = 10 кадров в секунду.
    // Делаем медленно, чтобы глазами было видно движение шариков в консоли.
    const std::chrono::microseconds PERIOD(100000);

    // Создаем мотор. sequential_commit = true гарантирует порядок:
    // сначала Физика (0), потом Ввод (1), потом Рендер (2).
    motor::Motor m(N_MEASUREMENTS, MAX_TICKS, PERIOD, true);

    // Добавляем 1-е измерение: Физика
    m.add(
        [&](std::uint64_t t, int id) { physics_work(world); },
        [&](std::uint64_t t, int id) { physics_commit(world); }
    );

    // Добавляем 2-е измерение: Внешний ввод (Сознание)
    m.add(
        [&](std::uint64_t t, int id) { input_work(world); },
        [&](std::uint64_t t, int id) { input_commit(world); }
    );

    // Добавляем 3-е измерение: Рендеринг
    m.add(
        [&](std::uint64_t t, int id) { render_work(world); },
        [&](std::uint64_t t, int id) { render_commit(world); }
    );

    // Запускаем мотор!
    m.run();

    std::cout << "Симуляция завершена.\n";

#ifdef _WIN32
    timeEndPeriod(1);
#endif
    return 0;
}