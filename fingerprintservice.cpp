#include "fingerprintservice.h"
#include <QSerialPort>
#include <QDebug>

FingerprintService::FingerprintService(QObject *parent)
    : QObject(parent), m_arduino(nullptr), m_reconnectTimer(nullptr)
{
}

FingerprintService::~FingerprintService()
{
    if (m_arduino) {
        m_arduino->close_arduino();
        delete m_arduino;
    }
    if (m_reconnectTimer) {
        m_reconnectTimer->stop();
        delete m_reconnectTimer;
    }
}

void FingerprintService::initialize()
{
    // Create Arduino wrapper if not already done.
    if (!m_arduino) {
        m_arduino = new Arduino();
    }

    // Already connected: do not close/reopen the port on every reconnect tick.
    if (isConnected()) {
        if (m_reconnectTimer && m_reconnectTimer->isActive()) {
            m_reconnectTimer->stop();
        }
        connect(m_arduino->getserial(), &QSerialPort::readyRead,
                this, &FingerprintService::onArduinoDataAvailable, Qt::UniqueConnection);
        sendCommand(QStringLiteral("LOGIN_ON"));
        emit ready();
        return;
    }

    // Attempt connection.
    int rc = m_arduino->connect_arduino();
    if (rc != 0) {
        emit error(QStringLiteral("Arduino fingerprint terminal not connected (rc=%1). Check USB cable, COM port, and firmware baud rate 115200.").arg(rc));
        // Start reconnect timer so plugging the board after app startup still works.
        if (!m_reconnectTimer) {
            m_reconnectTimer = new QTimer(this);
            connect(m_reconnectTimer, &QTimer::timeout, this, &FingerprintService::onReconnectTimeout);
        }
        if (!m_reconnectTimer->isActive()) {
            m_reconnectTimer->start(RECONNECT_INTERVAL_MS);
        }
        return;
    }

    if (m_reconnectTimer && m_reconnectTimer->isActive()) {
        m_reconnectTimer->stop();
    }

    // Connect to serial readyRead signal.
    connect(m_arduino->getserial(), &QSerialPort::readyRead,
            this, &FingerprintService::onArduinoDataAvailable, Qt::UniqueConnection);

    qDebug() << "[FingerprintService] Connected to Arduino on" << m_arduino->getarduino_port_name();

    // Send LOGIN_ON to start scanning.
    sendCommand(QStringLiteral("LOGIN_ON"));
    emit ready();
}

void FingerprintService::sendCommand(const QString &command)
{
    if (!isConnected()) {
        qWarning() << "[FingerprintService] Cannot send command, not connected:" << command;
        emit error(QStringLiteral("Arduino not connected"));
        return;
    }

    QByteArray payload = command.toUtf8() + "\n";
    m_arduino->write_to_arduino(payload);
    qDebug() << "[FingerprintService] Sent:" << command;
}

void FingerprintService::startScanning()
{
    if (!isConnected()) {
        sendCommand(QStringLiteral("LOGIN_ON"));
        return;
    }
    sendCommand(QStringLiteral("LOGIN_ON"));
    m_scanning = true;
    emit scanningStateChanged(true);
}

void FingerprintService::stopScanning()
{
    if (!isConnected()) {
        sendCommand(QStringLiteral("LOGIN_OFF"));
        return;
    }
    sendCommand(QStringLiteral("LOGIN_OFF"));
    m_scanning = false;
    emit scanningStateChanged(false);
}

void FingerprintService::requestEnrollment(int preferredId)
{
    if (m_enrollmentInProgress > 0) {
        qWarning() << "[FingerprintService] Enrollment already in progress";
        return;
    }

    if (preferredId > 0) {
        m_enrollmentInProgress = preferredId;
        sendCommand(QStringLiteral("ENROLL:%1").arg(preferredId));
    } else {
        m_enrollmentInProgress = 0; // Signal to find free slot on Arduino
        sendCommand(QStringLiteral("ENROLL"));
    }
}

