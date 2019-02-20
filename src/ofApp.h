#pragma once

#include "ofMain.h"
#include "ofxImGui.h"
#include "ofxVideoRecorder.h"

//#ifdef TARGET_WIN32
//#include "ofxWMFVideoPlayer.h"
//#endif

#define INPUT_FILE 0
#define INPUT_WEBCAM 1

#define SWAP_X 0
#define SWAP_Y 1

#define DEFAULT_FPS 25

#define NOT_SAVED -1
#define SAVE_IMG_SEQ 0
#define SAVE_FFMPEG 1

#define EXT_JPG 0
#define EXT_PNG 1

class ofApp : public ofBaseApp{

public:

    void setup();
    void update();
    void draw();
    void exit();
    
    void initializeWebcam();
    void updateWebcam();
    void videoLoaded();
    void updateVideo();
    void updateOutputProps();
    void updateBufferSize();
    void updateSourceSize();
    void updateResultSize();
    void updateCurrentFrame();
    bool bufferIsFilled();
    void fillBuffer();
    void saveBuffer();
    void saveImageSequence();
    void saveFFmpeg();
    ofPixels getOutputFrame(const int frameNum) const;
		
    void keyPressed(int key);
    void keyReleased(int key);
    void mouseMoved(int x, int y );
    void mouseDragged(int x, int y, int button);
    void mousePressed(int x, int y, int button);
    void mouseReleased(int x, int y, int button);
    void mouseEntered(int x, int y);
    void mouseExited(int x, int y);
    void windowResized(int w, int h);
    void dragEvent(ofDragInfo dragInfo);
    void gotMessage(ofMessage msg);
    void recordingComplete(ofxVideoRecorderOutputFileCompleteEventArgs& args);

    bool                webcamAvailable;
    ofVideoGrabber      webcam;
//#ifdef TARGET_WIN32
//	ofxWMFVideoPlayer       originalPlayer;
//	ofxWMFVideoPlayer       convertedPlayer;
//#else
	ofVideoPlayer       originalPlayer;
	ofVideoPlayer       convertedPlayer;
//#endif
    ofBaseVideoDraws*   source;
    bool                errorLoading;
    bool                isLoading;
    bool                isLoading2;
    bool                isLoaded;
    bool                isLoaded2;
    bool                isSaving;
    int                 savedAs = -1;
    ofxVideoRecorder    videoRecorder;
    int                 startFrame = 0;
    int                 numFramesSaved;
    std::vector<ofPixels> buffer;
    int                 bufferSize;
    int                 bufferMaxSize;
    ofxImGui::Gui       gui;
    int                 inputType;
    char                inputFilePath[512];
    char                inputStartFrame[24];
    char                outputWidth[24];
    char                outputHeight[24];
    char                outputFps[24];
    char                videoBitrate[24];
    char                videoCodec[24];
    char                videoPixelFormat[24];
    std::string         outputFilePath;
    int                 swapAxis;
    ofPixels            currentPixels;
    int                 previewFrame;
    float               previewTime = 0;
    float               inputFrameRate;
    float               outputFrameRate;
    int                 saveAs;
    int                 imgSeqExt = 0;
	int					stuckFrames = 0;

    void setInputFilePath(const std::string &value);
    void browseMovie();
    void reload();
    void drawSource();
    void drawResult();
    void drawBufferProgress();
    void drawSaveProgress();
    void drawInputPanel();
    void drawOutputPanel();
    void startSaving();
    void stopSaving();
    void onSavingStopped();
    
    int inputWidth;
    int inputHeight;
    int inputFrames;
    int outputFrames;
    
    float halfW;
    float halfH;
    float webcamW;
    float webcamH;
    float sourceW;
    float sourceH;
    float resultW;
    float resultH;
};

