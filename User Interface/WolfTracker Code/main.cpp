#include "mainwindow.h"

//UI Header files
#include <QApplication>
#include <QtCore>
#include <QDebug>
#include <QFileSystemWatcher>

int main(int argc, char *argv[])
{
    //UI program
    /* initializes window system and constructs
     * an application object
     */
    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    //enters the main loop and waits until exit is called
    return a.exec();
}

