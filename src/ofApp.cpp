#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup(){
	ofBackground(255,255,255);
	ofSetVerticalSync(true);

	// Uncomment this to show movies with alpha channels
	originalPlayer.setPixelFormat(OF_PIXELS_RGB);
    originalPlayer.setLoopState(OF_LOOP_NORMAL);
    ofAddListener(videoRecorder.outputFileCompleteEvent, this, &ofApp::recordingComplete);
    
    strcpy(inputStartFrame, ofToString("0").c_str());
    strcpy(outputWidth, ofToString("").c_str());
    strcpy(outputHeight, ofToString("").c_str());
    strcpy(outputFps, ofToString(DEFAULT_FPS).c_str());
    strcpy(videoBitrate, ofToString("20M").c_str());
    strcpy(videoCodec, ofToString("libx264").c_str());
    //videoCodec = "mpeg4";
    strcpy(videoPixelFormat, "yuv420p");
    //videoPixelFormat = "";
    swapAxis = SWAP_X;

    ofEnableAlphaBlending();

    webcamW = 640;
    webcamH = 480;
    webcamAvailable = webcam.listDevices().size() > 0;
    inputType = INPUT_FILE;
    gui.setup();
    windowResized(ofGetWidth(), ofGetHeight());
}

void ofApp::setInputFilePath(const std::string &value) {
    inputType = INPUT_FILE;
    strcpy(inputFilePath, value.c_str());
}

void ofApp::browseMovie() {
    ofFileDialogResult result = ofSystemLoadDialog("Select video");
    if(result.bSuccess) {
        setInputFilePath(result.getPath());
        reload();
    }
}

void ofApp::reload() {
    errorLoading = false;
    isLoaded = false;
    savedAs = NOT_SAVED;
    strcpy(inputStartFrame, std::string("0").c_str());
    numFramesSaved = 0;
    if (inputType == INPUT_WEBCAM) {
        previewFrame = 0;
        buffer.clear();
    }
    else {
        bool bOk = originalPlayer.load(inputFilePath);
        if (!bOk) {
            errorLoading = true;
        }
        isLoading = bOk;
    }
}

//--------------------------------------------------------------
void ofApp::update() {
    if (inputType == INPUT_WEBCAM) {
        updateWebcam();
    }
    else {
        webcam.close();
        if (isLoaded) {
            updateVideo();
        }
        else if (isLoading && originalPlayer.isLoaded()) {
            videoLoaded();
        }
    }
    
    if (isSaving) {
        if (outputFrames > numFramesSaved) {
            saveBuffer();
        }
    }
    else if (savedAs == SAVE_FFMPEG) {
        if (!isLoaded2) {
            if (!isLoading2) {
                isLoading2 = convertedPlayer.load(outputFilePath);
            }
            else if (convertedPlayer.isLoaded()) {
                isLoading2 = false;
                isLoaded2 = true;
                convertedPlayer.play();
            }
        }
        else {
            convertedPlayer.update();
        }
    }
}

void ofApp::initializeWebcam() {
    webcam.setDeviceID(0);
    webcam.setDesiredFrameRate(DEFAULT_FPS);
    webcam.initGrabber(webcamW, webcamH);
    strcpy(outputWidth, ofToString(webcamW).c_str());
    strcpy(outputHeight, ofToString(webcamH).c_str());
    savedAs = NOT_SAVED;
    source = &webcam;
}

void ofApp::updateWebcam() {
    if (!webcam.isInitialized()) {
        initializeWebcam();
    }
    webcam.update();
    inputWidth = webcamW;
    inputHeight = webcamH;
    inputFrames = inputWidth;
    inputFrameRate = DEFAULT_FPS;
    updateOutputProps();
    updateBufferSize();
    updateSourceSize();
    updateResultSize();
    if (!bufferIsFilled()) {
        if (webcam.isFrameNew()) {
            buffer.push_back(webcam.getPixels());
        }
    }
    else if (savedAs != SAVE_FFMPEG) {
        updateCurrentFrame();
    }
}

