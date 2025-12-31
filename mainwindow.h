#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSqlTableModel>
#include <QtCharts> // 必须包含图表头文件

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_btnAdd_clicked();      // 添加按钮
    void updateStatistics();       // 多线程统计函数

private:
    Ui::MainWindow *ui;
    QSqlTableModel *model;         // Model/View 核心模型 [cite: 133]
    void initDatabase();           // 数据库初始化
    void refreshChart(const QMap<QString, double> &data); // 刷新图表
};
#endif
