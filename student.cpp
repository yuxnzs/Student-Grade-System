#include "student.h"
#include "./ui_student.h"

Student::Student(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::Student) {
    ui->setupUi(this);
    // 將表格延展來填滿可用空間
    ui->studentTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    // 用來設定 CSS
    ui->btnSort->setObjectName("sortButton");

    // 初始化按鈕連接
    initConnections();

    // 用來儲存「最後一次」查詢，以保持排序順序
    // 因為插入、刪除、更新操作不執行 SELECT
    // 表示如果表格要在插入、刪除、更新後顯示上次的資料
    // 需要執行最近一次的排序查詢
    // 等同於在不執行 SELECT 的情況自動執行一次 SELECT，但因為這次不執行，所以改執行上一次進行操作之 SELECT
    lastSortingQuery = "SELECT id, name, grade FROM Students";  // 預設不排序

    // 打開資料庫並創建資料表（如果不存在）
    createDatabase();
    createTable();

    // 使用資料庫中的資料寫入表格
    populateTableWidget();

    // 將 QTableWidget 設置為唯讀模式（避免使用者動手更改表格內資料，雖然更改後不影響資料庫內資料，但還是將其關閉）
    ui->studentTable->setEditTriggers(QAbstractItemView::NoEditTriggers);

    // 設置插入按鈕為默認按鈕，按下 Enter 時觸發
    ui->btnInsert->setDefault(true);

    // 設置下拉選單內容
    ui->comboBoxSortField->addItem("ID");
    ui->comboBoxSortField->addItem("Grade");

    ui->comboBoxSortOrder->addItem("Ascending");
    ui->comboBoxSortOrder->addItem("Descending");

    // 為 UI 設定 CSS
    qApp->setStyleSheet(
        "QPushButton {"
        "   background-color: #e5e5e5;"
        "   border: 1px solid #9e9e9e;"
        "   padding: 2px;"
        "   border-radius: 5px;"
        "   min-width: 80px;"
        "   min-height: 15px;"
        "}"

        "QPushButton:hover {"
        "   background-color: rgba(229, 229, 229, 0);"
        "}"

        "QPushButton#sortButton {"
        "   max-width: 80px;"
        "}"

        "QComboBox {"
        "   background-color: #e5e5e5;"
        "   border: 1px solid #9e9e9e;"
        "   border-radius: 5px;"
        "   min-width: 80px;"
        "   min-height: 15px;"
        "   padding: 2px 5px;"

        "}"

        "QComboBox:hover {"
        "   background-color: rgba(229, 229, 229, 0);"
        "}"

        "QComboBox::drop-down {"
        "   border: none;" // 移除邊距
        "}"

        "QComboBox::down-arrow {"
        "   image: url(:/src/images/drop-down-arrow.png);"
        "}"

        "QLineEdit {"
        "   border: 1px solid #9e9e9e;"
        "   border-radius: 5px;"
        "}"
    );

    this->setFixedSize(910, 470);
}

Student::~Student() {
    delete ui;
}

// 按鈕跟 UI 連接
void Student::initConnections() {
    connect(ui->btnSort, &QPushButton::clicked, this, &Student::btnSort_clicked);
    connect(ui->btnInsert, &QPushButton::clicked, this, &Student::btnInsert_clicked);
    connect(ui->btnDelete, &QPushButton::clicked, this, &Student::btnDelete_clicked);
    connect(ui->btnUpdate, &QPushButton::clicked, this, &Student::btnUpdate_clicked);
    connect(ui->btnSearch, &QPushButton::clicked, this, &Student::btnSearch_clicked);
}

void Student::createDatabase() {
    // 初始化 SQLite 資料庫連接
    sqldb = QSqlDatabase::addDatabase("QSQLITE");

    // 設置資料庫文件的名稱（路徑）
    sqldb.setDatabaseName("StudentMis.db");

    // 打開資料庫並根據結果顯示消息
    if (!sqldb.open()) {
        QString errorMessage = sqldb.lastError().text();
        QMessageBox::critical(this, "資料庫連接失敗", "無法打開資料庫，請再試一次" + errorMessage, QMessageBox::Ok);
    }
}

