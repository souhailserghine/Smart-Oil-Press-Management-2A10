#ifndef FINGERPRINTSERVICE_H
#define FINGERPRINTSERVICE_H

#include <QObject>
#include <QByteArray>
#include <QString>
#include <QTimer>
#include "arduino.h"

/**
 * @class FingerprintService
 * @brief High-level service for fingerprint scanning, enrollment, and recognition.
 *
 * This service encapsulates all fingerprint protocol handling, serial communication,
 * and state management. It provides a clean signal/slot interface for the UI and
 * delegates database operations to the model layer (Employe).
 *
 * MVC Layer: Model layer, but sits between Arduino (transport) and MainWindow (view).
 */
class FingerprintService : public QObject
{
    Q_OBJECT

public:
    explicit FingerprintService(QObject *parent = nullptr);
    ~FingerprintService();

    /**
     * Initialize the fingerprint service: open Arduino serial connection,
     * verify sensor, and start listening for serial data.
     * Emits ready() on success, error() on failure.
     */
    void initialize();

    /**
     * Send a command to the Arduino.
     * @param command e.g., "LOGIN_ON", "ENROLL", "DELETE:5"
     */
    void sendCommand(const QString &command);

    /**
     * Start fingerprint scanning (enable login mode).
     */
    void startScanning();

    /**
     * Stop fingerprint scanning.
     */
    void stopScanning();

    /**
     * Request enrollment of a new fingerprint.
     * @param preferredId Optional specific slot ID (1-127); if -1, auto-find.
     */
    void requestEnrollment(int preferredId = -1);

    /**
     * Delete a fingerprint from the sensor database.
     * @param fingerprintId Slot ID (1-127)
     */
    void deleteFingerprint(int fingerprintId);

    /**
     * Send employee name to Arduino for display on LCD.
     * @param name Employee name (will be truncated to 16 chars by Arduino)
     */
    void sendName(const QString &name);

    /**
     * Send access denied message to Arduino.
     */
    void sendDenied();

    /**
     * Get whether service is currently initialized and connected.
     */
    bool isConnected() const { return m_arduino != nullptr && m_arduino->getserial()->isOpen(); }

    /**
     * Get the Arduino wrapper (for advanced use).
     */
    Arduino* getArduino() { return m_arduino; }

signals:
    /**
     * Emitted when service is ready (sensor online, templates loaded).
     */
    void ready();

    /**
     * Emitted when template count is known.
     * @param count Number of enrolled fingerprints
     */
    void templatesCountReceived(int count);

    /**
     * Emitted when a fingerprint is matched during scanning.
     * @param fingerprintId Sensor template ID (1-127)
     */
    void matched(int fingerprintId);

    /**
     * Emitted when enrollment completes.
     * @param success True if enrollment successful
     * @param fingerprintId Assigned slot ID (valid only if success)
     * @param reason Error message if not successful
     */
    void enrollmentResult(bool success, int fingerprintId, const QString &reason);

    /**
     * Emitted when fingerprint deletion completes.
     * @param fingerprintId Deleted slot ID
     * @param success True if deletion successful
     */
    void deletionResult(int fingerprintId, bool success);

    /**
     * Emitted on any error (sensor failure, serial error, protocol error).
     * @param message Error description
     */
    void error(const QString &message);

    /**
     * Emitted when scanning state changes.
     * @param scanning True if now scanning, False if stopped
     */
    void scanningStateChanged(bool scanning);

    /**
     * Emitted when service detects a device disconnection.
     */
    void disconnected();

private slots:
    /**
     * Called when Arduino serial data is available.
     * Buffers data and processes complete lines.
     */
    void onArduinoDataAvailable();

    /**
     * Internal: Reconnection timer timeout.
     */
    void onReconnectTimeout();

private:
    /**
     * Parse a complete line from Arduino and emit appropriate signals.
     * @param line e.g., "READY", "TEMPLATES:5", "MATCH:10", "ENROLL_OK:5"
     */
    void processLine(const QString &line);

    /**
     * Validate and extract command parameters.
     */
    int extractIdFromMessage(const QString &message);

    Arduino *m_arduino = nullptr;
    QByteArray m_rxBuffer;
    QTimer *m_reconnectTimer = nullptr;

    // State
    bool m_scanning = false;
    int m_enrollmentInProgress = -1; // -1 = not enrolling, >0 = slot being enrolled

    // Configuration
    static constexpr int RECONNECT_INTERVAL_MS = 3000;
};

#endif // FINGERPRINTSERVICE_H
