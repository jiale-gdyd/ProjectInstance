#ifndef ALGORITHM_SORT_OBJECTTRACKER_HPP
#define ALGORITHM_SORT_OBJECTTRACKER_HPP

#include <set>
#include <map>

#include "KalmanTracker.hpp"

API_BEGIN_NAMESPACE(Ai)

class API_HIDDEN ObjectTracker {
public:
    ObjectTracker();

    void update(const std::vector<bbox> &result);
    void get_boxes(std::vector<bbox> &result);

    void clear();

    static double GetIOU(const cv::Rect_<float> &bb_test, const cv::Rect_<float> &bb_gt);

private:
    int                                            max_age = 60;
    int                                            min_hits = 3;
    double                                         iouThreshold = 0.2;

    std::vector<KalmanTracker>                     trackers;
    std::vector<std::pair<cv::Rect_<float>, bbox>> detections;
    std::vector<cv::Rect_<float>>                  predictedBoxes;
    std::vector<std::vector<double>>               iouMatrix;
    std::vector<int>                               assignment;
    std::set<int>                                  unmatchedDetections;
    std::set<int>                                  unmatchedTrajectories;
    std::set<int>                                  allItems;
    std::set<int>                                  matchedItems;
    bool                                           pending_update = false;

    std::vector<cv::Point>                         matchedPairs;

    unsigned int                                   trkNum = 0;
    unsigned int                                   detNum = 0;
};

API_END_NAMESPACE(Ai)

#endif
