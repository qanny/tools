#include <opencv2\imgproc\imgproc.hpp>
#include <opencv2\features2d\features2d.hpp>
#include <opencv2\highgui\highgui.hpp>
#include <opencv2\calib3d\calib3d.hpp>

#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>

using namespace cv;

#include <iostream>
#include <fstream>
using namespace std;


#include <QDir>
#include <QFileInfoList>
#include <QFileInfo>
#include <QtCore/QString>

class harris
{
private:
	cv::Mat  cornerStrength;  //opencv harris函数检测结果，也就是每个像素的角点响应函数值  
	cv::Mat cornerTh; //cornerStrength阈值化的结果  
	cv::Mat localMax; //局部最大值结果  
	int neighbourhood; //邻域窗口大小  
	int aperture;//sobel边缘检测窗口大小（sobel获取各像素点x，y方向的灰度导数）  
	double k;
	double maxStrength;//角点响应函数最大值  
	double threshold;//阈值除去响应小的值  
	int nonMaxSize;//这里采用默认的3，就是最大值抑制的邻域窗口大小  
	cv::Mat kernel;//最大值抑制的核，这里也就是膨胀用到的核  
public:
	harris() :neighbourhood(3), aperture(3), k(0.01), maxStrength(0.0), threshold(0.01), nonMaxSize(3){

	};

	void setLocalMaxWindowsize(int nonMaxSize){
		this->nonMaxSize = nonMaxSize;
	};

	//计算角点响应函数以及非最大值抑制  
	void detect(const cv::Mat &image){
		//opencv自带的角点响应函数计算函数  
		cv::cornerHarris(image, cornerStrength, neighbourhood, aperture, k);
		double minStrength;
		//计算最大最小响应值  
		cv::minMaxLoc(cornerStrength, &minStrength, &maxStrength);

		cv::Mat dilated;
		//默认3*3核膨胀，膨胀之后，除了局部最大值点和原来相同，其它非局部最大值点被  
		//3*3邻域内的最大值点取代  
		cv::dilate(cornerStrength, dilated, cv::Mat());
		//与原图相比，只剩下和原图值相同的点，这些点都是局部最大值点，保存到localMax  
		cv::compare(cornerStrength, dilated, localMax, cv::CMP_EQ);
	}

	//获取角点图  
	cv::Mat getCornerMap(double qualityLevel) {
		cv::Mat cornerMap;
		// 根据角点响应最大值计算阈值  
		threshold = qualityLevel*maxStrength;
		cv::threshold(cornerStrength, cornerTh,
			threshold, 255, cv::THRESH_BINARY);
		// 转为8-bit图  
		cornerTh.convertTo(cornerMap, CV_8U);
		// 和局部最大值图与，剩下角点局部最大值图，即：完成非最大值抑制  
		cv::bitwise_and(cornerMap, localMax, cornerMap);
		return cornerMap;
	}

	void getCorners(std::vector<cv::Point2f> &points,
		double qualityLevel) {
		//获取角点图  
		cv::Mat cornerMap = getCornerMap(qualityLevel);
		// 获取角点  
		getCorners(points, cornerMap);
	}

	// 遍历全图，获得角点  
	void getCorners(std::vector<cv::Point2f> &points,
		const cv::Mat& cornerMap) {

		for (int y = 0; y < cornerMap.rows; y++) {
			const uchar* rowPtr = cornerMap.ptr<uchar>(y);
			for (int x = 0; x < cornerMap.cols; x++) {
				// 非零点就是角点  
				if (rowPtr[x]) {
					points.push_back(cv::Point2f(x, y));
				}
			}
		}
	}

	//用圈圈标记角点  
	void drawOnImage(cv::Mat &image,
		const std::vector<cv::Point2f> &points,
		cv::Scalar color = cv::Scalar(255, 255, 255),
		int radius = 3, int thickness = 2) {
		std::vector<cv::Point2f>::const_iterator it = points.begin();
		while (it != points.end()) {
			// 角点处画圈  
			cv::circle(image, *it, radius, color, thickness);
			++it;
		}
	}

};

