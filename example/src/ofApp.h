#pragma once
#include "ofMain.h"

#include "ofxElevenLabs.h"
#include "ofxGui.h"

class ofApp : public ofBaseApp {
public:
	ofxElevenLabs tts;

	ofxPanel gui;
	float v = 0;

	void setup();
	void draw();
	void drawBg();
	void keyPressed(int key);
};
