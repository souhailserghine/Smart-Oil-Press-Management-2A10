#include "face_recognition_service.h"

#include <cstring>

#include <QSqlQuery>
#include <QSqlError>

#ifdef OPENCV_AVAILABLE
#include <opencv2/objdetect.hpp>
#include <opencv2/dnn.hpp>
#include <opencv2/imgcodecs.hpp>
#endif

FaceRecognitionService::FaceRecognitionService() = default;

void FaceRecognitionService::ensureModelsLoaded()
{
#ifdef OPENCV_AVAILABLE
    if (m_modelsAttempted) return;
    m_modelsAttempted = true;

    const std::string detectorModel   = "face_detection_yunet_2023mar.onnx";
    const std::string recognizerModel = "face_recognition_sface_2021dec.onnx";

    try {
        m_faceDetector   = cv::FaceDetectorYN::create(detectorModel, "", cv::Size(320, 320));
        m_faceRecognizer = cv::FaceRecognizerSF::create(recognizerModel, "");
        m_available = true;
    } catch (const cv::Exception&) {
        m_available = false;
    }
#else
    m_modelsAttempted = true;
    m_available = false;
#endif
}

bool FaceRecognitionService::isAvailable() const
{
    return m_available;
}

QByteArray FaceRecognitionService::encodeFaceFromFile(const QString& imagePath)
{
#ifdef OPENCV_AVAILABLE
    ensureModelsLoaded();
    if (!m_available) return {};

    cv::Mat image = cv::imread(imagePath.toStdString());
    if (image.empty()) return {};

    m_faceDetector->setInputSize(image.size());

    cv::Mat faces;
    m_faceDetector->detect(image, faces);
    if (faces.empty() || faces.rows <= 0) return {};

    int bestIdx = 0;
    float bestArea = -1.f;
    for (int i = 0; i < faces.rows; ++i) {
        const float w = faces.at<float>(i, 2);
        const float h = faces.at<float>(i, 3);
        const float area = w * h;
        if (area > bestArea) {
            bestArea = area;
            bestIdx = i;
        }
    }

    cv::Rect faceRect(
        static_cast<int>(faces.at<float>(bestIdx, 0)),
        static_cast<int>(faces.at<float>(bestIdx, 1)),
        static_cast<int>(faces.at<float>(bestIdx, 2)),
        static_cast<int>(faces.at<float>(bestIdx, 3))
    );
    faceRect &= cv::Rect(0, 0, image.cols, image.rows);
    if (faceRect.width <= 0 || faceRect.height <= 0) return {};

    cv::Mat aligned;
    cv::Mat embedding;
    m_faceRecognizer->alignCrop(image, faces.row(bestIdx), aligned);
    m_faceRecognizer->feature(aligned, embedding);
    if (embedding.empty()) return {};

    const int bytes = embedding.total() * embedding.elemSize();
    return QByteArray(reinterpret_cast<const char*>(embedding.ptr<float>()), bytes);
#else
    Q_UNUSED(imagePath)
    return {};
#endif
}

void FaceRecognitionService::loadFaceEmbeddings()
{
#ifdef OPENCV_AVAILABLE
    m_faceEmbeddings.clear();

    QSqlQuery q;
    if (!q.exec("SELECT id_emp, modele_faciale FROM employe WHERE modele_faciale IS NOT NULL"))
        return;

    while (q.next()) {
        const int id = q.value(0).toInt();
        const QByteArray blob = q.value(1).toByteArray();
        if (blob.isEmpty()) continue;
        if (blob.size() % static_cast<int>(sizeof(float)) != 0) continue;

        const int count = blob.size() / static_cast<int>(sizeof(float));
        cv::Mat emb(1, count, CV_32F);
        std::memcpy(emb.data, blob.constData(), static_cast<size_t>(blob.size()));
        m_faceEmbeddings.insert(id, emb);
    }
#endif
}

#ifdef OPENCV_AVAILABLE
int FaceRecognitionService::matchFaceEmbedding(const cv::Mat& embedding) const
{
    if (embedding.empty()) return -1;
    if (!m_available || !m_faceRecognizer) return -1;
    if (m_faceEmbeddings.isEmpty()) return -1;

    int bestId = -1;
    double bestScore = -1e9;

    for (auto it = m_faceEmbeddings.constBegin(); it != m_faceEmbeddings.constEnd(); ++it) {
        const cv::Mat& ref = it.value();
        if (ref.empty() || ref.total() != embedding.total()) continue;

        const double score = m_faceRecognizer->match(ref, embedding, cv::FaceRecognizerSF::DisType::FR_COSINE);
        if (score > bestScore) {
            bestScore = score;
            bestId = it.key();
        }
    }

    return (bestScore >= 0.363) ? bestId : -1;
}
#endif

int FaceRecognitionService::matchFaceEmbeddingBlob(const QByteArray& embeddingBlob) const
{
#ifdef OPENCV_AVAILABLE
    if (embeddingBlob.isEmpty()) return -1;
    if (embeddingBlob.size() % static_cast<int>(sizeof(float)) != 0) return -1;

    const int count = embeddingBlob.size() / static_cast<int>(sizeof(float));
    cv::Mat emb(1, count, CV_32F);
    std::memcpy(emb.data, embeddingBlob.constData(), static_cast<size_t>(embeddingBlob.size()));
    return matchFaceEmbedding(emb);
#else
    Q_UNUSED(embeddingBlob)
    return -1;
#endif
}