void Student::createTable() {
    const QString tableName = "Students";
    QSqlQuery query;

    // 檢查 Students 表是否已存在
    if (!QSqlDatabase::database().tables().contains(tableName)) {
        const QString createTableSQL = QString("CREATE TABLE %1 ("
                                               "id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,"
                                               "name TEXT NOT NULL,"
                                               "grade REAL NOT NULL)").arg(tableName);

        // 執行 SQL 以創建資料表
        if (!query.exec(createTableSQL)) {
            QString errorMessage = query.lastError().text();
            QMessageBox::critical(this, "表格建立失敗", "無法建立表格" + errorMessage, QMessageBox::Ok);
        } else {
            // 此提示主要是在告知用戶初始設置已成功完成
            // 雖然實際上是關於表格創建，但使用者不會知道
            QMessageBox::information(this, "成功", "資料庫已成功建立", QMessageBox::Ok);
        }
    }
}

void Student::btnSort_clicked() {
    // 根據用戶選擇設定排序方式
    QString field = ui->comboBoxSortField->currentText() == "ID" ? "id" : "grade";
    QString order = ui->comboBoxSortOrder->currentText() == "Ascending" ? "ASC" : "DESC";

    QString strQuery = QString("SELECT * FROM Students ORDER BY %1 %2").arg(field, order);

    // 儲存排序查詢供之後遇到沒有執行 SELECT 的情況下使用
    lastSortingQuery = strQuery;

    QSqlQuery query;
    if (!query.exec(strQuery)) {
        QMessageBox::critical(this, "失敗", "無法對資料進行排序" + query.lastError().text(), QMessageBox::Ok);
        return;
    }
    populateTableWidget(query);
}


void Student::btnSearch_clicked() {
    bool ok;
    int id = ui->lineEditID->text().toInt(&ok);
    if (!ok || id <= 0) {
        QMessageBox::information(this, "輸入錯誤", "請輸入有效的 ID（ID 不能為 0、負數、文字或空白）");
        return;
    }

    // 使用參數化查詢方式防止 SQL Injection
    QSqlQuery query;
    query.prepare("SELECT * FROM Students WHERE id = :id");
    query.bindValue(":id", id);

    // 嘗試執行查詢
    if (!query.exec()) {
        QMessageBox::critical(this, "搜尋錯誤", "搜尋資料時發生錯誤：" + query.lastError().text(), QMessageBox::Ok);
        return;
    }

    // 檢查是否有返回資料
    if (!query.first()) {
        QMessageBox::information(this, "搜尋結果", QString("未找到 ID %1 的資料").arg(id), QMessageBox::Ok);
    } else {
        // 如果有資料，則更新表格
        query.first(); // 移動到第一筆
        query.previous(); // 回到開始之前的位置
        populateTableWidget(query); // 重新填充表格以顯示搜尋結果
    }
}

void Student::btnInsert_clicked() {
    bool ok;
    int id = ui->lineEditID->text().toInt(&ok);
    if (!ok || id <= 0) {
        QMessageBox::information(this, "輸入錯誤", "請輸入有效的 ID（ID 不能為 0、負數、文字或空白）");
        return;
    }

    QString name = ui->lineEditName->text().trimmed();
    if (name.isEmpty()) {
        QMessageBox::information(this, "輸入錯誤", "姓名不能為空。請輸入有效的姓名");
        return;
    }

    double grade;
    if (ui->lineEditGrade->text().isEmpty()) {
        grade = 0; // 如果沒有輸入，將成績自動設為 0
    } else {
        bool ok;
        grade = ui->lineEditGrade->text().toDouble(&ok);
        if (!ok || grade < 0 || grade > 100) {
            QMessageBox::information(this, "輸入錯誤", "成績必須在 0 到 100 之間。請輸入有效的成績");
            return;
        }
    }

    QSqlQuery query;
    query.prepare("INSERT INTO Students (id, name, grade) VALUES (:id, :name, :grade)");
    query.bindValue(":id", id);
    query.bindValue(":name", name);
    query.bindValue(":grade", grade);

    if (!query.exec()) {
        QString errorMessage = query.lastError().text();
        QMessageBox::critical(this, "插入失敗", "無法插入資料。原因：" + errorMessage);
    } else {
        populateTableWidget(); // 刷新表格以顯示新資料
    }
}

