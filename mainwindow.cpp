#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QtConcurrent> // 必须包含多线程头文件 [cite: 133]

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow) {
    ui->setupUi(this);
    initDatabase();
}

// 1. 数据库与 Model 绑定 (MV + DB 模块) [cite: 133]
void MainWindow::initDatabase() {
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName("finance.db"); // 确保此文件在编译生成的目录下
    if (db.open()) {
        model = new QSqlTableModel(this);
        model->setTable("finance");
        model->setEditStrategy(QSqlTableModel::OnFieldChange);
        model->select();
        ui->tableView->setModel(model); // 绑定视图 [cite: 133]
    }
}

// 2. 添加数据
void MainWindow::on_btnAdd_clicked() {
    int row = model->rowCount();
    model->insertRow(row);
    // 假设你的输入框分别叫 lineCategory 和 lineAmount
    model->setData(model->index(row, 1), QDate::currentDate().toString("yyyy-MM-dd"));
    model->setData(model->index(row, 2), ui->lineCategory->text());
    model->setData(model->index(row, 3), ui->lineAmount->text().toDouble());
    model->submitAll();
    updateStatistics(); // 数据变动后更新统计
}

// 3. 多线程统计逻辑 (THR 模块) [cite: 133]
void MainWindow::updateStatistics() {
    // 满足任务书要求的：多线程(THR) + 数据库(DB) 融合
    QtConcurrent::run([this]() {
        // 关键：在子线程内创建一个临时的、独立的数据库连接
        QSqlDatabase tempDb;
        if (QSqlDatabase::contains("thread_db")) {
            tempDb = QSqlDatabase::database("thread_db");
        } else {
            tempDb = QSqlDatabase::addDatabase("QSQLITE", "thread_db");
            tempDb.setDatabaseName("finance.db"); // 确保与主线程文件名一致
        }

        QMap<QString, double> statData;
        if (tempDb.open()) {
            QSqlQuery query(tempDb); // 显式指定使用子线程的连接
            query.exec("SELECT category, SUM(amount) FROM finance GROUP BY category");
            while (query.next()) {
                statData.insert(query.value(0).toString(), query.value(1).toDouble());
            }
            tempDb.close();
        }

        // 回到主线程更新 UI [cite: 95]
        QMetaObject::invokeMethod(this, [this, statData]() {
            refreshChart(statData);
            double total = 0;
            for(double v : statData.values()) total += v;
            ui->labelTotal->setText(QString("总额统计: %1").arg(total));
            model->select(); // 刷新 TableView 视图
        });
    });
}

// 4. 刷新图表 (Charts 模块) [cite: 8]
void MainWindow::refreshChart(const QMap<QString, double> &data) {
    QPieSeries *series = new QPieSeries();
    for (auto it = data.begin(); it != data.end(); ++it) {
        series->append(it.key(), it.value());
    }

    QChart *chart = new QChart();
    chart->addSeries(series);
    chart->setTitle("财务支出占比");

    // 注意：ui->chartView 必须是你在 UI 编辑器里“提升”后的 QChartView
    ui->chartView->setChart(chart);
}

MainWindow::~MainWindow() { delete ui; }