void ofApp::videoLoaded() {
    isLoading = false;
    isLoaded = true;
    previewFrame = 0;
    buffer.clear();
    source = &originalPlayer;
    if (swapAxis == SWAP_X) {
        strcpy(outputWidth, ofToString(MIN(originalPlayer.getWidth(), originalPlayer.getTotalNumFrames())).c_str());
        strcpy(outputHeight, ofToString(originalPlayer.getHeight()).c_str());
    }
    else {
        strcpy(outputWidth, ofToString(originalPlayer.getWidth()).c_str());
        strcpy(outputHeight, ofToString(MIN(originalPlayer.getHeight(), originalPlayer.getTotalNumFrames())).c_str());
    }
    strcpy(outputFps, ofToString(originalPlayer.getTotalNumFrames() / originalPlayer.getDuration()).c_str());
	originalPlayer.setPaused(true);
}

void ofApp::updateVideo() {
    originalPlayer.update();
    inputWidth = originalPlayer.getWidth();
    inputHeight = originalPlayer.getHeight();
    int newStartFrame = MIN(MAX(0, ofToInt(inputStartFrame)), originalPlayer.getTotalNumFrames() - 1);
    if (newStartFrame != startFrame) {
        startFrame = newStartFrame;
        originalPlayer.setFrame(startFrame);
        previewFrame = 0;
        buffer.clear();
    }
    inputFrames = originalPlayer.getTotalNumFrames() - startFrame;
    inputFrameRate = originalPlayer.getTotalNumFrames() / originalPlayer.getDuration();
    updateOutputProps();
    updateBufferSize();
    updateSourceSize();
    updateResultSize();
    if (!bufferIsFilled()) {
		if (originalPlayer.isFrameNew() || stuckFrames > 15) {
            buffer.push_back(originalPlayer.getPixels());
            originalPlayer.nextFrame();
			stuckFrames = 0;
        }
		else if ((originalPlayer.getCurrentFrame() - startFrame) == buffer.size()) {
			stuckFrames++;
		}
    }
    else if (savedAs != SAVE_FFMPEG) {
        updateCurrentFrame();
    }
}

void ofApp::updateOutputProps() {
    if (swapAxis == SWAP_X) {
        strcpy(outputHeight, ofToString(inputHeight).c_str());
        outputFrames = inputWidth;
    }
    else {
        strcpy(outputWidth, ofToString(inputWidth).c_str());
        outputFrames = inputHeight;
    }
    outputFrameRate = ofToFloat(outputFps);
    if (outputFrameRate <= 0 || outputFrameRate > 60) {
        outputFrameRate = DEFAULT_FPS;
    }
}

void ofApp::updateBufferSize() {
    int newBufferSize = MAX(ofToInt(outputWidth), MIN(inputWidth, inputFrames));
    if (swapAxis == SWAP_Y) {
        newBufferSize = MAX(ofToInt(outputHeight), MIN(inputHeight, inputFrames));
    }
    if (newBufferSize != bufferSize || MIN(newBufferSize, inputFrames) != bufferMaxSize) {
        bufferSize = newBufferSize;
        bufferMaxSize = MIN(bufferSize, inputFrames);
        // todo: resize
        previewFrame = 0;
        buffer.clear();
        if (inputType == INPUT_FILE) {
            originalPlayer.setFrame(startFrame);
        }
    }
}

void ofApp::updateSourceSize() {
    sourceW = MIN(halfW, inputWidth);
    sourceH = sourceW / inputWidth * inputHeight;
    if (sourceH > halfH) {
        sourceH = MIN(halfH, inputHeight);
        sourceW = sourceH / inputHeight * inputWidth;
    }
}

void ofApp::updateResultSize() {
    resultW = MIN(halfW, inputWidth);
    resultH = resultW / (float) ofToInt(outputWidth) * inputHeight;
    if (resultH > halfH) {
        resultH = MIN(halfH, inputHeight);
        resultW = resultH / inputHeight * (float) ofToInt(outputWidth);
    }
}

void ofApp::updateCurrentFrame() {
    int currentFrame = previewFrame;
    if (isSaving) {
        currentFrame = numFramesSaved % outputFrames;
    }
    currentPixels = getOutputFrame(currentFrame);
    float timef = ofGetElapsedTimef();
    if (!isSaving && (timef - previewTime) >= (1.f / outputFrameRate)) {
        previewTime = timef;
        previewFrame++;
        if (previewFrame == outputFrames) {
            previewFrame = 0;
        }
    }
}

