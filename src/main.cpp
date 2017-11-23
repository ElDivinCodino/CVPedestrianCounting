#include "main.hpp"
#include "hog.hpp"
#include "Blob.h"

#define N_GATES 4
#define EROSION_SIZE 50

// global variables ///////////////////////////////////////////////////////////////////////////////
const cv::Scalar SCALAR_BLACK = cv::Scalar(0.0, 0.0, 0.0);
const cv::Scalar SCALAR_WHITE = cv::Scalar(255.0, 255.0, 255.0);
const cv::Scalar SCALAR_YELLOW = cv::Scalar(0.0, 255.0, 255.0);
const cv::Scalar SCALAR_GREEN = cv::Scalar(0.0, 200.0, 0.0);
const cv::Scalar SCALAR_RED = cv::Scalar(0.0, 0.0, 255.0);

///////////////////////////////////////////////////////////////////////////////////////////////////
void drawAndShowContours(cv::Size imageSize, std::vector<std::vector<cv::Point> > contours, std::string strImageName) {
    cv::Mat image(imageSize, CV_8UC3, SCALAR_BLACK);

    cv::drawContours(image, contours, -1, SCALAR_WHITE, -1);

    //cv::imshow(strImageName, image);
}

void addBlobToExistingBlobs(Blob &currentFrameBlob, std::vector<Blob> &existingBlobs, int &intIndex) {

    existingBlobs[intIndex].currentContour = currentFrameBlob.currentContour;
    existingBlobs[intIndex].currentBoundingRect = currentFrameBlob.currentBoundingRect;

    existingBlobs[intIndex].centerPositions.push_back(currentFrameBlob.centerPositions.back());

    existingBlobs[intIndex].dblCurrentDiagonalSize = currentFrameBlob.dblCurrentDiagonalSize;
    existingBlobs[intIndex].dblCurrentAspectRatio = currentFrameBlob.dblCurrentAspectRatio;

    existingBlobs[intIndex].blnStillBeingTracked = true;
    existingBlobs[intIndex].blnCurrentMatchFoundOrNewBlob = true;
}

