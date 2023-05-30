// ofxElevenLabs.h

#define USE_TTF_ELEVEN_LABS
// Official servers requires a registered account.
// Currently a free account has a limitation of 
// 10.000 characters per month.
// Paid accounts start at 5$ / month.

#define USE_TTF_CUSTOM_SERVER
// Allows alternate between the two servers. 
// To administrate credits or when server downs as backup maybe..
// endpoint can be settled on the ofxElevenLabs_Server.json file.

//--

/*

	TODO

	fix some update blocking
		when overlap petitions.

	replace threading from std to OF classes.
		add stop thread.
		fix error when submit new text,
		before previous is answer received.

	store audios history.
		allow browse.

	some exceptions could be related to some Spanish chars?
	I found that áé fails!
	on ofJson dump: invalid UTF-8 byte at index
	#include <utf8proc.h>
	http://juliastrings.github.io/utf8proc/

*/

//--

#pragma once
#include "ofMain.h"

//--

/*

	API Help

	https://docs.elevenlabs.io/welcome/introduction

	Endpoint:
	https://api.elevenlabs.io/v1/text-to-speech/

	ApiKey format:
	xx64e8d4ca4xxxxxx398705a7c4bf6dxx

	Voice ID format:
	pNInz6obpgDQGcFmaJgB (Adam)
	https://api.elevenlabs.io/v1/text-to-speech/<voice-id>

	List of all available voices with their respective ID's:
	https://api.elevenlabs.io/v1/voicesNames

	Stability
	The stability slider determines how stable the voice is and the randomness of each new generation. Lowering this slider introduces a broader emotional range for the character - this, as mentioned before, is also influenced heavily by the original voice. Setting the slider too low may result in odd performances that are overly random and cause the character to speak too quickly. On the other hand, setting it too high can lead to a monotonous voice with limited emotion.
	​
	Similarity
	The similarity slider dictates how closely the AI should adhere to the original voice when attempting to replicate it. If the original audio is of poor quality and the similarity slider is set too high, the AI may reproduce artefacts or background noise when trying to mimic the voice if those were present in the original recording.

*/

//--

class ofxElevenLabs
{
public:
	ofxElevenLabs()
	{
		ofLogNotice("ofxElevenLabs") << "Constructor";

		setup();
	}

	~ofxElevenLabs()
	{
		ofLogNotice("ofxElevenLabs") << "Destructor";

		//TODO:
		// Join the thread to ensure that it is properly cleaned up
		if (m_thread.joinable()) {
			m_thread.join();
		}

		exit();
	}

	void setLogLevel(ofLogLevel logLevel) {
		ofLogNotice("ofxElevenLabs") << "setLogLevel:" << logLevel;
		ofSetLogLevel("ofxElevenLabs", logLevel);
	}

private:
	void setup() {
		ofLogNotice("ofxElevenLabs") << "setup";

		ofAddListener(ofEvents().update, this, &ofxElevenLabs::update);

		voiceName.setSerializable(false);
		voiceId.setSerializable(false);

		params.add(bEnable);

#if defined(USE_TTF_CUSTOM_SERVER) && defined(USE_TTF_ELEVEN_LABS)
		params.add(bModeUseCustomServer);
#else
		bModeUseCustomServer.setSerializable(false);
#endif
		params.add(voiceIndex);
		params.add(voiceName); // to ui display only. not in settings nor callbacks.
		params.add(stability);
		params.add(similarity_boost);
		params.add(vReset);
		params.add(vResend);
		params.add(vReplay);
		params.add(vRestart);

		ofAddListener(params.parameterChangedE(), this, &ofxElevenLabs::Changed_Params);

		startup();
	}

private:
	void startup() {
		ofLogNotice("ofxElevenLabs") << "startup";

		// default
		voiceIndex = 0;

		// Server settings
		doLoadSettingsServer();

		// User settings
		doLoadSettingsUser();

		//workflow
#if defined(USE_TTF_CUSTOM_SERVER) && defined(USE_TTF_ELEVEN_LABS)
#else
#ifdef USE_TTF_ELEVEN_LABS
		bModeUseCustomServer = 0;//force
#else
#ifdef USE_TTF_CUSTOM_SERVER
		bModeUseCustomServer = 1;//force
#endif
#endif
#endif
	}

