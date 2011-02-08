#include "testApp.h"

float interpScalars[SPLINE_LENGTH] = {0.4, 0.3, 0.2, 0.05, 0.05};

inline float testApp::getGaussMean(queue<float> &in, float last)
{
	float mean = last*interpScalars[0];
	for(int scalarInd = 1; scalarInd < SPLINE_LENGTH; scalarInd++)
	{
		float num = in.front();
		in.pop();
		mean += num*interpScalars[scalarInd];
	}
	return mean;
}

float interpScalars2[SPLINE_LENGTH] = {0.25, 0.45, 0.15, 0.1, 0.05};

inline float testApp::getGaussMean2(queue<float> &in, float last)
{
	float mean = last*interpScalars2[0];
	for(int scalarInd = 1; scalarInd < SPLINE_LENGTH; scalarInd++)
	{
		float num = in.front();
		in.pop();
		mean += num*interpScalars[scalarInd];
	}
	return mean;
}

//--------------------------------------------------------------
testApp::~testApp(){
	cvReleasePixelBackgroundGMM(&pGMM);
	
	videoOut.finishMovie();
	videoOut2.finishMovie();
	fileOutput.close();
}
void testApp::setup(){
	
	drawmovie = true;
	isdebug = false;
	dorandom = true;
	
	// load movies
	int i;
	for(i = 0; i < NUM_VIDEOS; i++)
	{
		char sbuf[256];
		sprintf(sbuf, "%d.mov", i+1);
		//sprintf(sbuf, "old//video%d.mov", i+1);
		//sprintf(sbuf, "counter %d.mov", i+1);
		vidPlayer[i].loadMovie(sbuf);
		vidPlayer[i].setLoopState(OF_LOOP_NORMAL);
	}
	
	// init and sync movies
	Fixed rate = X2Fix(1.0);
	for(i = 0; i < NUM_VIDEOS; i++)
	{
		vidPlayer[i].preRollMovie(rate);
	}
	
	TimeBase mtb = vidPlayer[0].getTimeBase();
	for(i = 0; i < NUM_VIDEOS; i++)
	{
		vidPlayer[i].setTimeBase(rate, mtb);
	}
	MoviesTask(NULL, 0);

	// init video input
	vidInput.initGrabber(CAMERA_WIDTH,CAMERA_HEIGHT);
	vidInput.setUseTexture(false);
	
	// gmm models
	colorImg.allocate(CAMERA_WIDTH,CAMERA_HEIGHT);
	colorImg.setUseTexture(false);
	grayImage.allocate(CAMERA_WIDTH,CAMERA_HEIGHT);
	colorImg.setUseTexture(false);
	grayBg.allocate(CAMERA_WIDTH,CAMERA_HEIGHT);
	grayBg.setUseTexture(false);
	grayDiff.allocate(CAMERA_WIDTH,CAMERA_HEIGHT);
	grayDiff.setUseTexture(false);
	pGMM=cvCreatePixelBackgroundGMM(CAMERA_WIDTH,CAMERA_HEIGHT);
	pGMMTiming = 10.0;
	pGMM->fAlphaT = 1. / pGMMTiming;
	pGMM->fTb = 5*4;
	pGMM->fSigma = 10;
	bLearnBackground = true;
	threshold = 20;
	block_size = 9;
	
	low_threshold = 16;	frame_num = 0;
	
	bLearnBackground = true;
	threshold = 15;
	option1 = true;	// learn a background once
	
	colorImg.allocate(CAMERA_WIDTH,CAMERA_HEIGHT);
	colorImg.setUseTexture(false);
	grayDiff.allocate(CAMERA_WIDTH,CAMERA_HEIGHT);
	grayDiff.setUseTexture(false);
	
    blobTracker.setListener( this );
	
	inImage = new unsigned char[CAMERA_WIDTH*CAMERA_HEIGHT];
	outImage = new unsigned char[CAMERA_WIDTH*CAMERA_HEIGHT];
	
	// setup offset parameters
	for(i = 0; i < NUM_ROWS; i++)
	{
		rowIndexes.push_back(i);
	}
	curNumBlobs = 1;
	offsets.resize(NUM_ROWS);
	for(vector<queue<float > >::iterator it = offsets.begin(); it != offsets.end(); it++)
	{
		for(i = 0; i < SPLINE_LENGTH; i++)
		{
			(*it).push(0.0f);
		}
	}
	// setup shader..
	shader.loadShader((char *)ofToDataPath("cc.bright.fp.glsl").c_str(), (char *)ofToDataPath("cc.bright.vp.glsl").c_str());
	shader.setUniform("brightness", 2.0f);
	//shader.setUniform("amount", 2.0f);
	//shader.setUniform("angle", 0.0f);
	shader.setShaderActive(false);

#if USE_FBO
	myFBO.allocate(SCREEN_WIDTH, SCREEN_HEIGHT, GL_RGBA, 1);
#endif
	
	//////////////////////////////////////////////////////
	// Output movie to
	//////////////////////////////////////////////////////
	
	videoOut.listCodecs();
	videoOut.setCodecQualityLevel(OF_QT_SAVER_CODEC_QUALITY_HIGH);
	videoOut2.setCodecQualityLevel(OF_QT_SAVER_CODEC_QUALITY_HIGH);
	
	char dateString[256];
	sprintf(dateString, "%d%d%d-%02d%02d.%02d\0", ofGetYear(), ofGetMonth(), ofGetDay(), ofGetHours(), ofGetMinutes(), ofGetSeconds() );
	string filename, filename2;
	int fileno = 1;
	ifstream in;
	do {
		in.close();
		stringstream str;
		// format the filename here
		if(fileno == 1)
			str << "recording_" << dateString << ".mov";
		else
			str << "recording_" << dateString << "(" << (fileno-1) << ").mov";
		filename = str.str();
		++fileno; 
		// attempt to open for read
		in.open( ofToDataPath(filename).c_str() );
	} while( in.is_open() );
	in.close();     
	// found a file that does not exists
	
	// now create the file so that we can start adding frames to it:
	ofstream tmpptr(filename.c_str());
	tmpptr.close();
	
	stringstream str;
	str << "camera_" << dateString << "(" << (fileno-1) << ").mov";
	ofstream tmpptr2(str.str().c_str());
	tmpptr2.close();
	
	fileOutput.open(string(filename + ".txt").c_str());
	
	//ofDisableDataPath();
	//string f = "\\data\\output\\" + movie_name + "\\" + filename;
	videoOut2.setup(CAMERA_WIDTH, CAMERA_HEIGHT, str.str().c_str());
	videoOut.setup(MOVIE_OUT_WIDTH, MOVIE_OUT_HEIGHT, filename);
	
	fboCopy.allocate(SCREEN_WIDTH, SCREEN_HEIGHT);   
	fboCopyScaled.allocate(MOVIE_OUT_WIDTH, MOVIE_OUT_HEIGHT);
	//ofEnableDataPath();
	
	
	// window setup
	ofSetWindowShape(WINDOW_WIDTH, WINDOW_HEIGHT);
	ofSetVerticalSync(true);
	ofSetFrameRate(30);
	ofSetBackgroundAuto(false);
	ofBackground(0,0,0);
	
	myTime = ofGetElapsedTimef();
	
	
	// setup asynchronous readback for 2 pbo objects
	pboRGBPCopy = new unsigned char[SCREEN_WIDTH*SCREEN_HEIGHT*3];
	pboIds = new GLuint[2];
	pboIds[0] = 0; pboIds[1] = 0;
	const int DATA_SIZE = SCREEN_WIDTH*SCREEN_HEIGHT*4;
	const int PBO_COUNT = 2;
	glGenBuffersARB(PBO_COUNT, pboIds);
	glBindBufferARB(GL_PIXEL_PACK_BUFFER_ARB, pboIds[0]);
	glBufferDataARB(GL_PIXEL_PACK_BUFFER_ARB, DATA_SIZE, 0, GL_STREAM_READ_ARB);
	glBindBufferARB(GL_PIXEL_PACK_BUFFER_ARB, pboIds[1]);
	glBufferDataARB(GL_PIXEL_PACK_BUFFER_ARB, DATA_SIZE, 0, GL_STREAM_READ_ARB);
	glBindBufferARB(GL_PIXEL_PACK_BUFFER_ARB, 0);
	
	ofSetFullscreen(true);
}

