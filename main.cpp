#include "mainwindow.h"
#include <QMessageBox>
#include <QApplication>
#include <QFile>
#include "connection.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    
    // Load and apply stylesheet
    QFile styleFile(":/style.qss");
    if (styleFile.open(QFile::ReadOnly)) {
        QString style = QLatin1String(styleFile.readAll());
        a.setStyleSheet(style);
        styleFile.close();
    }
    
    MainWindow w;
    Connection c;
    bool test = c.createconnect();
    if (test) {
        w.show();
        QMessageBox::information(nullptr, QObject::tr("database is open"),
                                 QObject::tr("connection successful.\n"
                                             "Click Cancel to exit."), QMessageBox::Cancel);
    } else {
        QMessageBox::critical(nullptr, QObject::tr("database is not open"),
                              QObject::tr("connection failed.\n"
                                          "Click Cancel to exit."), QMessageBox::Cancel);
    }
    return a.exec();
}





