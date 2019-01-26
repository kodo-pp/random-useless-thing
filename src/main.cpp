#include <iostream>
#include <iomanip>
#include <vector>
#include <stdexcept>
#include <utility>
#include <random>
#include <chrono>

#include <unistd.h>
#include <signal.h>


const int FPS_LIMIT = 60;       // Maximum frames per seconds
const int WIDTH = 100;          // Field width (in terminal columns)
const int HEIGHT = 30;          // Field height (in terminal lines)
const double EAT_RATE = 0.1;    // Probability of eating a cell. Must be in range [0.0, 1.0].
const double SPAWN_RATE = 0.5;  // Probability of spawning a random cell. Must be in range [0.0, 1.0].


enum class Cell
{
    dead,
    food,
    plant,
    virus,
    water,
    fungus
};


class RandomHelper
{
public:
    RandomHelper(): random_engine(std::random_device()())
    { }

    double random_double(double min = 0.0, double max = 1.0)
    {
        std::uniform_real_distribution<double> dist(min, max);
        return dist(random_engine);
    }

    bool chance(double probability)
    {
        return random_double(0.0, 1.0) < probability;
    }

    int random_int(int min, int max)
    {
        std::uniform_int_distribution<int> dist(min, max);
        return dist(random_engine);
    }

private:
    std::default_random_engine random_engine;
};


Cell get_cell_combination(Cell eater, Cell eaten)
{
    if (eater == Cell::plant && eaten == Cell::food) {
        return Cell::food;
    } else if (eater == Cell::virus && eaten == Cell::food) {
        return Cell::virus;
    } else if (eater == Cell::virus && eaten == Cell::plant) {
        return Cell::virus;
    } else if (eater == Cell::water && eaten == Cell::virus) {
        return Cell::water;
    } else if (eater == Cell::plant && eaten == Cell::water) {
        return Cell::plant;
    } else if (eater == Cell::water && eaten == Cell::food) {
        return Cell::fungus;
    } else if (eater == Cell::fungus && eaten == Cell::fungus) {
        return Cell::fungus;
    } else if (eater == Cell::plant && eaten == Cell::fungus) {
        return Cell::plant;
    } else if (eater == Cell::fungus && eaten == Cell::plant) {
        return Cell::food; /*
    } else if (eater == Cell::fungus && eaten == Cell::water) {
        return Cell::fungus;
    } else if (eater == Cell::virus && eaten == Cell::fungus) {
        return Cell::food;*/
    } else {
        return eaten;
    }
}


Cell random_cell()
{
    static RandomHelper random_helper;
    int option = random_helper.random_int(1, 5);
    switch (option) {
    case 1:
        return Cell::plant;
    case 2:
        return Cell::virus;
    case 3:
        return Cell::food;
    case 4:
        return Cell::water;
    case 5:
        return Cell::fungus;
    default:
        throw std::logic_error("Internal logic error: random int is not in range [1, 5]");
    }
}


class Field
{
public:
    Field(int height, int width): cells(height, std::vector<Cell>(width, Cell::dead)), random_helper()
    { }

    std::vector<Cell>& operator[](int row)
    {
        return cells[row];
    }

    std::vector<Cell>& at(int row)
    {
        return cells.at(row);
    }

    void process_cell(int row, int col)
    {
        const std::vector<std::pair<int, int>> offsets = {{-1, 0}, {0, -1}, {1, 0}, {0, 1}};
        for (const auto [offset_row, offset_col] : offsets) {
            try {
                cells.at(row + offset_row).at(col + offset_col);
            } catch(const std::out_of_range&) {
                continue;
            }

            if (random_helper.chance(EAT_RATE)) {
                cells[row + offset_row][col + offset_col] = get_cell_combination(
                    cells[row][col],
                    cells[row + offset_row][col + offset_col]
                );
            }
        }
    }
    
    void process_all_cells()
    {
        for (int row = 0; row < static_cast<int>(cells.size()); ++row) {
            for (int col = 0; col < static_cast<int>(cells[row].size()); ++col) {
                process_cell(row, col);
            }
        }
        if (random_helper.chance(SPAWN_RATE)) {
            spawn_random_cell();
        }
    }

    void spawn_random_cell()
    {
        const int height = static_cast<int>(cells.size());
        const int width = static_cast<int>(cells[0].size());
        const int row = random_helper.random_int(0, height - 1);
        const int col = random_helper.random_int(0, width - 1);
        cells[row][col] = random_cell();
    }

    auto begin()
    {
        return cells.begin();
    }

    auto end()
    {
        return cells.end();
    }

    auto begin() const
    {
        return cells.begin();
    }

    auto end() const
    {
        return cells.end();
    }

private:
    std::vector<std::vector<Cell>> cells;
    RandomHelper random_helper;
};


double high_precision_time()
{
    auto now_time_point = std::chrono::high_resolution_clock::now();
    auto now_time_since_epoch = now_time_point.time_since_epoch();

    using std::chrono::duration;
    using std::chrono::duration_cast;

    return duration_cast<duration<double>>(now_time_since_epoch).count();
}


void draw_cell(std::ostream& out, Cell c)
{
    switch (c)
    {
    case Cell::dead:
        out << ' ';
        break;
    case Cell::food:
        out << ".";
        break;
    case Cell::plant:
        out << "\x1b[32m$\x1b[0m";
        break;
    case Cell::water:
        out << "\x1b[34m~\x1b[0m";
        break;
    case Cell::virus:
        out << "\x1b[31m*\x1b[0m";
        break;
    case Cell::fungus:
        out << "\x1b[35m%\x1b[0m";
        break;
    }
}

void draw_field(std::ostream& out, const Field& f)
{
    out << "\x1b[0;0H";
    for (const auto& row : f) {
        for (Cell cell : row) {
            draw_cell(out, cell);
        }
        out << '\n';
    }
    out.flush();
}


Field generate_random_field(int height, int width)
{
    Field f(height, width);
    for (auto& row : f) {
        for (auto& cell : row) {
            cell = random_cell();
        }
    }
    return f;
}


void prepare_output(std::ostream& out)
{
    out.sync_with_stdio(false);
    out << std::fixed << std::setprecision(1);
    out << "\x1b[3J\x1b[0;0H\x1b[2J";
    out.flush();
}

void cleanup_output(std::ostream& out)
{
    out << "\x1b[0m\x1b[3J\x1b[0;0H\x1b[2J";
    out.flush();
}


class Clock
{
public:
    Clock(int max_fps): max_fps(max_fps), frame_interval(1.0 / max_fps)
    { }

    void tick()
    {
        const auto now = high_precision_time();
        const auto time_to_sleep = frame_interval - (now - last_time);
        if (time_to_sleep > 0.0) {
            usleep(std::round(time_to_sleep * 1e+6));
        }
        last_time = high_precision_time();
    }

private:
    const int max_fps;
    const double frame_interval;
    double last_time = 0.0;
};


void signal_handler(int signal)
{
    switch (signal) {
    case SIGINT: case SIGTERM:
        cleanup_output(std::cout);
        std::exit(130);
    };
}


int main()
{
    prepare_output(std::cout);
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);


    auto time_before_simulation = high_precision_time();

    Clock clock(FPS_LIMIT);
    auto field = generate_random_field(HEIGHT, WIDTH);
    int epoch = 0;
    while (true) {
        clock.tick();
        field.process_all_cells();
        draw_field(std::cout, field);
        const double avg_fps = static_cast<double>(epoch) / (high_precision_time() - time_before_simulation);
        std::cout << "Epoch: " << epoch << ", average FPS: " << avg_fps << std::endl;
        ++epoch;
    }
}
