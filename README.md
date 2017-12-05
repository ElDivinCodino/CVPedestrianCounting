# Computer Vision - Pedestrian Counting System

**Computer Vision and Multimedia Analysis - Master Course in Computer Science @ UniTn  2017/18**

## Author:

**Francesco Alzetta** - francesco.alzetta@studenti.unitn.it

```bash
git clone https://github.com/ElDivinCodino/CVPedestrianCounting.git
```

---

In this project I have tried several algorithms in order to do a satisfying traking of cyclists and pedestrians.
In the end I found that it was a job hard to achieve without background subtraction.
For this reason I decided to mix a performing background subtraction algorithm, as MOG2, set with a high threshold in order to delete the biggest noise, and an algorithm able to recognize pretty well the elements from the remaining background, found at https://github.com/andrewssobral/bgslibrary.

Unfortunately during the tuning of the parameters I understood that this solution is very sensitive, and changing slightly a relevant variable could make the code recognize better the pedestrians but introducing also lot of noise at the same time.

For this reason I decided to loose the detection of the most problematic pedestrians, but avoiding as much as possible false positives. 


## Build/Run the project

In the root folder there is a Makefile, where all the dependencies are included.

* Move into the Assignment directory.
* Place the video in the Assignment/res folder (removed for file size purposes).
* In order to compile the code just run the `make` command from terminal.
* To execute the code run `./main`.

## Recorded video

A video showing how the code performs can be found at https://drive.google.com/file/d/18sfNGjN3pMVY1UqovKRmif4TMGAHA9ha/view?usp=sharing