//--------------------------------------------------------------
void testApp::update(){
	int i;
	
	vidInput.update();
	if(vidInput.isFrameNew())
	{
		inImage = vidInput.getPixels();
		if(inImage != NULL)
		{
			colorImg.setFromPixels(inImage, CAMERA_WIDTH,CAMERA_HEIGHT);
			grayImage = colorImg;

			if (bLearnBackground){
				cvUpdatePixelBackgroundGMM(pGMM, inImage, outImage);
				grayDiff.setFromPixels(outImage, CAMERA_WIDTH, CAMERA_HEIGHT);
			}
			grayBg = grayDiff;
			
			// subtraction and threshold
			grayDiff.blur( block_size );
			grayDiff.threshold(threshold);
			//cvAdaptiveThreshold(grayDiff.getCvImage(), grayDiff.getCvImage(), high_threshold, CV_ADAPTIVE_THRESH_GAUSSIAN_C, CV_THRESH_BINARY_INV, block_size, threshold);
			grayDiff.flagImageChanged();
		}
		
		// blob tracking
		contourFinder.findContours(grayDiff, 150, CAMERA_WIDTH*CAMERA_HEIGHT/3, MAX_BLOBS, false);
		blobTracker.trackBlobs( contourFinder.blobs );
	}
	
	
	// update videos
	for(i = 0; i < NUM_VIDEOS; i++)
	{
		vidPlayer[i].update();	
	}	
	
	//myFBO.clear(0.0, 0.0, 0.0, 0.01);
}

