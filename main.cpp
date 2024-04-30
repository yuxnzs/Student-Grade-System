#include "student.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    Student w;
    w.show();
    return a.exec();
}
