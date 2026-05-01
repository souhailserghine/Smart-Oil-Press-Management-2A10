#pragma once

#include <QByteArray>
#include <QMap>
#include <QString>

#include <opencv2/core.hpp>
#include <opencv2/objdetect.hpp>

// A tiny service that owns OpenCV face models + the embeddings cache.
// This keeps facial-recognition code out of MainWindow.
class FaceRecognitionService final
{
public:
    FaceRecognitionService();

    // Load YuNet + SFace models from relative paths (same behavior as before).
    // Safe to call multiple times.
    void ensureModelsLoaded();

    // Encode the largest face in 'imagePath' → 128-float blob, empty on failure.
    QByteArray encodeFaceFromFile(const QString& imagePath);

    // Load all stored embeddings from DB into cache.
    void loadFaceEmbeddings();

    // Attempt to match 'embedding' against cache; returns id_emp or -1.
    int matchFaceEmbedding(const cv::Mat& embedding) const;

    // Same as matchFaceEmbedding(), but takes the raw float blob (1xN float32).
    int matchFaceEmbeddingBlob(const QByteArray& embeddingBlob) const;

    bool isAvailable() const;

    // Accessors for live-camera workflows (ownership stays in the service).
    // These return null if models aren't available.
    cv::Ptr<cv::FaceDetectorYN> faceDetector() const { return m_faceDetector; }
    cv::Ptr<cv::FaceRecognizerSF> faceRecognizer() const { return m_faceRecognizer; }

private:
    bool m_modelsAttempted = false;
    bool m_available = false;

    cv::Ptr<cv::FaceDetectorYN>   m_faceDetector;
    cv::Ptr<cv::FaceRecognizerSF> m_faceRecognizer;

    // id_emp → 128-float Mat loaded from EMPLOYE.modele_faciale
    QMap<int, cv::Mat> m_faceEmbeddings;
};
