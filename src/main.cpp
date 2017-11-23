#include "main.hpp"
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
bool checkIfBlobsCrossedTheLeftLine(std::vector<Blob> &blobs, int &linePosition, int &pedestrianExitingCount, int &pedestrianEnteringCount, int direction) {
    bool blnAtLeastOneBlobCrossedTheLine = false;

    for (auto blob : blobs) {

        if (blob.blnStillBeingTracked == true && blob.centerPositions.size() >= 2) {
            int prevFrameIndex = (int)blob.centerPositions.size() - 2;
            int currFrameIndex = (int)blob.centerPositions.size() - 1;

            switch( direction ) {
                case Direction::LEFT:
                    if (blob.centerPositions[prevFrameIndex].x > linePosition && blob.centerPositions[currFrameIndex].x <= linePosition) {
                        pedestrianExitingCount++;
                        blnAtLeastOneBlobCrossedTheLine = true;
                    } else if (blob.centerPositions[prevFrameIndex].x <= linePosition && blob.centerPositions[currFrameIndex].x > linePosition){
                        pedestrianEnteringCount++;
                        blnAtLeastOneBlobCrossedTheLine = true;
                    }
                    break;
                case Direction::RIGHT:
                    if (blob.centerPositions[prevFrameIndex].x <= linePosition && blob.centerPositions[currFrameIndex].x > linePosition) {
                        pedestrianExitingCount++;
                        blnAtLeastOneBlobCrossedTheLine = true;
                    } else if (blob.centerPositions[prevFrameIndex].x > linePosition && blob.centerPositions[currFrameIndex].x <= linePosition){
                        pedestrianEnteringCount++;
                        blnAtLeastOneBlobCrossedTheLine = true;
                    }
                    break;
                case Direction::UP:
                    if (blob.centerPositions[prevFrameIndex].y > linePosition && blob.centerPositions[currFrameIndex].y <= linePosition) {
                        pedestrianExitingCount++;
                        blnAtLeastOneBlobCrossedTheLine = true;
                    } else if (blob.centerPositions[prevFrameIndex].x <= linePosition && blob.centerPositions[currFrameIndex].x > linePosition){
                        pedestrianEnteringCount++;
                        blnAtLeastOneBlobCrossedTheLine = true;
                    }
                    break;
                case Direction::DOWN:
                    if (blob.centerPositions[prevFrameIndex].y <= linePosition && blob.centerPositions[currFrameIndex].y > linePosition) {
                        pedestrianExitingCount++;
                        blnAtLeastOneBlobCrossedTheLine = true;
                    } else if (blob.centerPositions[prevFrameIndex].y > linePosition && blob.centerPositions[currFrameIndex].y <= linePosition){
                        pedestrianEnteringCount++;
                        blnAtLeastOneBlobCrossedTheLine = true;
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
            ptTextPosition.x = imgFrame.size().width/20;
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
    //int intHorizontalLinePosition = (int)std::round((double)imgFrame.rows * 0.35);
    leftLine[0].x = imgFrame.size().width/20;
    leftLine[0].y = imgFrame.size().height*3/7;
    leftLine[1].x = imgFrame.size().width/20;
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
    int historyLength = 3;

    Ptr<BackgroundSubtractor> pMOG; // pointer to Mixture Of Gaussian BG subtractor
    double learning_rate = 0.1;
    pMOG = createBackgroundSubtractorMOG2(500, 25, false);

    Mat maskHistory[historyLength];

    while (capVideo.isOpened() && chCheckForEscKey != 27)
    {
        std::vector<Blob> currentFrameBlobs;

        // Clone the first frame
        cv::Mat imgFrameCopy = imgFrame.clone();

        cv::Mat imgDifference, imgThresh, mask;

        // Conver to YUV     
        cv::cvtColor(imgFrameCopy, imgFrameCopy, CV_BGR2YUV);

        Mat channels[3];
        split( imgFrameCopy, channels );
        imgFrameCopy = channels[0];
        //imgFrame1Copy.convertTo(imgFrame1Copy, -1, 2, 0); //increase the contrast

        pMOG -> apply(imgFrameCopy, mask, learning_rate);

        maskHistory[frameCount % historyLength] = mask;
        
        // Define structuring elements
        cv::Mat structuringElement15x15 = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(15, 15));

        // Apply a closing operation
        cv::morphologyEx(mask, mask, cv::MORPH_CLOSE, structuringElement15x15);
        
        // Copy the image
        cv::Mat imgThreshCopy = mask.clone();

        if (frameCount > historyLength) {
            for(int i = 0; i < historyLength; i++) {
                imgThreshCopy += maskHistory[i];
            }
        }

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

        drawBlobInfoOnImage(blobs, imgFrameCopy);

        bool blnAtLeastOneBlobCrossedTheLine = false;

        blnAtLeastOneBlobCrossedTheLine ^= checkIfBlobsCrossedTheLeftLine(blobs, leftLine[0].x, pedestrianExitingLeftCount, pedestrianEnteringLeftCount, Direction::LEFT);
        blnAtLeastOneBlobCrossedTheLine ^= checkIfBlobsCrossedTheLeftLine(blobs, leftLine[0].x, pedestrianExitingRightCount, pedestrianEnteringRightCount, Direction::RIGHT);
        blnAtLeastOneBlobCrossedTheLine ^= checkIfBlobsCrossedTheLeftLine(blobs, leftLine[0].x, pedestrianExitingUpperCount, pedestrianEnteringUpperCount, Direction::UP);
        blnAtLeastOneBlobCrossedTheLine ^= checkIfBlobsCrossedTheLeftLine(blobs, leftLine[0].x, pedestrianExitingLowerCount, pedestrianEnteringLowerCount, Direction::DOWN);

        drawPedestrianCountOnImage(pedestrianExitingLeftCount, pedestrianEnteringLeftCount, imgFrameCopy, Direction::LEFT);
        drawPedestrianCountOnImage(pedestrianExitingRightCount, pedestrianEnteringRightCount, imgFrameCopy, Direction::RIGHT);
        drawPedestrianCountOnImage(pedestrianExitingUpperCount, pedestrianEnteringUpperCount, imgFrameCopy, Direction::UP);
        drawPedestrianCountOnImage(pedestrianExitingLowerCount, pedestrianEnteringLowerCount, imgFrameCopy, Direction::DOWN);

        cv::line(imgFrameCopy, leftLine[0], leftLine[1], SCALAR_RED, 2);
        cv::line(imgFrameCopy, rightLine[0], rightLine[1], SCALAR_RED, 2);
        cv::line(imgFrameCopy, upperLine[0], upperLine[1], SCALAR_RED, 2);
        cv::line(imgFrameCopy, lowerLine[0], lowerLine[1], SCALAR_RED, 2);

        cv::resize(imgFrameCopy, imgFrameCopy, cv::Size(), 0.5, 0.5);
        cv::imshow("imgFrameCopy", imgFrameCopy);
        
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
