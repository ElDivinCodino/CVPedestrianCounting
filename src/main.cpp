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

int historySize = 10;

///////////////////////////////////////////////////////////////////////////////////////////////////
void drawAndShowContours(cv::Size imageSize, std::vector<std::vector<cv::Point> > contours, std::string strImageName) {
    cv::Mat image(imageSize, CV_8UC3, SCALAR_BLACK);

    cv::drawContours(image, contours, -1, SCALAR_WHITE, -1);
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

        if (dblLeastDistance < currentFrameBlob.dblCurrentDiagonalSize) {
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

    for (auto &blob : blobs) {

        if (blob.blnStillBeingTracked == true && blob.centerPositions.size() >= 2) {    // >= 2 in order to avoid noise
            
            int prevFrameIndex;

            if ((int)blob.centerPositions.size() > historySize) {
                prevFrameIndex = (int)blob.centerPositions.size() - historySize;
            } else {
                prevFrameIndex = 0;
            }
            
            int currFrameIndex = (int)blob.centerPositions.size() - 1;

            switch( direction ) {
                case Direction::LEFT:
                    if (blob.centerPositions[currFrameIndex].y >= startingLinePosition && blob.centerPositions[currFrameIndex].y <= endingLinePosition && blob.counted != 1) { // if touching the line (checking the y)
                        if (blob.centerPositions[prevFrameIndex].x > fixedLinePosition && blob.centerPositions[currFrameIndex].x <= fixedLinePosition) {    // if going towards left
                            pedestrianExitingCount++;
                            blnAtLeastOneBlobCrossedTheLine = true;
                            blob.counted = 1;
                        } else if (blob.centerPositions[prevFrameIndex].x < fixedLinePosition && blob.centerPositions[currFrameIndex].x >= fixedLinePosition){  // otherwise going towards right
                            pedestrianEnteringCount++;
                            blnAtLeastOneBlobCrossedTheLine = true;
                            blob.counted = 1;
                        }
                    }
                    
                    break;
                case Direction::RIGHT:
                    if (blob.centerPositions[currFrameIndex].y >= startingLinePosition && blob.centerPositions[currFrameIndex].y <= endingLinePosition && blob.counted != 2) {    
                        if (blob.centerPositions[prevFrameIndex].x < fixedLinePosition && blob.centerPositions[currFrameIndex].x >= fixedLinePosition) {
                            pedestrianExitingCount++;
                            blnAtLeastOneBlobCrossedTheLine = true;
                            blob.counted = 2;
                        } else if (blob.centerPositions[prevFrameIndex].x > fixedLinePosition && blob.centerPositions[currFrameIndex].x <= fixedLinePosition){
                            pedestrianEnteringCount++;
                            blnAtLeastOneBlobCrossedTheLine = true;
                            blob.counted = 2;
                        }
                    }
                    
                    break;
                case Direction::UP:
                    if (blob.centerPositions[currFrameIndex].x >= startingLinePosition && blob.centerPositions[currFrameIndex].x <= endingLinePosition && blob.counted != 3) {
                        if (blob.centerPositions[prevFrameIndex].y > fixedLinePosition && blob.centerPositions[currFrameIndex].y <= fixedLinePosition) {
                            pedestrianExitingCount++;
                            blnAtLeastOneBlobCrossedTheLine = true;
                            blob.counted = 3;
                        } else if (blob.centerPositions[prevFrameIndex].y < fixedLinePosition && blob.centerPositions[currFrameIndex].y >= fixedLinePosition){
                            pedestrianEnteringCount++;
                            blnAtLeastOneBlobCrossedTheLine = true;
                            blob.counted = 3;
                        }
                    }
                    
                    break;
                case Direction::DOWN:
                    if (blob.centerPositions[currFrameIndex].x >= startingLinePosition && blob.centerPositions[currFrameIndex].x <= endingLinePosition && blob.counted != 4) {
                        if (blob.centerPositions[prevFrameIndex].y < fixedLinePosition && blob.centerPositions[currFrameIndex].y >= fixedLinePosition) {
                            pedestrianExitingCount++;
                            blnAtLeastOneBlobCrossedTheLine = true;
                            blob.counted = 4;
                        } else if (blob.centerPositions[prevFrameIndex].y > fixedLinePosition && blob.centerPositions[currFrameIndex].y <= fixedLinePosition){
                            pedestrianEnteringCount++;
                            blnAtLeastOneBlobCrossedTheLine = true;
                            blob.counted = 4;
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
            ptTextPosition.x = imgFrame.size().width*1/40;
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
            ptTextPosition.y = imgFrame.size().height*1/9;

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
    //cv::VideoWriter newVideo;

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
    //newVideo.open("../Assignment/out/video.avi", CV_FOURCC('X','2','6','4'), 30, cv::Size(capVideo.get(CV_CAP_PROP_FRAME_WIDTH), capVideo.get(CV_CAP_PROP_FRAME_HEIGHT)), true);

    if (!capVideo.isOpened()) {                                                 // if unable to open video file
        std::cout << "error reading video file" << std::endl << std::endl;      // show error message
        return(0);                                                              // and exit program
    }

    int totalFramesNum = capVideo.get(CV_CAP_PROP_FRAME_COUNT);

    if (totalFramesNum < 2) {
        std::cout << "error: video file must have at least two frames";
        return(0);
    }

    // Read the first frame
    capVideo.read(imgFrame);

    // Left lines
    leftLine[0].x = imgFrame.size().width*1/20;
    leftLine[0].y = imgFrame.size().height*3/7;
    leftLine[1].x = imgFrame.size().width*1/20;
    leftLine[1].y = imgFrame.size().height*2/3;

    
    // Right line 
    rightLine[0].x = imgFrame.size().width*9/10;
    rightLine[0].y = imgFrame.size().height*5/11;
    rightLine[1].x = imgFrame.size().width*9/10;
    rightLine[1].y = imgFrame.size().height*3/5;

    // Upper lines 
    upperLine[0].x = imgFrame.size().width/3;
    upperLine[0].y = imgFrame.size().height*1/6;
    upperLine[1].x = imgFrame.size().width*2/3;
    upperLine[1].y = imgFrame.size().height*1/6;

    // Lower line 
    lowerLine[0].x = imgFrame.size().width/3;
    lowerLine[0].y = imgFrame.size().height*19/20;
    lowerLine[1].x = imgFrame.size().width*4/7;
    lowerLine[1].y = imgFrame.size().height*19/20;

    bool blnFirstFrame = true;
    int frameCount = 0;
    double learning_rate = 0.1;

    Ptr<BackgroundSubtractor> pMOG;
    pMOG = createBackgroundSubtractorMOG2(500, 50, false);

    IBGS *bgs;
    bgs = new AdaptiveBackgroundLearning;

    while (capVideo.isOpened()) {

        std::vector<Blob> currentFrameBlobs;

        // Clone the first frame
        cv::Mat imgFrameCopy = imgFrame.clone();

        cv::Mat imgDifference, imgThresh, mask, mask1, backgroundModel, backgroundModel1;

        // Conver to YUV     
        cv::cvtColor(imgFrameCopy, imgFrameCopy, CV_BGR2YUV);

        Mat channels[3];
        split(imgFrameCopy, channels);
        imgFrameCopy = channels[0];

        pMOG -> apply(imgFrameCopy, mask1, learning_rate);

        pMOG -> getBackgroundImage(backgroundModel);

        cv::absdiff(imgFrameCopy, backgroundModel, imgFrameCopy);

        // Process the Adaptive Background Learning
        bgs -> process(imgFrameCopy, mask, backgroundModel1);

        // Define structuring elements
        cv::Mat structuringElement3x3 = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3));
        cv::Mat structuringElement5x5 = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(5, 5));
        cv::Mat structuringElement19x19 = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(19, 19));
        
        // Apply erosion in order to eliminate the noise, and dilation to increase the blobs
        cv::erode(mask, mask, structuringElement3x3);
        //cv::erode(mask, mask, structuringElement3x3);
        cv::dilate(mask, mask, structuringElement19x19);

        // Apply a closing operation
        cv::morphologyEx(mask, mask, cv::MORPH_CLOSE, structuringElement5x5);
        
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

        //newVideo.write(imgFrame);
        //std::cout << "Computing " << frameCount << " frame of " << totalFramesNum << " total frames" << endl;
        cv::resize(imgFrame, imgFrame, cv::Size(), 0.4, 0.4);
        cv::imshow("imgFrame", imgFrame);
        
        if ((capVideo.get(CV_CAP_PROP_POS_FRAMES) + 1) < capVideo.get(CV_CAP_PROP_FRAME_COUNT)) {
            capVideo.read(imgFrame);
        } else {
            std::cout << "End of video\n";
            std::cout << "Entering from north: " + std::to_string(pedestrianEnteringUpperCount) + "\n" << endl;
            std::cout << "Exiting from north: " + std::to_string(pedestrianExitingUpperCount) + "\n\n" << endl;
            std::cout << "Entering from left: " + std::to_string(pedestrianEnteringLeftCount) + "\n" << endl;
            std::cout << "Exiting from left: " + std::to_string(pedestrianExitingLeftCount) + "\n\n" << endl;
            std::cout << "Entering from right: " + std::to_string(pedestrianEnteringRightCount) + "\n" << endl;
            std::cout << "Exiting from right: " + std::to_string(pedestrianExitingRightCount) + "\n\n" << endl;
            std::cout << "Entering from south: " + std::to_string(pedestrianEnteringLowerCount) + "\n" << endl;
            std::cout << "Exiting from south: " + std::to_string(pedestrianExitingLowerCount) + "\n\n" << endl;
            break;
        }

        blnFirstFrame = false;
        frameCount++;

        waitKey(1);

        if (waitKey(10) == 27) //wait for 'esc' key press for 30ms. If 'esc' key is pressed, break loop
        {
            cout << "esc key is pressed by user" << endl;
            break; 
        }
    }

	return 0;
}
