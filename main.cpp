/*-------------------------------------------------
#
# neural-style GUI with QT by AbsurdePhoton
#
# v1 2018-01-08
#
#-------------------------------------------------*/

#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.show();

    return a.exec();
}
