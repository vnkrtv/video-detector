#include "speed_detector.hpp"

namespace detector {

    unordered_map<int, float> class2width{
            {CL_BACKGROUND,   12.},
            {CL_AEROPLANE,    12.},
            {CL_BICYCLE,      12.},
            {CL_BIRD,         12.},
            {CL_BOAT,         12.},
            {CL_BOTTLE,       12.},
            {CL_BUS,          2.5},
            {CL_CAR,          1.6},
            {CL_CAT,          12.},
            {CL_CHAIR,        12.},
            {CL_COW,          12.},
            {CL_DINING_TABLE, 12.},
            {CL_DOG,          12.},
            {CL_HORSE,        12.},
            {CL_MOTORBIKE,    12.},
            {CL_PERSON,       0.5},
            {CL_POTTED_PLANT, 12.},
            {CL_SHEEP,        12.},
            {CL_SOFA,         12.},
            {CL_TRAIN,        12.},
            {CL_TV_MONITOR,   12.},
    };

    DetectedObject::DetectedObject(cv::Rect2i bbox, const int &objClass) : bbox(std::move(bbox)) {
        meanWidth = class2width[objClass];
        centroid = cv::Point2i(bbox.x + (bbox.width / 2), bbox.y + (bbox.height / 2));
    }

    double SpeedDetector::getDist(const int &x1, const int &x2, const int &y1, const int &y2) {
#ifdef USE_TAXICAB_SQRT
        return double(abs(x2 - x1) + abs(y2 - y1));
#else
        return sqrt(pow(x2 - x1, 2) + pow(y2 - y1, 2));
    }

#endif

    double SpeedDetector::estimateSpeed(const cv::Point2i &prevLoc,
                                        const cv::Point2i &curLoc,
                                        const cv::Rect2i &bbox,
                                        const float &meanObjWidth,
                                        const double &fps) {
        auto dPixels = getDist(prevLoc.x, curLoc.x, prevLoc.y, curLoc.y);
        std::clog << "dPixels: " << dPixels << std::endl;
        auto pixelPerMeter = (bbox.width / meanObjWidth);
        std::clog << "pixelPerMeter: " << pixelPerMeter << std::endl;
        auto dMeters = dPixels / pixelPerMeter;
        std::clog << "dMeters: " << dMeters << std::endl;
        auto speed = dMeters * fps * 3.6; // 3.6 - for converting m/s to km/h
        return speed;
    }

    SpeedDetector::SpeedDetector() {
        _detectedObjects = unordered_map<int, queue<DetectedObject>>();
    }

    void SpeedDetector::addObject(const int &objID, const cv::Rect2i &objBbox, const int &objClass) {
        if (_detectedObjects.find(objID) != _detectedObjects.end()) {
            _detectedObjects[objID].push(DetectedObject(objBbox, objClass));
        } else {
            queue<DetectedObject> objQueue;
            objQueue.push(DetectedObject(objBbox, objClass));
            _detectedObjects[objID] = objQueue;
        }
    }

    map<int, double> SpeedDetector::getObjectsSpeed(const double &fps) {
        map<int, double> objSpeed;
        for (auto&[objID, trackHistory]: _detectedObjects) {
            auto recordsCount = trackHistory.size();
            if (recordsCount > 1) {
                auto prevLoc = trackHistory.back().centroid;
                auto curLoc = trackHistory.front().centroid;
                trackHistory.pop();
                objSpeed[objID] = estimateSpeed(prevLoc, curLoc, trackHistory.back().bbox, trackHistory.back().meanWidth, fps);
            }
        }
        return objSpeed;
    }

} // namespace detector
