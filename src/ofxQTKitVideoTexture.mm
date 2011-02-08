#include "ofxQTKitVideoTexture.h"
#include "ofMain.h"
#import "Cocoa/Cocoa.h"
#import "QTKit/QTKit.h"

int ofxQTKitVideoTexture::videoCount = 0;
ofxQTKitVideoTexture::ofxQTKitVideoTexture() {
	videoCount++;
	qtVisualContext = NULL;
	currentFrame = NULL;
	movie = NULL;
	bLoaded = false;
	
	speed = 1;
	bPlaying = false;
	width = 0;
	height = 0;
	duration = 0;
}

ofxQTKitVideoTexture::~ofxQTKitVideoTexture() {
	videoCount--;
	if(movie==NULL) return;
	QTMovie *mov = (QTMovie*)movie;
	[mov release];
	
}
	

void ofxQTKitVideoTexture::play() {
	if(movie==NULL) return;
	QTMovie *mov = (QTMovie*)movie;
	[mov play];
	bPlaying = true;
}
void ofxQTKitVideoTexture::stop() {
	if(movie==NULL) return;
	QTMovie *mov = (QTMovie*)movie;
	[mov stop];
	bPlaying = false;
}


void ofxQTKitVideoTexture::setLoopState(int looping) {
	if(movie==NULL) return;
	QTMovie *mov = (QTMovie*)movie;
	bool loop = false;
	if(looping!=OF_LOOP_NONE) {
		loop = true;
	}
	[mov setAttribute:[NSNumber numberWithBool:loop] forKey:QTMovieLoopsAttribute];
}

float ofxQTKitVideoTexture::getPosition() {
	if(movie==NULL) return 0;
	QTMovie *mov = (QTMovie*)movie;
	QTTime dur = [mov currentTime];
	float pos = (float)dur.timeValue/dur.timeScale;
	return pos/duration;
}


float ofxQTKitVideoTexture::getDuration() {
	return duration;
}


void ofxQTKitVideoTexture::setPosition(float pct) {
}

float ofxQTKitVideoTexture::getSpeed() {
	return speed;
}

void ofxQTKitVideoTexture::setSpeed(float _speed) {
	if(movie==NULL) return;
	speed = _speed;
	QTMovie *mov = (QTMovie*)movie;
	[mov setRate:speed];
}


void ofxQTKitVideoTexture::setPaused(bool bPause) {
	if(bPlaying && !bPause) play();
	else if(!bPlaying && bPause) stop();
	bPlaying = !bPause;
}



float ofxQTKitVideoTexture::getHeight() { return height; }
float ofxQTKitVideoTexture::getWidth() { return width; }




		
bool ofxQTKitVideoTexture::loadMovie(string name) {
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	string path = ofToDataPath(name, true);
	NSString *nsPath = [NSString stringWithCString:path.c_str() encoding: NSASCIIStringEncoding];
		
	
	// check quicktime file
	if([QTMovie canInitWithFile:nsPath] == false) {
		NSLog(@"Couldn't load %@\n", nsPath);
		
		return false;
	}
	
	
	// load quicktime file
	movie = [[QTMovie alloc] initWithFile:nsPath error:NULL];

	NSError *err;
	
/*	NSDictionary *attrs = nil;
	attrs = [NSDictionary dictionaryWithObjectsAndKeys:
			 
			 nsPath, QTMovieFileNameAttribute, 
			 
			 YES, QTMovieOpenForPlaybackAttribute,
			 nil
			 
			 ];
	*/
	/*attrs = [NSDictionary dictionaryWithObjectsAndKeys:
						  nsPath, QTMovieURLAttribute,
						  [NSNumber numberWithBool:YES], QTMovieOpenForPlaybackAttribute,
						  nil ];
	*/
	
	//movie = [[QTMovie alloc] initWithAttributes:attrs error: &err];
	
	
	if(movie == NULL) {
		NSLog(@"File %@ could not be loaded\n", nsPath);
		return false;
	}
	
	
	NSLog(@"Loaded %@\n", nsPath);	
	width = 640;
	height = 480;

	NSArray* vtracks = [movie tracksOfMediaType:QTMediaTypeVideo];
	if([vtracks count]>0) {
		QTTrack* track = [vtracks objectAtIndex:0];
		NSSize size = [track apertureModeDimensionsForMode:QTMovieApertureModeClean];
		width = size.width;
		height = size.height;
	}
	
	
	
	NSDictionary	    *attributes = nil;
    attributes = [NSDictionary dictionaryWithObjectsAndKeys:
                  [NSDictionary dictionaryWithObjectsAndKeys:
                   [NSNumber numberWithFloat:width],
                   kQTVisualContextTargetDimensions_WidthKey,
                   [NSNumber numberWithFloat:height],
                   kQTVisualContextTargetDimensions_HeightKey, nil], 
                  kQTVisualContextTargetDimensionsKey, 
                 
				  
				  [NSDictionary dictionaryWithObjectsAndKeys:
                   [NSNumber numberWithFloat:width], 
                   kCVPixelBufferWidthKey, 
                   [NSNumber numberWithFloat:height], 
                   kCVPixelBufferHeightKey, nil], 
				  
				  
				  
                  kQTVisualContextPixelBufferAttributesKey,
                  nil];
	
	
	OSStatus error = QTOpenGLTextureContextCreate(NULL,
                                         CGLGetCurrentContext(),
										CGLGetPixelFormat(CGLGetCurrentContext()),
                                         (CFDictionaryRef)attributes,
                                         &qtVisualContext);
	if(error!=noErr) printf("Got error %d\n", error);
	QTMovie *mov = (QTMovie*)movie;
    // Associate it with our movie.
    SetMovieVisualContext([mov quickTimeMovie],qtVisualContext);
	
	[pool release];
	
	QTTime dur = [mov duration];
	duration = (float)dur.timeValue/dur.timeScale;
	printf("Duration: %f\n", duration);
	
	//SetMoviePlayHints([mov quickTimeMovie],hintsHighQuality, hintsHighQuality);	
	
	bLoaded = true;
	return true;
}

