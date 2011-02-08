#pragma once

#include "ofConstants.h"
#include "ofTexture.h"
#include "Quicktime/Quicktime.h"
#import <QuartzCore/CoreVideo.h>



//---------------------------------------------

class ofxQTKitVideoTexture {

public:


						ofxQTKitVideoTexture ();
	virtual				~ofxQTKitVideoTexture();

	bool				loadMovie(string name);
	
	// It's a Cocoa QTMovie, void*'s so it's C++ compatible
	void				*movie;
	
	// other quicktime defs
	QTVisualContextRef  qtVisualContext;
	CVImageBufferRef	currentFrame;
	void				threadedFunction();

	void				draw(int x, int y, int w, int h);
	void				draw(int x, int y, int w, int h, float s, float t, float sw, float th);
	float				width;
	float				height;

	void				preRollMovie(Fixed rate);
	void				setTimeBase(Fixed rate, TimeBase masterTimeBase);
	void 				play();
	void 				stop();
	
	bool 				bLoaded;

	float 				getPosition();
	float 				getSpeed();
	float 				getDuration();
	TimeBase			getTimeBase();
	
	void 				setPaused(bool bPause);	
	void 				setPosition(float pct);
	void 				setLoopState(int state);
	void   				setSpeed(float speed);
	


	float 				getHeight();
	float 				getWidth();

	void				bind();
	void				unbind();
	bool				update();	
	
	static int			videoCount;
	
protected:
	float  				speed;
	bool 				bPlaying;
	float				duration;
	

};

