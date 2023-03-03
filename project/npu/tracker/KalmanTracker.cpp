#include "KalmanTracker.hpp"

API_BEGIN_NAMESPACE(Ai)

int KalmanTracker::kf_count = 0;

void KalmanTracker::init_kf(StateType stateMat)
{
    int stateNum = 7;
    int measureNum = 4;
    kf = cv::KalmanFilter(stateNum, measureNum, 0);

    measurement = cv::Mat::zeros(measureNum, 1, CV_32F);

    kf.transitionMatrix = (cv::Mat_<float>(stateNum, stateNum) <<
        1, 0, 0, 0, 1, 0, 0,
        0, 1, 0, 0, 0, 1, 0,
        0, 0, 1, 0, 0, 0, 1,
        0, 0, 0, 1, 0, 0, 0,
        0, 0, 0, 0, 1, 0, 0,
        0, 0, 0, 0, 0, 1, 0,
        0, 0, 0, 0, 0, 0, 1
    );

    setIdentity(kf.measurementMatrix);
    setIdentity(kf.processNoiseCov, cv::Scalar::all(1e-2));
    setIdentity(kf.measurementNoiseCov, cv::Scalar::all(1e-1));
    setIdentity(kf.errorCovPost, cv::Scalar::all(1));

    kf.statePost.at<float>(0, 0) = stateMat.x + stateMat.width / 2;
    kf.statePost.at<float>(1, 0) = stateMat.y + stateMat.height / 2;
    kf.statePost.at<float>(2, 0) = stateMat.area();
    kf.statePost.at<float>(3, 0) = stateMat.width / stateMat.height;
}

StateType KalmanTracker::predict()
{
    cv::Mat p = kf.predict();
    m_age += 1;

    if (m_time_since_update > 0) {
        m_hit_streak = 0;
    }
    m_time_since_update += 1;

    StateType predictBox = get_rect_xysr(p.at<float>(0, 0), p.at<float>(1, 0), p.at<float>(2, 0), p.at<float>(3, 0));
    m_history.emplace_back(predictBox);

    return m_history.back();
}

void KalmanTracker::update(StateType stateMat, bbox bbox_)
{
    m_time_since_update = 0;
    m_history.clear();
    m_hits += 1;
    m_hit_streak += 1;

    measurement.at<float>(0, 0) = stateMat.x + stateMat.width / 2;
    measurement.at<float>(1, 0) = stateMat.y + stateMat.height / 2;
    measurement.at<float>(2, 0) = stateMat.area();
    measurement.at<float>(3, 0) = stateMat.width / stateMat.height;
    m_score = m_score * 0.9 + bbox_.score * 0.1;
    m_clsid = bbox_.classify;

    kf.correct(measurement);
}

StateType KalmanTracker::get_state()
{
    cv::Mat s = kf.statePost;
    return get_rect_xysr(s.at<float>(0, 0), s.at<float>(1, 0), s.at<float>(2, 0), s.at<float>(3, 0));
}

StateType KalmanTracker::get_rect_xysr(float cx, float cy, float s, float r)
{
    float w = sqrt(s * r);
    float h = s / w;
    float x = (cx - w / 2);
    float y = (cy - h / 2);

    if ((x < 0) && (cx > 0)) {
        x = 0;
    }

    if ((y < 0) && (cy > 0)) {
        y = 0;
    }

    return StateType(x, y, w, h);
}

bbox KalmanTracker::get_attr()
{
    return this->orig_attr;
}

API_END_NAMESPACE(Ai)
