#include "mainwindow.h"
#include "ui_mainwindow.h"

//UI Header files
#include <QFile>
#include <QDir>
#include <QList>
#include <QFileDialog>
#include <QTextStream>
#include <QDebug>
#include <QScrollBar>
#include <QtGui>
#include <QDesktopServices>
#include <QUrl>
#include <QProcess>
#include <QInputDialog>
#include <QDateTime>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    //Setting controller image
    QPixmap pix("xbox.png");
    ui->label_pic->setPixmap(pix);

    //initialize private members
    vRotate = 0;
    cRotate = 0;
    tilt = 0;
    coordinate = "";
    oldcontroller = "";
    oldlift = "";
    oldturn = "";
    direction1 = 0;
    direction2 = 0;

    //"Save" files
    (void)QDir::current().mkdir("Logs");
    debugfile.append(QDateTime::currentDateTime().toString());
    debugfile.append(".txt");
    gpsfile = "GPS Coordinates - ";
    gpsfile.append(QDateTime::currentDateTime().toString());
    gpsfile.append(".txt");


    //Files from which the streams are read
    QString ctrlrpath; //controller stream
    QString gpspath; //GPS stream
    QString vpath, cpath; //Vehicle and camera accelerometer streams

    ctrlrpath.append("WolfTracker Text/feedback.txt");
    gpspath.append("WolfTracker Text/gps.txt");
    vpath.append("WolfTracker Text/vehicle.txt");
    cpath.append("WolfTracker Text/camera.txt");

    //Monitor user-friendly actions sent by controller over stream
    controller = new QFileSystemWatcher(this);
    controller->addPath(ctrlrpath);
    connect(controller, SIGNAL(fileChanged(const QString &)), this, SLOT(feedback(const QString &)));

    //Monitor text files connected to vehicle, camera, and gps streams
    vStream = new QFileSystemWatcher(this);
    vStream->addPath(vpath);
    connect(vStream, SIGNAL(fileChanged(const QString &)), this, SLOT(vehicledeg(const QString &)));

    cStream = new QFileSystemWatcher(this);
    cStream->addPath(cpath);
    connect(cStream, SIGNAL(fileChanged(const QString &)), this, SLOT(cameradeg(const QString &)));

    gps = new QFileSystemWatcher(this);
    gps->addPath(gpspath);
    connect(gps, SIGNAL(fileChanged(const QString &)), this, SLOT(gpscoord(const QString &)));


    createActions();
    createMenus();

    ui->textBrowser->append("Welcome to the Wolftracker!");
    ui->textBrowser->append("---------------------------------------------------------");

}

MainWindow::~MainWindow()
{
    delete ui;
    delete controller;
    delete vStream;
    delete cStream;
    delete gps;
    delete exit;
    delete info;
    delete calibrate;
    delete saveFB;
    delete saveGPS;
}

/*
 * Vehicle and camera compass modules that are automatically redrawn with new values
 */
