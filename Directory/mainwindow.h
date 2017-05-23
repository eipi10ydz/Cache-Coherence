#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QString>
#include <QTableWidget>
#include <QComboBox>

#include <vector>
#include <cstdint>

const int8_t MODIFIED = 0;
const int8_t SHARED = 1;
const int8_t INVALID = 2;
const int8_t CPU_NUM = 4;
const int8_t CACHE_SIZE = 4;
const int8_t MEMORY_UNIT_SIZE = 8;
const int8_t MEMORY_SIZE = 32;
const QString GREEN = "#00ff7f";
const QString YELLOW = "#ffff80";
const std::vector<QString> color = {"#fa8072", "#80ffff", "#a9a9a9"};
const std::vector<int8_t> ASSOCIATE = {1, 2, 4};
const std::vector<int8_t> CPU_TO_CHAR = {'A', 'B', 'C', 'D'};

struct cache_line
{
    int8_t state;
    int8_t addr;
    bool is_read;
};

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    void read(int8_t cpu, int8_t addr);
    void write(int8_t cpu, int8_t addr);
    void init();
    void reset();
    void display();
    void display_memory();
    void update_color(QTableWidget *, QTableWidgetItem *, QTimer *, int8_t &);
    void process(int8_t cpu);
    void read_memory(int8_t cpu, int8_t addr);
    void write_memory(int8_t cpu, int8_t addr);

private slots:
    void on_cpu1_exec_clicked();

    void on_cpu2_exec_clicked();

    void on_cpu3_exec_clicked();

    void on_cpu4_exec_clicked();

    void on_reset_clicked();

private:
    std::vector<std::vector<std::vector<cache_line>>> caches;
    std::vector<cache_line*> modify_state;
    std::vector<int8_t> modify_state_cpu;
    std::vector<QTableWidget*> cache_display;
    std::vector<QLineEdit*> addr_display;
    std::vector<QComboBox*> read_write_display;
    std::vector<QTableWidget*> memory_display;
    std::vector<std::vector<int8_t>> memory_cpus;
    int8_t associate;
    int8_t group_size;
    bool processing;
    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
