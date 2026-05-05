#include "face_capture_dialog.h"

#include "face_recognition_service.h"

#include <QHBoxLayout>
#include <QImage>
#include <QLabel>
#include <QPushButton>
#include <QTimer>
#include <QVBoxLayout>

#include <opencv2/imgproc.hpp>
#include <opencv2/videoio.hpp>

struct FaceCaptureDialog::Impl {
    cv::VideoCapture cap;
    cv::Mat lastFrame;
    cv::Mat lastFaces;
    bool faceVisible = false;
};

FaceCaptureDialog::FaceCaptureDialog(FaceRecognitionService* service, QWidget* parent)
    : QDialog(parent), m_service(service), m_impl(new Impl)
{
    setWindowTitle(tr("Capture du visage"));
    resize(700, 600);

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(12, 12, 12, 12);
    root->setSpacing(10);

    m_camLabel = new QLabel(this);
    m_camLabel->setAlignment(Qt::AlignCenter);
    m_camLabel->setMinimumSize(640, 480);
    m_camLabel->setStyleSheet("background:#111; border-radius: 6px;");
    root->addWidget(m_camLabel, 1);

    m_statusLabel = new QLabel(tr("Ouverture de la caméra…"), this);
    m_statusLabel->setAlignment(Qt::AlignCenter);
    root->addWidget(m_statusLabel);

    auto* btnRow = new QHBoxLayout();
    btnRow->addStretch();

    m_captureBtn = new QPushButton(tr("Capturer"), this);
    m_captureBtn->setEnabled(false);
    m_cancelBtn = new QPushButton(tr("Annuler"), this);

    btnRow->addWidget(m_captureBtn);
    btnRow->addWidget(m_cancelBtn);
    root->addLayout(btnRow);

    m_timer = new QTimer(this);

    connect(m_timer, &QTimer::timeout, this, &FaceCaptureDialog::onTick);
    connect(m_cancelBtn, &QPushButton::clicked, this, [this]() {
        m_result.clear();
        reject();
    });
    connect(m_captureBtn, &QPushButton::clicked, this, &FaceCaptureDialog::captureNow);
}

FaceCaptureDialog::~FaceCaptureDialog()
{
    stopCamera();
    delete m_impl;
    m_impl = nullptr;
}

QByteArray FaceCaptureDialog::execAndGetEmbeddingBlob()
{
    m_result.clear();
    m_captureBtn->setEnabled(false);

    if (!m_service) {
        m_statusLabel->setText(tr("Service indisponible."));
        return {};
    }

    m_service->ensureModelsLoaded();
    if (!m_service->isAvailable()) {
        m_statusLabel->setText(tr("Modèles indisponibles."));
        return {};
    }

    startCamera();
    if (!m_impl->cap.isOpened()) {
        m_statusLabel->setText(tr("Impossible d'ouvrir la webcam (index 0)."));
        return {};
    }

    m_timer->start(30);
    const int rc = exec();

    m_timer->stop();
    stopCamera();

    if (rc != QDialog::Accepted) return {};
    return m_result;
}

void FaceCaptureDialog::startCamera()
{
    if (m_impl->cap.isOpened()) return;
    m_impl->cap.open(0);
}

void FaceCaptureDialog::stopCamera()
{
    if (m_timer) m_timer->stop();
    if (m_impl && m_impl->cap.isOpened()) {
        m_impl->cap.release();
    }
}

void FaceCaptureDialog::onTick()
{
    if (!m_impl->cap.isOpened()) return;

    cv::Mat frame;
    m_impl->cap >> frame;
    if (frame.empty()) return;

    auto detector = m_service->faceDetector();
    auto recognizer = m_service->faceRecognizer();
    if (!detector || !recognizer) {
        m_statusLabel->setText(tr("Modèles non chargés."));
        return;
    }

    detector->setInputSize(frame.size());

    cv::Mat faces;
    detector->detect(frame, faces);

    m_impl->lastFrame = frame.clone();
    m_impl->lastFaces = faces.clone();

    const bool faceVisible = (faces.rows > 0);
    m_impl->faceVisible = faceVisible;
    m_captureBtn->setEnabled(faceVisible);

    // Draw boxes
    for (int i = 0; i < faces.rows; ++i) {
        const int fx = static_cast<int>(faces.at<float>(i, 0));
        const int fy = static_cast<int>(faces.at<float>(i, 1));
        const int fw = static_cast<int>(faces.at<float>(i, 2));
        const int fh = static_cast<int>(faces.at<float>(i, 3));
        cv::rectangle(frame, cv::Rect(fx, fy, fw, fh), cv::Scalar(0, 220, 80), 2);
    }

    m_statusLabel->setText(faceVisible
        ? tr("✔ Visage détecté — cliquez sur « Capturer »")
        : tr("Aucun visage détecté — repositionnez-vous"));

    cv::Mat rgb;
    cv::cvtColor(frame, rgb, cv::COLOR_BGR2RGB);
    QImage qimg(rgb.data, rgb.cols, rgb.rows, static_cast<int>(rgb.step), QImage::Format_RGB888);
    m_camLabel->setPixmap(QPixmap::fromImage(qimg.copy()).scaled(
        m_camLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
}

void FaceCaptureDialog::captureNow()
{
    if (!m_impl->faceVisible) return;
    if (m_impl->lastFrame.empty() || m_impl->lastFaces.empty() || m_impl->lastFaces.rows <= 0) return;

    auto recognizer = m_service->faceRecognizer();
    if (!recognizer) return;

    // Use first detected face
    cv::Mat aligned, embedding;
    recognizer->alignCrop(m_impl->lastFrame, m_impl->lastFaces.row(0), aligned);
    recognizer->feature(aligned, embedding);
    if (embedding.empty()) return;

    m_result = QByteArray(reinterpret_cast<const char*>(embedding.data),
                          static_cast<int>(embedding.total() * embedding.elemSize()));

    accept();
}
