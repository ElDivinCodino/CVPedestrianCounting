#include "main.hpp"
#include "bgslibrary.h"
#include "Blob.h"

#define N_GATES 4
#define EROSION_SIZE 50

// global variables ///////////////////////////////////////////////////////////////////////////////
const cv::Scalar SCALAR_BLACK = cv::Scalar(0.0, 0.0, 0.0);
const cv::Scalar SCALAR_WHITE = cv::Scalar(255.0, 255.0, 255.0);
const cv::Scalar SCALAR_YELLOW = cv::Scalar(0.0, 255.0, 255.0);
const cv::Scalar SCALAR_GREEN = cv::Scalar(0.0, 200.0, 0.0);
const cv::Scalar SCALAR_RED = cv::Scalar(0.0, 0.0, 255.0);

enum Direction { RIGHT, LEFT, UP, DOWN };

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

void drawBlobInfoOnImage(std::vector<Blob> &blobs, cv::Mat &imgFrameCopy) {

    for (unsigned int i = 0; i < blobs.size(); i++) {

        if (blobs[i].blnStillBeingTracked == true) {
            cv::rectangle(imgFrameCopy, blobs[i].currentBoundingRect, SCALAR_RED, 2);

            
            int intFontFace = CV_FONT_HERSHEY_SIMPLEX;
            double dblFontScale = blobs[i].dblCurrentDiagonalSize / 60.0;
            int intFontThickness = (int)std::round(dblFontScale * 1.0);

            cv::putText(imgFrameCopy, std::to_string(i), blobs[i].centerPositions.back(), intFontFace, dblFontScale, SCALAR_GREEN, intFontThickness);
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

    vector<Blob> newExistingBlobs;

    for (auto &existingBlob : existingBlobs) {  // delete obsolete blobs for memory efficiency
        if (existingBlob.blnStillBeingTracked != false)
            newExistingBlobs.push_back(existingBlob);
    }

    existingBlobs = newExistingBlobs;
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
bool checkIfBlobsCrossedLine(std::vector<Blob> &blobs, int &fixedLinePosition, int &startingLinePosition, int &endingLinePosition, int &pedestrianExitingCount, int &pedestrianEnteringCount, int direction) {
    bool blnAtLeastOneBlobCrossedTheLine = false;

    for (auto blob : blobs) {

        if (blob.blnStillBeingTracked == true && blob.centerPositions.size() >= 3) {    // >= 3 in order to avoid noise
            int prevFrameIndex = (int)blob.centerPositions.size() - 2;
            int currFrameIndex = (int)blob.centerPositions.size() - 1;

            switch( direction ) {
                case Direction::LEFT:
                    if (blob.centerPositions[currFrameIndex].y >= startingLinePosition && blob.centerPositions[currFrameIndex].y <= endingLinePosition && blob.counted != 0) { // if touching the line (checking the y)
                        if (blob.centerPositions[prevFrameIndex].x > fixedLinePosition && blob.centerPositions[currFrameIndex].x <= fixedLinePosition) {    // if going towards left
                            pedestrianExitingCount++;
                            blnAtLeastOneBlobCrossedTheLine = true;
                            blob.counted = 0;
                        } else if (blob.centerPositions[prevFrameIndex].x < fixedLinePosition && blob.centerPositions[currFrameIndex].x >= fixedLinePosition){  // otherwise going towards right
                            pedestrianEnteringCount++;
                            blnAtLeastOneBlobCrossedTheLine = true;
                            blob.counted = 0;
                        }
                    }
                    
                    break;
                case Direction::RIGHT:
                    if (blob.centerPositions[currFrameIndex].y >= startingLinePosition && blob.centerPositions[currFrameIndex].y <= endingLinePosition && blob.counted != 1) {    
                        if (blob.centerPositions[prevFrameIndex].x < fixedLinePosition && blob.centerPositions[currFrameIndex].x >= fixedLinePosition) {
                            pedestrianExitingCount++;
                            blnAtLeastOneBlobCrossedTheLine = true;
                            blob.counted = 1;
                        } else if (blob.centerPositions[prevFrameIndex].x > fixedLinePosition && blob.centerPositions[currFrameIndex].x <= fixedLinePosition){
                            pedestrianEnteringCount++;
                            blnAtLeastOneBlobCrossedTheLine = true;
                            blob.counted = 1;
                        }
                    }
                    
                    break;
                case Direction::UP:
                    if (blob.centerPositions[currFrameIndex].x >= startingLinePosition && blob.centerPositions[currFrameIndex].x <= endingLinePosition && blob.counted != 2) {
                        if (blob.centerPositions[prevFrameIndex].y > fixedLinePosition && blob.centerPositions[currFrameIndex].y <= fixedLinePosition) {
                            pedestrianExitingCount++;
                            blnAtLeastOneBlobCrossedTheLine = true;
                            blob.counted = 2;
                        } else if (blob.centerPositions[prevFrameIndex].x < fixedLinePosition && blob.centerPositions[currFrameIndex].x >= fixedLinePosition){
                            pedestrianEnteringCount++;
                            blnAtLeastOneBlobCrossedTheLine = true;
                            blob.counted = 2;
                        }
                    }
                    
                    break;
                case Direction::DOWN:
                    if (blob.centerPositions[currFrameIndex].x >= startingLinePosition && blob.centerPositions[currFrameIndex].x <= endingLinePosition && blob.counted != 3) {
                        if (blob.centerPositions[prevFrameIndex].y < fixedLinePosition && blob.centerPositions[currFrameIndex].y >= fixedLinePosition) {
                            pedestrianExitingCount++;
                            blnAtLeastOneBlobCrossedTheLine = true;
                            blob.counted = 3;
                        } else if (blob.centerPositions[prevFrameIndex].y > fixedLinePosition && blob.centerPositions[currFrameIndex].y <= fixedLinePosition){
                            pedestrianEnteringCount++;
                            blnAtLeastOneBlobCrossedTheLine = true;
                            blob.counted = 3;
                        }
                    }

                    break;
                default:

                    break;
            }
        }

    }

    return blnAtLeastOneBlobCrossedTheLine;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void drawPedestrianCountOnImage(int &pedestrianExitingCount, int &pedestrianEnteringCount, cv::Mat &imgFrame, int direction) {

    int intFontFace = CV_FONT_HERSHEY_SIMPLEX;
    double dblFontScale = (imgFrame.rows * imgFrame.cols) / 2000000.0;
    int intFontThickness = (int)std::round(dblFontScale * 1.5);

    cv::Point ptTextPosition;

    switch( direction ) {
        case Direction::LEFT:
            ptTextPosition.x = imgFrame.size().width*3/40;
            ptTextPosition.y = imgFrame.size().height*1/3;

            cv::putText(imgFrame, "Entering: " + std::to_string(pedestrianEnteringCount), ptTextPosition, intFontFace, dblFontScale, SCALAR_GREEN, intFontThickness);

            ptTextPosition.y += imgFrame.size().height*1/20;
            cv::putText(imgFrame, "Exiting: " + std::to_string(pedestrianExitingCount), ptTextPosition, intFontFace, dblFontScale, SCALAR_RED, intFontThickness);
            break;
        case Direction::RIGHT:
            ptTextPosition.x = imgFrame.size().width*11/15;
            ptTextPosition.y = imgFrame.size().height*2/5;

            cv::putText(imgFrame, "Entering: " + std::to_string(pedestrianEnteringCount), ptTextPosition, intFontFace, dblFontScale, SCALAR_GREEN, intFontThickness);

            ptTextPosition.y += imgFrame.size().height*1/20;
            cv::putText(imgFrame, "Exiting: " + std::to_string(pedestrianExitingCount), ptTextPosition, intFontFace, dblFontScale, SCALAR_RED, intFontThickness);
            break;
        case Direction::UP:
            ptTextPosition.x = imgFrame.size().width*11/15;
            ptTextPosition.y = imgFrame.size().height*1/5;

            cv::putText(imgFrame, "Entering: " + std::to_string(pedestrianEnteringCount), ptTextPosition, intFontFace, dblFontScale, SCALAR_GREEN, intFontThickness);

            ptTextPosition.y += imgFrame.size().height*1/20;
            cv::putText(imgFrame, "Exiting: " + std::to_string(pedestrianExitingCount), ptTextPosition, intFontFace, dblFontScale, SCALAR_RED, intFontThickness);
            break;
        case Direction::DOWN:
            ptTextPosition.x = imgFrame.size().width*1/9;
            ptTextPosition.y = imgFrame.size().height*9/10;

            cv::putText(imgFrame, "Entering: " + std::to_string(pedestrianEnteringCount), ptTextPosition, intFontFace, dblFontScale, SCALAR_GREEN, intFontThickness);

            ptTextPosition.y += imgFrame.size().height*1/20;
            cv::putText(imgFrame, "Exiting: " + std::to_string(pedestrianExitingCount), ptTextPosition, intFontFace, dblFontScale, SCALAR_RED, intFontThickness);
            break;
        default:
            break;
    }

}

///////////////////////////////////////////////////////////////////////////////////////////////////

int main() {
    cv::VideoCapture capVideo;

    cv::Mat imgFrame;
    std::vector<Blob> blobs;

    cv::Point leftLine[2];
    cv::Point rightLine[2];
    cv::Point upperLine[2];
    cv::Point lowerLine[2];

    int pedestrianExitingLeftCount = 0;
    int pedestrianEnteringLeftCount = 0;
    int pedestrianExitingRightCount = 0;
    int pedestrianEnteringRightCount = 0;
    int pedestrianExitingUpperCount = 0;
    int pedestrianEnteringUpperCount = 0;
    int pedestrianExitingLowerCount = 0;
    int pedestrianEnteringLowerCount = 0;

    capVideo.open("../Assignment/res/video.mov");

    if (!capVideo.isOpened()) {                                                 // if unable to open video file
        std::cout << "error reading video file" << std::endl << std::endl;      // show error message
        return(0);                                                              // and exit program
    }

    if (capVideo.get(CV_CAP_PROP_FRAME_COUNT) < 2) {
        std::cout << "error: video file must have at least two frames";
        return(0);
    }

    // Read the first frame
    capVideo.read(imgFrame);

    // Left lines
    leftLine[0].x = imgFrame.size().width*3/40;
    leftLine[0].y = imgFrame.size().height*3/7;
    leftLine[1].x = imgFrame.size().width*3/40;
    leftLine[1].y = imgFrame.size().height*2/3;

    
    // Right line 
    rightLine[0].x = imgFrame.size().width*19/20;
    rightLine[0].y = imgFrame.size().height*4/9;
    rightLine[1].x = imgFrame.size().width*19/20;
    rightLine[1].y = imgFrame.size().height*4/7;

    // Upper lines 
    upperLine[0].x = imgFrame.size().width/3;
    upperLine[0].y = imgFrame.size().height*1/5;
    upperLine[1].x = imgFrame.size().width*2/3;
    upperLine[1].y = imgFrame.size().height*1/5;

    // Lower line 
    lowerLine[0].x = imgFrame.size().width/3;
    lowerLine[0].y = imgFrame.size().height*19/20;
    lowerLine[1].x = imgFrame.size().width*4/7;
    lowerLine[1].y = imgFrame.size().height*19/20;

    char chCheckForEscKey = 0;
    bool blnFirstFrame = true;
    int frameCount = 0;

    Ptr<BackgroundSubtractor> pMOG; // pointer to Mixture Of Gaussian BG subtractor
    double learning_rate = 0.1;
    pMOG = createBackgroundSubtractorMOG2(100, 30, false);

    IBGS *bgs;
    bgs = new AdaptiveBackgroundLearning;

    while (capVideo.isOpened() && chCheckForEscKey != 27) {

        std::vector<Blob> currentFrameBlobs;

        // Clone the first frame
        cv::Mat imgFrameCopy = imgFrame.clone();

        cv::Mat imgDifference, imgThresh, mask, mask1, backgroundModel;

        // Conver to YUV     
        cv::cvtColor(imgFrameCopy, imgFrameCopy, CV_BGR2YUV);

        Mat channels[3];
        split( imgFrameCopy, channels );
        imgFrameCopy = channels[0];

        //pMOG -> apply(imgFrameCopy, mask1, learning_rate);
        //cv::medianBlur(imgFrameCopy, mask, 5);

        bgs -> process(imgFrameCopy, mask, backgroundModel);

        //cv::absdiff(backgroundModel, mask, mask);

        // Define structuring elements
        cv::Mat structuringElement5x5 = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(5, 5));
        cv::Mat structuringElement7x7 = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(19, 19));
        
        // Apply a opening operation
        //cv::morphologyEx(mask, mask, cv::MORPH_CLOSE, structuringElement7x7);
        cv::erode(mask, mask, structuringElement5x5);
        cv::dilate(mask, mask, structuringElement7x7);        
        //cv::dilate(mask, mask, structuringElement7x7);
        //cv::dilate(mask, mask, structuringElement7x7);

        //cv::morphologyEx(mask, mask, cv::MORPH_OPEN, structuringElement15x15);

        // Apply a closing operation
        //cv::morphologyEx(mask1, mask1, cv::MORPH_CLOSE, structuringElement5x5);
        
        // Copy the image
        cv::Mat imgThreshCopy = mask.clone();

        // Find contours
        std::vector<std::vector<cv::Point> > contours;
        cv::findContours(imgThreshCopy, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
        drawAndShowContours(imgThresh.size(), contours, "imgContours");

        // Matrix of pixels for each contours
        std::vector<std::vector<cv::Point> > convexHulls(contours.size());

        // Apply convexhull function in convexHulls vector
        for (unsigned int i = 0; i < contours.size(); i++) {
            cv::convexHull(contours[i], convexHulls[i]);
        }

        drawAndShowContours(imgThresh.size(), convexHulls, "imgConvexHulls");

        // For each convex shape draw the rect
        for (auto &convexHull : convexHulls) {

            // Find possible rect
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

        drawBlobInfoOnImage(blobs, imgFrame);

        std::cout << blobs.size() << endl;

        bool blnAtLeastOneBlobCrossedTheLine = false;

        blnAtLeastOneBlobCrossedTheLine ^= checkIfBlobsCrossedLine(blobs, leftLine[0].x, leftLine[0].y, leftLine[1].y, pedestrianExitingLeftCount, pedestrianEnteringLeftCount, Direction::LEFT);
        blnAtLeastOneBlobCrossedTheLine ^= checkIfBlobsCrossedLine(blobs, rightLine[0].x, rightLine[0].y, rightLine[1].y, pedestrianExitingRightCount, pedestrianEnteringRightCount, Direction::RIGHT);
        blnAtLeastOneBlobCrossedTheLine ^= checkIfBlobsCrossedLine(blobs, upperLine[0].y, upperLine[0].x, upperLine[1].x, pedestrianExitingUpperCount, pedestrianEnteringUpperCount, Direction::UP);
        blnAtLeastOneBlobCrossedTheLine ^= checkIfBlobsCrossedLine(blobs, lowerLine[0].y, lowerLine[0].x, lowerLine[1].x, pedestrianExitingLowerCount, pedestrianEnteringLowerCount, Direction::DOWN);

        drawPedestrianCountOnImage(pedestrianExitingLeftCount, pedestrianEnteringLeftCount, imgFrame, Direction::LEFT);
        drawPedestrianCountOnImage(pedestrianExitingRightCount, pedestrianEnteringRightCount, imgFrame, Direction::RIGHT);
        drawPedestrianCountOnImage(pedestrianExitingUpperCount, pedestrianEnteringUpperCount, imgFrame, Direction::UP);
        drawPedestrianCountOnImage(pedestrianExitingLowerCount, pedestrianEnteringLowerCount, imgFrame, Direction::DOWN);

        cv::line(imgFrame, leftLine[0], leftLine[1], SCALAR_RED, 2);
        cv::line(imgFrame, rightLine[0], rightLine[1], SCALAR_RED, 2);
        cv::line(imgFrame, upperLine[0], upperLine[1], SCALAR_RED, 2);
        cv::line(imgFrame, lowerLine[0], lowerLine[1], SCALAR_RED, 2);

        cv::resize(imgFrame, imgFrame, cv::Size(), 0.3, 0.3);
        cv::imshow("imgFrame", imgFrame);
        cv::resize(mask, mask, cv::Size(), 0.3, 0.3);
        cv::imshow("mask", mask);
        
        if ((capVideo.get(CV_CAP_PROP_POS_FRAMES) + 1) < capVideo.get(CV_CAP_PROP_FRAME_COUNT)) {
            capVideo.read(imgFrame);
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