bool sortCom(const Point2f &a, const Point2f &b)
{
	return a.x < b.x;
}

void calculateRt(char *maskName, Mat &R, Mat &t)
{
	double md[] = { 0, 0, 0, 0, 0 };
	double mk[] = { 1.3753678584038005e+003, 0, 9.6106264871042674e+002,
		0, 1.3759215451876044e+003, 5.4792865832740983e+002,
		0, 0, 1 };
	Mat diff(1, 5, CV_64F, md);
	Mat K(3, 3, CV_64F, mk);

	//载入原始图和Mat变量定义   
	Mat image = imread(maskName);
	Mat dstimg;

	if (true) {
		namedWindow("origin");
		imshow("origin", image);
		waitKey(500);
	}

	//灰度变换  
	cv::cvtColor(image, dstimg, CV_BGR2GRAY);

	// 经典的harris角点方法  
	harris Harris;
	// 计算角点  
	Harris.detect(dstimg);
	//获得角点  
	std::vector<cv::Point2f> pts;
	Harris.getCorners(pts, 0.01);
	// 标记角点  
	Harris.drawOnImage(image, pts);

	if (false) {
		cv::namedWindow("harris");
		cv::imshow("harris", image);
		cv::waitKey(500);
	}

	//**************************************
	std::vector<cv::Point3f> thrpts;

	thrpts.push_back(Point3f(0.0, 316, 0));
	thrpts.push_back(Point3f(145.0, 316, 0));
	thrpts.push_back(Point3f(0.0, 0, 0));
	thrpts.push_back(Point3f(145.0, 0, 0));

	assert(thrpts.size() == pts.size());

	//sort
	sort(pts.begin(), pts.begin() + 2, sortCom);
	sort(pts.begin() + 2, pts.end(), sortCom);

	//	for (int i = 0; i < pts.size(); i++) {
	//		cout << pts[i].x << " " << pts[i].y << endl;
	//	}

	solvePnPRansac(thrpts, pts, K, diff, R, t);

	Mat newR;
	Rodrigues(R, newR);
	R = newR;
}
//antenna image point to object point to compute Rotation and Translation
int main() {
	std::string videoNameStr = "D:/useAsE/RealFootData/160119_QR_imu_vid/VID_20160119_143818.mp4";
	cv::Mat frame;
	cv::VideoCapture capture;
	capture.open(videoNameStr);
	if (!capture.isOpened()) {
		std::cout << "Could not open video" << videoNameStr << "with opencv..." << std::endl;
		return false;
	}
	for (;;) {
		capture >> frame;
		if (frame.empty())
			break;

		QString picname = QString("hello") + ".jpg";
		char name[40];
		sprintf(name, "%03d", 2);
		strcat(name, std::string("hello.jpg").c_str());
		cv::imwrite(name, frame);
	}

	return 0;
}
int p2pProj()
{
	Mat R, t;


	int fileIdx[] = { 14, 21, 22, 31, 33, 41, 51, 52, 61, 63, 71, 72, 73, 74 };
	for (int index = 0; index < 14; index++) {
		int idx = fileIdx[index];
		char maskName[100], paramName[100];

		for (int imgno = 1; imgno < 5; imgno++) {
			sprintf(maskName, "F:/mycode/meso/data/pic/000%d.jpg", imgno);

			calculateRt(maskName, R, t);

			sprintf(paramName, "000%d.txt", imgno);
			ofstream fs(paramName);
			fs << imgno << endl;
			for (int i = 0; i < 3; i++) {
				fs << t.at<double>(i) << endl;
			}
			for (int i = 0; i < 3; i++) {
				for (int j = 0; j < 3; j++) {
					fs << R.at<double>(i, j) << endl;
				}
			}
			fs.close();
		}
	}

	return 0;
}