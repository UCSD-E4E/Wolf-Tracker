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

MainWindow::MainWindow(QWidget *parent) ://constructs main application window with the given parent
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
    (void)QDir::current().mkdir("Logs");//makes a directory called Logs in the current directory
    debugfile.append(QDateTime::currentDateTime().toString()); //add current date and time to debug file
    debugfile.append(".txt"); //add text file extension to debug file
    gpsfile = "GPS Coordinates - ";
    gpsfile.append(QDateTime::currentDateTime().toString());//add current date and time to gps file
    gpsfile.append(".txt");//add text file extension to gps file


    //Files from which the streams are read
    QString ctrlrpath; //controller stream
    QString gpspath; //GPS stream
    QString vpath, cpath; //Vehicle and camera accelerometer streams

    ctrlrpath.append("WolfTracker Text/feedback.txt");//feedback file to controller path
    gpspath.append("WolfTracker Text/gps.txt");//add gps file to gps path
    vpath.append("WolfTracker Text/vehicle.txt");//add vehicle file to vehicle path
    cpath.append("WolfTracker Text/camera.txt");//add camera file to camera path

    //Monitor user-friendly actions sent by controller over stream
    controller = new QFileSystemWatcher(this);//provides an interface for monitoring controller files and directories for modifications
    controller->addPath(ctrlrpath);//adds controller path to file system watcher
    connect(controller, SIGNAL(fileChanged(const QString &)), this, SLOT(feedback(const QString &)));

    //Monitor text files connected to vehicle, camera, and gps streams
    vStream = new QFileSystemWatcher(this);//provides an interface for monitoring vehicle files and directories for modifications
    vStream->addPath(vpath);//adds vehicle path to file system watcher
    connect(vStream, SIGNAL(fileChanged(const QString &)), this, SLOT(vehicledeg(const QString &)));

    cStream = new QFileSystemWatcher(this);//provides an interface for monitoring camera files and directories for modifications
    cStream->addPath(cpath);//adds camera path to file system watcher
    connect(cStream, SIGNAL(fileChanged(const QString &)), this, SLOT(cameradeg(const QString &)));

    gps = new QFileSystemWatcher(this);//provides an interface for monitoring gps files and directories for modifications
    gps->addPath(gpspath);//adds vehicle path to file system watcher
    connect(gps, SIGNAL(fileChanged(const QString &)), this, SLOT(gpscoord(const QString &)));


    createActions();
    createMenus();

    ui->textBrowser->append("Welcome to the Wolftracker!");
    ui->textBrowser->append("---------------------------------------------------------");

}