//--------------------------------------------------------------
void testApp::draw(){

#if USE_FBO

	myFBO.begin();
	ofEnableAlphaBlending();
	myFBO.clear(0.0f, 0.0f, 0.0f, 0.0f);
	//ofSetColor(255, 255, 255, 255);
#endif
	
	for(vector<int>::iterator r = rowIndexes.begin(); r != rowIndexes.end(); r++)
	{
		queue<float> rowInterp = offsets.at(*r);
		float thisOffset = getGaussMean(rowInterp, 0.0f);
		offsets.at(*r).pop();
		offsets.at(*r).push(thisOffset);
		//float thisOffset = offsets.at(*r).front();
		int row = *r;
//#pragma omp parallel for 
		for(int nummov = 0; nummov < NUM_VIDEOS; nummov++)
		{				
			shader.setShaderActive(true);
			shader.setUniform("brightness", 1.0f + thisOffset / 5000.0f);
			
			int x;
			if(dorandom)
				x = ((int)((((random()%5)-2) * thisOffset) + (nummov)*MOVIE_WIDTH) - SCREEN_WIDTH);
			else 
				x = ((int)(0 * thisOffset + (nummov)*MOVIE_WIDTH) - SCREEN_WIDTH);

			if(x + MOVIE_WIDTH > 0 && x < SCREEN_WIDTH)
			{
				vidPlayer[nummov].draw(x, (MOVIE_HEIGHT/NUM_ROWS)*row, 
									   MOVIE_WIDTH, (MOVIE_HEIGHT/NUM_ROWS), 
									   0, (MOVIE_HEIGHT/NUM_ROWS)*row, 
									   MOVIE_WIDTH, (MOVIE_HEIGHT/NUM_ROWS));
			}
					 
			if(dorandom)
				x = MOVIE_WIDTH*nummov + (((random()%5)-2) * thisOffset);
			else
				x = MOVIE_WIDTH*nummov + (0 * thisOffset);	 
					 
			if(x + MOVIE_WIDTH > 0 && x < SCREEN_WIDTH)
			{
				vidPlayer[nummov].draw(x, (MOVIE_HEIGHT/NUM_ROWS)*row, 
									   MOVIE_WIDTH, (MOVIE_HEIGHT/NUM_ROWS), 
									   0, (MOVIE_HEIGHT/NUM_ROWS)*row, 
									   MOVIE_WIDTH, (MOVIE_HEIGHT/NUM_ROWS));
			}
			 
			 
			
			shader.setShaderActive(false);
		}
	}
	
	for(vector<blobOffset>::iterator blob = blobRowAssignments.begin(); blob != blobRowAssignments.end(); blob++)
	{
		for(vector<int>::iterator r = (*blob).rows.begin(); r != (*blob).rows.end(); r++)
		{
			queue<float> rowInterp = offsets.at(*r);
			float thisOffset = getGaussMean(rowInterp, (float)(*blob).m_offset);
			offsets.at(*r).pop();
			offsets.at(*r).push(thisOffset);
			
			int row = *r;
//#pragma omp parallel for
			for(int nummov = 0; nummov < NUM_VIDEOS; nummov++)
			{				
				shader.setShaderActive(true);
				shader.setUniform("brightness", 1.0f + (float)((*blob).m_offset / 5000.0f));
				
				int x = ((int)(thisOffset + (nummov)*MOVIE_WIDTH) - SCREEN_WIDTH);
				if(x + MOVIE_WIDTH > 0 && x < SCREEN_WIDTH)
				{
					vidPlayer[nummov].draw(x, (MOVIE_HEIGHT/NUM_ROWS)*row, 
										   MOVIE_WIDTH, (MOVIE_HEIGHT/NUM_ROWS), 
										   0, (MOVIE_HEIGHT/NUM_ROWS)*row, 
										   MOVIE_WIDTH, (MOVIE_HEIGHT/NUM_ROWS));
				}
				x = MOVIE_WIDTH*nummov + thisOffset;
				if(x + MOVIE_WIDTH > 0 && x < SCREEN_WIDTH)
				{
					vidPlayer[nummov].draw(x, (MOVIE_HEIGHT/NUM_ROWS)*row, 
										   MOVIE_WIDTH, (MOVIE_HEIGHT/NUM_ROWS), 
										   0, (MOVIE_HEIGHT/NUM_ROWS)*row, 
										   MOVIE_WIDTH, (MOVIE_HEIGHT/NUM_ROWS));
				}
				
				
				shader.setShaderActive(false);
			}
		}
	}

	
	if(isdebug)
	{
		ofSetColor( 0xffffff );
		
		colorImg.draw(20,20);
		grayDiff.draw(20+CAMERA_WIDTH+20+CAMERA_WIDTH+20,20);	
		
		// debug text output
		ofSetColor(0,255,131);
		char buf[256];
		sprintf(buf, "['space'] to learn background\n['+'] or ['-'] to change threshold: %d\n", threshold);
		ofDrawBitmapString( buf, 20,290 );

		grayBg.draw(20+CAMERA_WIDTH+20, 20);
		ofSetColor(0,255,131);
		sprintf(buf, "['9'] or ['0'] to change gmm timing: %f\n['['] or [']'] to change block size: %d", pGMMTiming, block_size);
		ofDrawBitmapString( buf, 20,316 );
		
		
		// blob drawing
		blobTracker.draw(20+CAMERA_WIDTH+20+CAMERA_WIDTH+20,20 );
		
	}
	
	
#if USE_FBO
	ofDisableAlphaBlending();
	myFBO.end();
		
	//myFBO.draw(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
	
	// 2nd and 3rd movies playing on 1st and 2nd displays 
	// going to 2nd and 3rd projection
	
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA); 
	
	 myFBO.draw(0, 0, MOVIE_WIDTH*2, MOVIE_HEIGHT, 
				1280 , MOVIE_HEIGHT, MOVIE_WIDTH*2, -MOVIE_HEIGHT);
	 
	 //myFBO.draw(1280, 0, MOVIE_WIDTH, MOVIE_HEIGHT, 
	 //		   1280*2, 0, MOVIE_WIDTH, MOVIE_HEIGHT);
	 
	 
	 // first movie on 3rd monitor - going to 1st projection
	 myFBO.draw(MOVIE_WIDTH*2, 0, 1152, MOVIE_HEIGHT, 
				128, MOVIE_HEIGHT, 1152, -MOVIE_HEIGHT);
	 
	 // 4th movie on 4th monitor going to 4th projection
	 myFBO.draw(MOVIE_WIDTH*3-128, 0, 1280, MOVIE_HEIGHT, 
				MOVIE_WIDTH*3, MOVIE_HEIGHT, 1152, -MOVIE_HEIGHT);
	
	glDisable(GL_BLEND);
	
	if (ofGetElapsedTimef() - myTime > 5.0f && curNumBlobs > 1) {
		
		// Asynchronous readback
		glReadBuffer(GL_BACK);
		
		glBindBufferARB(GL_PIXEL_PACK_BUFFER_ARB, pboIds[0]);
		glReadPixels(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, GL_BGRA, GL_UNSIGNED_BYTE, 0);
		//glReadPixels(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, GL_RGB, GL_UNSIGNED_BYTE, 0);
		glBindBufferARB(GL_PIXEL_PACK_BUFFER_ARB, 0);
		
		//glBindBufferARB(GL_PIXEL_PACK_BUFFER_ARB, pboIds[1]);
		//glReadPixels(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, GL_BGRA, GL_UNSIGNED_BYTE, 0);
		//glBindBufferARB(GL_PIXEL_PACK_BUFFER_ARB, 0);
		
		glBindBufferARB(GL_PIXEL_PACK_BUFFER_ARB, pboIds[0]);
		GLubyte* src = (GLubyte*)glMapBufferARB(GL_PIXEL_PACK_BUFFER_ARB, GL_READ_ONLY_ARB);
		
		float width_ratio = SCREEN_WIDTH / (float)MOVIE_OUT_WIDTH;
		float height_ratio = SCREEN_HEIGHT / (float)MOVIE_OUT_HEIGHT;
		// save movie // RGB <- BGRA
		if(src)
		{       
			for (int x = 0; x < MOVIE_OUT_WIDTH; x++)     
			{
				for (int y = 0; y < MOVIE_OUT_HEIGHT; y++)    
				{
					int idx = (x+y*(MOVIE_OUT_WIDTH))*3;
					int idx2 = (x*width_ratio+(SCREEN_HEIGHT-y*height_ratio-1)*(SCREEN_WIDTH))*4;
					pboRGBPCopy[idx+0] = src[idx2+2];           
					pboRGBPCopy[idx+1] = src[idx2+1];                           
					pboRGBPCopy[idx+2] = src[idx2+0];                                           
				}
			}               
			//memcpy(colorized, src, sizeof(unsigned char)*mov.width*mov.height*3);
			videoOut.addFrame(pboRGBPCopy, 1.0f / 60.0f); 
			
			videoOut2.addFrame(vidInput.getPixels(), 1.0f / 60.0f);
		}
		
		//saver.addFrame(src, 1.0f / FPS);
		
		glUnmapBufferARB(GL_PIXEL_PACK_BUFFER_ARB);     // release pointer to the mapped buffer 
		glBindBufferARB(GL_PIXEL_PACK_BUFFER_ARB, 0);           
		
		/*
		fboCopy.setFromPixels((unsigned char *)myFBO.getPixels(), SCREEN_WIDTH, SCREEN_HEIGHT);
		fboCopyScaled.scaleIntoMe(fboCopy, CV_INTER_NN);
		videoOut.addFrame(fboCopyScaled.getPixels(), 1/60.0f);
		 */
		
		myTime = ofGetElapsedTimef();
		
		if (myTime > 32000.0f) {
			ofResetElapsedTimeCounter();
			myTime = ofGetElapsedTimef();
		}
	}
	
