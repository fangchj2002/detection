#include "Detect.h"
#include <mutex>
#include <opencv2/opencv.hpp>
#include <opencv2/stitching/stitcher.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <thread>
#include <chrono>
#include "Vortex.h"
#include "RectStruct.h"

using namespace std;
using namespace cv;

Detect::Detect()
{
}


Detect::~Detect()
{
}



void Detect::detectPoints(vector<Frame>& frames, mutex& mutex_frames)
{
	bool need2Init = true;
	vector<Point2f> pointsNow, pointsPrev;
//	vector <int> rStPrev(MAXCORNERS),rStNow(MAXCORNERS);
	vector<set<int>> rSets;
//	vector<Rect> rects;
	vector<uchar> status;
	vector<float> err;
	TermCriteria termcrit(CV_TERMCRIT_ITER | CV_TERMCRIT_EPS, 20, 0.03);
	Size subPixWinSize(10, 10), winSize(31, 31);

	while (true)
	{
		if (frames.size() > 5)
		{
			mutex_frames.lock();
			auto it = frames.end() - 1;
			vector<Rect> rectsNow = (*it).getRects();
			Mat imgNow = (*it).getImg();
			it--;
			Mat imgPrev = (*it).getImg();
			mutex_frames.unlock();
			Mat grayNow, grayPrev;
			cvtColor(imgNow, grayNow, COLOR_BGR2GRAY);
			cvtColor(imgPrev, grayPrev, COLOR_BGR2GRAY);
			if (need2Init)
			{
				pointsNow.clear();
				pointsPrev.clear();
				rSets.clear();
				goodFeaturesToTrack(grayNow, pointsNow, MAXCORNERS, 0.01, 10, Mat(), 3, 0, 0.04);
				need2Init = false;
				for (auto r = rectsNow.begin(); r != rectsNow.end(); r++)
				{
					Rect rect = *r;
					set<int> rSet;
					for (auto p = pointsNow.begin(); p != pointsNow.end();p++)
					{
						Point2f point = *p;
						if ((point.x >= rect.x) && (point.x <= (rect.x + rect.width)) && (point.y >= rect.y) && (point.y <= (rect.y + rect.height)))
						{
							int i = p - pointsNow.begin();
							rSet.insert(i);
						}
					}
					rSets.push_back(rSet);

				}
				cout << "init" << endl;
			}
			else
			{
				if (!pointsPrev.empty())
				{
					calcOpticalFlowPyrLK(grayPrev, grayNow, pointsPrev, pointsNow, status, err, winSize, 3, termcrit, 0, 0.001);
					int count = 0;
					for (auto r = rSets.begin(); r != rSets.end(); r++)
					{
						set<int> rSet = *r;
						int xMin = grayNow.cols;
						int xMax = 0;
						int yMin = grayNow.rows;
						int yMax = 0;
						int number = r - rSets.begin();
						for (auto s = rSet.begin(); s != rSet.end(); s++)
						{
							int num = *s;
							Point2f point = pointsNow[num];
							if (point.x < xMin) xMin = (int) point.x;
							if (point.x > xMax) xMax = (int) point.x;
							if (point.y < yMin) yMin = (int) point.y;
							if (point.y > yMax) yMax = (int) point.y;
						}
						Rect rect(Point(xMin, yMin), Point(xMax, yMax));
//						rectangle(imgNow, rect, Scalar(0, 255, 0));

						for (auto rn = rectsNow.begin(); rn != rectsNow.end(); rn++)
						{
	//						rectangle(imgNow, *rn, Scalar(255, 0, 0));
							bool fl = true;
							if ((rect & *rn).width != 0)
							{
								rect = rect | *rn;
									fl = false;
							}
							if (fl) count++;
						}
						rectangle(imgNow, rect, Scalar(0, 0, 255));
						stringstream ss;
						ss << number;
						string stringNumber = ss.str();
						putText(imgNow, stringNumber, Point(rect.x + 5, rect.y + 5), FONT_HERSHEY_COMPLEX_SMALL, 1, Scalar::all(255), 1, 8);

					}
					if (count>3) need2Init = true;




/*
					for (int i = 0; i < pointsNow.size(); i++)
					{
						line(imgNow, pointsPrev[i], pointsNow[i], Scalar(255, 255, 0));
					}
*/

/*
					for (auto jj = rectsStNow.begin(); jj != rectsStNow.end(); jj++)
					{
						Rect r = (*jj).getRect();
						int number = (*jj).getNumber();
						rectangle(imgNow, r, Scalar(0, 0, 255));
						stringstream ss;
						ss << number;
						string stringNumber = ss.str();
						putText(imgNow, stringNumber, Point(r.x + 5, r.y + 5), FONT_HERSHEY_COMPLEX_SMALL, 1, Scalar::all(255), 1, 8);

					}
*/





				}
				imshow("points", imgNow);
			}
			swap(pointsNow, pointsPrev);
//			swap(rSetsNow, rSetsPrev);
		}
		waitKey(20);
	}
}


void Detect::display(vector<Frame>& frames, mutex& mutex_frames)
{
	while (true)
	{
		if (!frames.empty())
		{
			mutex_frames.lock();
			auto iter = frames.end()-1;
			vector<Rect> rects = (*iter).getRects();
			Mat img = (*iter).getImg();
			long long time = (*iter).getTime();
			mutex_frames.unlock();
			for (auto r = rects.begin(); r != rects.end(); r++)
			{
				int number = r - rects.begin();
				stringstream ss;
				ss << number;
				string stringNumber = ss.str();
				rectangle(img, *r, Scalar(255, 0, 0), 1, 8, 0);
				putText(img, stringNumber, Point((*r).x + 5, (*r).y + 5), FONT_HERSHEY_COMPLEX_SMALL, 1, Scalar::all(255), 1, 8);
			}
			//			putText(img, ctime(&time), Point(20, img.rows - 40), FONT_HERSHEY_COMPLEX, 1, Scalar::all(255), 1, 8);

			imshow("img", img);
			waitKey(20);
		}
	}





}


/*
sort(vortexes.begin(), vortexes.end(), Vortex::cmpByAngle);
vector<vector<Vortex>> fragments;
vector<Vortex> region;

auto i = vortexes.begin();
region.push_back(*i);
i++;
for (; i != vortexes.end(); i++)
{
	if ((*i).getAngle() == (*(i - 1)).getAngle())
	{
		region.push_back(*i);
	}
	else
	{
		fragments.push_back(region);
		region.clear();
		region.push_back(*i);
	}
}
fragments.push_back(region);
const Scalar colors[13] = { BLACK, WHITE, RED, ORANGE, YELLOW, GREEN, LIGHTBLUE, BLUE, VIOLET, GINGER, PINK, LIGHTGREEN, BROWN };

int c = 0;
for (auto i = fragments.begin(); i != fragments.end(); i++)
{
	vector<Vortex> reg = *i;
	for (auto j = reg.begin(); j != reg.end(); j++)
	{
		Point2f point = (*j).getP1();
		circle(imgNow, point, 3, colors[c], -1);
	}
	c++;
}
*/
