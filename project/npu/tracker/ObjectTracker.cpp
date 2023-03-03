#include "Hungarian.hpp"
#include "ObjectTracker.hpp"

API_BEGIN_NAMESPACE(Ai)

void ObjectTracker::clear()
{
    detections.clear();
    trackers.clear();
}

void ObjectTracker::update(const std::vector<bbox> &result) 
{
    detections.clear();
    for (bbox box : result) {
        detections.emplace_back(cv::Rect_<float>(box.left, box.top, box.right - box.left, box.bottom - box.top), box);
    }

    if (trackers.empty()) {
        for (bbox box : result) {
            KalmanTracker trk = KalmanTracker(cv::Rect_<float>(box.left, box.top, box.right - box.left, box.bottom - box.top), box);
            trackers.emplace_back(trk);
        }
    } else {
        pending_update = true;
    }
}

void ObjectTracker::get_boxes(std::vector<bbox> &result) 
{
    predictedBoxes.clear();
    for (auto it = trackers.begin(); it != trackers.end(); ) {
        cv::Rect_<float> pBox = (*it).predict();
        if ((pBox.x >= 0) && (pBox.y >= 0)) {
            predictedBoxes.emplace_back(pBox);
            it++;
        } else {
            it = trackers.erase(it);
        }
    }

    trkNum = predictedBoxes.size();
    detNum = detections.size();
    if ((trkNum == 0) || (detNum == 0)) {
        return;
    }

    iouMatrix.clear();
    iouMatrix.resize(trkNum, std::vector<double>(detNum, 0));

    for (unsigned int i = 0; i < trkNum; i++) {
        for (unsigned int j = 0; j < detNum; j++) {
            iouMatrix[i][j] = 1 - GetIOU(predictedBoxes[i], detections[j].first);
        }
    }

    HungarianAlgorithm HungAlgo;
    assignment.clear();
    HungAlgo.Solve(iouMatrix, assignment);

    unmatchedTrajectories.clear();
    unmatchedDetections.clear();
    allItems.clear();
    matchedItems.clear();

    if (detNum > trkNum) {
        for (unsigned int n = 0; n < detNum; n++) {
            allItems.insert(n);
        }

        for (unsigned int i = 0; i < trkNum; ++i) {
            matchedItems.insert(assignment[i]);
        }

        set_difference(allItems.begin(), allItems.end(), matchedItems.begin(), matchedItems.end(), std::insert_iterator<std::set<int>>(unmatchedDetections, unmatchedDetections.begin()));
    } else if (detNum < trkNum) { 
        for (unsigned int i = 0; i < trkNum; ++i) {
            if (assignment[i] == -1) {
                unmatchedTrajectories.insert(i);
            }
        }
    }

    matchedPairs.clear();
    for (unsigned int i = 0; i < trkNum; ++i) {
        if (assignment[i] == -1) {
            continue;
        }

        if ((1 - iouMatrix[i][assignment[i]]) < iouThreshold) {
            unmatchedTrajectories.insert(i);
            unmatchedDetections.insert(assignment[i]);
        } else {
            matchedPairs.emplace_back(i, assignment[i]);
        }
    }

    int detIdx, trkIdx;
    for (auto &matchedPair : matchedPairs) {
        trkIdx = matchedPair.x;
        detIdx = matchedPair.y;
        trackers[trkIdx].update(detections[detIdx].first, detections[detIdx].second);
    }

    if (pending_update) {
        for (auto umd : unmatchedDetections) {
            KalmanTracker tracker = KalmanTracker(detections[umd].first, detections[umd].second);
            trackers.emplace_back(tracker);
        }

        pending_update = false;
    }

    result.clear();
    for (auto it = trackers.begin(); it != trackers.end(); ) {
        if (((*it).m_time_since_update < 1) && ((*it).m_hit_streak >= min_hits)) {
            cv::Rect_<float> data = (*it).get_state();
            bbox res = (*it).get_attr();
            res.left = data.x;
            res.right = data.x + data.width;
            res.top = data.y;
            res.bottom = data.y + data.height;
            res.trackerId = (*it).m_id;
            res.classify = (*it).m_clsid;
            res.score = (*it).m_score;
            result.emplace_back(res);
            it++;
        } else {
            it++;
        }

        if ((it != trackers.end()) && ((*it).m_time_since_update > max_age)) {
            it = trackers.erase(it);
        }
    }
}

ObjectTracker::ObjectTracker() 
{

}

double ObjectTracker::GetIOU(const cv::Rect_<float> &bb_test, const cv::Rect_<float> &bb_gt)
{
    float in = (bb_test & bb_gt).area();
    float un = bb_test.area() + bb_gt.area() - in;

    if (un < DBL_EPSILON) {
        return 0;
    }

    return (double)(in / un);
}

API_END_NAMESPACE(Ai)