#endif
}


//--------------------------------------------------------------
void testApp::keyPressed  (int key){
	
	switch (key){
		case 's':
			vidInput.videoSettings();
			break;
		case 'f':
			ofToggleFullscreen();
			break;
		case 'd':
			isdebug = !isdebug;
			break;
		case 'm':
			drawmovie = !drawmovie;
			break;
		case 'r':
			dorandom = !dorandom;
			break;
			
			
		case ' ':
			bLearnBackground = !bLearnBackground;
			break;
		case '=': case '+':
			threshold ++;
			if (threshold > 255) threshold = 255;
			break;
		case '-':
			threshold --;
			if (threshold < 0) threshold = 0;
			break;
		case '9':
			pGMMTiming -= 1.0;
			pGMMTiming = MAX(1,pGMMTiming);
			pGMM->fAlphaT = 1. / pGMMTiming;
			break;
		case '0':
			pGMMTiming += 1.0;
			pGMMTiming = MIN(320.0,pGMMTiming);
			pGMM->fAlphaT = 1. / pGMMTiming;
			break;
		case '[':
			block_size = MAX(3,block_size--);
			break;
		case ']':
			block_size = MIN(11,block_size++);
			break;
			
	}
}

//--------------------------------------------------------------
void testApp::mouseMoved(int x, int y ){
}

