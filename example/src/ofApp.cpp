#pragma once

#include "ofApp.h"

void ofApp::setup()
{
	//tts.setLogLevel(OF_LOG_SILENT);

	gui.setup("ofApp");
	gui.add(tts.params);
}

void ofApp::draw()
{
	drawBg();

	tts.drawDebugHelp();

	gui.draw();
}

void ofApp::drawBg()
{
	// Blink dark if stand by.
	// Blink blue if waiting reply. 
	// Blink red if received error. 
	ofColor c;
	float v = glm::cos(10 * ofGetElapsedTimef());
	float a1 = ofMap(v, -1, 1, 100, 200, true);
	float a2 = ofMap(v, -1, 1, 16, 32, true);
	if (tts.isError()) c = ofColor(a1, 0, 0);
	else if (tts.isWaiting()) c = ofColor(0, 0, a1);
	else c = ofColor(a2);
	ofClear(c);
}

void ofApp::keyPressed(int key)
{
	ofLogNotice(__FUNCTION__) << char(key);

	// Flash Bg
	v = 1;

	//--

	// Testing Helpers
	tts.keyPressed(key);

	// Browse voices
	if (key == OF_KEY_LEFT) {
		tts.setPreviousVoice();
		return;
	}
	if (key == OF_KEY_RIGHT) {
		tts.setNextVoice();
		return;
	}

	// Request a text to voice
	if (key == OF_KEY_RETURN) {
		string s = "";
		s += "Super Hi-Fi Partners with ElevenLabs to Create 'Personalized Radio' \nPowered by AI, Releases Online Radio Station to Illustrate the Incredible Potential\n";
		tts.doSend(s);
		return;
	}
}