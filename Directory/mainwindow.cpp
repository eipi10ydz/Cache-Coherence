#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "log.h"
#include <QTimer>
#include <iostream>
#include <QDebug>
#include <functional>

using std::cout;
using std::endl;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    cache_display.reserve(CPU_NUM);
    cache_display.push_back(ui->cpu1_cache);
    cache_display.push_back(ui->cpu2_cache);
    cache_display.push_back(ui->cpu3_cache);
    cache_display.push_back(ui->cpu4_cache);

    addr_display.reserve(CPU_NUM);
    addr_display.push_back(ui->cpu1_addr);
    addr_display.push_back(ui->cpu2_addr);
    addr_display.push_back(ui->cpu3_addr);
    addr_display.push_back(ui->cpu4_addr);

    read_write_display.reserve(CPU_NUM);
    read_write_display.push_back(ui->cpu1_read);
    read_write_display.push_back(ui->cpu2_read);
    read_write_display.push_back(ui->cpu3_read);
    read_write_display.push_back(ui->cpu4_read);

    memory_display.reserve(CPU_NUM);
    memory_display.push_back(ui->memory1);
    memory_display.push_back(ui->memory2);
    memory_display.push_back(ui->memory3);
    memory_display.push_back(ui->memory4);

    setWindowTitle("多Cache一致性模拟器--目录法");
    reset();
    display();
    display_memory();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::init()
{
    if (processing)
        return;
    processing = true;
    modify_state = std::vector<cache_line*>(MEMORY_SIZE, nullptr);
    modify_state_cpu = std::vector<int8_t>(MEMORY_SIZE, -1);
    associate = ASSOCIATE[ui->associate->currentIndex()];
    group_size = CACHE_SIZE / associate;
    cache_line tmp;
    tmp.addr = -1;
    tmp.state = INVALID;
    caches = std::vector<std::vector<std::vector<cache_line>>>(CPU_NUM, std::vector<std::vector<cache_line>>(group_size, std::vector<cache_line>(associate, tmp)));
    memory_cpus = std::vector<std::vector<int8_t>>(MEMORY_SIZE, std::vector<int8_t>());
    ui->associate->setEnabled(false);
}

void MainWindow::reset()
{
    processing = false;
    init();
    display();
    display_memory();
    ui->associate->setEnabled(true);
    processing = false;
}

void MainWindow::read(int8_t cpu, int8_t addr)
{
    int8_t group_num = addr % group_size;
    int8_t to_be_swap = -1;
    cache_line *swap_line;
    for (int8_t i = 0; i < caches[cpu][group_num].size(); ++i)
    {
        if (caches[cpu][group_num][i].addr == -1 || caches[cpu][group_num][i].addr == addr)
        {
            to_be_swap = i;
            break;
        }
    }
    for (int8_t i = to_be_swap; i < caches[cpu][group_num].size(); ++i)
    {
        if (caches[cpu][group_num][i].addr == addr)
        {
            to_be_swap = i;
            break;
        }
    }
    if (to_be_swap == -1)
    {
        auto tmp = caches[cpu][group_num][0];
        for (int8_t i = 1; i < caches[cpu][group_num].size(); ++i)
        {
            caches[cpu][group_num][i-1] = caches[cpu][group_num][i];
        }
        caches[cpu][group_num][caches[cpu][group_num].size() - 1] = tmp;
        swap_line = &caches[cpu][group_num][caches[cpu][group_num].size() - 1];
    }
    else// if (caches[cpu][group_num][to_be_swap].addr == -1)
    {
        swap_line = &caches[cpu][group_num][to_be_swap];
    }
    switch (swap_line->state)
    {
        case INVALID:
        {
            if (modify_state[addr])
            {
                write_memory(modify_state_cpu[addr], addr);
                modify_state[addr]->state = SHARED;
                modify_state[addr] = nullptr;
                modify_state_cpu[addr] = -1;

                QTimer *timer = new QTimer();
                timer->setInterval(3000);
                timer->setSingleShot(true);
                connect(timer, &QTimer::timeout, this, std::bind(&MainWindow::read_memory, this, cpu, addr));
                timer->start();

            }
            else
                read_memory(cpu, addr);
            swap_line->state = SHARED;
            swap_line->addr = addr;
            swap_line->is_read = true;
            memory_cpus[addr].push_back(cpu);
            break;
        }
        case SHARED:
        {
            if (swap_line->addr != addr)
            {
                read_memory(cpu, addr);
                memory_cpus[swap_line->addr].clear();
                swap_line->addr = addr;
                swap_line->is_read = true;
            }
            memory_cpus[addr].push_back(cpu);
            break;
        }
        case MODIFIED:
        {
            if (swap_line->addr != addr)
            {
                write_memory(modify_state_cpu[swap_line->addr], swap_line->addr);
                // read_memory(cpu, addr);
                // if (modify_state[swap_line->addr])
                swap_line->state = SHARED;
                modify_state[swap_line->addr] = nullptr;
                modify_state_cpu[swap_line->addr] = -1;
                memory_cpus[swap_line->addr].clear();
                swap_line->addr = addr;
                swap_line->is_read = true;

                QTimer *timer = new QTimer();
                timer->setInterval(3000);
                timer->setSingleShot(true);
                connect(timer, &QTimer::timeout, this, std::bind(&MainWindow::read_memory, this, cpu, addr));
                timer->start();

            }
            memory_cpus[addr].clear();
            memory_cpus[addr].push_back(cpu);
            break;
        }
    }
}

