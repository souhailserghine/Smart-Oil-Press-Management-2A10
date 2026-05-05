#include "face_recognition_dialog.h"

#include "face_recognition_service.h"

#include <QHBoxLayout>
#include <QImage>
#include <QLabel>
#include <QPushButton>
#include <QTimer>
#include <QVBoxLayout>

#ifdef OPENCV_AVAILABLE
#include <opencv2/imgproc.hpp>
#include <opencv2/videoio.hpp>

struct FaceRecognitionDialog::Impl {
    cv::VideoCapture cap;
};
#else
struct FaceRecognitionDialog::Impl {};
#endif

FaceRecognitionDialog::FaceRecognitionDialog(FaceRecognitionService* service, QWidget* parent)
    : QDialog(parent), m_service(service), m_impl(new Impl)
{
    setWindowTitle(tr("Reconnaissance Faciale"));
    resize(700, 600);

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(12, 12, 12, 12);
    root->setSpacing(10);

    m_camLabel = new QLabel(this);
    m_camLabel->setAlignment(Qt::AlignCenter);
    m_camLabel->setMinimumSize(640, 480);
    m_camLabel->setStyleSheet("background:#000; border-radius: 6px;");
    root->addWidget(m_camLabel, 1);

    m_statusLabel = new QLabel(tr("Ouverture de la caméra…"), this);
    m_statusLabel->setAlignment(Qt::AlignCenter);
    root->addWidget(m_statusLabel);

    auto* btnRow = new QHBoxLayout();
    btnRow->addStretch();

    m_confirmBtn = new QPushButton(tr("Confirmer"), this);
    m_confirmBtn->setEnabled(false);
    m_cancelBtn = new QPushButton(tr("Annuler"), this);

    btnRow->addWidget(m_confirmBtn);
    btnRow->addWidget(m_cancelBtn);
    root->addLayout(btnRow);

    m_timer = new QTimer(this);

    connect(m_timer, &QTimer::timeout, this, &FaceRecognitionDialog::onTick);
    connect(m_cancelBtn, &QPushButton::clicked, this, [this]() {
        m_candidateId = -1;
        reject();
    });
    connect(m_confirmBtn, &QPushButton::clicked, this, [this]() {
        accept();
    });
}

FaceRecognitionDialog::~FaceRecognitionDialog()
{
    stopCamera();
    delete m_impl;
    m_impl = nullptr;
}

int FaceRecognitionDialog::execAndGetMatchedId()
{
    m_candidateId = -1;
    m_confirmBtn->setEnabled(false);

    if (!m_service) {
        m_statusLabel->setText(tr("Service de reconnaissance faciale indisponible."));
        return -1;
    }

    m_service->ensureModelsLoaded();
    if (!m_service->isAvailable()) {
        m_statusLabel->setText(tr("Modèles de reconnaissance faciale indisponibles."));
        return -1;
    }

    // Fresh cache from DB.
    m_service->loadFaceEmbeddings();

    startCamera();
#ifdef OPENCV_AVAILABLE
    if (!m_impl->cap.isOpened()) {
        m_statusLabel->setText(tr("Impossible d'ouvrir la webcam (index 0)."));
        return -1;
    }
#endif

    m_timer->start(30);
    const int rc = exec();

    m_timer->stop();
    stopCamera();

    if (rc != QDialog::Accepted) return -1;
    return m_candidateId;
}

void FaceRecognitionDialog::startCamera()
{
#ifdef OPENCV_AVAILABLE
    if (m_impl->cap.isOpened()) return;
    m_impl->cap.open(0);
#endif
}

void FaceRecognitionDialog::stopCamera()
{
    if (m_timer) m_timer->stop();
#ifdef OPENCV_AVAILABLE
    if (m_impl && m_impl->cap.isOpened()) {
        m_impl->cap.release();
    }
#endif
}

void FaceRecognitionDialog::onTick()
{
#ifdef OPENCV_AVAILABLE
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

    bool foundAny = (faces.rows > 0);
    bool matched = false;

    for (int i = 0; i < faces.rows; ++i) {
        cv::Mat aligned, embedding;
        recognizer->alignCrop(frame, faces.row(i), aligned);
        recognizer->feature(aligned, embedding);
        if (embedding.empty()) continue;

        const QByteArray blob(reinterpret_cast<const char*>(embedding.data),
                              static_cast<int>(embedding.total() * embedding.elemSize()));

        const int id = m_service->matchFaceEmbeddingBlob(blob);

        const int fx = static_cast<int>(faces.at<float>(i, 0));
        const int fy = static_cast<int>(faces.at<float>(i, 1));
        const int fw = static_cast<int>(faces.at<float>(i, 2));
        const int fh = static_cast<int>(faces.at<float>(i, 3));

        if (id > 0) {
            cv::rectangle(frame, cv::Rect(fx, fy, fw, fh), cv::Scalar(0, 220, 0), 3);
            m_candidateId = id;
            matched = true;
            break;
        } else {
            cv::rectangle(frame, cv::Rect(fx, fy, fw, fh), cv::Scalar(0, 200, 220), 2);
        }
    }

    if (matched) {
        m_timer->stop();
        m_confirmBtn->setEnabled(true);
        m_statusLabel->setText(tr("Visage reconnu. Confirmer la connexion ?"));
    } else {
        m_confirmBtn->setEnabled(false);
        if (!foundAny)
            m_statusLabel->setText(tr("Aucun visage détecté…"));
        else
            m_statusLabel->setText(tr("Visage détecté, recherche en cours…"));
    }

    cv::Mat rgb;
    cv::cvtColor(frame, rgb, cv::COLOR_BGR2RGB);
    QImage qimg(rgb.data, rgb.cols, rgb.rows, static_cast<int>(rgb.step), QImage::Format_RGB888);
    m_camLabel->setPixmap(QPixmap::fromImage(qimg.copy()).scaled(
        m_camLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
#else
    m_statusLabel->setText(tr("Reconnaissance faciale non disponible (OpenCV absent)."));
#endif
}