	void exit() {
		ofLogNotice("ofxElevenLabs") << "exit";

		ofRemoveListener(ofEvents().update, this, &ofxElevenLabs::update);
		ofRemoveListener(params.parameterChangedE(), this, &ofxElevenLabs::Changed_Params);

		doSaveSettingsServer();
		doSaveSettingsUser();
	}

public:
	ofParameterGroup params{ "ofxElevenLabs" };

	ofParameter<bool> bEnable{ "Enable", 1 };
	vector<string> voicesNames = { "Adam", "Antoni", "Arnold", "Bella", "Josh", "Rachel", "Domi", "Elli", "Sam" };
	ofParameter<int> voiceIndex{ "VoiceIndex", 0, 0, voicesNames.size() - 1 };
	ofParameter<string> voiceName{ "VoiceName", "" };//for display only

	// Only implemented on ElevenLabs mode.
	ofParameter<float> stability{"Stability", 0.5f, 0, 1};
	ofParameter<float> similarity_boost{"Similarity Boost", 0.5f, 0, 1};

	ofParameter<void> vReset{ "Reset"};
	ofParameter<void> vReplay{ "Replay"};
	ofParameter<void> vRestart{ "Restart"};
	ofParameter<void> vResend{ "Resend"};

	ofParameter<bool> bModeUseCustomServer{ "CustomServer", 0 };
	// Enable to use your custom alternative server,
	// instead of the official ElevenLabs servers.
	// (maybe cheaper or to not spend credits or maybe an alternative TTS platform) 

private:
	ofParameter<string> voiceId{ "voiceID", "" };
	vector<string> voicesIds = { "pNInz6obpgDQGcFmaJgB", "ErXwobaYiN019PkySvjV", "VR6AewLTigWG4xSOukaG", "EXAVITQu4vr4xnSDxMaL", "TxGEqnHWrfWFTfGW9XjX", "21m00Tcm4TlvDq8ikWAM", "AZnzlk1XvdvUeBnXmlld", "MF3mGyEYCl7XYWbV9V6O", "yoZ06aMxZJJ28mfd3POQ" };

	// Hardcoded
	string strModelId = "eleven_multilingual_v1";
	//string strModelId = "eleven_monolingual_v1";

public:
	bool isError() const { return m_bError; };
	bool isWaiting() const { return m_bWaiting; };

	// To interact from the parent scope
public:
	string getErrorMessage() const {
		return m_strError;
	}
	string getResponseStatus() const {
		return m_strStatus;
	}
	string getTextDisplayHelp() const {
		return m_strTextHelpDebug;
	}
	string getText() const {
		return m_strText;
	}

private:
	ofSoundPlayer soundPlayer;
	string m_filename = "temp/audio.mp3"; // temp audio file from the last submit response.

	string pathFileSettingsUsers = "ofxElevenLabs_User.json"; // user
	string pathFileSettingsServer = "ofxElevenLabs_Server.json"; // server

	string m_strText;
	string m_strTextHelpDebug;
	string m_strError;
	string m_strStatus;

	//TODO: this is a callback flag.. not a boolean to monitor the state.
	bool m_bFlagResponseReady;

	bool m_bWaiting;
	bool m_bError;

	float m_timeLast;

	ofHttpResponse m_httpResponse;

	string urlEndpointCustomServer; // for custom servers mode only. ElevenLabs endpoint is hardcoded!
	string apiKey; // Required for official ElevenLabs servers only.

private:
	//TODO:
	std::thread m_thread;

	float timeReq; // last request time starting

	//--

	void doLoadSettingsUser() {
		ofLogNotice("ofxElevenLabs") << "doLoadSettingsUser";
		ofJson settings;
		settings = ofLoadJson(pathFileSettingsUsers);
		ofDeserialize(settings, params);
	}

	void doSaveSettingsUser() {
		ofLogNotice("ofxElevenLabs") << "doSaveSettingsUser";
		ofJson settings;
		ofSerialize(settings, params);
		ofSavePrettyJson(pathFileSettingsUsers, settings);
	}

	void doSaveSettingsServer()
	{
		ofJson j;
		j["urlEndpointCustomServer"] = urlEndpointCustomServer;
		j["apiKey"] = apiKey;

		ofSavePrettyJson(pathFileSettingsServer, j);
		ofLogNotice("ofxElevenLabs") << "doSaveSettingsServer: Saved JSON file: " << pathFileSettingsServer;
	}