void ofxQTKitVideoTexture::preRollMovie(Fixed rate)
{    
	QTMovie *mov = (QTMovie*)movie;
	[mov gotoBeginning];
	MoviesTask([mov quickTimeMovie], 0); 
	PrerollMovie([mov quickTimeMovie], 0, rate);
}

void ofxQTKitVideoTexture::setTimeBase(Fixed rate, TimeBase masterTimeBase)
{
	if (NULL != masterTimeBase) 
    {
		// set the movie to run from the master timebase unless it is the same as its own
		TimeBase selfTimeBase = getTimeBase();
		if (masterTimeBase != selfTimeBase)
			SetTimeBaseMasterTimeBase(selfTimeBase, masterTimeBase, nil);
    }	
	QTMovie *mov = (QTMovie *)movie;
	SetMovieRate([mov quickTimeMovie], rate);	// this starts the playback of the movie
}

TimeBase ofxQTKitVideoTexture::getTimeBase()
{
	QTMovie *mov = (QTMovie *)movie;
	return GetMovieTimeBase([mov quickTimeMovie]);
}

bool ofxQTKitVideoTexture::update() {

	//int start = ofGetElapsedTimeMillis();
	if(QTVisualContextIsNewImageAvailable(qtVisualContext,NULL)) {
		// Release the previous frame
		CVOpenGLTextureRelease(currentFrame);
		
		// Copy the current frame into our image buffer
		QTVisualContextCopyImageForTime(qtVisualContext, NULL, NULL, &currentFrame);
//		printf("Time: %d\n", ofGetElapsedTimeMillis() - start);
		return true;
	} else {
		return false;
	}
}

void ofxQTKitVideoTexture::bind() {
	if(currentFrame==NULL) {
		printf("Current Frame is null\n");
		return;
	}
	
	int id2 = CVOpenGLTextureGetTarget(currentFrame);
	if(GL_TEXTURE_2D == id2)
		printf("GL_TEXTURE_2D\n");
	else if(GL_TEXTURE_RECTANGLE_ARB == CVOpenGLTextureGetTarget(currentFrame))
		printf("GL_TEXTURE_RECTANGLE_ARB\n");
	
	
	glEnable(CVOpenGLTextureGetTarget(currentFrame));
	glBindTexture(CVOpenGLTextureGetTarget(currentFrame), 
                  CVOpenGLTextureGetName(currentFrame));
}

void ofxQTKitVideoTexture::unbind() {
	if(currentFrame==NULL) return;
	QTVisualContextTask(qtVisualContext);
	glDisable(CVOpenGLTextureGetTarget(currentFrame));

}

void ofxQTKitVideoTexture::draw(int x, int y, int w, int h) {

	if(currentFrame==NULL) return;
	bind();
	glBegin(GL_QUADS);
	glTexCoord2f(0, 0);
	glVertex2f(x, y);
	glTexCoord2f(width, 0);
	glVertex2f(x+w, y);
	glTexCoord2f(width, height);
	glVertex2f(x+w, y+h);
	glTexCoord2f(0, height);
	glVertex2f(x, y+h);
	glEnd();
	unbind();
}

void ofxQTKitVideoTexture::draw(int x, int y, int w, int h, float s, float t, float sw, float th) {
	
	if(currentFrame==NULL) return;
	bind();
	glBegin(GL_QUADS);
	glTexCoord2f(s, t);
	glVertex2f(x, y);
	glTexCoord2f(s+sw, t);
	glVertex2f(x+w, y);
	glTexCoord2f(s+sw, t+th);
	glVertex2f(x+w, y+h);
	glTexCoord2f(s, t+th);
	glVertex2f(x, y+h);
	glEnd();
	unbind();
}