void MainWindow::write(int8_t cpu, int8_t addr)
{
    int8_t group_num = addr % group_size;
    int8_t to_be_swap = -1;
    cache_line *swap_line;
    for (int i = 0; i < caches[cpu][group_num].size(); ++i)
    {
        if (caches[cpu][group_num][i].addr == -1 || caches[cpu][group_num][i].addr == addr)
        {
            to_be_swap = i;
            break;
        }
    }
    for (int8_t i = to_be_swap; i < caches[cpu][group_num].size(); ++i)
    {
        if (caches[cpu][group_num][i].addr == addr)
        {
            to_be_swap = i;
            break;
        }
    }
    if (to_be_swap == -1)
    {
        auto tmp = caches[cpu][group_num][0];
        for (int8_t i = 1; i < caches[cpu][group_num].size(); ++i)
        {
            caches[cpu][group_num][i-1] = caches[cpu][group_num][i];
        }
        caches[cpu][group_num][caches[cpu][group_num].size() - 1] = tmp;
        swap_line = &caches[cpu][group_num][caches[cpu][group_num].size() - 1];
    }
    else// if (caches[cpu][group_num][to_be_swap].addr == -1)
    {
        swap_line = &caches[cpu][group_num][to_be_swap];
    }
    memory_cpus[swap_line->addr].clear();
    swap_line->is_read = false;
    memory_cpus[addr].clear();
    switch (swap_line->state)
    {
        case INVALID:
        {
            if (modify_state[addr])
            {
                write_memory(modify_state_cpu[addr], addr);
                modify_state[addr]->state = INVALID;
                modify_state[addr]->addr = -1;
                modify_state_cpu[addr] = -1;

                QTimer *timer = new QTimer();
                timer->setInterval(3000);
                timer->setSingleShot(true);
                connect(timer, &QTimer::timeout, this, std::bind(&MainWindow::read_memory, this, cpu, addr));
                timer->start();
            }
            else
                read_memory(cpu, addr);
            // break;
        }
        case SHARED:
        {
            for (int8_t i = 0; i < CPU_NUM; ++i)
            {
                if (cpu == i)
                    continue;
                for (int8_t j = 0; j < group_size; ++j)
                {
                    for (int8_t k = 0; k < associate; ++k)
                    {
                        if (caches[i][j][k].addr == addr && caches[i][j][k].state == SHARED)
                        {
                            caches[i][j][k].addr = -1;
                            caches[i][j][k].state = INVALID;
                        }
                    }
                }
            }
            // write_memory(modify_state_cpu[addr], addr);
            modify_state[addr] = swap_line;
            modify_state_cpu[addr] = cpu;
            memory_cpus[addr].push_back(cpu);
            // display_memory();
            swap_line->addr = addr;
            break;
        }
        case MODIFIED:
        {
            if (swap_line->addr != addr && modify_state[addr])
            {
                write_memory(modify_state_cpu[swap_line->addr], swap_line->addr);

                QTimer *timer = new QTimer();
                timer->setInterval(3000);
                timer->setSingleShot(true);
                connect(timer, &QTimer::timeout, this, std::bind(&MainWindow::write_memory, this, modify_state_cpu[addr], addr));
                timer->start();

                timer = new QTimer();
                timer->setInterval(6000);
                timer->setSingleShot(true);
                connect(timer, &QTimer::timeout, this, std::bind(&MainWindow::read_memory, this, cpu, addr));
                timer->start();

                modify_state[addr]->state = INVALID;
                modify_state[addr]->addr = -1;
                modify_state[addr] = swap_line;
                modify_state_cpu[addr] = cpu;
                swap_line->addr = addr;
                swap_line->is_read = true;
            }
            else if (swap_line->addr != addr)
            {
                write_memory(modify_state_cpu[swap_line->addr], swap_line->addr);
                modify_state[addr] = swap_line;
                modify_state_cpu[addr] = cpu;
                memory_cpus[addr].clear();
                swap_line->addr = addr;
                swap_line->is_read = false;

                QTimer *timer = new QTimer();
                timer->setInterval(3000);
                timer->setSingleShot(true);
                connect(timer, &QTimer::timeout, this, std::bind(&MainWindow::read_memory, this, cpu, addr));
                timer->start();
            }
            memory_cpus[addr].push_back(cpu);
            // write_memory(modify_state_cpu[addr], addr);
            swap_line->addr = addr;
            break;
        }
    }
    swap_line->state = MODIFIED;
}

