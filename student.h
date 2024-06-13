#ifndef STUDENT_H
#define STUDENT_H

#include <QDialog>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlQueryModel>
#include <QSqlError>
#include <QDebug>
#include <QMessageBox>
#include <QTableWidgetItem>

QT_BEGIN_NAMESPACE
namespace Ui { class Student; }
QT_END_NAMESPACE

class Student : public QDialog {
    Q_OBJECT

public:
    Student(QWidget *parent = nullptr);
    ~Student();

private slots:
    void btnSort_clicked();

    void btnInsert_clicked();

    void btnDelete_clicked();

    void btnUpdate_clicked();

    void btnSearch_clicked();

    void populateTableWidget();

    void populateTableWidget(QSqlQuery &query);


private: // Self-defined functions
    void createDatabase();
    void createTable();
    void queryTable();
    void initConnections();

    QSqlDatabase sqldb; // Represents a connection to a database.
    QSqlQueryModel sqlmodel; // Represents the result set of a SQL query for use with data-aware widgets.
    // 用於表示 SQL 查詢結果的模型。常用於與數據感知的小部件（如 QTableView）一起使用，以顯示查詢結果。
    QString lastSortingQuery;

private:
    Ui::Student *ui;
};
#endif // STUDENT_H
