#ifndef ALGORITHM_SORT_KALMANTRACKER_HPP
#define ALGORITHM_SORT_KALMANTRACKER_HPP

#include <utils/export.h>
#include <opencv2/video/tracking.hpp>

#include "../AiStruct.hpp"

#define StateType   cv::Rect_<float>

API_BEGIN_NAMESPACE(Ai)

class API_HIDDEN KalmanTracker {
public:
    KalmanTracker(StateType initRect, bbox origInfo) {
        init_kf(initRect);

        m_time_since_update = 0;
        m_hits = 0;
        m_hit_streak = 0;
        m_age = 0;
        m_clsid = origInfo.classify;
        m_id = kf_count;
        m_score = origInfo.score;
        orig_attr = origInfo;
        kf_count++;
    }

    ~KalmanTracker() {
        m_history.clear();
    }

    StateType predict();
    void update(StateType stateMat, bbox bbox_);

    StateType get_state();

    bbox get_attr();

    StateType get_rect_xysr(float cx, float cy, float s, float r);

public:
    static int kf_count;

    int        m_time_since_update;
    int        m_hits;
    int        m_hit_streak;
    int        m_age;
    int        m_id;
    int        m_clsid;
    float      m_score;

private:
    void init_kf(StateType stateMat);

    cv::KalmanFilter       kf;
    cv::Mat                measurement;
    bbox                   orig_attr;
    std::vector<StateType> m_history;
};

API_END_NAMESPACE(Ai)

#endif