//--------------------------------------------------------------
void testApp::mouseDragged(int x, int y, int button){
}

//--------------------------------------------------------------
void testApp::mousePressed(int x, int y, int button){
}

//--------------------------------------------------------------
void testApp::mouseReleased(int x, int y, int button){
	
}

//--------------------------------------------------------------
void testApp::windowResized(int w, int h){
	
}

//--------------------------------------------------
void testApp::blobOn( int x, int y, int id, int order ) {
	id--;
	int i;
#if _DEBUG
	printf("blobOn() - id:%d\n", id);
#endif
	for(i = 0; i < blobRowAssignments.size(); i++) 
	{
		if (blobRowAssignments[i].active) {			
			// while the number of psosible rows assigned is greater than what should be 
			// allocated to each blob
#if _DEBUG
			printf("adding %d els from blob %d to rowIndexes\n", blobRowAssignments[i].rows.size() - (NUM_ROWS/(curNumBlobs + 1)), i);
#endif
			while (blobRowAssignments[i].rows.size() > (NUM_ROWS/(curNumBlobs+1))) 
			{
#if _DEBUG
				printf("rowIndexes size: %d\n", rowIndexes.size()+1);
#endif
				// put them back into the pile of rows
				rowIndexes.push_back(blobRowAssignments[i].rows.back());
				blobRowAssignments[i].rows.pop_back();
			}
		}
	}
	
	curNumBlobs++;
	blobRowAssignments.push_back(blobOffset());
	blobRowAssignments.back().m_offset = atan((((float)x - (CAMERA_WIDTH/2)) / (CAMERA_WIDTH/2)) / 
											  (((float)y - (CAMERA_HEIGHT/2)) / (CAMERA_HEIGHT/2))) * 814.897;
	
    if(id >= idHashTable.size())
	{
#if _DEBUG
		printf("id received by blob detection: %d greater than hash table: %d. Resizing to %d!\n", id, idHashTable.size(), id+1);
#endif
		idHashTable.resize(id+1);
	}
	idHashTable[id] = blobRowAssignments.size()-1;
#if _DEBUG
	printf("Setting id: %d to blob: %d\n", id, idHashTable[id]);
#endif	
	blobRowAssignments[idHashTable[id]].active = true;
#if _DEBUG	
	printf("Resizing blob %d from %d elements to %d\n", idHashTable[id], blobRowAssignments[idHashTable[id]].rows.size(), (NUM_ROWS/(curNumBlobs)));
#endif
	// now we can assign the new blob their share of rows
	while (blobRowAssignments[idHashTable[id]].rows.size() < (NUM_ROWS/(curNumBlobs)))
	{
#if _DEBUG
		printf("rowIndexes size: %d\n", rowIndexes.size()-1);
#endif
		int ind = (random() % (rowIndexes.size()));
		vector<int>::iterator it = rowIndexes.begin() + ind;
		blobRowAssignments[idHashTable[id]].rows.push_back(*it);
		rowIndexes.erase(it);
	}
	
}

