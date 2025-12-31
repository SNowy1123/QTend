/**
 * @file mainwindow.cpp
 * @brief 个人财务管理系统主逻辑实现
 * @details 包含数据库连接、CRUD操作、多线程统计与图表绘制逻辑
 * @author 郑伟业2023414290440
 * @date 2025-01-01
 * @version 1.0.0
 */


#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QDate>
#include <QtConcurrent>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow) {
    ui->setupUi(this);

    initDatabase();
    updateStatistics();

    // 快捷键：回车添加
    connect(ui->lineAmount, &QLineEdit::returnPressed, ui->btnAdd, &QPushButton::click);
}

void MainWindow::initDatabase() {
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName("finance.db");

    if (!db.open()) {
        QMessageBox::critical(this, "错误", "数据库无法打开！");
        return;
    }

    QSqlQuery query;
    query.exec("CREATE TABLE IF NOT EXISTS finance ("
               "id INTEGER PRIMARY KEY AUTOINCREMENT, "
               "date TEXT, category TEXT, amount REAL)");

    model = new QSqlTableModel(this);
    model->setTable("finance");
    model->setEditStrategy(QSqlTableModel::OnManualSubmit);
    model->select();

    ui->tableView->setModel(model);
    ui->tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->statusbar->showMessage("系统就绪", 3000);
}

// 添加功能
void MainWindow::on_btnAdd_clicked() {
    QString cat = ui->lineCategory->text().trimmed();
    double amt = ui->lineAmount->text().toDouble();

    if (cat.isEmpty() || amt <= 0) {
        QMessageBox::warning(this, "提示", "分类不能为空且金额需大于0");
        return;
    }

    ui->statusbar->showMessage("录入失败：数据格式错误", 3000);

    QSqlQuery query;
    query.prepare("INSERT INTO finance (date, category, amount) VALUES (:d, :c, :a)");
    query.bindValue(":d", QDate::currentDate().toString("yyyy-MM-dd"));
    query.bindValue(":c", cat);
    query.bindValue(":a", amt);

    if (query.exec()) {
        model->select();
        updateStatistics();
        ui->lineCategory->clear();
        ui->lineAmount->clear();
        ui->statusbar->showMessage("添加成功", 2000);
    }
}

// 删除功能
void MainWindow::on_btnDelete_clicked() {
    int row = ui->tableView->currentIndex().row();
    if (row < 0) {
        QMessageBox::warning(this, "提示", "请先选中要删除的行");
        return;
    }

    if (QMessageBox::question(this, "确认", "确定删除该记录？") == QMessageBox::Yes) {
        model->removeRow(row);
        if (model->submitAll()) {
            updateStatistics();
            ui->statusbar->showMessage("记录已删除", 2000);
        } else {
            model->revertAll();
        }
    }
}

// 筛选功能
void MainWindow::on_btnFilter_clicked() {
    QString keyword = ui->lineCategory->text().trimmed();
    if (keyword.isEmpty()) {
        model->setFilter("");
    } else {
        model->setFilter(QString("category LIKE '%%1%'").arg(keyword));
    }
    model->select();
}

// 重置功能
void MainWindow::on_btnReset_clicked() {
    ui->lineCategory->clear();
    model->setFilter("");
    model->select();
    ui->statusbar->showMessage("视图已重置", 2000);
}

// 报表功能
void MainWindow::on_btnReport_clicked() {
    QString month = QDate::currentDate().toString("yyyy-MM");
    QSqlQuery q;
    q.prepare("SELECT SUM(amount) FROM finance WHERE date LIKE :m");
    q.bindValue(":m", month + "%");

    if (q.exec() && q.next()) {
        double total = q.value(0).toDouble();
        QMessageBox::information(this, "月度报表",
                                 QString("月份: %1\n总支出: %2 元").arg(month).arg(total));
    }
}

// 多线程统计
void MainWindow::updateStatistics() {
    QtConcurrent::run([this]() {
        QSqlDatabase subDb;
        const QString connName = "sub_thread_conn";

        if (QSqlDatabase::contains(connName)) {
            subDb = QSqlDatabase::database(connName);
        } else {
            subDb = QSqlDatabase::addDatabase("QSQLITE", connName);
            subDb.setDatabaseName("finance.db");
        }

        QMap<QString, double> data;
        if (subDb.open()) {
            QSqlQuery q(subDb);
            q.exec("SELECT category, SUM(amount) FROM finance GROUP BY category");
            while (q.next()) {
                data.insert(q.value(0).toString(), q.value(1).toDouble());
            }
            subDb.close();
        }

        QMetaObject::invokeMethod(this, [this, data]() {
            refreshChart(data);
            double total = 0;
            for(double v : data.values()) total += v;
            ui->labelTotal->setText(QString("总额统计: %1 元").arg(QString::number(total, 'f', 2)));
        }, Qt::QueuedConnection);
    });
}

// 图表刷新
void MainWindow::refreshChart(const QMap<QString, double> &data) {
    QPieSeries *series = new QPieSeries();
    for (auto it = data.begin(); it != data.end(); ++it) {
        series->append(it.key(), it.value());
    }

    for(auto slice : series->slices()) slice->setLabelVisible(true);

    QChart *chart = new QChart();
    chart->addSeries(series);
    chart->setTitle("支出占比");
    chart->setAnimationOptions(QChart::AllAnimations);
    chart->legend()->setAlignment(Qt::AlignRight);

    ui->chartView->setChart(chart);
    ui->chartView->setRenderHint(QPainter::Antialiasing);
}

MainWindow::~MainWindow() { delete ui; }
