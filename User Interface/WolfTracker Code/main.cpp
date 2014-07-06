#include "mainwindow.h"

//UI Header files
#include <QApplication>
#include <QtCore>
#include <QDebug>
#include <QFileSystemWatcher>

int main(int argc, char *argv[])
{
    //UI program
    QApplication a(argc, argv);
    MainWindow w;
    w.show();

    return a.exec();
}