	void doLoadSettingsServer()
	{
		ofLogNotice("ofxElevenLabs") << "doLoadSettingsServer";

		ofFile f;
		if (f.doesFileExist(pathFileSettingsServer))
		{
			ofJson j = ofLoadJson(pathFileSettingsServer);

			ofLogNotice("ofxElevenLabs") << "Loaded JSON file: " << pathFileSettingsServer;

			if (j.find("urlEndpointCustomServer") != j.end())
			{
				urlEndpointCustomServer = j["urlEndpointCustomServer"].get<string>();
			}
			else // tag not found
			{
				ofLogError("ofxElevenLabs") << "Could not find \"urlEndpointCustomServer\" tag.";
				urlEndpointCustomServer = "https://api.myCustomServer.com/text-to-speech"; // force default
			}

			if (j.find("apiKey") != j.end())
			{
				apiKey = j["apiKey"].get<string>();
			}
			else // tag not found
			{
				ofLogError("ofxElevenLabs") << "Could not find \"apiKey\" tag.";
				apiKey = "<your-api-key>"; // force default
			}
		}
		else
		{
			ofLogError("ofxElevenLabs") << "JSON file not found: " << pathFileSettingsServer;
			urlEndpointCustomServer = "https://api.myCustomServer.com/text-to-speech"; // force default
			apiKey = "<your-api-key>"; // force default
		}
	}

	//--

private:
	void Changed_Params(ofAbstractParameter& e)
	{
		string n = e.getName();
		ofLogNotice("ofxElevenLabs") << "Changed_Params: " << n << ": " << e;

		if (n == voiceIndex.getName())
		{
			voiceIndex = ofClamp(voiceIndex, voiceIndex.getMin(), voiceIndex.getMax());
			voiceName = voicesNames[voiceIndex];
			voiceId = voicesIds[voiceIndex];
			return;
		}
		if (n == vReset.getName())
		{
			doReset();
			return;
		}
		if (n == vRestart.getName())
		{
			doRestart();
			return;
		}
		if (n == vResend.getName())
		{
			doResend();
			return;
		}
		if (n == vReplay.getName())
		{
			doReplayAudio();
			return;
		}
	}

	void update(ofEventArgs& args)
	{
		if (!bEnable) return;

		update();
	}

	void update()
	{
		if (!bEnable) return;

		updateDebugInfo();

		// Check if the response is ready and play the audio if it is
		if (m_bFlagResponseReady)
		{
			m_bFlagResponseReady = 0;

			ofLogNotice("ofxElevenLabs") << "Received response ready.";
			saveAudioFile(m_filename);
			doPlayAudio();
		}
	}

	void updateDebugInfo()
	{
		m_strTextHelpDebug = "";

		if (m_bError) m_strTextHelpDebug += m_strError;
		else m_strTextHelpDebug += m_strStatus;

		if (m_bWaiting)
		{
			// Waiting from last request
			m_timeLast = ofGetElapsedTimef() - timeReq;
			m_strTextHelpDebug += "SENT:\nWaiting...";
			m_strTextHelpDebug += " " + ofToString(m_timeLast, 1) + "secs";
		}
		else
		{
			// Not waiting. stand by.
			m_strTextHelpDebug += "\n\nLast Delay: ";
			m_strTextHelpDebug += " " + ofToString(m_timeLast, 1) + "secs";
		}
	}

public:
	void setText(const string& text) {
		m_strText = text;
		ofLogNotice("ofxElevenLabs") << "setText: " << text;
	}

public:
	void doReset() { // reset user settings
		ofLogNotice("ofxElevenLabs") << "doReset";

		bEnable = 1;
		stability = 0.5;
		similarity_boost = 0.5;
	}
	void doRestart() { // force restart to solve problems.
		ofLogNotice("ofxElevenLabs") << "doRestart";

		doCancelRequest();
	}
	void doResend() { // send the last text again. ie for browsing voices.
		ofLogNotice("ofxElevenLabs") << "doResend";

		if (m_strText == "") return;
		doSend(m_strText);
	}

	//TODO:
	// Stop/reset last request. (can be maybe hang or failed..) 
	void doCancelRequest() {
		ofLogNotice("ofxElevenLabs") << "doCancelRequest";

		//TODO:
		// Check if a previous thread is still running and cancel it if necessary
		if (m_thread.joinable()) {
			m_thread.detach();
		}

		m_bFlagResponseReady = 0;
		m_bWaiting = 0;
		m_bError = 0;
		m_strStatus = "";
		m_strError = "Request canceled.";
		m_strTextHelpDebug = "";

		//m_thread.join();
	}