bool ofApp::bufferIsFilled() {
    return buffer.size() > 0 && bufferMaxSize == buffer.size();
}

void ofApp::saveBuffer() {
    if (saveAs == SAVE_IMG_SEQ) {
        saveImageSequence();
    }
    else {
        saveFFmpeg();
    }
    numFramesSaved++;
    if (numFramesSaved == outputFrames) {
        stopSaving();
    }
}

void ofApp::saveImageSequence() {
    std::string path = outputFilePath + "/" + ofToString(numFramesSaved) + "." + (imgSeqExt == EXT_JPG ? "jpg" : "png");
    ofSaveImage(currentPixels, path);
}

void ofApp::saveFFmpeg() {
    if (numFramesSaved == 0) {
        videoRecorder.setVideoCodec(videoCodec);
        videoRecorder.setVideoBitrate(videoBitrate);
        videoRecorder.setOutputPixelFormat(videoPixelFormat);
        if (swapAxis == SWAP_X) {
            videoRecorder.setup(outputFilePath, bufferSize, ofToInt(outputHeight), outputFrameRate/*, sampleRate, channels*/);
        }
        else {
            videoRecorder.setup(outputFilePath, ofToInt(outputHeight), bufferSize, outputFrameRate/*, sampleRate, channels*/);
        }
        videoRecorder.start();
    }
    
    bool success = videoRecorder.addFrame(currentPixels);
    if (!success) {
        ofLogWarning("This frame was not added!");
    }
    
    if (videoRecorder.hasVideoError()) {
        ofLogWarning("The video recorder failed to write some frames!");
    }
}

ofPixels ofApp::getOutputFrame(const int frameNum) const {
    ofPixels pix;
    if (swapAxis == SWAP_X) {
        pix.allocate(bufferSize, buffer[0].getHeight(), buffer[0].getNumChannels());
        for (int i=0; i<pix.getWidth(); i++) {
            for (int j=0; j<pix.getHeight(); j++) {
                if (i < buffer.size()) {
                    pix.setColor(i, j, buffer[i].getColor(frameNum, j));
                }
                else {
                    pix.setColor(i, j, ofColor::black);
                }
            }
        }
    }
    else {
        pix.allocate(buffer[0].getWidth(), bufferSize, buffer[0].getNumChannels());
        for (int i=0; i<pix.getWidth(); i++) {
            for (int j=0; j<pix.getHeight(); j++) {
                if (j < buffer.size()) {
                    pix.setColor(i, j, buffer[j].getColor(i, frameNum));
                }
                else {
                    pix.setColor(i, j, ofColor::black);
                }
            }
        }

    }
    return pix;
}

//--------------------------------------------------------------
void ofApp::draw() {
    drawSource();
    drawResult();
    drawBufferProgress();
    drawSaveProgress();
    gui.begin();
    drawInputPanel();
    drawOutputPanel();
    gui.end();
}

void ofApp::drawSource() {
    ofSetHexColor(0xFFFFFF);
    if (inputType == INPUT_WEBCAM || isLoaded) {
        source->draw(20 + (halfW - sourceW) / 2.f, 20 + (halfH - sourceH) / 2.f, sourceW, sourceH);
    }
}

void ofApp::drawResult() {
    ofSetHexColor(0xFFFFFF);
    if (savedAs == SAVE_FFMPEG) {
        if (isLoaded2) {
            convertedPlayer.draw(40 + halfW + (halfW - resultW) / 2.f, 20 + (halfH - resultH) / 2.f, resultW, resultH);
        }
    }
    else {
        if (bufferIsFilled()) {
            ofImage image;
            image.setFromPixels(currentPixels);
            image.draw(40 + halfW + (halfW - resultW) / 2.f, 20 + (halfH - resultH) / 2.f, resultW, resultH);
        }
    }
}