MainWindow::~MainWindow()//destructor for MainWindow
{
    //deallocate pointers
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
    QPainter container1(this);//performs low-level painting on widgets and other paint devices
    container1.translate(132, 348 + 120);//translates coordinate system with given offset (348+120)
    container1.drawEllipse(QPointF(0,0), 75, 75);//draws ellipse at center (0,0)with radii 75 and 75
    //QPainter container2(this);
    //container2.translate(290, 348 + 10);
    //container2.drawLine(QPointF(0,0), QPointF(0, 140));
    //container2.drawLine(QPointF(0, 140), QPointF(150, 140));

    //Main vehicle module
    QPainter vehicle(this);
    QPainterPath triangle;
    triangle.setFillRule(Qt::WindingFill);//determines whether a point is inside the shape using nonzero winding rule

    vehicle.translate(132, 348 + 120);//translates coordinate system with given offset (348+120)

    vehicle.rotate(vRotate);//rotates the given angle clockwise

    triangle.moveTo(0, -55);//moves current position to given coordinates and starts new subpath closing old path
    triangle.lineTo(-10, -45);//draws line from current position to given coordinates
    triangle.lineTo(10, -45);//draws line from current position to given coordinates
    triangle.lineTo(0, -55);//draws line from current position to given coordinates
    vehicle.fillPath(triangle, Qt::black);//fills the path using black

    //Vehicle body
    //fills the rectangle beginning at the coordinates(-31,-45) with a width of 60 and height of 90 using dark gray
    vehicle.fillRect(-31, -45, 60, 90, Qt::darkGray);
    //draws the rectangle beginning at the coordinates(-31,-45) with a width of 60 and height of 90
    vehicle.drawRect(-31, -45, 60, 90);

    //Vehicle wheels
    //fills the rectangle beginning at the coordinates(-38,-55) with a width of 15 and height of 20 using black
    vehicle.fillRect(-38, -55, 15, 20, Qt::black);
    //fills the rectangle beginning at the coordinates(23,-55) with a width of 15 and height of 20 using black
    vehicle.fillRect(23, -55, 15, 20, Qt::black);
    //fills the rectangle beginning at the coordinates(-38,-10) with a width of 15 and height of 20 using black
    vehicle.fillRect(-38, -10, 15, 20, Qt::black);
    //fills the rectangle beginning at the coordinates(23,-10) with a width of 15 and height of 20 using black
    vehicle.fillRect(23, -10, 15, 20, Qt::black);
    //fills the rectangle beginning at the coordinates(-38,35) with a width of 15 and height of 20 using black
    vehicle.fillRect(-38, 35, 15, 20, Qt::black);
    //fills the rectangle beginning at the coordinates(23,35) with a width of 15 and height of 20 using black
    vehicle.fillRect(23, 35, 15, 20, Qt::black);

    /********************************************************/
    //Vehicle-lift module
    QPainterPath triangle2;
    triangle2.setFillRule(Qt::WindingFill);//determines whether a point is inside the shape using nonzero winding rule

    QPainter lift(this);
    lift.translate(385, 348 + 122); //translates coordinate system with given offset (348+122)
    lift.rotate(tilt);//rotates the given angle clockwise

    //Vehicle body
    //fills the rectangle beginning at the coordinates(-50,-30) with a width of 100 and height of 30 using dark gray
    lift.fillRect(-50, -30, 100, 30, Qt::darkGray);
    //draws the rectangle beginning at the coordinates(-50,-30) with a width of 100 and height of 30
    lift.drawRect(-50, -30, 100, 30);

    //set brush to black
    lift.setBrush(Qt::black);
    //draws ellipse at center (0,0)with radii 15 and 15
    lift.drawEllipse(QPoint(0,0), 15, 15);
    //draws ellipse at center (-40,0)with radii 15 and 15
    lift.drawEllipse(QPoint(-40, 0), 15, 15);
    //draws ellipse at center (40,0)with radii 15 and 15
    lift.drawEllipse(QPoint(40, 0), 15, 15);
    //set brush to gray
    lift.setBrush(Qt::gray);
    //draws ellipse at center (0,0)with radii 7 and 7
    lift.drawEllipse(QPoint(0,0), 7, 7);
    //draws ellipse at center (-40,0)with radii 7 and 7
    lift.drawEllipse(QPoint(-40, 0), 7, 7);
    //draws ellipse at center (40,0)with radii 7 and 7
    lift.drawEllipse(QPoint(40, 0), 7, 7);
    //lift.fillRect(-55, -10, 25, 15, Qt::black);
    //lift.fillRect(-12, -10, 25, 15, Qt::black);
    //lift.fillRect(30, -10, 25, 15, Qt::black);

    //Camera
    //fills the rectangle beginning at the coordinates(-5,50) with a width of 10 and height of 20 using blue
    lift.fillRect(-5, -50, 10, 20, Qt::blue);
    //moves current position to given coordinates and starts new subpath closing old path
    triangle2.moveTo(-13, -42);
    //draws line from current position to given coordinates
    triangle2.lineTo(-5, -50);
    //draws line from current position to given coordinates
    triangle2.lineTo(-5, -35);
    //draws line from current position to given coordinates
    triangle2.lineTo(-13, -42);
    //fills the path using blue
    lift.fillPath(triangle2, Qt::blue);

    /*********************************************************/
    //Camera module
    QPainter camera(this);
    QPainterPath camdir;
    camdir.setFillRule(Qt::WindingFill);//determines whether a point is inside the shape using nonzero winding rule

    camera.translate(132, 348 + 110);//translates coordinate system with given offset (348+110)
    camera.rotate(cRotate);//rotates the given angle clockwise

    //moves current position to given coordinates and starts new subpath closing old path
    camdir.moveTo(0, -33);
    //draws line from current position to given coordinates
    camdir.lineTo(-8, -25);
    //draws line from current position to given coordinates
    camdir.lineTo(8, -25);
    //draws line from current position to given coordinates
    camdir.lineTo(0, -33);
    //fills the path using blue
    camera.fillPath(camdir, Qt::blue);

    //fills the rectangle beginning at the coordinates(-5,-25) with a width of 10 and height of 25 using blue
    camera.fillRect(-5, -25, 10, 25, Qt::blue);

    //Redraw modules for new positions
    update();
}