	// This is the main point to feed the API.
	// We will call by passing as argument 
	// the text that we want to speech!
	void doSend(const string& text) {
		if (!bEnable) return;
		if (text == "") return;

		ofLogNotice("ofxElevenLabs") << "doSend: " << text;

		//TODO:
		doCancelRequest();

		m_strText = text;

		timeReq = ofGetElapsedTimef();//store starting time

		//TODO:
		// Lock the mutex to ensure only one thread is executing the request at a time
		std::lock_guard<std::mutex> lock(m_mutex);

		//TODO:
		//// Check if a previous thread is still running and cancel it if necessary
		//if (m_thread.joinable()) {
		//	m_thread.detach();
		//}

		////TODO:
		//// Check if a request is already in progress and cancel it if necessary
		//if (m_bWaiting)
		//{
		//	m_bWaiting = 0;
		//	m_bFlagResponseReady = 0;
		//	m_bError = 0;
		//	m_strError = "Request canceled.";
		//	m_thread.join();
		//}

		//TODO:
		// Start a new thread to send the request
		m_thread = std::thread(&ofxElevenLabs::sendRequestThreaded, this);
		m_thread.detach();
	}

	//TODO:
private:
	std::mutex m_mutex;

	//TODO:
private:
	void sendRequestThreaded()
	{
		if (!bEnable) return;
		if (m_strText == "") return;
		const string t = m_strText;

		ofLogNotice("ofxElevenLabs") << "sendRequestThreaded";

		//TODO:
		// Lock the mutex to ensure only one thread is executing the request at a time
		std::lock_guard<std::mutex> lock(m_mutex);

		// Clear
		m_bError = 0;
		m_bFlagResponseReady = 0;
		m_strStatus = "";
		m_strError = "";
		m_strTextHelpDebug = "";

		//--

		/*
		// GET

		// Build the URL with the text and voice parameters

		string url = urlEndpointCustomServer + "?text=" + t + "&voice=" + v;
		//url = ofxSurfingHelpers::urlEncode(url);

		ofLogNotice("ofxElevenLabs")<<(__FUNCTION__) << "ofLoadURL: \n" << url;
		ofLogNotice("ofxElevenLabs")<<(__FUNCTION__) << "Waiting reply...";

		m_bWaiting = 1;

		// Send the GET request to the API endpoint and wait for the response
		m_httpResponse = ofLoadURL(url);
		*/

		//--

		// POST

		ofHttpResponse response;
		m_bWaiting = 1;

		if (bModeUseCustomServer)
		{
#ifdef USE_TTF_CUSTOM_SERVER
			const string url = urlEndpointCustomServer;
			const string v = voiceName.get();

			// Create the JSON data
			ofJson j;
			j["text"] = t;
			j["voice"] = ofToLower(v);//required in my custom server.

			// Convert the JSON data to a string and set as the request body
			ofLogNotice("ofxElevenLabs") << "ofLoadURL: \n" << url;
			ofLogNotice("ofxElevenLabs") << "json: \n" << j.dump(4);
			ofLogNotice("ofxElevenLabs") << "Waiting reply...";
			response = sendRequestPostCustomServer(urlEndpointCustomServer, j.dump());
#endif
		}
		else
		{
#ifdef USE_TTF_ELEVEN_LABS
			response = sendRequestPostElevenLabs(t);
#endif
		}

		//--

		// Set the response as the httpResponse object
		m_httpResponse = response;

		ofLogNotice("ofxElevenLabs") << "Done HTTP Response.";
		m_bWaiting = 0;

		// Check the response status code

		if (m_httpResponse.status == 200) // OK
		{
			m_bFlagResponseReady = 1;
			m_bError = 0;
			m_strError = "";

			m_strStatus = "SUCCESS: \nAudio received.";
			ofLogNotice("ofxElevenLabs") << m_strStatus;
		}
		else // Error
		{
			m_bFlagResponseReady = 0;
			m_bError = 1;

			m_strError = "ERROR: Response not ready. \n";
			m_strError += "API returned status code " + ofToString(m_httpResponse.status) + ".\n";

			if (m_httpResponse.status == 404) m_strError += "404 (Not Found)";
			else if (m_httpResponse.status == 500) m_strError += "500 (Internal Server Error)";
			else if (m_httpResponse.status == 302) m_strError += "302 (Found/Redirect)";
			else if (m_httpResponse.status == 401) m_strError += "401 (Check Authentication Credentials)";

			try
			{
				string r = response.error;
				ofLogNotice("ofxElevenLabs") << r;
			}
			catch (nlohmann::detail::parse_error& e)
			{
				ofLogError("ofxElevenLabs") << "JSON parsing error: " << e.what() << " at position " << e.byte;
			}

			ofLogNotice("ofxElevenLabs") << m_strError;
			//ofLogNotice("ofxElevenLabs")<<(__FUNCTION__) << "apiKey: " << apiKey;
		}
	}

private:
	void saveAudioFile(const string& filename) {
		ofLogNotice("ofxElevenLabs") << "saveAudioFile: " << filename;

		// Save the response body to a local file
		ofBufferToFile(filename, m_httpResponse.data);
	}

