#include "face_recognition_service.h"

#include <cstring>

#include <QSqlQuery>
#include <QSqlError>

#include <opencv2/objdetect.hpp>
#include <opencv2/dnn.hpp>
#include <opencv2/imgcodecs.hpp>

FaceRecognitionService::FaceRecognitionService() = default;

void FaceRecognitionService::ensureModelsLoaded()
{
    if (m_modelsAttempted) return;
    m_modelsAttempted = true;

    // Relative to exe; copied by CMake POST_BUILD
    const std::string detectorModel   = "face_detection_yunet_2023mar.onnx";
    const std::string recognizerModel = "face_recognition_sface_2021dec.onnx";

    try {
        m_faceDetector   = cv::FaceDetectorYN::create(detectorModel, "", cv::Size(320, 320));
        m_faceRecognizer = cv::FaceRecognizerSF::create(recognizerModel, "");
        m_available = true;
    } catch (const cv::Exception&) {
        // Non-fatal — facial recognition will be unavailable but app still works
        m_available = false;
    }
}

bool FaceRecognitionService::isAvailable() const
{
    return m_available;
}

QByteArray FaceRecognitionService::encodeFaceFromFile(const QString& imagePath)
{
    ensureModelsLoaded();
    if (!m_available) return {};

    cv::Mat image = cv::imread(imagePath.toStdString());
    if (image.empty()) return {};

    // Match previous behavior: detect faces then encode the largest one.
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

    // Store as raw float bytes (same contract as before)
    const int bytes = embedding.total() * embedding.elemSize();
    return QByteArray(reinterpret_cast<const char*>(embedding.ptr<float>()), bytes);
}

void FaceRecognitionService::loadFaceEmbeddings()
{
    m_faceEmbeddings.clear();

    QSqlQuery q;
    if (!q.exec("SELECT id_emp, modele_faciale FROM employe WHERE modele_faciale IS NOT NULL")) {
        return;
    }

    while (q.next()) {
        const int id = q.value(0).toInt();
        const QByteArray blob = q.value(1).toByteArray();
        if (blob.isEmpty()) continue;

        // 128 floats expected
        if (blob.size() % static_cast<int>(sizeof(float)) != 0) continue;

        const int count = blob.size() / static_cast<int>(sizeof(float));
        cv::Mat emb(1, count, CV_32F);
        std::memcpy(emb.data, blob.constData(), static_cast<size_t>(blob.size()));
        m_faceEmbeddings.insert(id, emb);
    }
}

int FaceRecognitionService::matchFaceEmbedding(const cv::Mat& embedding) const
{
    if (embedding.empty()) return -1;
    if (!m_available || !m_faceRecognizer) return -1;
    if (m_faceEmbeddings.isEmpty()) return -1;

    // Minimal matching: cosine similarity (higher is better), keep same threshold behavior.
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

    // Conservative threshold; adjust if needed.
    return (bestScore >= 0.363) ? bestId : -1;
}

int FaceRecognitionService::matchFaceEmbeddingBlob(const QByteArray& embeddingBlob) const
{
    if (embeddingBlob.isEmpty()) return -1;
    if (embeddingBlob.size() % static_cast<int>(sizeof(float)) != 0) return -1;

    const int count = embeddingBlob.size() / static_cast<int>(sizeof(float));
    cv::Mat emb(1, count, CV_32F);
    std::memcpy(emb.data, embeddingBlob.constData(), static_cast<size_t>(embeddingBlob.size()));
    return matchFaceEmbedding(emb);
}
