#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QDate>
#include <QtConcurrent>
#include <QtCharts>
#include <QDebug>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow) {
    ui->setupUi(this);
    // 1. 初始化数据库
    initDatabase();
    // 2. 初始运行统计
    updateStatistics();
}

// --- 核心模块：数据库初始化 ---
void MainWindow::initDatabase() {
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName("finance.db");

    if (!db.open()) {
        qDebug() << "错误：无法打开数据库 -" << db.lastError().text();
        return;
    }

    // 自动建表：确保表结构完全匹配代码
    QSqlQuery query;
    // 使用 IF NOT EXISTS 防止重复创建
    bool success = query.exec("CREATE TABLE IF NOT EXISTS finance ("
                              "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                              "date TEXT, "
                              "category TEXT, "
                              "amount REAL)");
    if (!success) {
        qDebug() << "建表失败:" << query.lastError().text();
    }

    // Model/View 绑定
    model = new QSqlTableModel(this);
    model->setTable("finance");
    model->setEditStrategy(QSqlTableModel::OnManualSubmit);
    model->select(); // 首次加载数据

    ui->tableView->setModel(model);
    ui->tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    qDebug() << "数据库初始化完毕，路径:" << QCoreApplication::applicationDirPath() + "/finance.db";
}

// --- 核心模块：添加数据 (使用命名绑定修复参数错误) ---
void MainWindow::on_btnAdd_clicked() {
    QString category = ui->lineCategory->text();
    QString amountStr = ui->lineAmount->text();
    double amount = amountStr.toDouble();
    QString date = QDate::currentDate().toString("yyyy-MM-dd");

    if (category.isEmpty() || amount <= 0) {
        qDebug() << "输入无效：分类为空或金额为0";
        return;
    }

    QSqlQuery query;
    // 【关键修复】改用命名占位符 (:dt, :cat, :amt)
    // 这种写法比 ? 更稳定，绝对不会出现 mismatch 错误
    query.prepare("INSERT INTO finance (date, category, amount) VALUES (:dt, :cat, :amt)");

    query.bindValue(":dt", date);
    query.bindValue(":cat", category);
    query.bindValue(":amt", amount);

    if (query.exec()) {
        qDebug() << "数据插入成功！";
        model->select();       // 刷新表格
        updateStatistics();    // 刷新图表

        // 清空输入框
        ui->lineCategory->clear();
        ui->lineAmount->clear();
    } else {
        qDebug() << "插入失败详情:" << query.lastError().text();
    }
}

// --- 核心模块：多线程统计 (修复跨线程报错) ---
void MainWindow::updateStatistics() {
    // 启动后台线程
    QtConcurrent::run([this]() {
        // 子线程必须使用独立的数据库连接名 "sub_conn"
        QSqlDatabase subDb;
        if (QSqlDatabase::contains("sub_conn")) {
            subDb = QSqlDatabase::database("sub_conn");
        } else {
            subDb = QSqlDatabase::addDatabase("QSQLITE", "sub_conn");
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
        } else {
            qDebug() << "子线程数据库打开失败";
        }

        // 回调主线程更新 UI
        QMetaObject::invokeMethod(this, [this, data]() {
            refreshChart(data);

            double total = 0;
            for(double v : data.values()) total += v;
            ui->labelTotal->setText(QString("总额统计: %1 元").arg(QString::number(total, 'f', 2)));
        }, Qt::QueuedConnection);
    });
}

// --- 核心模块：刷新图表 ---
void MainWindow::refreshChart(const QMap<QString, double> &data) {
    QPieSeries *series = new QPieSeries();
    for (auto it = data.begin(); it != data.end(); ++it) {
        series->append(it.key(), it.value());
    }

    QChart *chart = new QChart();
    chart->addSeries(series);
    chart->setTitle("支出分类统计");
    chart->setAnimationOptions(QChart::SeriesAnimations);
    chart->legend()->setAlignment(Qt::AlignRight);

    // 绑定到 UI 的 QChartView
    ui->chartView->setChart(chart);
    ui->chartView->setRenderHint(QPainter::Antialiasing);
}

MainWindow::~MainWindow() {
    delete ui;
}
