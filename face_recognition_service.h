#pragma once

#include <QByteArray>
#include <QMap>
#include <QString>

#ifdef OPENCV_AVAILABLE
#include <opencv2/core.hpp>
#include <opencv2/objdetect.hpp>
#endif

// A tiny service that owns OpenCV face models + the embeddings cache.
// This keeps facial-recognition code out of MainWindow.
class FaceRecognitionService final
{
public:
    FaceRecognitionService();

    void ensureModelsLoaded();
    QByteArray encodeFaceFromFile(const QString& imagePath);
    void loadFaceEmbeddings();

#ifdef OPENCV_AVAILABLE
    int matchFaceEmbedding(const cv::Mat& embedding) const;
#endif
    int matchFaceEmbeddingBlob(const QByteArray& embeddingBlob) const;

    bool isAvailable() const;

#ifdef OPENCV_AVAILABLE
    cv::Ptr<cv::FaceDetectorYN>   faceDetector()   const { return m_faceDetector; }
    cv::Ptr<cv::FaceRecognizerSF> faceRecognizer() const { return m_faceRecognizer; }
#endif

private:
    bool m_modelsAttempted = false;
    bool m_available = false;

#ifdef OPENCV_AVAILABLE
    cv::Ptr<cv::FaceDetectorYN>   m_faceDetector;
    cv::Ptr<cv::FaceRecognizerSF> m_faceRecognizer;
    QMap<int, cv::Mat> m_faceEmbeddings;
#endif
};
