#ifndef PERSONNEL_H
#define PERSONNEL_H

#include <QMainWindow>

namespace Ui {
class personnel;
}

class personnel : public QMainWindow
{
    Q_OBJECT

public:
    explicit personnel(QWidget *parent = nullptr);
    ~personnel();

private:
    Ui::personnel *ui;
};

#endif // PERSONNEL_H
