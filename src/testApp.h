// parag k mital
// responsive ecologies
// pkmital.com
//
// developed in collaboration with captincaptin.co.uk

#ifndef _TEST_APP
#define _TEST_APP

#define USE_FBO 1
#define _DEBUG 0

#include "ofMain.h"
#include "ofxFBOTexture.h"
#include "ofxQTKitVideoTexture.h"
#include "ofxQTKitVideoGrabber.h"
#include "ofxOpenCv.h"
#include "cv.h"
#include "cvaux.h"

#include "CvPixelBackgroundGMM.h"

#include "ofCvMain.h"
#include <queue>
#include <fstream.h>
#include "ofShader.h"

#include "ofxQtVideoSaver.h"

const int SPLINE_LENGTH = 5;
const int CAMERA_WIDTH = 320;
const int CAMERA_HEIGHT = 240;
const int MOVIE_WIDTH = 1280;
const int MOVIE_HEIGHT = 720;
const int NUM_VIDEOS = 4;
const int SCREEN_WIDTH = 1280*NUM_VIDEOS;
const int SCREEN_HEIGHT = MOVIE_HEIGHT;
const int MOVIE_OUT_WIDTH = SCREEN_WIDTH;
const int MOVIE_OUT_HEIGHT = SCREEN_HEIGHT;
const int NUM_ROWS = MOVIE_HEIGHT/1;
const int MAX_BLOBS = NUM_ROWS / 2;


const int WINDOW_WIDTH = SCREEN_WIDTH ;
const int WINDOW_HEIGHT = SCREEN_HEIGHT ;

class testApp : public ofBaseApp, public ofCvBlobListener{

	public:

	~testApp();
	void setup();
	void update();
	void draw();

	void keyPressed  (int key);
	void mouseMoved(int x, int y );
	void mouseDragged(int x, int y, int button);
	void mousePressed(int x, int y, int button);
	void mouseReleased(int x, int y, int button);
	void windowResized(int w, int h);
	
	inline float getGaussMean(queue<float> &in, float last);
	inline float getGaussMean2(queue<float> &in, float last);

	// video and camera

	ofxQTKitVideoTexture 	vidPlayer[NUM_VIDEOS];
	//ofxQTKitVideoGrabber	vidInput;
	ofVideoGrabber			vidInput;

	// for camera analysis:
	ofxCvColorImage			colorImg;
	ofxCvGrayscaleImage		grayImage;

	ofxCvGrayscaleImage		grayBg;
	ofxCvGrayscaleImage		grayDiff;
	CvPixelBackgroundGMM*	pGMM;
	int						threshold;
	int						low_threshold, high_threshold, block_size;
	double					pGMMTiming;
	unsigned long			frame_num;
	bool					option1;
	
	ofCvContourFinder		contourFinder;		
	ofCvBlobTracker			blobTracker;	
	
	bool					bLearnBackground;
	unsigned char *			inImage;
	unsigned char *			outImage;
	
	// blob callbacks 

	void blobOn( int x, int y, int id, int order );
	void blobMoved( int x, int y, int id, int order );    
	void blobOff( int x, int y, int id, int order );    

	// for video drawing offset
	class blobOffset {
	public:
		blobOffset() { rows.resize(0); m_offset = 0; active = false; }
		vector<int> rows;
		int m_offset;
		bool active;
	};
	vector<int> rowIndexes;
	vector< blobOffset > blobRowAssignments;
	vector<int> idHashTable;
	vector<queue<float > > offsets;
	int curNumBlobs;

	ofShader				shader;
	ofxFBOTexture			myFBO;
	
	ofxQtVideoSaver			videoOut;
	ofxQtVideoSaver			videoOut2;
	
	// control variables
	bool					isdebug, drawmovie, dorandom;
	
	float					myTime;
	
	unsigned char			*pboRGBPCopy;
	GLuint					*pboIds;
	ofstream				fileOutput;
	
	ofxCvColorImage			fboCopy;
	ofxCvColorImage			fboCopyScaled;
};

#endif
