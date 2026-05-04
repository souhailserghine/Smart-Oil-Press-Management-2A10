#include "arduino.h"

// NOTE : on n'inclut plus QThread ici.
// Le délai de stabilisation Arduino (2 s) est géré côté Qt avec QTimer::singleShot()
// pour ne jamais bloquer le thread principal de l'interface.

Arduino::Arduino()
{
    data                 = "";
    arduino_port_name    = "";
    arduino_is_available = false;
    serial               = new QSerialPort;
}

QString Arduino::getarduino_port_name()
{
    return arduino_port_name;
}

QSerialPort* Arduino::getserial()
{
    return serial;
}

// ─── Ouvrir le port série ─────────────────────────────────────────────────────
// IMPORTANT : le QThread::msleep(2000) a été SUPPRIMÉ.
// Le reset de l'Arduino au démarrage est géré dans Citernes::on_btnConnecterArduino_clicked()
// via QTimer::singleShot(2000, ...) pour ne pas geler l'UI.
int Arduino::ouvrirPort()
{
    serial->setPortName(arduino_port_name);

    if (serial->open(QSerialPort::ReadWrite)) {
        serial->setBaudRate(QSerialPort::Baud9600);
        serial->setDataBits(QSerialPort::Data8);
        serial->setParity(QSerialPort::NoParity);
        serial->setStopBits(QSerialPort::OneStop);
        serial->setFlowControl(QSerialPort::NoFlowControl);
        arduino_is_available = true;

        // ── PAS de QThread::msleep ici ──
        // L'attente de stabilisation (2 s) est déléguée à un QTimer::singleShot
        // dans l'interface Qt afin de ne pas bloquer l'event loop.

        qDebug() << "[Arduino] Port ouvert avec succès :" << arduino_port_name;
        return 0;   // succès
    }

    qDebug() << "[Arduino] Impossible d'ouvrir le port :" << arduino_port_name
             << " — Erreur :" << serial->errorString();
    return 1;       // port trouvé mais ouverture échouée
}

// ─── Connexion automatique par VID/PID puis fallback ─────────────────────────
int Arduino::connect_arduino()
{
    // Étape 1 : recherche par VID/PID (Arduino UNO original)
    foreach (const QSerialPortInfo &info, QSerialPortInfo::availablePorts()) {
        qDebug() << "[Arduino] Port :" << info.portName()
        << " VID:" << info.vendorIdentifier()
        << " PID:" << info.productIdentifier()
        << " Desc:" << info.description();

        if (info.hasVendorIdentifier() && info.hasProductIdentifier()) {
            if (info.vendorIdentifier()  == arduino_uno_vendor_id &&
                info.productIdentifier() == arduino_uno_product_id) {
                arduino_port_name    = info.portName();
                arduino_is_available = true;
                qDebug() << "[Arduino] UNO original trouvé sur :" << arduino_port_name;
                return ouvrirPort();
            }
        }
    }

    // Étape 2 : fallback — chercher un clone CH340/FTDI par description
    foreach (const QSerialPortInfo &info, QSerialPortInfo::availablePorts()) {
        QString desc = info.description().toUpper();
        QString mfr  = info.manufacturer().toUpper();

        if (desc.contains("CH340")   || desc.contains("CH341")   ||
            desc.contains("ARDUINO") || desc.contains("USB-SERIAL") ||
            desc.contains("USB SERIAL") ||
            mfr.contains("CH340")    || mfr.contains("ARDUINO")  ||
            mfr.contains("WICED")    || mfr.contains("FTDI")) {

            arduino_port_name    = info.portName();
            arduino_is_available = true;
            qDebug() << "[Arduino] Clone détecté sur :" << arduino_port_name
                     << "(" << info.description() << ")";
            return ouvrirPort();
        }
    }

    // Étape 3 : fallback final sur COM5
    qDebug() << "[Arduino] Aucun Arduino trouvé — tentative directe sur COM5...";
    return connect_arduino_port("COM5");
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
    return ouvrirPort();
}

// ─── Fermeture ────────────────────────────────────────────────────────────────
int Arduino::close_arduino()
{
    if (serial->isOpen()) {
        serial->close();
        arduino_is_available = false;
        return 0;
    }
    return 1;
}

// ─── Lecture / Écriture ───────────────────────────────────────────────────────
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
        qDebug() << "[Arduino] Impossible d'écrire sur le port série !";
    }
}
