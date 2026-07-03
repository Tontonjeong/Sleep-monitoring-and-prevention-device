// ear.cpp — OpenCV Facemark LBF EAR engine used by run_ear.sh
// Build: g++ -O2 -std=c++17 src/ear.cpp -o ear `pkg-config --cflags --libs opencv4`
#include <opencv2/opencv.hpp>
#include <opencv2/face.hpp>
#include <iostream>
#include <vector>
#include <chrono>
#include <cmath>
#include <iomanip>

static double dist2d(const cv::Point2f& a, const cv::Point2f& b) {
    double dx = a.x - b.x, dy = a.y - b.y;
    return std::sqrt(dx * dx + dy * dy);
}

static double eye_aspect_ratio(const std::vector<cv::Point2f>& lm, int left_start) {
    // 68-point model: left eye 36-41, right eye 42-47.
    const cv::Point2f& p1 = lm[left_start + 0];
    const cv::Point2f& p2 = lm[left_start + 1];
    const cv::Point2f& p3 = lm[left_start + 2];
    const cv::Point2f& p4 = lm[left_start + 3];
    const cv::Point2f& p5 = lm[left_start + 4];
    const cv::Point2f& p6 = lm[left_start + 5];
    double denom = 2.0 * dist2d(p1, p4);
    if (denom <= 1e-6) return -1.0;
    return (dist2d(p2, p6) + dist2d(p3, p5)) / denom;
}

static long long now_ms() {
    using namespace std::chrono;
    return duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
}

int main(int argc, char** argv) {
    if (argc < 7) {
        std::cerr << "usage: " << argv[0]
                  << " <lbfmodel.yaml> <haarcascade.xml> <video_device> <ear_thr> <closed_sec> <show>\n";
        return 1;
    }
    std::string lbf_model = argv[1];
    std::string face_cascade_path = argv[2];
    std::string video_device = argv[3];
    double ear_thr = std::atof(argv[4]);
    double closed_sec = std::atof(argv[5]);
    int show = std::atoi(argv[6]);

    cv::CascadeClassifier face_cascade;
    if (!face_cascade.load(face_cascade_path)) {
        std::cerr << "cannot load cascade: " << face_cascade_path << "\n";
        return 1;
    }
    cv::Ptr<cv::face::Facemark> facemark = cv::face::FacemarkLBF::create();
    facemark->loadModel(lbf_model);

    cv::VideoCapture cap;
    if (video_device.rfind("/dev/video", 0) == 0) {
        int idx = std::atoi(video_device.substr(10).c_str());
        cap.open(idx);
    } else {
        cap.open(video_device);
    }
    if (!cap.isOpened()) {
        std::cerr << "cannot open camera: " << video_device << "\n";
        return 1;
    }

    long long closed_start = -1;
    while (true) {
        cv::Mat frame;
        if (!cap.read(frame) || frame.empty()) break;
        cv::Mat gray;
        cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);
        cv::equalizeHist(gray, gray);
        std::vector<cv::Rect> faces;
        face_cascade.detectMultiScale(gray, faces, 1.1, 3, 0, cv::Size(80, 80));

        double ear = -1.0;
        int eye_closed = 0;
        long long closed_ms = 0;
        int drowsy = 0;
        long long t = now_ms();

        if (!faces.empty()) {
            std::vector<std::vector<cv::Point2f>> landmarks;
            bool ok = facemark->fit(frame, faces, landmarks);
            if (ok && !landmarks.empty() && landmarks[0].size() >= 68) {
                double left = eye_aspect_ratio(landmarks[0], 36);
                double right = eye_aspect_ratio(landmarks[0], 42);
                ear = (left + right) / 2.0;
                eye_closed = (ear > 0.0 && ear < ear_thr) ? 1 : 0;
                if (eye_closed) {
                    if (closed_start < 0) closed_start = t;
                    closed_ms = t - closed_start;
                    drowsy = (closed_ms >= (long long)(closed_sec * 1000.0)) ? 1 : 0;
                } else {
                    closed_start = -1;
                }
                if (show) {
                    for (auto& p : landmarks[0]) cv::circle(frame, p, 2, cv::Scalar(0, 255, 0), -1);
                }
            }
        } else {
            closed_start = -1;
        }

        if (show) {
            cv::putText(frame, cv::format("EAR: %.4f", ear), {20, 40}, cv::FONT_HERSHEY_SIMPLEX, 0.8, {255, 255, 255}, 2);
            if (drowsy) cv::putText(frame, "DROWSY!", {20, 80}, cv::FONT_HERSHEY_SIMPLEX, 1.0, {0, 0, 255}, 3);
            cv::imshow("EAR", frame);
            if (cv::waitKey(1) == 27) break;
        }
        std::cout << "[ear] " << t << "," << std::fixed << std::setprecision(4) << ear
                  << "," << eye_closed << "," << closed_ms << "," << drowsy << std::endl;
    }
    return 0;
}
