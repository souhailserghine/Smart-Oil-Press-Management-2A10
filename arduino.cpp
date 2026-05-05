#include "arduino.h"

#include <QList>

namespace {
static QString portText(const QSerialPortInfo& info)
{
    return (info.portName() + QLatin1Char(' ') +
            info.description() + QLatin1Char(' ') +
            info.manufacturer() + QLatin1Char(' ') +
            info.serialNumber()).toLower();
}

static bool isLikelyArduinoPort(const QSerialPortInfo& info)
{
    const QString text = portText(info);

    if (info.hasVendorIdentifier() && info.hasProductIdentifier()) {
        const quint16 vid = info.vendorIdentifier();
        const quint16 pid = info.productIdentifier();

        if (vid == 9025 && pid == 67) return true;      // Official Arduino Uno R3: 2341:0043
        if (vid == 9025) return true;                   // Other official Arduino boards
        if (vid == 6790 && pid == 29987) return true;   // CH340 / CH341 clones: 1A86:7523
        if (vid == 1027 && pid == 24577) return true;   // FTDI USB serial: 0403:6001
        if (vid == 4292) return true;                   // Silicon Labs CP210x: 10C4:xxxx
    }

    return text.contains(QStringLiteral("arduino")) ||
           text.contains(QStringLiteral("ch340")) ||
           text.contains(QStringLiteral("ch341")) ||
           text.contains(QStringLiteral("cp210")) ||
           text.contains(QStringLiteral("ftdi")) ||
           text.contains(QStringLiteral("usb serial")) ||
           text.contains(QStringLiteral("usb-serial")) ||
           text.contains(QStringLiteral("wch"));
}
}

Arduino::Arduino()
{
    data = "";
    arduino_port_name = "";
    arduino_is_available = false;
    serial = new QSerialPort;
}

QString Arduino::getarduino_port_name()
{
    return arduino_port_name;
}

QSerialPort *Arduino::getserial()
{
   return serial;
}

int Arduino::connect_arduino()
{
    if (serial->isOpen()) {
        return 0;
    }

    arduino_is_available = false;
    arduino_port_name.clear();

    const QList<QSerialPortInfo> ports = QSerialPortInfo::availablePorts();
    qDebug() << "[Arduino] Serial ports found:" << ports.size();

    QSerialPortInfo fallbackPort;
    for (const QSerialPortInfo& info : ports) {
        qDebug() << "[Arduino] Port" << info.portName()
                 << "description=" << info.description()
                 << "manufacturer=" << info.manufacturer()
                 << "VID=" << (info.hasVendorIdentifier() ? QString::number(info.vendorIdentifier(), 16) : QStringLiteral("none"))
                 << "PID=" << (info.hasProductIdentifier() ? QString::number(info.productIdentifier(), 16) : QStringLiteral("none"));

        if (fallbackPort.isNull()) {
            fallbackPort = info;
        }

        if (isLikelyArduinoPort(info)) {
            arduino_is_available = true;
            arduino_port_name = info.portName();
            break;
        }
    }

    // If there is exactly one serial port and it did not expose Arduino IDs,
    // try it anyway. This helps many clone boards on Windows.
    if (!arduino_is_available && ports.size() == 1 && !fallbackPort.isNull()) {
        arduino_is_available = true;
        arduino_port_name = fallbackPort.portName();
        qDebug() << "[Arduino] No Arduino VID/PID match; trying the only available port:" << arduino_port_name;
    }

    qDebug() << "[Arduino] selected port:" << arduino_port_name;

    if (!arduino_is_available || arduino_port_name.isEmpty()) {
        return -1;
    }

    serial->setPortName(arduino_port_name);
    if (serial->open(QSerialPort::ReadWrite)) {
        serial->setBaudRate(QSerialPort::Baud9600); // capteur niveau d'eau
        serial->setDataBits(QSerialPort::Data8);
        serial->setParity(QSerialPort::NoParity);
        serial->setStopBits(QSerialPort::OneStop);
        serial->setFlowControl(QSerialPort::NoFlowControl);
        return 0;
    }

    qDebug() << "[Arduino] Failed to open" << arduino_port_name << ":" << serial->errorString();
    return 1;
}

// ─── Connexion directe par nom de port ───────────────────────────────────────
int Arduino::connect_arduino_port(const QString& portName)
{
    bool portExiste = false;
    foreach (const QSerialPortInfo &info, QSerialPortInfo::availablePorts()) {
        if (info.portName().compare(portName, Qt::CaseInsensitive) == 0) {
            portExiste = true;
            qDebug() << "[Arduino] Port" << portName << "trouvé :"
                     << info.description();
            break;
        }
    }

    if (!portExiste) {
        qDebug() << "[Arduino] Port" << portName << "introuvable.";
        qDebug() << "[Arduino] Ports disponibles :";
        foreach (const QSerialPortInfo &info, QSerialPortInfo::availablePorts())
            qDebug() << "  -" << info.portName() << ":" << info.description();
        return -1;
    }

    arduino_port_name    = portName;
    arduino_is_available = true;

    serial->setPortName(arduino_port_name);
    if (serial->open(QSerialPort::ReadWrite)) {
        serial->setBaudRate(QSerialPort::Baud9600);
        serial->setDataBits(QSerialPort::Data8);
        serial->setParity(QSerialPort::NoParity);
        serial->setStopBits(QSerialPort::OneStop);
        serial->setFlowControl(QSerialPort::NoFlowControl);
        qDebug() << "[Arduino] Port ouvert avec succès :" << arduino_port_name;
        return 0;
    }

    qDebug() << "[Arduino] Impossible d'ouvrir le port :" << arduino_port_name
             << " — Erreur :" << serial->errorString();
    return 1;
}

int Arduino::close_arduino(){
    if (serial->isOpen()) {
        serial->close();
        return 0;
    }
    return 1;
}

QByteArray Arduino::read_from_arduino()
{
    if (serial->isReadable()) {
        data = serial->readAll();
        return data;
    }
    return QByteArray();
}

void Arduino::write_to_arduino(QByteArray d)
{
    if (serial->isWritable()) {
        serial->write(d);
    } else {
        qDebug() << "Couldn't write to serial!";
    }
}
