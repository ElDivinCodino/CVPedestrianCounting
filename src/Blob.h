// Blob.h

#ifndef MY_BLOB
#define MY_BLOB

#include<opencv2/core/core.hpp>
#include<opencv2/highgui/highgui.hpp>
#include<opencv2/imgproc/imgproc.hpp>

///////////////////////////////////////////////////////////////////////////////////////////////////
class Blob {
public:
    // member variables ///////////////////////////////////////////////////////////////////////////
    std::vector<cv::Point> currentContour;

    cv::Rect currentBoundingRect;

    cv::Point currentCenter;
    
    std::vector<cv::Point> centerPositions;

    double dblCurrentDiagonalSize;
    double dblCurrentAspectRatio;

    bool blnCurrentMatchFoundOrNewBlob;

    bool blnStillBeingTracked;

    int counted; // 0 == left, 1 == right, 2 == up, 3 == down

    int intNumOfConsecutiveFramesWithoutAMatch;

    cv::Point predictedNextPosition;

    // function prototypes ////////////////////////////////////////////////////////////////////////
    Blob(std::vector<cv::Point> _contour);
    void predictNextPosition(void);

};

#endif    // MY_BLOB

