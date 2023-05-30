#pragma once
#include "ofMain.h"

#include "ofxElevenLabs.h"
#include "ofxGui.h"

class ofApp : public ofBaseApp {
public:

	void setup();
	void draw();
	void drawBg();
	void keyPressed(int key);
	
	ofxElevenLabs tts;

	ofxPanel gui;
	float v = 0;

};
