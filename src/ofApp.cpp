#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup(){
	ofBackground(255,255,255);
	ofSetVerticalSync(true);

	// Uncomment this to show movies with alpha channels
	videoPlayer.setPixelFormat(OF_PIXELS_RGB);
    videoPlayer.setLoopState(OF_LOOP_NORMAL);
    ofAddListener(videoRecorder.outputFileCompleteEvent, this, &ofApp::recordingComplete);
    
    strcpy(inputStartFrame, ofToString("0").c_str());
    strcpy(slitSize, ofToString("1").c_str());
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
    ofFileDialogResult result = ofSystemLoadDialog("Select video or image...");
    if(result.bSuccess) {
        setInputFilePath(result.getPath());
        reload();
    }
}

void ofApp::reload() {
    errorLoading = false;
    isLoaded = false;
    savedAs = NOT_SAVED;
    strcpy(inputStartFrame, ofToString("0").c_str());
    numFramesSaved = 0;
    if (inputType == INPUT_WEBCAM) {
        clearBuffer();
    }
    else {
        ofFile file(inputFilePath);
        std::string ext = ofToLower(file.getExtension());
        bool bOk = false;
        if (file.isDirectory() || ofxImageSequencePlayer::isAllowedExt(ext)) {
            bOk = imagePlayer.load(inputFilePath);
            if (!bOk) {
                errorLoading = true;
            }
            source = &imagePlayer;
            sourcePlayer = &imagePlayer;
        }
        else {
            bOk = videoPlayer.load(inputFilePath);
            if (!bOk) {
                errorLoading = true;
            }
            source = &videoPlayer;
            sourcePlayer = &videoPlayer;
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
            updateSource();
        }
        else if (isLoading && sourcePlayer->isLoaded()) {
            sourceLoaded();
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
                isLoading2 = resultPlayer.load(outputFilePath);
            }
            else if (resultPlayer.isLoaded()) {
                isLoading2 = false;
                isLoaded2 = true;
                resultPlayer.play();
            }
        }
        else {
            resultPlayer.update();
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

void ofApp::sourceLoaded() {
    isLoading = false;
    isLoaded = true;
    clearBuffer();
    if (swapAxis == SWAP_X) {
        strcpy(outputWidth, ofToString(MIN(sourcePlayer->getWidth(), sourcePlayer->getTotalNumFrames())).c_str());
        strcpy(outputHeight, ofToString(sourcePlayer->getHeight()).c_str());
    }
    else {
        strcpy(outputWidth, ofToString(sourcePlayer->getWidth()).c_str());
        strcpy(outputHeight, ofToString(MIN(sourcePlayer->getHeight(), sourcePlayer->getTotalNumFrames())).c_str());
    }
    strcpy(outputFps, ofToString(sourcePlayer->getTotalNumFrames() / sourcePlayer->getDuration()).c_str());
    strcpy(slitSize, ofToString("1").c_str());
	sourcePlayer->setPaused(true);
}

void ofApp::updateSource() {
    sourcePlayer->update();
    inputWidth = sourcePlayer->getWidth();
    inputHeight = sourcePlayer->getHeight();
    int newStartFrame = MIN(MAX(0, ofToInt(inputStartFrame)), sourcePlayer->getTotalNumFrames() - 1);
    if (newStartFrame != startFrame) {
        startFrame = newStartFrame;
        sourcePlayer->setFrame(startFrame);
        clearBuffer();
    }
    inputFrames = sourcePlayer->getTotalNumFrames() - startFrame;
    inputFrameRate = sourcePlayer->getTotalNumFrames() / sourcePlayer->getDuration();
    updateOutputProps();
    updateBufferSize();
    updateSourceSize();
    updateResultSize();
    if (!bufferIsFilled()) {
		if (sourcePlayer->isFrameNew() || stuckFrames > 15) {
            buffer.push_back(sourcePlayer->getPixels());
            sourcePlayer->nextFrame();
			stuckFrames = 0;
        }
		else if ((sourcePlayer->getCurrentFrame() - startFrame) == buffer.size()) {
			stuckFrames++;
		}
    }
    else if (savedAs != SAVE_FFMPEG) {
        updateCurrentFrame();
    }
}

void ofApp::updateOutputProps() {
    slit = MAX(1, ofToInt(slitSize));
    if (swapAxis == SWAP_X) {
        strcpy(outputHeight, ofToString(inputHeight).c_str());
        outputFrames = ceil(inputWidth / (float) MIN(inputWidth, slit));
    }
    else {
        strcpy(outputWidth, ofToString(inputWidth).c_str());
        outputFrames = ceil(inputHeight / (float) MIN(inputHeight, slit));
    }
    outputFrameRate = ofToFloat(outputFps);
    if (outputFrameRate <= 0 || outputFrameRate > 60) {
        outputFrameRate = DEFAULT_FPS;
    }
}

void ofApp::clearBuffer() {
    previewFrame = 0;
    stuckFrames = 0;
    buffer.clear();
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
        clearBuffer();
        if (inputType == INPUT_FILE) {
            sourcePlayer->setFrame(startFrame);
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
                int k = MIN(i, buffer.size() - 1);
                pix.setColor(i, j, buffer[k / (float)slit].getColor((frameNum * slit + frameNum + i % slit) % buffer[0].getWidth(), j));
            }
        }
    }
    else {
        pix.allocate(buffer[0].getWidth(), bufferSize, buffer[0].getNumChannels());
        for (int i=0; i<pix.getWidth(); i++) {
            for (int j=0; j<pix.getHeight(); j++) {
                int k = MIN(j, buffer.size() - 1);
                pix.setColor(i, j, buffer[k / (float)slit].getColor(i, (frameNum * slit + frameNum + j % slit) % buffer[0].getHeight()));
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
            resultPlayer.draw(40 + halfW + (halfW - resultW) / 2.f, 20 + (halfH - resultH) / 2.f, resultW, resultH);
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
        ImGui::InputText("slit size", slitSize, IM_ARRAYSIZE(slitSize));
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
            fileName = ofFilePath::getBaseName(ofToString(inputFilePath));
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
            sourcePlayer->setFrame(startFrame);
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
            resultPlayer.setPaused(sourcePlayer->isPlaying());
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
