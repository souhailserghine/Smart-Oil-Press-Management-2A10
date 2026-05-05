#pragma once

#include <QDialog>

class QLabel;
class QPushButton;
class QTimer;

class FaceRecognitionService;

// Live webcam recognition dialog.
// - Shows camera preview.
// - Detects faces continuously.
// - When a known employee is recognized, it pauses and asks for confirmation.
class FaceRecognitionDialog final : public QDialog
{
    Q_OBJECT

public:
    explicit FaceRecognitionDialog(FaceRecognitionService* service, QWidget* parent = nullptr);
    ~FaceRecognitionDialog() override;

    // Runs modally and returns matched employee id (or -1 if canceled / not found).
    int execAndGetMatchedId();

private:
    void startCamera();
    void stopCamera();
    void onTick();

    FaceRecognitionService* m_service = nullptr;

    QLabel* m_camLabel = nullptr;
    QLabel* m_statusLabel = nullptr;
    QPushButton* m_confirmBtn = nullptr;
    QPushButton* m_cancelBtn = nullptr;

    QTimer* m_timer = nullptr;

    int m_candidateId = -1;

    // OpenCV objects are kept in the .cpp to avoid leaking OpenCV into headers.
    struct Impl;
    Impl* m_impl = nullptr;
};
