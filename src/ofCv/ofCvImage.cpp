
#include "ofCvImage.h"
#include "ofCvGrayscaleImage.h"
#include "ofCvColorImage.h"
#include "ofCvFloatImage.h"




ofCvImage::ofCvImage() {
    width = 0;
    height = 0;
    bUseTexture = true;
}


ofCvImage::~ofCvImage() {
    clear();
}

void ofCvImage::clear() {
	cvReleaseImage( &cvImage );
	cvReleaseImage( &cvImageTemp );
	delete pixels;
    width = 0;
    height = 0;
    
    if( bUseTexture ) {
        tex.clear();
    }
}


void ofCvImage::setUseTexture( bool bUse ) {
	bUseTexture = bUse;
}


void ofCvImage::swapTemp() {
	IplImage*  temp;
	temp = cvImage;
	cvImage	= cvImageTemp;
	cvImageTemp	= temp;
}





// Set Pixel Data - Scalars
//
//
void ofCvImage::set( int value ) {
	cvSet( cvImage, cvScalar(value) );
}

void ofCvImage::operator -= ( float scalar ) {
	cvSubS( cvImage, cvScalar(scalar), cvImageTemp );
	swapTemp();
}

void ofCvImage::operator += ( float scalar ) {
	cvAddS( cvImage, cvScalar(scalar), cvImageTemp );
	swapTemp();
}





// Set Pixel Data - Arrays
//
//    




// Image Filter Operations
//
//
void ofCvImage::dilate( int nIterations ) {
	cvDilate( cvImage, cvImageTemp, 0, nIterations );
	swapTemp();
}

void ofCvImage::erode( int nIterations ) {
	cvErode( cvImage, cvImageTemp, 0, nIterations );
	swapTemp();
}


void ofCvImage::blur( int value ) {
	cvSmooth( cvImage, cvImageTemp, CV_BLUR , value);
	swapTemp();
}

void ofCvImage::blurGaussian( int value ) {
	cvSmooth( cvImage, cvImageTemp, CV_GAUSSIAN ,value );
	swapTemp();
}





// Image Transformation Operations
//
//
void ofCvImage::mirror( bool bFlipVertically, bool bFlipHorizontally ) {
	int flipMode = 0;
    
	if( bFlipVertically && !bFlipHorizontally ) flipMode = 0;
	else if( !bFlipVertically && bFlipHorizontally ) flipMode = 1;
	else if( bFlipVertically && bFlipHorizontally ) flipMode = -1;
	else return;

	cvFlip( cvImage, cvImageTemp, flipMode );
	swapTemp();
}


void ofCvImage::translate( float x, float y ) {
    transform( 0, 0,0, 1,1, x,y );
}

void ofCvImage::rotate( float angle, float centerX, float centerY ) {
    transform( angle, centerX, centerY, 1,1, 0,0 );
}

void ofCvImage::scale( float scaleX, float scaleY ) {
    transform( 0, 0,0, scaleX,scaleY, 0,0 );
}

void ofCvImage::transform( float angle, float centerX, float centerY, 
                           float scaleX, float scaleY,
                           float moveX, float moveY )
{
    float sina = sin(angle * DEG_TO_RAD);
    float cosa = cos(angle * DEG_TO_RAD);
    CvMat*  transmat = cvCreateMat( 2,3, CV_32F );
    cvmSet( transmat, 0,0, scaleX*cosa );
    cvmSet( transmat, 0,1, scaleY*sina );
    cvmSet( transmat, 0,2, -centerX*scaleX*cosa - centerY*scaleY*sina + moveX + centerX );
    cvmSet( transmat, 1,0, -1.0*scaleX*sina );
    cvmSet( transmat, 1,1, scaleY*cosa );
    cvmSet( transmat, 1,2, -centerY*scaleY*cosa + centerX*scaleX*sina + moveY + centerY);
    
    cvWarpAffine( cvImage, cvImageTemp, transmat );
	swapTemp();
    
    cvReleaseMat( &transmat );
}



void ofCvImage::undistort( float radialDistX, float radialDistY,
                           float tangentDistX, float tangentDistY,
                           float focalX, float focalY,
                           float centerX, float centerY )
{   
    float camIntrinsics[] = { focalX, 0, centerX, 0, focalY, centerY, 0, 0, 1 };
    float distortionCoeffs[] = { radialDistX, radialDistY, tangentDistX, tangentDistY };
    cvUnDistortOnce( cvImage, cvImageTemp, camIntrinsics, distortionCoeffs, 1 );
	swapTemp();
}


void ofCvImage::remap( IplImage* mapX, IplImage* mapY ) {
    cvRemap( cvImage, cvImageTemp, mapX, mapY );
	swapTemp();
}




/**
*    A  +-------------+  B
*      /               \
*     /                 \
*    /                   \
* D +-------------------- +  C
*/
void ofCvImage::warpPerspective( const ofPoint& A, const ofPoint& B,
                                 const ofPoint& C, const ofPoint& D )
{
	// compute matrix for perspectival warping (homography)
	CvPoint2D32f cvsrc[4];
	CvPoint2D32f cvdst[4];
	CvMat* translate = cvCreateMat( 3,3, CV_32FC1 );
	cvSetZero( translate );
    
    cvsrc[0].x = 0;
    cvsrc[0].y = 0;    
    cvsrc[1].x = width;
    cvsrc[1].y = 0;    
    cvsrc[2].x = width;
    cvsrc[2].y = height;    
    cvsrc[3].x = 0;
    cvsrc[3].y = height;    
    
    cvdst[0].x = A.x;
    cvdst[0].y = A.y;    
    cvdst[1].x = B.x;
    cvdst[1].y = B.y;    
    cvdst[2].x = C.x;
    cvdst[2].y = C.y;    
    cvdst[3].x = D.x;
    cvdst[3].y = D.y;    
    
	cvWarpPerspectiveQMatrix( cvsrc, cvdst, translate );  // calculate homography
	cvWarpPerspective( cvImage, cvImageTemp, translate );
    swapTemp();
	cvReleaseMat( &translate );
}



void ofCvImage::warpIntoMe( const ofCvGrayscaleImage& mom, 
                            ofPoint src[4], ofPoint dst[4] )
{
	// compute matrix for perspectival warping (homography)
	CvPoint2D32f cvsrc[4];
	CvPoint2D32f cvdst[4];
	CvMat* translate = cvCreateMat( 3, 3, CV_32FC1 );
	cvSetZero( translate );
	for (int i = 0; i < 4; i++ ) {
		cvsrc[i].x = src[i].x;
		cvsrc[i].y = src[i].y;
		cvdst[i].x = dst[i].x;
		cvdst[i].y = dst[i].y;
	}
	cvWarpPerspectiveQMatrix( cvsrc, cvdst, translate );  // calculate homography
	cvWarpPerspective( mom.getCvImage(), cvImage, translate);
	cvReleaseMat( &translate );
}




// Other Image Operations
//
// 
int ofCvImage::countNonZeroInRegion( int x, int y, int w, int h ) const {
	int count = 0;
	cvSetImageROI( cvImage, cvRect(x,y,w,h) );
	count = cvCountNonZero( cvImage );
	cvResetImageROI( cvImage );
	return count;
}