void MainWindow::paintEvent(QPaintEvent *)
{

    //Clean circle
    QPainter container1(this);
    container1.translate(132, 348 + 120);
    container1.drawEllipse(QPointF(0,0), 75, 75);
    //QPainter container2(this);
    //container2.translate(290, 348 + 10);
    //container2.drawLine(QPointF(0,0), QPointF(0, 140));
    //container2.drawLine(QPointF(0, 140), QPointF(150, 140));

    //Main vehicle module
    QPainter vehicle(this);
    QPainterPath triangle;
    triangle.setFillRule(Qt::WindingFill);

    vehicle.translate(132, 348 + 120);

    vehicle.rotate(vRotate);

    triangle.moveTo(0, -55);
    triangle.lineTo(-10, -45);
    triangle.lineTo(10, -45);
    triangle.lineTo(0, -55);
    vehicle.fillPath(triangle, Qt::black);

    //Vehicle body
    vehicle.fillRect(-31, -45, 60, 90, Qt::darkGray);
    vehicle.drawRect(-31, -45, 60, 90);

    //Vehicle wheels
    vehicle.fillRect(-38, -55, 15, 20, Qt::black);
    vehicle.fillRect(23, -55, 15, 20, Qt::black);
    vehicle.fillRect(-38, -10, 15, 20, Qt::black);
    vehicle.fillRect(23, -10, 15, 20, Qt::black);
    vehicle.fillRect(-38, 35, 15, 20, Qt::black);
    vehicle.fillRect(23, 35, 15, 20, Qt::black);

    /********************************************************/
    //Vehicle-lift module
    QPainterPath triangle2;
    triangle2.setFillRule(Qt::WindingFill);

    QPainter lift(this);
    lift.translate(385, 348 + 122);
    lift.rotate(tilt);

    //Vehicle body
    lift.fillRect(-50, -30, 100, 30, Qt::darkGray);
    lift.drawRect(-50, -30, 100, 30);

    lift.setBrush(Qt::black);
    lift.drawEllipse(QPoint(0,0), 15, 15);
    lift.drawEllipse(QPoint(-40, 0), 15, 15);
    lift.drawEllipse(QPoint(40, 0), 15, 15);
    lift.setBrush(Qt::gray);
    lift.drawEllipse(QPoint(0,0), 7, 7);
    lift.drawEllipse(QPoint(-40, 0), 7, 7);
    lift.drawEllipse(QPoint(40, 0), 7, 7);
    //lift.fillRect(-55, -10, 25, 15, Qt::black);
    //lift.fillRect(-12, -10, 25, 15, Qt::black);
    //lift.fillRect(30, -10, 25, 15, Qt::black);

    //Camera
    lift.fillRect(-5, -50, 10, 20, Qt::blue);
    triangle2.moveTo(-13, -42);
    triangle2.lineTo(-5, -50);
    triangle2.lineTo(-5, -35);
    triangle2.lineTo(-13, -42);
    lift.fillPath(triangle2, Qt::blue);

    /*********************************************************/
    //Camera module
    QPainter camera(this);
    QPainterPath camdir;
    camdir.setFillRule(Qt::WindingFill);

    camera.translate(132, 348 + 110);
    camera.rotate(cRotate);

    camdir.moveTo(0, -33);
    camdir.lineTo(-8, -25);
    camdir.lineTo(8, -25);
    camdir.lineTo(0, -33);
    camera.fillPath(camdir, Qt::blue);

    camera.fillRect(-5, -25, 10, 25, Qt::blue);

    //Redraw modules for new positions
    update();
}

/*
 * Displays controller button input onto the feedback section.
 */
void MainWindow::feedback(const QString & path)
{
    QFile file(path);
    controller->addPath(path);

  file.open(QIODevice::ReadOnly | QIODevice::Text);

    QTextStream stream(&file);
    QString newfbLine;

    //Get last line i.e. newest info
    while(!stream.atEnd())
    {
        newfbLine = stream.readLine();

    }

    //Display only if different direction (control repeating messages)
    //TO_DO: For each QChar('character'), change 'character' to corresponding char

    //Check first character
    //if(newfbLine[0] == QChar('Turn character'))  //Turning vehicle
    //else if(newfbLine[0] == QChar('Look character'))  //Turning camera
        if(newfbLine != oldcontroller)
        {

             oldcontroller = newfbLine;
             ui->textBrowser->append(newfbLine);

        }
    //else if(newfbLine[0] == QChar('X character'); //pushing either x or y
        //ui->textBrowser->append("Ending stream");
    //else if(newfbLine[0] == QChar('Y character');
      //ui->textBrowser->append("Centering camera");

  file.flush();
  file.close();

    //Immediately scroll to the bottom of the text box i.e. latest line
    QScrollBar *sb = ui->textBrowser->verticalScrollBar();
    sb->setValue(sb->maximum());

}

/*
 * Get the position data from the vehcile accelerometer
 */