void ofApp::drawBufferProgress() {
    if (savedAs == NOT_SAVED) {
        if (inputType == INPUT_WEBCAM || isLoaded) {
            ofSetHexColor(0x000000);
            if (!bufferIsFilled()) {
                float percDone = ((float) buffer.size() / bufferMaxSize);
                ofNoFill();
                ofDrawRectangle(20, ofGetHeight() / 2.f - 10, halfW, 20);
                ofFill();
                ofDrawRectangle(20, ofGetHeight() / 2.f - 10, percDone * halfW, 20);
            }
        }
    }
}

void ofApp::drawSaveProgress() {
    if (isSaving) {
        float percDone = ((float) numFramesSaved / outputFrames);
        ofNoFill();
        ofDrawRectangle(40 + halfW, ofGetHeight() / 2.f - 10, halfW, 20);
        ofFill();
        ofDrawRectangle(40 + halfW, ofGetHeight() / 2.f - 10, percDone * halfW, 20);
    }
}

void ofApp::drawInputPanel() {
    ImGui::SetNextWindowPos(ImVec2(20, ofGetHeight() / 2.f + 20), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(halfW, halfH), ImGuiCond_Always);
    ImGui::Begin("input", NULL, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus);
    if (inputType == INPUT_WEBCAM || isLoaded) {
        ImGui::Text((ofToString(inputWidth) + " x " + ofToString(inputHeight)).c_str());
        ImGui::Text((ofToString(inputFrames) + " frames").c_str());
        ImGui::Text((ofToString(inputFrameRate) + " fps").c_str());
        ImGui::Spacing();
    }
    if (isSaving) {
        ImGui::Text(("input file: " + ofToString(inputFilePath)).c_str());
    }
    else {
        if (webcamAvailable) {
            ImGui::RadioButton("file", &inputType, INPUT_FILE);
            ImGui::RadioButton("webcam", &inputType, INPUT_WEBCAM);
        }
        if (inputType == INPUT_FILE) {
            ImGui::InputText("input file", inputFilePath, IM_ARRAYSIZE(inputFilePath));
            ImGui::InputText("start frame", inputStartFrame, IM_ARRAYSIZE(inputStartFrame));
            if (ImGui::Button("browse")) {
                browseMovie();
            }
        }
        if (ImGui::Button("reload")) {
            reload();
        }
    }
    if (errorLoading) {
        ImGui::Text("Failed to load video. Make sure you have codecs installed on your system. We recommend the free K-Lite Codec pack.");
    }
    ImGui::End();
}

void ofApp::drawOutputPanel() {
    ImGui::SetNextWindowPos(ImVec2(ofGetWidth() / 2.f + 10, ofGetHeight() / 2.f + 20), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(halfW, halfH), ImGuiCond_Always);
    ImGui::Begin("output", NULL, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus);
    if (inputType == INPUT_WEBCAM || isLoaded) {
        float w = ofToInt(outputWidth);
        float h = ofToInt(outputHeight);
        if (swapAxis == SWAP_X) {
            if (w < inputFrames)
                w = inputFrames;
        }
        else {
            if (h < inputFrames)
                h = inputFrames;
        }
        ImGui::Text((ofToString(w) + " x " + ofToString(h)).c_str());
        ImGui::Text((ofToString(outputFrames) + " frames").c_str());
        ImGui::Text((ofToString(outputFrameRate) + " fps").c_str());
        ImGui::Spacing();
    }
    if (isSaving) {
        if (saveAs == SAVE_IMG_SEQ) {
            ImGui::Text(("output dir: " + outputFilePath).c_str());
            ImGui::Text(("output ext: " + ofToString(imgSeqExt == EXT_JPG ? "jpg" : "png")).c_str());
        }
        else {
            ImGui::Text(("output file: " + outputFilePath).c_str());
            ImGui::Text(("bitrate: " + ofToString(videoBitrate)).c_str());
            ImGui::Text(("codec: " + ofToString(videoCodec)).c_str());
            ImGui::Text(("pixel format:" + ofToString(videoPixelFormat)).c_str());
        }
        if (ImGui::Button("cancel")) {
            stopSaving();
        }
    }
    else {
        ImGui::RadioButton("x <-> time", &swapAxis, SWAP_X);
        ImGui::RadioButton("y <-> time", &swapAxis, SWAP_Y);
        if (swapAxis == SWAP_X) {
            ImGui::InputText("width", outputWidth, IM_ARRAYSIZE(outputWidth));
        }
        else {
            ImGui::InputText("height", outputHeight, IM_ARRAYSIZE(outputHeight));
        }
        ImGui::InputText("framerate", outputFps, IM_ARRAYSIZE(outputFps));
        ImGui::Spacing();
        ImGui::Text("save as: ");
        ImGui::RadioButton("image sequence", &saveAs, SAVE_IMG_SEQ);
        #ifndef TARGET_WIN32
        ImGui::RadioButton("ffmpeg", &saveAs, SAVE_FFMPEG);
        #endif
        ImGui::Spacing();
        if (saveAs == SAVE_IMG_SEQ) {
            ImGui::Text("image sequence options: ");
            ImGui::RadioButton("jpg", &imgSeqExt, EXT_JPG);
            ImGui::RadioButton("png", &imgSeqExt, EXT_PNG);
        }
        else {
            ImGui::Text("ffmpeg options: ");
            ImGui::InputText("bitrate", videoBitrate, IM_ARRAYSIZE(videoBitrate));
            ImGui::InputText("codec", videoCodec, IM_ARRAYSIZE(videoCodec));
            ImGui::InputText("pixel format", videoPixelFormat, IM_ARRAYSIZE(videoPixelFormat));
        }
        if (bufferIsFilled()) {
            if (strlen(inputFilePath) == 0) {
                //ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
            }
            if (ImGui::Button("save")) {
                startSaving();
            }
            if (strlen(inputFilePath) == 0) {
                //ImGui::PopItemFlag();
            }
        }
    }
    ImGui::End();
}

