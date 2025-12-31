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
    // 使用 QtConcurrent 在子线程执行计算，防止 UI 卡顿 [cite: 13, 133]
    QtConcurrent::run([this]() {
        QMap<QString, double> statData;
        QSqlQuery query("SELECT category, SUM(amount) FROM finance GROUP BY category");
        while (query.next()) {
            statData.insert(query.value(0).toString(), query.value(1).toDouble());
        }

        // 回到主线程更新 UI (通过 invokeMethod)
        QMetaObject::invokeMethod(this, [this, statData]() {
            refreshChart(statData);
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
