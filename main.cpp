#include "mainwindow.h"

#include <QApplication>
#include<AIS_Axis.hxx>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    return a.exec();
}