/*
 * Displays controller button input onto the feedback section.
 */
void MainWindow::feedback(const QString & path)
{
    QFile file(path);//provides interface for reading and writing to files
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

  file.flush();//transmits all accumulated characters to the file
  file.close();//close file

    //Immediately scroll to the bottom of the text box i.e. latest line
    QScrollBar *sb = ui->textBrowser->verticalScrollBar();
    sb->setValue(sb->maximum());

}

/*
 * Get the position data from the vehcile accelerometer
 */
void MainWindow::vehicledeg(const QString & path)//vehicle degree
{
    QFile file(path);//provides interface for reading and writing to files
    vStream->addPath(path);

  file.open(QIODevice::ReadOnly | QIODevice::Text);

    QTextStream stream(&file);
    QString data;
    int datanum;

    //Get newest info
    while(!stream.atEnd())
    {
        data = stream.readLine();//read line from file

    }
    datanum = data.toInt() % 360; //turn last line into integer and get remainder of dividing by 360
    qDebug() << datanum; //writes number to the stream

    //Output turns to feedback
    if(datanum == 359 && vRotate == 0)
    {
        if(direction1 != -1)
        {
            ui->textBrowser->append("Turning left"); //if direction 1 is not -1 the device is turning left
            direction1 = -1;
        }
        else
            ui->textBrowser->insertPlainText(".");
    }
    else if(datanum == 0 && vRotate == 359)
    {
        if(direction1 == 1)
        {
            ui->textBrowser->append("Turning right"); //if direction 1 is 1 the device is turning right
            direction1 = 1;
        }
        else
            ui->textBrowser->insertPlainText(".");
    }
    else if(datanum - vRotate < 0) //Turning left
    {
        if(direction1 != -1)
        {
            ui->textBrowser->append("Turning left");//if direction 1 is not -1 the device is turning left
            direction1 = -1;
        }
        else
            ui->textBrowser->insertPlainText(".");
    }
    else if(datanum - vRotate > 0)  //Turning right
    {
        if(direction1 != 1)
        {
            ui->textBrowser->append("Turning right");//if direction 1 is not 1 the device is turning right
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

  file.flush();//transmits all accumulated characters to the file
  file.close();//closes file

}

/*
 * Get the rotation degree data from the camera accelerometer
 */
void MainWindow::cameradeg(const QString & path)//camera degree
{
    QFile file(path);//provides interface for reading and writing to files
    cStream->addPath(path);

  file.open(QIODevice::ReadOnly | QIODevice::Text);

    QTextStream stream(&file);
    QString data;
    int datanum;

    //Get newest info
    while(!stream.atEnd())
    {
        data = stream.readLine();//read line from file

    }
    datanum = data.toInt() % 360;//turn last line into integer and get remainder of dividing by 360

    //Output turns to feedback
    if(datanum == 359 && vRotate == 0)
    {
        if(direction2 != -1)
        {
            ui->textBrowser->append("Looking left");//if direction 2 is not -1 the device is looking left
            direction2 = -1;
        }
        else
            ui->textBrowser->insertPlainText(".");
    }
    else if(datanum == 0 && vRotate == 359)
    {
        if(direction2 == 1)
        {
            ui->textBrowser->append("Looking right");//if direction 1 is 1 the device is looking right
            direction2 = 1;
        }
        else
            ui->textBrowser->insertPlainText(".");
    }
    else if(datanum - vRotate < 0) //Looking left
    {
        if(direction2 != -1)
        {
            ui->textBrowser->append("Looking left");//if direction 2 is not -1 the device is looking left
            direction2 = -1;
        }
        else
            ui->textBrowser->insertPlainText(".");
    }
    else if(datanum - vRotate > 0)  //Looking right
    {
        if(direction2 != 1)
        {
            ui->textBrowser->append("Looking right");//if direction 2 is not 1 the device is looking right
            direction2 = 1;
        }
        else
            ui->textBrowser->insertPlainText(".");
    }


    //Rotate degree vals
    cRotate = datanum;

  file.flush();//transmits all accumulated characters to the file
  file.close();//closes file

}

/*
 * Get the coordinates from the GPS
 */
void MainWindow::gpscoord(const QString & path)//gps coordinates
{
    QFile file(path);//provides interface for reading and writing to files
    gps->addPath(path);

  file.open(QIODevice::ReadOnly | QIODevice::Text);

    QTextStream stream(&file);
    QString data;

    //Get newest info
    while(!stream.atEnd())
    {
        data = stream.readLine();//read line from file

    }

    //GPS coordinate
    coordinate = data;//set last line from file as gps coordinate

    ui->textBrowser_2->setPlainText(coordinate);
    ui->textBrowser_2->setAlignment(Qt::AlignCenter);//centers vertically and horizontally

  file.flush();//transmits all accumulated characters to the file
  file.close();//closes file

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
    args << "192.168.1.102"<<"8888"<<"640"<<"480";//creates list of strings
    process->start(temp, args, 0);//starts client program in a new process passing the command line arguments in args

    ui->textBrowser->append("Camera stream restarted.");
}

/*
 * Window actions
 */
void MainWindow::createActions()
{
    exit = new QAction(tr("Exit"), this);//constructs exit action
    exit->setShortcut(QKeySequence::Close);//close window using Ctrl+F4 or Ctrl+W
    connect(exit, SIGNAL(triggered()), this, SLOT(fExit()));

    info = new QAction(tr("Info"), this);//constructs info action
    info->setShortcut(QKeySequence::HelpContents);//open help contents with F1 on Windows and Ctrl+? on Mac OS X
    connect(info, SIGNAL(triggered()), this, SLOT(iInfo()));

    calibrate = new QAction(tr("Calibrate Vehicle"), this);//constructs calibrate vehicle action
    calibrate->setShortcut(Qt::Key_F5);//calibrate vehicle by hitting F5
    connect(calibrate, SIGNAL(triggered()), this, SLOT(eCalibrate()));

    saveFB = new QAction(tr("Save Feedback"), this);//constructs save feedback action
    saveFB->setShortcut(QKeySequence::Save);//save feedback using Ctrl+S
    connect(saveFB, SIGNAL(triggered()), this, SLOT(fSaveFB()));

    saveGPS = new QAction(tr("Save GPS Coordinate"), this);//constructs save GPS coordinate action
    saveGPS->setShortcut(Qt::Key_F6);//save GPS coordinate using F6
    connect(saveGPS, SIGNAL(triggered()), this, SLOT(fSaveGPS()));

}

/*
 * Method to add new menu options
 */
void MainWindow::createMenus()
{
    file = menuBar()->addMenu(tr("File"));//add file menu to menu bar
    file->addAction(saveFB);//add save feedback under file menu
    file->addAction(saveGPS);//add save gps under file menu
    file->addAction(exit);//add exit under file menu

    edit = menuBar()->addMenu(tr("Edit"));//add edit menu to menu bar
    edit->addAction(calibrate);//add calibrate under edit menu


    about = menuBar()->addMenu(tr("Help"));//add help menu to menu bar
    about->addAction(info);//add info under help menu

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
    out << ui->textBrowser->document()->toPlainText();//write current feedback stream to output file
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
    for(QStringList::iterator it = gpslist.begin(); it != gpslist.end(); ++it)//prints coordinates to output file
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

    msgBox->setText(helpBox);//writes information to help menu

    //msgBox->setIcon(QMessageBox::Question);
    msgBox->setStandardButtons(QMessageBox::Ok);//sets OK button
    msgBox->exec();//shows the message block
}