	void doPlayAudio() {
		ofLogNotice("ofxElevenLabs") << "doPlayAudio";

		// Load the response body into the audio player and play it
		//TODO: must fix get ofBuffer to avoid save and load from file
		//soundPlayer.load(m_httpResponse.data);
		soundPlayer.load(m_filename);
		soundPlayer.play();
	}

public:
	void doReplayAudio() {
		ofLogNotice("ofxElevenLabs") << "doReplayAudio";

		soundPlayer.play();
	}

	//--

public:
	// For the official ElevenLabs servers.
#ifdef USE_TTF_ELEVEN_LABS
	ofHttpResponse sendRequestPostElevenLabs(const std::string& text)
	{
		ofLogNotice("ofxElevenLabs") << "sendRequestPostElevenLabs: " << text;

		ofHttpRequest request;

		ofJson voice_settings;
		voice_settings["stability"] = stability.get();
		voice_settings["similarity_boost"] = similarity_boost.get();

		request.method = ofHttpRequest::Method::POST;
		request.url = "https://api.elevenlabs.io/v1/text-to-speech/" + voiceId.get();
		request.timeoutSeconds = 30;
		request.headers["accept"] = "audio/mpeg";
		request.headers["Content-Type"] = "application/json";
		request.headers["xi-api-key"] = apiKey;

		ofJson j;
		j["text"] = text;
		j["model_id"] = strModelId;
		j["voice_settings"] = voice_settings;

		bool bMustFix_utf8 = 0;

		//TODO:
		//avoid some exceptions
		try {
			ofLogNotice("ofxElevenLabs") << j.dump(4);
		}
		catch (const std::exception& e) {
			ofLogError("ofxElevenLabs") << "Exception caught: " << e.what();
			m_bError = 1;

			bMustFix_utf8 = 1;

			ofLogError("ofxElevenLabs") << "Probably an UTF 8 error!";
			ofLogError("ofxElevenLabs") << "Request is ignored to aovid exceptions!";
			
			//TODO: workaround
			// return empty and skip that send request!
			ofURLFileLoader loader;
			ofHttpRequest request;
			return loader.handleRequest(request);
		}

		//TODO:
		// #include <utf8proc.h>
#if(0)//from ChatGpt
		if (bMustFix_utf8) {
			try {
				std::string jsonStr = j.dump();

				// Replace invalid UTF-8 characters with a replacement character
				utf8proc_ssize_t size = jsonStr.size();
				std::vector<char> buffer(size + 1);
				utf8proc_uint8_t* input = reinterpret_cast<utf8proc_uint8_t*>(jsonStr.data());
				utf8proc_ssize_t result = utf8proc_replace_invalid(input, size, reinterpret_cast<utf8proc_uint8_t*>(buffer.data()));

				if (result >= 0) {
					buffer[result] = '\0';
					jsonStr = buffer.data();
				}
				else {
					ofLogError("ofApp") << "Error replacing invalid UTF-8 characters: " << result;
				}

				// use j and jsonStr as needed
		}
			catch (const std::exception& e) {
				ofLogError("ofApp") << "Error dumping JSON data: " << e.what();
			}
	}
#endif

		try {
			request.body = j.dump();
		}
		catch (const std::exception& e) {
			ofLogError("ofxElevenLabs") << "Exception caught: " << e.what();
			m_bError = 1;
		}

		ofURLFileLoader loader;
		return loader.handleRequest(request);
}
#endif