void FingerprintService::deleteFingerprint(int fingerprintId)
{
    if (fingerprintId <= 0 || fingerprintId > 127) {
        emit error(QStringLiteral("Invalid fingerprint ID: %1").arg(fingerprintId));
        return;
    }
    sendCommand(QStringLiteral("DELETE:%1").arg(fingerprintId));
}

void FingerprintService::sendName(const QString &name)
{
    sendCommand(QStringLiteral("NAME:") + name.left(16));
}

void FingerprintService::sendDenied()
{
    sendCommand(QStringLiteral("DENIED"));
}

void FingerprintService::onArduinoDataAvailable()
{
    QByteArray data = m_arduino->read_from_arduino();
    if (data.isEmpty()) return;

    m_rxBuffer.append(data);

    // Process complete lines (delimited by \n)
    int eolIndex = -1;
    while ((eolIndex = m_rxBuffer.indexOf('\n')) >= 0) {
        QString line = QString::fromUtf8(m_rxBuffer.left(eolIndex)).trimmed();
        m_rxBuffer.remove(0, eolIndex + 1);

        if (!line.isEmpty()) {
            processLine(line);
        }
    }
}

void FingerprintService::onReconnectTimeout()
{
    if (isConnected()) {
        if (m_reconnectTimer) {
            m_reconnectTimer->stop();
        }
        return;
    }

    qDebug() << "[FingerprintService] Attempting reconnect...";
    initialize();
}

void FingerprintService::processLine(const QString &line)
{
    qDebug() << "[FingerprintService] Received:" << line;

    // Boot/info messages (ignore or log)
    if (line == QStringLiteral("READY") || line == QStringLiteral("PONG")) {
        return;
    }

    // Diagnostic lines (ignore)
    if (line.startsWith(QStringLiteral("DBG:"))) {
        return;
    }

    // Template count
    if (line.startsWith(QStringLiteral("TEMPLATES:"))) {
        bool ok = false;
        int count = line.mid(10).toInt(&ok);
        if (ok) {
            emit templatesCountReceived(count);
        }
        return;
    }

    // Error from Arduino
    if (line.startsWith(QStringLiteral("ERR:"))) {
        emit error(line);
        return;
    }

    // Fingerprint matched
    if (line.startsWith(QStringLiteral("MATCH:"))) {
        int id = extractIdFromMessage(line);
        if (id > 0) {
            emit matched(id);
        }
        return;
    }

    // Enrollment result
    if (line.startsWith(QStringLiteral("ENROLL_OK:"))) {
        int id = extractIdFromMessage(line);
        m_enrollmentInProgress = -1;
        if (id > 0) {
            emit enrollmentResult(true, id, QString());
        }
        return;
    }

    if (line.startsWith(QStringLiteral("ENROLL_FAIL"))) {
        QString reason = line.mid(11); // Skip "ENROLL_FAIL:"
        if (reason.isEmpty()) reason = QStringLiteral("Unknown error");
        m_enrollmentInProgress = -1;
        emit enrollmentResult(false, -1, reason);
        return;
    }

    // Deletion result
    if (line.startsWith(QStringLiteral("DELETE_OK:"))) {
        int id = extractIdFromMessage(line);
        if (id > 0) {
            emit deletionResult(id, true);
        }
        return;
    }

    if (line.startsWith(QStringLiteral("DELETE_FAIL:"))) {
        int id = extractIdFromMessage(line);
        if (id > 0) {
            emit deletionResult(id, false);
        }
        return;
    }

    // Unknown line
    qWarning() << "[FingerprintService] Unknown message:" << line;
}

int FingerprintService::extractIdFromMessage(const QString &message)
{
    // Format: "MATCH:123" or "ENROLL_OK:45"
    int colonPos = message.indexOf(':');
    if (colonPos < 0) return -1;

    QString idStr = message.mid(colonPos + 1);
    // Handle sub-ids like "ENROLL_OK:45" (take just the number)
    int secondColon = idStr.indexOf(':');
    if (secondColon > 0) {
        idStr = idStr.left(secondColon);
    }

    bool ok = false;
    int id = idStr.toInt(&ok);
    return ok ? id : -1;
}