void MainWindow::vehicledeg(const QString & path)
{
    QFile file(path);
    vStream->addPath(path);

  file.open(QIODevice::ReadOnly | QIODevice::Text);

    QTextStream stream(&file);
    QString data;
    int datanum;

    //Get newest info
    while(!stream.atEnd())
    {
        data = stream.readLine();

    }
    datanum = data.toInt() % 360;
    qDebug() << datanum;

    //Output turns to feedback
    if(datanum == 359 && vRotate == 0)
    {
        if(direction1 != -1)
        {
            ui->textBrowser->append("Turning left");
            direction1 = -1;
        }
        else
            ui->textBrowser->insertPlainText(".");
    }
    else if(datanum == 0 && vRotate == 359)
    {
        if(direction1 == 1)
        {
            ui->textBrowser->append("Turning right");
            direction1 = 1;
        }
        else
            ui->textBrowser->insertPlainText(".");
    }
    else if(datanum - vRotate < 0) //Turning left
    {
        if(direction1 != -1)
        {
            ui->textBrowser->append("Turning left");
            direction1 = -1;
        }
        else
            ui->textBrowser->insertPlainText(".");
    }
    else if(datanum - vRotate > 0)  //Turning right
    {
        if(direction1 != 1)
        {
            ui->textBrowser->append("Turning right");
            direction1 = 1;
        }
        else
            ui->textBrowser->insertPlainText(".");
    }



    //Rotate degree vals
    vRotate = datanum;

    //Tilt degree vals (Obtain through however the position data is parsed)
    tilt = datanum;
    tilt += 30;

  file.flush();
  file.close();

}

/*
 * Get the rotation degree data from the camera accelerometer
 */
void MainWindow::cameradeg(const QString & path)
{
    QFile file(path);
    cStream->addPath(path);

  file.open(QIODevice::ReadOnly | QIODevice::Text);

    QTextStream stream(&file);
    QString data;
    int datanum;

    //Get newest info
    while(!stream.atEnd())
    {
        data = stream.readLine();

    }
    datanum = data.toInt() % 360;

    //Output turns to feedback
    if(datanum == 359 && vRotate == 0)
    {
        if(direction2 != -1)
        {
            ui->textBrowser->append("Looking left");
            direction2 = -1;
        }
        else
            ui->textBrowser->insertPlainText(".");
    }
    else if(datanum == 0 && vRotate == 359)
    {
        if(direction2 == 1)
        {
            ui->textBrowser->append("Looking right");
            direction2 = 1;
        }
        else
            ui->textBrowser->insertPlainText(".");
    }
    else if(datanum - vRotate < 0) //Looking left
    {
        if(direction2 != -1)
        {
            ui->textBrowser->append("Looking left");
            direction2 = -1;
        }
        else
            ui->textBrowser->insertPlainText(".");
    }
    else if(datanum - vRotate > 0)  //Looking right
    {
        if(direction2 != 1)
        {
            ui->textBrowser->append("Looking right");
            direction2 = 1;
        }
        else
            ui->textBrowser->insertPlainText(".");
    }


    //Rotate degree vals
    cRotate = datanum;

  file.flush();
  file.close();

}

/*
 * Get the coordinates from the GPS
 */
void MainWindow::gpscoord(const QString & path)
{
    QFile file(path);
    gps->addPath(path);

  file.open(QIODevice::ReadOnly | QIODevice::Text);

    QTextStream stream(&file);
    QString data;

    //Get newest info
    while(!stream.atEnd())
    {
        data = stream.readLine();

    }

    //GPS coordinate
    coordinate = data;

    ui->textBrowser_2->setPlainText(coordinate);
    ui->textBrowser_2->setAlignment(Qt::AlignCenter);

  file.flush();
  file.close();

}
/*
 * "Restart" button:
 * Restarts the camera stream interface
 */
void MainWindow::on_pushButton_clicked()
{
    //Replace temp with path to executable
    QProcess* process = new QProcess(this);
    QString temp = "./Client";
    QStringList args;
    args << "192.168.1.102"<<"8888"<<"640"<<"480";
    process->start(temp, args, 0);

    ui->textBrowser->append("Camera stream restarted.");
}

/*
 * Window actions
 */
