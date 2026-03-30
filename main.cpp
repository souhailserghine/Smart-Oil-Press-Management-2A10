#include "mainwindow.h"
#include "connection.h"

#include <QApplication>
#include <QMessageBox>
#include <QFile>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // ✅ Apply QSS from resources (style.qrc فيه style.qss و prefix="/")
    QFile f(":/style.qss");
    if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        a.setStyleSheet(f.readAll());
        f.close();
    }

    bool ok = Connection::create(); // ما نوقفوش الواجهة حتى لو fail

    MainWindow w;
    w.show();

    if(!ok){
        QMessageBox::warning(&w, "DB",
                             "Connexion DB echouee.\n"
                             "L'interface tdhhor ama insert/select ma bech yekhdem.");
    }

    return a.exec();
}