void testApp::blobMoved( int x, int y, int id, int order) {
	id--;
	int i;
#if _DEBUG
    if(id > MAX_BLOBS)
		printf("id received by blob detection: %d greater than MAX_BLOBS: %d!!!\n", id, MAX_BLOBS);
	
	printf("blobMoved() - id:%d\n", id);
  
    // full access to blob object ( get a reference)
    ofCvTrackedBlob blob = blobTracker.getById( id+1 );
    cout << "area: " << blob.area << endl;
#endif	
	blobRowAssignments[idHashTable[id]].m_offset = atan((((float)x - (CAMERA_WIDTH/2)) / (CAMERA_WIDTH/2)) / 
														(((float)y - (CAMERA_HEIGHT/2)) / (CAMERA_HEIGHT/2))) * 814.897;
}

void testApp::blobOff( int x, int y, int id, int order ) {
	id--;
	int i;
#if _DEBUG   	
	printf("blobOff() - id:%d (blob: %d)\n", id, idHashTable[id]);
	
	printf("adding %d els from blob %d to rowIndexes\n", blobRowAssignments[idHashTable[id]].rows.size(), idHashTable[id]);
#endif	
	while (blobRowAssignments[idHashTable[id]].rows.size() > 0)
	{
#if _DEBUG		
		printf("rowIndexes size: %d\n", rowIndexes.size()+1);
#endif	
		rowIndexes.push_back(blobRowAssignments[idHashTable[id]].rows.back());
		blobRowAssignments[idHashTable[id]].rows.pop_back();
	}
	
	// reconfigure the hash table
	int toDelete = idHashTable[id];
	for(vector<int>::iterator it = idHashTable.begin(); it != idHashTable.end(); it++)
	{
		if(*it>toDelete)
			--*it;
	}
	
	
	blobRowAssignments[idHashTable[id]].active = false;
	vector<blobOffset>::iterator it = blobRowAssignments.begin() + idHashTable[id];
	blobRowAssignments.erase(it);
	curNumBlobs--;
	
	for(i = 0; i < blobRowAssignments.size(); i++) 
	{
		if (blobRowAssignments[i].active) {
			// while the number of psosible rows assigned is greater than what should be 
			// allocated to each blob
#if _DEBUG
			printf("Resizing blob %d from %d elements to %d\n", i, blobRowAssignments[i].rows.size(), (NUM_ROWS/(curNumBlobs)));
#endif	
			while (blobRowAssignments[i].rows.size() < (NUM_ROWS/(curNumBlobs))) 
			{
#if _DEBUG
				printf("rowIndexes size: %d\n", rowIndexes.size()-1);
#endif	
				// put them back into the pile of rows
				int ind = (random() % (rowIndexes.size()));
				vector<int>::iterator it = rowIndexes.begin() + ind;
				blobRowAssignments[i].rows.push_back(*it);
				rowIndexes.erase(it);
			}
		}
	}
	
}