void Student::btnDelete_clicked() {
    bool ok;
    int id = ui->lineEditID->text().toInt(&ok);
    if (!ok || id <= 0) {
        QMessageBox::information(this, "輸入錯誤", "請輸入有效的 ID（ID 不能為 0、負數、文字或空白）");
        return;
    }

    QSqlQuery query;
    query.prepare("DELETE FROM Students WHERE id = :id");
    query.bindValue(":id", id);

    if (!query.exec()) {
        QString errorMessage = query.lastError().text();
        QMessageBox::critical(this, "刪除失敗", "無法刪除資料。原因: " + errorMessage);
    } else {
        if (query.numRowsAffected() == 0) {
            QMessageBox::information(this, "刪除結果", QString("未找到 ID %1 的資料").arg(id));
        } else {
            populateTableWidget(); // 刷新表格以顯示當前資料
        }
    }
}


void Student::btnUpdate_clicked() {
    bool ok;
    int id = ui->lineEditID->text().toInt(&ok);
    if (!ok || id <= 0) {
        QMessageBox::information(this, "輸入錯誤", "請輸入有效的 ID（ID 不能為 0、負數、文字或空白）");
        return;
    }

    QString name = ui->lineEditName->text().trimmed();
    bool okGrade;
    double grade = ui->lineEditGrade->text().toDouble(&okGrade);

    bool updateName = !name.isEmpty();
    bool updateGrade = okGrade && (grade >= 0 && grade <= 100);

    QStringList fieldsToUpdate;
    if (updateName) {
        fieldsToUpdate << "name = :name";
    }
    if (updateGrade) {
        fieldsToUpdate << "grade = :grade";
    }

    if (fieldsToUpdate.isEmpty()) {
        QMessageBox::information(this, "更新訊息", "未輸入有效的姓名或成績。請提供要更新之資料");
        return;
    }

    QString strSql = QString("UPDATE Students SET %1 WHERE id = :id").arg(fieldsToUpdate.join(", "));
    QSqlQuery query;
    query.prepare(strSql);

    // 綁定共同參數：ID
    query.bindValue(":id", id);

    // 根據條件綁定其他參數
    if (updateName) {
        query.bindValue(":name", name);
    }
    if (updateGrade) {
        query.bindValue(":grade", grade);
    }

    if (!query.exec()) {
        QString errorMessage = query.lastError().text();
        QMessageBox::critical(this, "更新失敗", "無法更新資料。原因：" + errorMessage);
    } else {
        if (query.numRowsAffected() == 0) {
            QMessageBox::information(this, "更新訊息", QString("未找到 ID %1 的資料。未進行資料更新").arg(id));
        } else {
            populateTableWidget(); // 在顯示訊息前刷新表格
            QMessageBox::information(this, "成功", "資料已成功更新");
        }
    }
}


// Overload，確保其他無參數的函數可以被調用
void Student::populateTableWidget() {
    QSqlQuery query(lastSortingQuery);

    populateTableWidget(query);  // 重用有參數的版本
}

void Student::populateTableWidget(QSqlQuery &query) {
    // 若沒有以下，會變成沒有首先清除表格的現有內容，只是繼續在已有的行數後面添加新的行
    ui->studentTable->clear();             // 清除表格內容
    ui->studentTable->setRowCount(0);      // 重置行數
    ui->studentTable->setColumnCount(3);
    // 設置欄位標題
    ui->studentTable->setHorizontalHeaderLabels(QStringList() << "ID" << "姓名" << "成績");

    int row = 0;
    while (query.next()) {
        int id = query.value(0).toInt();
        QString name = query.value(1).toString();
        double grade = query.value(2).toDouble();

        // 新增新行至表格尾端
        ui->studentTable->insertRow(row);
        ui->studentTable->setItem(row, 0, new QTableWidgetItem(QString::number(id)));
        ui->studentTable->setItem(row, 1, new QTableWidgetItem(name));
        ui->studentTable->setItem(row, 2, new QTableWidgetItem(QString::number(grade)));
        row++;
    }
}
