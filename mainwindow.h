#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSqlTableModel>
#include <QMessageBox>
#include <QtCharts>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_btnAdd_clicked();      // 添加
    void on_btnDelete_clicked();   // 删除
    void on_btnFilter_clicked();   // 筛选
    void on_btnReset_clicked();    // 重置
    void on_btnReport_clicked();   // 报表
    void updateStatistics();       // 异步统计

private:
    Ui::MainWindow *ui;
    QSqlTableModel *model;
    void initDatabase();
    void refreshChart(const QMap<QString, double> &data);
};
#endif // MAINWINDOW_H
