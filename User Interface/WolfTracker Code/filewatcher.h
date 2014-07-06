#ifndef FILEWATCHER_H
#define FILEWATCHER_H

#include <QWidget>
#include <QMessageBox>
#include <QFile>
#include <QTextStream>
#include "mainwindow.h"


class FileWatcher: public QWidget
{
  Q_OBJECT

public:
  FileWatcher(QWidget* parent=0)
    :QWidget(parent){}

  ~FileWatcher(){}

public slots:
  void showModified(const QString& str, MainWindow ui)
  {
    //Feedback loop for constant reading
    QFile file(str);

    if(file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
      QTextStream stream(&file);

      ui->textBrowser->setText(stream.readAll());
      file.close();
    }
  
  }
};

#endif //FILEWATCHER_H

