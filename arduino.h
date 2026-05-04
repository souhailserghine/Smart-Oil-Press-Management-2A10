#ifndef ARDUINO_H
#define ARDUINO_H

#include <QSerialPort>
#include <QSerialPortInfo>
#include <QDebug>

class Arduino
{
public:
    Arduino();

    int connect_arduino();
    int connect_arduino_port(const QString& portName);
    int close_arduino();

    void       write_to_arduino(QByteArray data);
    QByteArray read_from_arduino();

    QSerialPort* getserial();
    QString      getarduino_port_name();
    bool         isAvailable() const { return arduino_is_available; }

private:
    QSerialPort* serial;

    static const quint16 arduino_uno_vendor_id  = 9025;
    static const quint16 arduino_uno_product_id = 67;

    QString    arduino_port_name;
    bool       arduino_is_available;
    QByteArray data;

    int ouvrirPort();   // ouvre le port une fois le nom fixé
};

#endif // ARDUINO_H