void MainWindow::createActions()
{
    exit = new QAction(tr("Exit"), this);
    exit->setShortcut(QKeySequence::Close);
    connect(exit, SIGNAL(triggered()), this, SLOT(fExit()));

    info = new QAction(tr("Info"), this);
    info->setShortcut(QKeySequence::HelpContents);
    connect(info, SIGNAL(triggered()), this, SLOT(iInfo()));

    calibrate = new QAction(tr("Calibrate Vehicle"), this);
    calibrate->setShortcut(Qt::Key_F5);
    connect(calibrate, SIGNAL(triggered()), this, SLOT(eCalibrate()));

    saveFB = new QAction(tr("Save Feedback"), this);
    saveFB->setShortcut(QKeySequence::Save);
    connect(saveFB, SIGNAL(triggered()), this, SLOT(fSaveFB()));

    saveGPS = new QAction(tr("Save GPS Coordinate"), this);
    saveGPS->setShortcut(Qt::Key_F6);
    connect(saveGPS, SIGNAL(triggered()), this, SLOT(fSaveGPS()));

}

/*
 * Method to add new menu options
 */
void MainWindow::createMenus()
{
    file = menuBar()->addMenu(tr("File"));
    file->addAction(saveFB);
    file->addAction(saveGPS);
    file->addAction(exit);

    edit = menuBar()->addMenu(tr("Edit"));
    edit->addAction(calibrate);


    about = menuBar()->addMenu(tr("Help"));
    about->addAction(info);

}

/*
 * Save current feedback stream
 */
void MainWindow::fSaveFB()
{
    QDir::setCurrent("Logs");  //Enter Logs folder

    QFile file(debugfile);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return;

    QTextStream out(&file);
    out << ui->textBrowser->document()->toPlainText();
    file.close();

    QDir::setCurrent("..");  //Return from Logs folder

    ui->textBrowser->append("Feedback session saved.");
}

/*
 * Adds to list of saved GPS coordinates
 */
void MainWindow::fSaveGPS()
{
    QDir::setCurrent("Logs");  //Enter "Logs" folder to save

    gpslist.append(coordinate);

    //Add a description of coordinate
    bool ok;
    QString desc = "";
    QString text = QInputDialog::getText(this, tr("QInputDialog::getText()"),
                                              tr("Add a location description:"), QLineEdit::Normal,
                                              desc, &ok);
    if (ok && !text.isEmpty())
         desc = text;


    //Write to file
    QFile file(gpsfile);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return;

    QTextStream out(&file);
    for(QStringList::iterator it = gpslist.begin(); it != gpslist.end(); ++it)
    {
      if(it->isEmpty())
          continue;

      out << *it;
      out << " - ";
      out << "\"";
      out << desc;
      out << "\"";
      out << "\n";
    }
    file.close();

    QDir::setCurrent("..");  //Return from Logs folder

    ui->textBrowser->append("GPS coordinate saved.");
}

/*
 * Exit the program.
 */
void MainWindow::fExit()
{
    MainWindow::close();
}

/*
 * Set calibration value for the accelerometer
 */
void MainWindow::eCalibrate()
{
    bool ok;
    double d = QInputDialog::getDouble(this, tr("Calibrate Vehicle"),
                                    tr("Calibration Value:"), calib, -10000,
                                       10000, 2, &ok);

    if(ok)
        calib = d;
}


/*
 * Help menu
 */
void MainWindow::iInfo()
{
    msgBox = new QMessageBox;

    QString helpBox;
    helpBox.append("\n");
    helpBox.append("\"GPS Coordinates\":\n");
    helpBox.append("-    Displays the GPS coordinates of the tracker\n\n");

    helpBox.append("\"Vehicle Control\":\n");
    helpBox.append("-    Press 'Restart' to relaunch manual control of the vehicle\n");
    helpBox.append("     after having exited\n\n");

    helpBox.append("\"Feedback\":\n");
    helpBox.append("-    View what interface functions and joystick controls are\n");
    helpBox.append("     being entered (for retracing steps)\n\n");

    helpBox.append("\"Vehicle Compass\":\n");
    helpBox.append("-    Visual representation of the vehicle and camera compass\n");
    helpBox.append("     directions.\n\n");

    helpBox.append("\"Vehicle Lift\":\n");
    helpBox.append("-    Visual representation of the raising and lowering of the\n");
    helpBox.append("     vehicle as it traverses terrain.");

    msgBox->setText(helpBox);

    //msgBox->setIcon(QMessageBox::Question);
    msgBox->setStandardButtons(QMessageBox::Ok);
    msgBox->exec();
}
