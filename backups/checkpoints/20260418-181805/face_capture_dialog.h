#pragma once

#include <QDialog>
#include <QByteArray>

class QLabel;
class QPushButton;
class QTimer;

class FaceRecognitionService;

// Live webcam capture dialog.
// - Shows camera preview.
// - Enables "Capture" when a face is detected.
// - On capture, returns the face embedding blob (float32 bytes).
class FaceCaptureDialog final : public QDialog
{
    Q_OBJECT

public:
    explicit FaceCaptureDialog(FaceRecognitionService* service, QWidget* parent = nullptr);
    ~FaceCaptureDialog() override;

    // Runs modally and returns the captured embedding blob (or empty if canceled).
    QByteArray execAndGetEmbeddingBlob();

private:
    void startCamera();
    void stopCamera();
    void onTick();
    void captureNow();

    FaceRecognitionService* m_service = nullptr;

    QLabel* m_camLabel = nullptr;
    QLabel* m_statusLabel = nullptr;
    QPushButton* m_captureBtn = nullptr;
    QPushButton* m_cancelBtn = nullptr;

    QTimer* m_timer = nullptr;

    QByteArray m_result;

    struct Impl;
    Impl* m_impl = nullptr;
};
