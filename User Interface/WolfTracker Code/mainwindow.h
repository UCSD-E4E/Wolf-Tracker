//include guard: prevents compilation error from MAINWINDOW_H being defined twice
#ifndef MAINWINDOW_H
//if it is not defined define it
#define MAINWINDOW_H

#include <QMainWindow>
#include <QDebug>
#include <QFileSystemWatcher>
#include <QString>
#include <QMessageBox>

namespace Ui {
class MainWindow;
}

class QAction;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

protected:
    void paintEvent(QPaintEvent *);

private slots:
    void feedback(const QString & path);
    void vehicledeg(const QString & path);
    void cameradeg(const QString & path);
    void gpscoord(const QString & path);

    void on_pushButton_clicked();

    void fExit();
    void eCalibrate();
    void fSaveGPS();
    void fSaveFB();

    void iInfo();
    //void eDebug();

private:
    Ui::MainWindow *ui;

    //Pointers to files
    QFileSystemWatcher *controller, *gps, *vStream, *cStream;

    //Menu variables
    void createMenus();
    void createActions();
    QMenu *file;
    QMenu *edit;
    QMenu *about;
    QAction *exit;
    QAction *calibrate;
    QAction *saveFB;
    QAction *saveGPS;
    QAction *info;
    QMessageBox *msgBox;

    //Module variables
    QStringList fbWindow;
    int vRotate, tilt, cRotate;
    QString coordinate;
    double calib;

    //Feedback and GPS coordinate files
    QString debugfile;
    QString gpsfile;
    QStringList gpslist;

    //Feedback filter strings
    QString oldcontroller, oldturn, oldlift;
    int direction1, direction2;

};

#endif // MAINWINDOW_H