void drawBlobInfoOnImage(std::vector<Blob> &blobs, cv::Mat &imgFrame2Copy) {

    for (unsigned int i = 0; i < blobs.size(); i++) {

        if (blobs[i].blnStillBeingTracked == true) {
            cv::rectangle(imgFrame2Copy, blobs[i].currentBoundingRect, SCALAR_RED, 2);

            int intFontFace = CV_FONT_HERSHEY_SIMPLEX;
            double dblFontScale = blobs[i].dblCurrentDiagonalSize / 60.0;
            int intFontThickness = (int)std::round(dblFontScale * 1.0);

            cv::putText(imgFrame2Copy, std::to_string(i), blobs[i].centerPositions.back(), intFontFace, dblFontScale, SCALAR_GREEN, intFontThickness);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void drawAndShowContours(cv::Size imageSize, std::vector<Blob> blobs, std::string strImageName) {

    cv::Mat image(imageSize, CV_8UC3, SCALAR_BLACK);

    std::vector<std::vector<cv::Point> > contours;

    for (auto &blob : blobs) {
        if (blob.blnStillBeingTracked == true) {
            contours.push_back(blob.currentContour);
        }
    }

    cv::drawContours(image, contours, -1, SCALAR_WHITE, -1);

    //cv::imshow(strImageName, image);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void addNewBlob(Blob &currentFrameBlob, std::vector<Blob> &existingBlobs) {

    currentFrameBlob.blnCurrentMatchFoundOrNewBlob = true;

    existingBlobs.push_back(currentFrameBlob);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
double distanceBetweenPoints(cv::Point point1, cv::Point point2) {

    int intX = abs(point1.x - point2.x);
    int intY = abs(point1.y - point2.y);

    return(sqrt(pow(intX, 2) + pow(intY, 2)));
}

void matchCurrentFrameBlobsToExistingBlobs(std::vector<Blob> &existingBlobs, std::vector<Blob> &currentFrameBlobs) {

    for (auto &existingBlob : existingBlobs) {
        existingBlob.blnCurrentMatchFoundOrNewBlob = false;
        existingBlob.predictNextPosition();
    }

    for (auto &currentFrameBlob : currentFrameBlobs) {
        int intIndexOfLeastDistance = 0;
        double dblLeastDistance = 100000.0;

        for (unsigned int i = 0; i < existingBlobs.size(); i++) {
            if (existingBlobs[i].blnStillBeingTracked == true) {
                double dblDistance = distanceBetweenPoints(currentFrameBlob.centerPositions.back(), existingBlobs[i].predictedNextPosition);
                if (dblDistance < dblLeastDistance) {
                    dblLeastDistance = dblDistance;
                    intIndexOfLeastDistance = i;
                }
            }
        }

        if (dblLeastDistance < currentFrameBlob.dblCurrentDiagonalSize * 2) {
            addBlobToExistingBlobs(currentFrameBlob, existingBlobs, intIndexOfLeastDistance);
        }
        else {
            addNewBlob(currentFrameBlob, existingBlobs);
        }

    }

    for (auto &existingBlob : existingBlobs) {

        if (existingBlob.blnCurrentMatchFoundOrNewBlob == false) {
            existingBlob.intNumOfConsecutiveFramesWithoutAMatch++;
        }

        if (existingBlob.intNumOfConsecutiveFramesWithoutAMatch >= 5) {
            existingBlob.blnStillBeingTracked = false;
        }

    }

}

///////////////////////////////////////////////////////////////////////////////////////////////////


int main() {
    cv::VideoCapture capVideo;

    cv::Mat imgFrame1;
    cv::Mat imgFrame2;
    std::vector<Blob> blobs;

    cv::Point crossingLine[2];

    int pedestrianCount = 0;

    capVideo.open("../Assignment/res/video.mov");

    if (!capVideo.isOpened()) {                                                 // if unable to open video file
        std::cout << "error reading video file" << std::endl << std::endl;      // show error message
        return(0);                                                              // and exit program
    }

    if (capVideo.get(CV_CAP_PROP_FRAME_COUNT) < 2) {
        std::cout << "error: video file must have at least two frames";
        return(0);
    }

    // Read the first two frames
    capVideo.read(imgFrame1);
    capVideo.read(imgFrame2);

    // Calcolate the horizontal line position
    int intHorizontalLinePosition = (int)std::round((double)imgFrame1.rows * 0.35);
    crossingLine[0].x = 0;
    crossingLine[0].y = intHorizontalLinePosition;
    crossingLine[1].x = imgFrame1.cols - 1;
    crossingLine[1].y = intHorizontalLinePosition;

    char chCheckForEscKey = 0;
    bool blnFirstFrame = true;
    int frameCount = 2;

    Ptr<BackgroundSubtractor> pMOG; // pointer to Mixture Of Gaussian BG subtractor
    double learning_rate = 0.1;
    pMOG = createBackgroundSubtractorMOG2(500, 20, false);

    while (capVideo.isOpened() && chCheckForEscKey != 27)
    {
        std::vector<Blob> currentFrameBlobs;

        // Clone the first two frames
        cv::Mat imgFrame1Copy = imgFrame1.clone();
        cv::Mat imgFrame2Copy = imgFrame2.clone();

        cv::Mat imgDifference, imgThresh, mask;

        // Conver to YUV     
        cv::cvtColor(imgFrame1Copy, imgFrame1Copy, CV_BGR2YUV);

        Mat channels_1[3];
        split( imgFrame1Copy, channels_1 );
        imgFrame1Copy = channels_1[0];
        //imgFrame1Copy.convertTo(imgFrame1Copy, -1, 2, 0); //increase the contrast


        cv::cvtColor(imgFrame2Copy, imgFrame2Copy, CV_BGR2YUV);

        Mat channels_2[3];
        split( imgFrame2Copy, channels_2 );
        imgFrame2Copy = channels_2[0];
        //imgFrame2Copy.convertTo(imgFrame2Copy, -1, 2, 0); //increase the contrast

        pMOG -> apply(imgFrame1Copy, mask, learning_rate);
        // Remove noise
        //cv::GaussianBlur(imgFrame1Copy, imgFrame1Copy, cv::Size(5, 5), 0);
        //cv::GaussianBlur(imgFrame2Copy, imgFrame2Copy, cv::Size(5, 5), 0);

        // Differences between two frames
        //cv::absdiff(mask, imgFrame2Copy, imgDifference);

        // Subtractk
        //cv::threshold(mask, imgThresh, 100, 255.0, CV_THRESH_BINARY);

        // Define structuring elemenets
        cv::Mat structuringElement3x3 = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3));
        cv::Mat structuringElement5x5 = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(5, 5));
        cv::Mat structuringElement7x7 = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(7, 7));
        cv::Mat structuringElement15x15 = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(15, 15));

        //for (unsigned int i = 0; i < 1; i++) {
            //cv::close(imgThresh, imgThresh, structuringElement15x15);
            cv::morphologyEx(mask, mask, cv::MORPH_CLOSE, structuringElement15x15);
            //cv::dilate(imgThresh, imgThresh, structuringElement15x15);
            //cv::dilate(imgThresh, imgThresh, structuringElement15x15);
            //cv::erode(imgThresh, imgThresh, structuringElement15x15);
        //}

        // Copy the image
        cv::Mat imgThreshCopy = mask.clone();

        // Find findContours
        std::vector<std::vector<cv::Point> > contours;
        cv::findContours(imgThreshCopy, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
        drawAndShowContours(imgThresh.size(), contours, "imgContours");

        // Matrix of pixels for each contours
        std::vector<std::vector<cv::Point> > convexHulls(contours.size());

        // apply convexhull function in convexHulls vector
        for (unsigned int i = 0; i < contours.size(); i++) {
            cv::convexHull(contours[i], convexHulls[i]);
        }

        drawAndShowContours(imgThresh.size(), convexHulls, "imgConvexHulls");

        // for each convex shape do something
        for (auto &convexHull : convexHulls) {

            // Find possible rect.
            Blob possibleBlob(convexHull);

            // If is small enough -> it is not a truck or a car -> consider it
            if (possibleBlob.currentBoundingRect.area() < 3000 &&
                possibleBlob.dblCurrentAspectRatio > 0.2 &&
                possibleBlob.dblCurrentAspectRatio < 4.0 &&
                possibleBlob.currentBoundingRect.width > 1 &&
                possibleBlob.currentBoundingRect.height > 1 &&
                possibleBlob.dblCurrentDiagonalSize > 1 &&
                (cv::contourArea(possibleBlob.currentContour) / (double)possibleBlob.currentBoundingRect.area()) > 0.50) {
                currentFrameBlobs.push_back(possibleBlob);
            }
        }

        drawAndShowContours(imgThresh.size(), currentFrameBlobs, "imgCurrentFrameBlobs");

        if (blnFirstFrame == true) {
            for (auto &currentFrameBlob : currentFrameBlobs) {
                blobs.push_back(currentFrameBlob);
            }
        } else {
            matchCurrentFrameBlobsToExistingBlobs(blobs, currentFrameBlobs);
        }

        drawAndShowContours(imgThresh.size(), blobs, "imgBlobs");
        imgFrame2Copy = imgFrame2.clone();          // get another copy of frame 2 since we changed the previous frame 2 copy in the processing above
        drawBlobInfoOnImage(blobs, imgFrame2Copy);
        cv::resize(imgFrame2Copy, imgFrame2Copy, cv::Size(), 0.5, 0.5);
        cv::imshow("imgFrame2Copy", imgFrame2Copy);
        cv::imshow("mask", mask);
        
        imgFrame1 = imgFrame2.clone();           // second frames becomes first frame, in order to continue the one-by-one analysis

        if ((capVideo.get(CV_CAP_PROP_POS_FRAMES) + 1) < capVideo.get(CV_CAP_PROP_FRAME_COUNT)) {
            capVideo.read(imgFrame2);
        } else {
            std::cout << "End of video\n";
            break;
        }


        blnFirstFrame = false;
        frameCount++;

        waitKey(1);
    }

	return 0;
}