	//TODO:
	// For the custom servers.
#ifdef USE_TTF_CUSTOM_SERVER
	// To send an HTTP request to the specified URL.
	// Custom endpoint. Requires a custom server.
	ofHttpResponse sendRequestPostCustomServer(const std::string& url, const std::string& body) {

		ofHttpRequest request;
		request.url = url;
		request.method = ofHttpRequest::Method::POST;
		request.headers["Content-Type"] = "application/json";
		request.body = body;
		request.timeoutSeconds = 30;

		ofURLFileLoader loader;

		return loader.handleRequest(request);
	}
#endif

	//--

	// Testing Helpers

public:
	void doSendTestingRandom() {
		ofLogNotice("ofxElevenLabs") << "doSendTestingRandom";

		doSendTestingSentece((int)ofRandom(10));
	}

	void doSendTestingSentece(int i) {
		ofLogNotice("ofxElevenLabs") << "doSendTestingSentece: " << i;

		switch (i)
		{
			// English
		case 1: doSend("Hello, world!"); break;
		case 2: doSend("Are you ready for the apocalypse?"); break;
		case 3: doSend("I can't take out of... my mind..."); break;
		case 4: doSend("\"I want a bit of peace! and air!\" he said, slowly."); break;
		case 5: doSend("What if I... go sleep a bit? \nI am really... tired - to still - working."); break;

			// Spanish
		case 6: doSend("Hola, mundo!"); break;
		case 7: doSend("Estas preparado para el ...apocalipsis?"); break;
		case 8: doSend("No me lo puedo sacar de la... cabeza..."); break;
		case 9: doSend("\"Quiero un poco de paz! Y de aire!\" dijo lentamente."); break;
		case 0: doSend("Y si... me voy a dormir un rato? \nEstoy muy... cansado - para seguir - trabajando."); break;
		}
	}

	void keyPressed(int key) {
		ofLogNotice("ofxElevenLabs") << "keyPressed: " << char(key);
		
		if (key == ' ') {
			doSendTestingRandom();
			return;
		}

		switch (key)
		{
			// English
		case '1': doSendTestingSentece(1); break;
		case '2': doSendTestingSentece(2); break;
		case '3': doSendTestingSentece(3); break;
		case '4': doSendTestingSentece(4); break;
		case '5': doSendTestingSentece(5); break;

			// Spanish
		case '6': doSendTestingSentece(6); break;
		case '7': doSendTestingSentece(7); break;
		case '8': doSendTestingSentece(8); break;
		case '9': doSendTestingSentece(9); break;
		case '0': doSendTestingSentece(0); break;
		}
	}

	void drawDebugHelp(bool bWithKeysInfo = 1)
	{
		string s = "";
		if (bWithKeysInfo)
		{
			s += "\n";
			s += "ofxElevenLabs\n\n";
			s += "PRESS KEYS\n";
			s += "for testing sentences.\n\n";
			s += "1-2-3-4-5    English\n";
			s += "6-7-8-9-0    Spanish\n";
			s += "Space        Random\n\n";
		}

		s += "Text:\n\n";
		s += m_strText + "\n\n";
		s += m_strTextHelpDebug;

		// Print on the screen
		static ofBitmapFont f;
		auto bb = f.getBoundingBox(s, 0, 0);

		// Top
		//ofDrawBitmapStringHighlight(s, ofGetWidth() / 2 - bb.getWidth() / 2, 30);

		// Bottom 
		ofDrawBitmapStringHighlight(s, MAX(0, ofGetWidth() / 2 - bb.getWidth() / 2), ofGetHeight() - bb.getHeight() - 30);
	}

public:
	void setVoice(int index) {
		ofLogNotice("ofxElevenLabs") << "setVoice: " << index;
		ofClamp(index, voiceIndex.getMin(), voiceIndex.getMax());
		voiceIndex = index;
	}

	void setPreviousVoice() {
		ofLogNotice("ofxElevenLabs") << "setPreviousVoice";
		int i = voiceIndex;
		if (i > voiceIndex.getMin()) i--;
		else i = voiceIndex.getMax();
		voiceIndex = i;
	}
	void setNextVoice() {
		ofLogNotice("ofxElevenLabs") << "setNextVoice";
		int i = voiceIndex;
		if (i < voiceIndex.getMax()) i++;
		else i = 0;
		voiceIndex = i;
	}
};

