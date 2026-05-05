#ifndef ARDUINO_H
#define ARDUINO_H
#include <QtSerialPort/QSerialPort>
#include <QtSerialPort/QSerialPortInfo>
#include <QDebug>
#include <QByteArray>
#include <QString>


class Arduino
{
public:     //méthodes de la classe Arduino
    Arduino();
    int connect_arduino(); // permet de connecter le PC à Arduino
    int connect_arduino_port(const QString& portName); // connexion directe par nom de port
    int close_arduino(); // permet de femer la connexion
    void write_to_arduino( QByteArray ); // envoyer des données vers arduino
    QByteArray read_from_arduino();  //recevoir des données de la carte Arduino
    QSerialPort* getserial();   // accesseur
    QString getarduino_port_name();
    bool isAvailable() const { return arduino_is_available; }
    static const quint16 arduino_uno_vendor_id=9025;
    static const quint16 arduino_uno_product_id=67;
private:
QSerialPort * serial; //Cet objet rassemble des informations (vitesse, bits de données, etc.)
//et des fonctions (envoi, lecture de réception,…) sur ce qu’est une voie série pour Arduino.
QString arduino_port_name;
bool arduino_is_available;
QByteArray data;  // contenant les données lues à partir d'Arduino
};



#endif // ARDUINO_H
