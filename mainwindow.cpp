#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QtConcurrent> // 必须包含多线程头文件 [cite: 133]
#include <QDebug>
MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow) {
    ui->setupUi(this);
    initDatabase();
}

// 1. 数据库与 Model 绑定 (MV + DB 模块) [cite: 133]

void MainWindow::initDatabase() {
    // 数据库模块应用
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    // 确保 finance.db 文件位于编译运行目录下 [cite: 5]
    db.setDatabaseName("finance.db");

    if (!db.open()) {
        qDebug() << "数据库打开失败:" << db.lastError().text();
        return;
    }

    // Model/View 框架应用
    model = new QSqlTableModel(this);
    model->setTable("finance");
    // 设置手动提交，方便控制刷新时机 [cite: 7]
    model->setEditStrategy(QSqlTableModel::OnManualSubmit);
    model->select();

    ui->tableView->setModel(model);
    qDebug() << "数据库初始化成功，路径:" << QCoreApplication::applicationDirPath() + "/finance.db";
}
void MainWindow::on_btnAdd_clicked() {
    // 1. 获取输入数据
    QString category = ui->lineCategory->text();
    double amount = ui->lineAmount->text().toDouble();
    QString date = QDate::currentDate().toString("yyyy-MM-dd");

    if (category.isEmpty() || amount <= 0) {
        qDebug() << "输入数据无效";
        return;
    }

    // 2. 执行数据库插入 [cite: 5]
    QSqlQuery query;
    query.prepare("INSERT INTO finance (date, category, amount) VALUES (?, ?, ?)");
    query.addBindValue(date);
    query.addBindValue(category);
    query.addBindValue(amount);

    if (query.exec()) {
        qDebug() << "数据插入成功";
        // 3. 核心：强制刷新 Model，让 TableView 显示新数据 [cite: 7]
        model->select();
        // 4. 核心：启动多线程异步统计更新图表
        updateStatistics();
    } else {
        qDebug() << "插入失败:" << query.lastError().text();
    }
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