void ofApp::startSaving() {
    ofFileDialogResult result;
    if (saveAs == SAVE_IMG_SEQ) {
        result = ofSystemLoadDialog("Select output folder", true);
    }
    else {
        std::string fileName = "webcam";
        if (inputType == INPUT_FILE) {
            fileName = ofFilePath::getBaseName(originalPlayer.getMoviePath());
        }
        std::string fileExt = ".mp4"; // ffmpeg uses the extension to determine the container type. run 'ffmpeg -formats' to see supported formats
        std::string defaultName = fileName + "_" + ofGetTimestampString() + fileExt;
        result = ofSystemSaveDialog(defaultName, "Save video file as...");
    }
    if (result.bSuccess) {
        outputFilePath = result.getPath();
        savedAs = NOT_SAVED;
        isSaving = true;
        if (inputType == INPUT_FILE) {
            originalPlayer.setFrame(startFrame);
        }
    }
}

void ofApp::stopSaving() {
    if (saveAs == SAVE_IMG_SEQ) {
        onSavingStopped();
    }
    else {
        videoRecorder.close();
        #ifdef TARGET_WIN32
        // on windows there is no event
        onSavingStopped();
        #endif
    }
}

void ofApp::onSavingStopped() {
    previewFrame = 0;
    numFramesSaved = 0;
    savedAs = saveAs;
    isSaving = false;
    isLoading2 = false;
    isLoaded2 = false;
}

void ofApp::recordingComplete(ofxVideoRecorderOutputFileCompleteEventArgs& args){
    onSavingStopped();
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){

}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){

}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ){

}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){
	
}


//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){
    if (isSaving) {
        return;
    }
    
    if (isLoaded2) {
        if (x >= 40 + halfW + (halfW - resultW) / 2.f && x <= 40 + halfW + (halfW - resultW) / 2.f + resultW &&
            y >= 40 + halfH + (halfH - resultH) / 2.f && y <= 20 + (halfH - resultH) / 2.f + resultH) {
            convertedPlayer.setPaused(originalPlayer.isPlaying());
        }
    }
}

//--------------------------------------------------------------
void ofApp::mouseEntered(int x, int y){

}

//--------------------------------------------------------------
void ofApp::mouseExited(int x, int y){

}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){
    halfW = ofGetWidth() / 2.f - 30;
    halfH = ofGetHeight() / 2.f - 40;
}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){
    if (!isSaving) {
        setInputFilePath(dragInfo.files[0]);
        reload();
    }
}

void ofApp::exit() {
    ofRemoveListener(videoRecorder.outputFileCompleteEvent, this, &ofApp::recordingComplete);
    videoRecorder.close();
}