void MainWindow::process(int8_t cpu)
{
    init();
    int8_t addr = addr_display[cpu]->text().toInt();
    if (addr < 0 || addr >31)
    {
        LOG_ERROR("invalid address.");
        return;
    }
    int8_t read_write = read_write_display[cpu]->currentIndex();
    if (read_write)
    {
        write(cpu, addr);
    }
    else
    {
        read(cpu, addr);
    }
}

void MainWindow::on_cpu1_exec_clicked()
{
    process(0);
    display();
}

void MainWindow::on_cpu2_exec_clicked()
{
    process(1);
    display();
}

void MainWindow::on_cpu3_exec_clicked()
{
    process(2);
    display();
}

void MainWindow::on_cpu4_exec_clicked()
{
    process(3);
    display();
}

void MainWindow::on_reset_clicked()
{
    reset();
    display();
}

void MainWindow::update_color(QTableWidget *table, QTableWidgetItem *item, QTimer *timer, int8_t &cnt)
{
    if (cnt-- < 0)
    {
        timer->stop();
        table->clearContents();
        display_memory();
        return;
    }
    if (cnt % 2 == 0)
        item->setBackgroundColor("#ffffff");
    else
        item->setBackgroundColor("#ff0000");
}

void MainWindow::read_memory(int8_t cpu, int8_t addr)
{
    memory_display[addr / MEMORY_UNIT_SIZE]->clearContents();
    QString tmp = "cpu" + QString::number(cpu + 1) + " ";
    memory_display[addr / MEMORY_UNIT_SIZE]->setItem(addr % MEMORY_UNIT_SIZE, 1, new QTableWidgetItem(tmp + "读"));
    memory_display[addr / MEMORY_UNIT_SIZE]->setItem(addr % MEMORY_UNIT_SIZE, 0, new QTableWidgetItem(""));
    int8_t cnt = 5;
    QTimer *timer = new QTimer();
    timer->setInterval(300);
    connect(timer, &QTimer::timeout, this, std::bind(&MainWindow::update_color, this, memory_display[addr / MEMORY_UNIT_SIZE], memory_display[addr / MEMORY_UNIT_SIZE]->item(addr % MEMORY_UNIT_SIZE, 0), timer, cnt));
    timer->start();
}

void MainWindow::write_memory(int8_t cpu, int8_t addr)
{
    memory_display[addr / MEMORY_UNIT_SIZE]->clearContents();
    QString tmp = "cpu" + QString::number(cpu + 1) + " ";
    memory_display[addr / MEMORY_UNIT_SIZE]->setItem(addr % MEMORY_UNIT_SIZE, 1, new QTableWidgetItem(tmp + "写"));
    memory_display[addr / MEMORY_UNIT_SIZE]->setItem(addr % MEMORY_UNIT_SIZE, 0, new QTableWidgetItem(""));
    int8_t cnt = 5;
    QTimer *timer = new QTimer();
    timer->setInterval(300);
    connect(timer, &QTimer::timeout, this, std::bind(&MainWindow::update_color, this, memory_display[addr / MEMORY_UNIT_SIZE], memory_display[addr / MEMORY_UNIT_SIZE]->item(addr % MEMORY_UNIT_SIZE, 0), timer, cnt));
    timer->start();
}

void MainWindow::display()
{
    for (int8_t i = 0; i < CPU_NUM; ++i)
    {
        cache_display[i]->clearContents();
        for (int8_t j = 0; j < group_size; ++j)
        {
            for (int8_t k = 0; k < associate; ++k)
            {
                int8_t row_num = j * associate + k;
                if (caches[i][j][k].state != INVALID)
                    cache_display[i]->setItem(row_num, 0, new QTableWidgetItem(caches[i][j][k].is_read ? "读" : "写"));
                cache_display[i]->setItem(row_num, 1, new QTableWidgetItem(caches[i][j][k].addr == -1 ? "" : QString::number(caches[i][j][k].addr)));
                cache_display[i]->item(row_num, 1)->setBackgroundColor(color[caches[i][j][k].state]);
            }
        }
    }
    // display_memory();
}

void MainWindow::display_memory()
{
    for (int8_t i = 0; i < CPU_NUM; ++i)
    {
        for (int8_t j = 0; j < MEMORY_UNIT_SIZE; ++j)
        {
            QString tmp = "";
            std::sort(memory_cpus[i * MEMORY_UNIT_SIZE + j].begin(), memory_cpus[i * MEMORY_UNIT_SIZE + j].end());
            for (int8_t k = 0; k < memory_cpus[i * MEMORY_UNIT_SIZE + j].size(); ++k)
            {
                tmp.append(CPU_TO_CHAR[memory_cpus[i * MEMORY_UNIT_SIZE + j][k]]);
            }
            // qDebug() << i * MEMORY_UNIT_SIZE + j << " " << tmp << endl;
            memory_display[i]->setItem(j, 0, new QTableWidgetItem(""));
            memory_display[i]->setItem(j, 1, new QTableWidgetItem(tmp));
            memory_display[i]->item(j, 0)->setBackgroundColor(GREEN);
            memory_display[i]->item(j, 1)->setBackgroundColor(YELLOW);
        }
    }
}
