
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <glib.h>
// no longer needed for headless demo
//#include <gtkmm.h>
#include <sstream>
#include <thread>
#include <sys/syscall.h>
#include <fstream>
#include <iosfwd>
#include <iostream>
#include <algorithm>
#include <stdexcept>
#include <random>
#include <chrono>



#include <string>  // for std::wstring, std::string
#include <locale>
#include <codecvt>


#include "zoom_sdk.h"
#include "auth_service_interface.h"
#include "meeting_service_interface.h"
#include "meeting_service_components/meeting_audio_interface.h"
#include "meeting_service_components/meeting_participants_ctrl_interface.h"
#include "meeting_service_components/meeting_video_interface.h"
#include "setting_service_interface.h"

//These are needed to readsettingsfromJSON named config.json
#include "json.hpp"
#include <curl/curl.h>

//used to accept prompts
#include "MeetingReminderEventListener.h"
//used to listen to callbacks from meeting related matters
#include "MeetingServiceEventListener.h"
//used to listen to callbacks from authentication related matters
#include "AuthServiceEventListener.h"




//used for event listener
#include "MeetingParticipantsCtrlEventListener.h"
#include "MeetingRecordingCtrlEventListener.h"




//references for SendVideoRawData
#include "ZoomSDKVideoSource.h"
#include <rawdata/zoom_rawdata_api.h>

//references for SendAudioRawData
#include "ZoomSDKVirtualAudioMicEvent.h"
#include <mutex>

//references for chatDemo
#include "MeetingChatEventListener.h"


USING_ZOOM_SDK_NAMESPACE

GMainLoop* loop;

//These are needed to readsettingsfromJSON named config.json
using Json = nlohmann::json;
std::string meeting_number, token, meeting_password, recording_token, remote_url, user_name, media_manifest;
int jwt_role = 0;
int media_index = -1;

//references for SendAudioRawData
std::string DEFAULT_AUDIO_SOURCE = "pcm1644m.wav";
int DEFAULT_AUDIO_SAMPLE_RATE = 44100;
int DEFAULT_AUDIO_CHANNELS = 1;

//references for SendVideoRawData
std::string DEFAULT_VIDEO_SOURCE = "Big_Buck_Bunny_720_10s_10MB.mp4";
int DEFAULT_VIDEO_WIDTH = WIDTH;
int DEFAULT_VIDEO_HEIGHT = HEIGHT;
int DEFAULT_VIDEO_FPS = 30;

//Services which are needed to initialize, authenticate and configure settings for the SDK
ZOOM_SDK_NAMESPACE::IAuthService* m_pAuthService;
ZOOM_SDK_NAMESPACE::IMeetingService* m_pMeetingService;
ZOOM_SDK_NAMESPACE::ISettingService* m_pSettingService;


IMeetingParticipantsController* m_pParticipantsController;


//references for GetAudioRawData
//ZoomSDKRawDataPipeDelegate* rawdatapipedelegate = new ZoomSDKRawDataPipeDelegate();
IZoomSDKAudioRawDataHelper* audioHelper;

//this is used to get a userID, there is no specific proper logic here. It just gets the first userID.
//userID is needed for video subscription.
unsigned int userID;



//this will fetch the JWT Token from a web service
bool useJWTTokenFromWebService = false;
//this will fetch the RecordingToken from a web service.
bool useRecordingTokenFromWebService = false;

//this will enable or disable logic to get raw video and raw audio
//do note that this will be overwritten by config.json

bool SendVideoRawData = false;
bool SendAudioRawData = false;
bool chatDemo = false;
bool exitOnMeetingEnd = true;


IUserInfo* getMyself() {
	m_pParticipantsController = m_pMeetingService->GetMeetingParticipantsController();
	IUserInfo* returnvalue = m_pParticipantsController->GetMySelfUser();
	//std::cout << "UserID is : " << returnvalue << std::endl;
	return returnvalue;
}




//check if you meet the requirements to send raw data
void CheckAndStartRawSending(bool isVideo, bool isAudio) {


	//SendVideoRawData
	if (isVideo) {

		ZoomSDKVideoSource* virtual_camera_video_source = new ZoomSDKVideoSource(DEFAULT_VIDEO_SOURCE, DEFAULT_VIDEO_WIDTH, DEFAULT_VIDEO_HEIGHT, DEFAULT_VIDEO_FPS);
		IZoomSDKVideoSourceHelper* p_videoSourceHelper = GetRawdataVideoSourceHelper();

		if (p_videoSourceHelper) {
			SDKError err = p_videoSourceHelper->setExternalVideoSource(virtual_camera_video_source);



			if (err != SDKERR_SUCCESS) {
				printf("attemptToStartRawVideoSending(): Failed to set external video source, error code: %d\n", err);
			}
			else {
				printf("attemptToStartRawVideoSending(): Success \n");
				IMeetingVideoController* meetingController = m_pMeetingService->GetMeetingVideoController();
				meetingController->UnmuteVideo();

			}
		}
		else {
			printf("attemptToStartRawVideoSending(): Failed to get video source helper\n");
		}
	}


	//SendAudioRawData
	if (isAudio) {
		ZoomSDKVirtualAudioMicEvent* audio_source = new ZoomSDKVirtualAudioMicEvent(DEFAULT_AUDIO_SOURCE, DEFAULT_AUDIO_SAMPLE_RATE, DEFAULT_AUDIO_CHANNELS);
		IZoomSDKAudioRawDataHelper* audioHelper = GetAudioRawdataHelper();
		if (audioHelper) {
			SDKError err = audioHelper->setExternalAudioSource(audio_source);
			if (err != SDKERR_SUCCESS) {
				printf("attemptToStartRawAudioSending(): Failed to set external audio source, error code: %d\n", err);
			}
			else {
				printf("attemptToStartRawAudioSending(): Success \n");
				IMeetingAudioController* meetingAudController = m_pMeetingService->GetMeetingAudioController();
				if (meetingAudController) {
					SDKError joinErr = meetingAudController->JoinVoip();
					printf("JoinVoip result: %d\n", joinErr);
					IUserInfo* myself = getMyself();
					if (myself) {
						SDKError unmuteErr = meetingAudController->UnMuteAudio(myself->GetUserID());
						printf("UnMuteAudio result: %d\n", unmuteErr);
					}
				}
			}
		}
		else {
			printf("attemptToStartRawAudioSending(): Failed to get audio raw data helper\n");
		}
	}


}


//callback when given host permission
void onIsHost() {
	printf("Is host now...\n");


}

//callback when given cohost permission
void onIsCoHost() {
	printf("Is co-host now...\n");


}

//callback when given recording permission
void onIsGivenRecordingPermission() {
	printf("Is given recording permissions now...\n");


}



void turnOnSendVideoAndAudio() {
	//testing WIP
	if (SendVideoRawData) {
		IMeetingVideoController* meetingVidController = m_pMeetingService->GetMeetingVideoController();
		meetingVidController->UnmuteVideo();
	}
	//testing WIP
	if (SendAudioRawData) {
		IMeetingAudioController* meetingAudController = m_pMeetingService->GetMeetingAudioController();
		meetingAudController->JoinVoip();
		printf("Is my audio muted: %d\n", getMyself()->IsAudioMuted());
		//meetingAudController->MuteAudio(getMyself()->GetUserID(),true);
		meetingAudController->UnMuteAudio(getMyself()->GetUserID());

		//m_pSettingService->GetAudioSettings()->GetMicList();
		//m_pSettingService->GetAudioSettings()->UseDefaultSystemMic();
	}
}
void turnOffSendVideoandAudio() {
	//testing WIP
	if (SendVideoRawData) {
		IMeetingVideoController* meetingVidController = m_pMeetingService->GetMeetingVideoController();
		meetingVidController->MuteVideo();
	}
	//testing WIP
	if (SendAudioRawData) {
		IMeetingAudioController* meetingAudController = m_pMeetingService->GetMeetingAudioController();
		meetingAudController->MuteAudio(getMyself()->GetUserID(), true);

	}
}

//callback when the SDK is inmeeting
void onInMeeting() {

	printf("onInMeeting Invoked\n");

	//double check if you are in a meeting
	if (m_pMeetingService->GetMeetingStatus() == ZOOM_SDK_NAMESPACE::MEETING_STATUS_INMEETING) {
		printf("In Meeting Now...\n");

		//print all list of participants
		IList<unsigned int>* participants = m_pMeetingService->GetMeetingParticipantsController()->GetParticipantsList();
		printf("Participants count: %d\n", participants->GetCount());


	}


	//chatDemo
	if (chatDemo) {
		IMeetingChatController* meetingchatcontroller= m_pMeetingService->GetMeetingChatController();
		meetingchatcontroller->SetEvent(new MeetingChatEventListener(&turnOnSendVideoAndAudio, &turnOffSendVideoandAudio));

		// Convert std::wstring to std::string (UTF-8)
		std::wstring wstr = L"Hello world!";
		std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
		std::string str = converter.to_bytes(wstr);

		// Ensure zchar_t is defined as char if not already defined
		typedef char zchar_t;

		char charArray[1024]; // Choose an appropriate size
		std::strncpy(charArray, str.c_str(), sizeof(charArray));

		// Ensure null-termination
		charArray[sizeof(charArray) / sizeof(charArray[0]) - 1] = '\0';

		const zchar_t* constCharArray = charArray;

		IChatMsgInfoBuilder* chatbuilder = meetingchatcontroller->GetChatMessageBuilder();
		chatbuilder->SetReceiver(0);
		chatbuilder->SetMessageType(SDKChatMessageType_To_All);
		chatbuilder->SetContent(constCharArray);
		chatbuilder->Build();

		meetingchatcontroller->SendChatMsgTo(chatbuilder->Build());



	}

	//first attempt to start raw recording  / sending, upon successfully joined and achieved "in-meeting" state.

	CheckAndStartRawSending(SendVideoRawData, SendAudioRawData);
}

//on meeting ended, typically by host, do something here. it is possible to reuse this SDK instance
void onMeetingEndsQuitApp() {
	if (exitOnMeetingEnd && loop) {
		g_main_loop_quit(loop);
	}
}

void onMeetingJoined() {

	printf("Joining Meeting...\n");
}

//get path, helper method used to read json config file
std::string getSelfDirPath()
{
	char dest[PATH_MAX];
	memset(dest, 0, sizeof(dest)); // readlink does not null terminate!
	if (readlink("/proc/self/exe", dest, PATH_MAX) == -1)
	{
	}
	char* tmp = strrchr(dest, '/');
	if (tmp)
		*tmp = 0;
	printf("getting project path\n");
	return std::string(dest);
}

// Function to parse and process JSON value as a string
bool processJsonValue(const Json& json, const std::string& key, std::string& value) {
	auto it = json.find(key);
	if (it != json.end() && !it->is_null()) {
		value = it->get<std::string>();
		std::string lower_key = key;
		std::transform(lower_key.begin(), lower_key.end(), lower_key.begin(), ::tolower);
		if (lower_key.find("token") != std::string::npos ||
			lower_key.find("password") != std::string::npos) {
			printf("config %s: %s\n", key.c_str(), value.empty() ? "[empty]" : "[set]");
		}
		else {
			printf("config %s: %s\n", key.c_str(), value.c_str());
		}
		return true;
	}
	return false;
}

// Function to parse and process JSON value as a boolean
bool processJsonBoolean(const Json& json, const std::string& key, bool& value) {
	auto it = json.find(key);
	if (it != json.end() && !it->is_null()) {
		std::string stringValue = it->get<std::string>();
		std::transform(stringValue.begin(), stringValue.end(), stringValue.begin(), ::tolower);
		value = (stringValue == "true");
		printf("%s value is %s\n", key.c_str(), (value ? "true" : "false"));
		return true;
	}
	return false;
}

bool processJsonInteger(const Json& json, const std::string& key, int& value) {
	auto it = json.find(key);
	if (it != json.end() && !it->is_null()) {
		if (it->is_number_integer()) {
			value = it->get<int>();
		}
		else {
			std::string stringValue = it->get<std::string>();
			value = std::stoi(stringValue);
		}
		printf("%s value is %d\n", key.c_str(), value);
		return true;
	}
	return false;
}

bool jsonValueToInteger(const Json& json, const std::string& key, int& value) {
	auto it = json.find(key);
	if (it == json.end() || it->is_null()) {
		return false;
	}
	if (it->is_number_integer()) {
		value = it->get<int>();
		return true;
	}
	if (it->is_string()) {
		value = std::stoi(it->get<std::string>());
		return true;
	}
	return false;
}

std::string getDirName(const std::string& path) {
	size_t slash = path.find_last_of('/');
	if (slash == std::string::npos) {
		return ".";
	}
	if (slash == 0) {
		return "/";
	}
	return path.substr(0, slash);
}

bool isAbsolutePath(const std::string& path) {
	return !path.empty() && path[0] == '/';
}

std::string joinPath(const std::string& base, const std::string& child) {
	if (child.empty() || isAbsolutePath(child)) {
		return child;
	}
	if (base.empty() || base == ".") {
		return child;
	}
	if (base[base.size() - 1] == '/') {
		return base + child;
	}
	return base + "/" + child;
}

bool fileIsReadable(const std::string& path) {
	std::ifstream file(path.c_str(), std::ios::binary);
	return static_cast<bool>(file);
}

bool selectMediaFromManifest(const std::string& bin_dir) {
	if (media_manifest.empty()) {
		if (SendVideoRawData || SendAudioRawData) {
			std::cerr << "media_manifest is required when raw video/audio sending is enabled." << std::endl;
			return false;
		}
		return true;
	}

	std::string manifest_path = joinPath(bin_dir, media_manifest);
	std::ifstream manifest_file(manifest_path.c_str());
	if (!manifest_file) {
		std::cerr << "Unable to read media manifest: " << manifest_path << std::endl;
		return false;
	}

	Json manifest_json;
	try {
		manifest_file >> manifest_json;
	}
	catch (Json::parse_error& ex) {
		std::cerr << "Unable to parse media manifest: " << manifest_path << std::endl;
		return false;
	}

	auto items = manifest_json.find("items");
	if (items == manifest_json.end() || !items->is_array() || items->empty()) {
		std::cerr << "Media manifest has no items: " << manifest_path << std::endl;
		return false;
	}

	int chosen_index = media_index;
	if (chosen_index < 0 || chosen_index >= static_cast<int>(items->size())) {
		unsigned seed = static_cast<unsigned>(
			std::chrono::high_resolution_clock::now().time_since_epoch().count() ^
			static_cast<unsigned long long>(getpid())
		);
		std::mt19937 rng(seed);
		std::uniform_int_distribution<int> distribution(0, static_cast<int>(items->size()) - 1);
		chosen_index = distribution(rng);
	}

	Json item = (*items)[chosen_index];
	auto video = item.find("video");
	auto audio = item.find("audio");
	if (video == item.end() || audio == item.end() || !video->is_string() || !audio->is_string()) {
		std::cerr << "Media manifest item is missing video/audio file names at index " << chosen_index << std::endl;
		return false;
	}

	std::string manifest_dir = getDirName(manifest_path);
	DEFAULT_VIDEO_SOURCE = joinPath(manifest_dir, video->get<std::string>());
	DEFAULT_AUDIO_SOURCE = joinPath(manifest_dir, audio->get<std::string>());
	jsonValueToInteger(item, "video_width", DEFAULT_VIDEO_WIDTH);
	jsonValueToInteger(item, "video_height", DEFAULT_VIDEO_HEIGHT);
	jsonValueToInteger(item, "video_fps", DEFAULT_VIDEO_FPS);
	jsonValueToInteger(item, "audio_sample_rate", DEFAULT_AUDIO_SAMPLE_RATE);
	jsonValueToInteger(item, "audio_channels", DEFAULT_AUDIO_CHANNELS);

	std::string name = item.find("name") != item.end() && item["name"].is_string()
		? item["name"].get<std::string>()
		: std::to_string(chosen_index);
	std::cout << "Selected media item: " << name << std::endl;
	std::cout << "Video source: " << DEFAULT_VIDEO_SOURCE << " (" << DEFAULT_VIDEO_WIDTH << "x" << DEFAULT_VIDEO_HEIGHT << "@" << DEFAULT_VIDEO_FPS << "fps)" << std::endl;
	std::cout << "Audio source: " << DEFAULT_AUDIO_SOURCE << " (" << DEFAULT_AUDIO_SAMPLE_RATE << " Hz, channels " << DEFAULT_AUDIO_CHANNELS << ")" << std::endl;

	if (SendVideoRawData && !fileIsReadable(DEFAULT_VIDEO_SOURCE)) {
		std::cerr << "Selected video file is not readable: " << DEFAULT_VIDEO_SOURCE << std::endl;
		return false;
	}
	if (SendAudioRawData && !fileIsReadable(DEFAULT_AUDIO_SOURCE)) {
		std::cerr << "Selected audio file is not readable: " << DEFAULT_AUDIO_SOURCE << std::endl;
		return false;
	}
	return true;
}


void ReadJsonSettings()
{


	std::string self_dir = getSelfDirPath();
	printf("self path: %s\n", self_dir.c_str());
	self_dir.append("/config.json");

	std::ifstream t(self_dir.c_str());
	if (!t) {
		std::cerr << "Unable to read config file: " << self_dir << std::endl;
		return;
	}
	t.seekg(0, std::ios::end);
	size_t size = t.tellg();
	std::string buffer(size, ' ');
	t.seekg(0);
	t.read(&buffer[0], size);

	Json config_json;
	try {
		config_json = Json::parse(buffer);
		std::cout << "configuration loaded from local file (" << buffer.size() << " bytes)" << std::endl;
	}
	catch (Json::parse_error& ex) {
		// Handle JSON parse error
	}

	if (!config_json.is_null()) {

		processJsonValue(config_json, "meeting_number", meeting_number);
		processJsonValue(config_json, "token", token);
		processJsonValue(config_json, "meeting_password", meeting_password);
		processJsonValue(config_json, "recording_token", recording_token);
		processJsonValue(config_json, "remote_url", remote_url);
		processJsonValue(config_json, "user_name", user_name);
		processJsonValue(config_json, "media_manifest", media_manifest);
		processJsonInteger(config_json, "jwt_role", jwt_role);
		processJsonInteger(config_json, "media_index", media_index);
		processJsonInteger(config_json, "video_width", DEFAULT_VIDEO_WIDTH);
		processJsonInteger(config_json, "video_height", DEFAULT_VIDEO_HEIGHT);
		processJsonInteger(config_json, "video_fps", DEFAULT_VIDEO_FPS);
		processJsonInteger(config_json, "audio_sample_rate", DEFAULT_AUDIO_SAMPLE_RATE);
		processJsonInteger(config_json, "audio_channels", DEFAULT_AUDIO_CHANNELS);


		processJsonBoolean(config_json, "useJWTTokenFromWebService", useJWTTokenFromWebService);
		processJsonBoolean(config_json, "useRecordingTokenFromWebService", useRecordingTokenFromWebService);
		processJsonBoolean(config_json, "SendVideoRawData", SendVideoRawData);
		processJsonBoolean(config_json, "SendAudioRawData", SendAudioRawData);
		processJsonBoolean(config_json, "chatDemo", chatDemo);
		processJsonBoolean(config_json, "exitOnMeetingEnd", exitOnMeetingEnd);
		// Additional processing or handling of parsed values can be done here
	}

	std::string bin_dir = getSelfDirPath();
	if (!selectMediaFromManifest(bin_dir)) {
		std::cerr << "Unable to select load-test media. Exiting before joining meeting." << std::endl;
		exit(78);
	}

	std::string bin_dir1 = bin_dir;
	if (SendAudioRawData) {
		DEFAULT_AUDIO_SOURCE = joinPath(bin_dir1, DEFAULT_AUDIO_SOURCE);
	}
	std::string bin_dir2 = bin_dir;
	if (SendVideoRawData) {
		DEFAULT_VIDEO_SOURCE = joinPath(bin_dir2, DEFAULT_VIDEO_SOURCE);
	}

	printf("directory of config file: %s\n", self_dir.c_str());
}

void InitMeetingSDK()
{
	ZOOM_SDK_NAMESPACE::SDKError err(ZOOM_SDK_NAMESPACE::SDKERR_SUCCESS);
	ZOOM_SDK_NAMESPACE::InitParam initParam;

	// set domain
	initParam.strWebDomain = "https://zoom.us";
	initParam.strSupportUrl = "https://zoom.us";

	// set language id
	initParam.emLanguageID = ZOOM_SDK_NAMESPACE::LANGUAGE_English;

	//set logging perferences
	initParam.enableLogByDefault = true;
	initParam.enableGenerateDump = true;

	// attempt to initialize
	err = ZOOM_SDK_NAMESPACE::InitSDK(initParam);
	if (err != ZOOM_SDK_NAMESPACE::SDKERR_SUCCESS)
	{
		std::cerr << "Init meetingSdk:error " << std::endl;
	}
	else
	{
		std::cerr << "Init meetingSdk:success" << std::endl;
	}
}

void CleanSDK()
{
	ZOOM_SDK_NAMESPACE::SDKError err(ZOOM_SDK_NAMESPACE::SDKERR_SUCCESS);

	if (m_pAuthService)
	{
		ZOOM_SDK_NAMESPACE::DestroyAuthService(m_pAuthService);
		m_pAuthService = NULL;
	}
	if (m_pSettingService)
	{
		ZOOM_SDK_NAMESPACE::DestroySettingService(m_pSettingService);
		m_pSettingService = NULL;
	}
	if (m_pMeetingService)
	{
		ZOOM_SDK_NAMESPACE::DestroyMeetingService(m_pMeetingService);
		m_pMeetingService = NULL;
	}

	//if (_network_connection_helper)
	//{
	//	ZOOM_SDK_NAMESPACE::DestroyNetworkConnectionHelper(_network_connection_helper);
	//	_network_connection_helper = NULL;
	//}
	//attempt to clean up SDK
	err = ZOOM_SDK_NAMESPACE::CleanUPSDK();
	if (err != ZOOM_SDK_NAMESPACE::SDKERR_SUCCESS)
	{
		std::cerr << "CleanSDK meetingSdk:error " << std::endl;
	}
	else
	{
		std::cerr << "CleanSDK meetingSdk:success" << std::endl;
	}
}

void changeMicrophoneAndSpeaker() {
	ZOOM_SDK_NAMESPACE::IAudioSettingContext* pAudioContext = m_pSettingService->GetAudioSettings();
	if (pAudioContext)
	{
		//setting speaker
		//if there are speakers detected
		if (pAudioContext->GetSpeakerList()->GetCount() >= 1) {
			std::cout << "Number of speaker(s) : " << pAudioContext->GetSpeakerList()->GetCount() << std::endl;
			ISpeakerInfo* sInfo = pAudioContext->GetSpeakerList()->GetItem(0);
			const zchar_t* deviceName = sInfo->GetDeviceName();

			//set speaker
			if (deviceName != nullptr && deviceName[0] != '\0') {
				std::cout << "Speaker(0) name : " << sInfo->GetDeviceName() << std::endl;
				std::cout << "Speaker(0) id : " << sInfo->GetDeviceId() << std::endl;
				pAudioContext->SelectSpeaker(sInfo->GetDeviceId(), sInfo->GetDeviceName());
				std::cout << "Is selected speaker? : " << pAudioContext->GetSpeakerList()->GetItem(0)->IsSelectedDevice() << std::endl;
			}
			else {
				std::cout << "Speaker(0) name is empty or null." << std::endl;
				std::cout << "Speaker(0) id is empty or null." << std::endl;
			}
		}

		//setting microphone
		//if there are microphone detected
		if (pAudioContext->GetMicList()->GetCount() >= 1) {
			IMicInfo* mInfo = pAudioContext->GetMicList()->GetItem(0);
			std::cout << "Number of mic(s) : " << pAudioContext->GetMicList()->GetCount() << std::endl;
			const zchar_t* deviceName = mInfo->GetDeviceName();

			//set microphone
			if (deviceName != nullptr && deviceName[0] != '\0') {
				std::cout << "Mic(0) name : " << mInfo->GetDeviceName() << std::endl;
				std::cout << "Mic(0) id : " << mInfo->GetDeviceId() << std::endl;
				pAudioContext->SelectMic(mInfo->GetDeviceId(), mInfo->GetDeviceName());
				std::cout << "Is selected Mic? : " << pAudioContext->GetMicList()->GetItem(0)->IsSelectedDevice() << std::endl;
			}
			else {
				std::cout << "Mic(0) name is empty or null." << std::endl;
				std::cout << "Mic(0) id is empty or null." << std::endl;
			}
		}
	}
}

void JoinMeeting()
{
	std::cerr << "Joining Meeting" << std::endl;
	SDKError err2(SDKError::SDKERR_SUCCESS);

	//try to create the meetingservice object,
	//this object will be used to join the meeting
	if ((err2 = CreateMeetingService(&m_pMeetingService)) != SDKError::SDKERR_SUCCESS) {};
	std::cerr << "MeetingService created." << std::endl;

	//before joining a meeting, create the setting service
	//this object is used to for settings
	CreateSettingService(&m_pSettingService);
	std::cerr << "Settingservice created." << std::endl;

	// Set the event listener for meeting status
	m_pMeetingService->SetEvent(new MeetingServiceEventListener(&onMeetingJoined, &onMeetingEndsQuitApp, &onInMeeting));


	// set event listnener for prompt handler
	IMeetingReminderController* meetingremindercontroller = m_pMeetingService->GetMeetingReminderController();
	MeetingReminderEventListener* meetingremindereventlistener = new MeetingReminderEventListener();
	meetingremindercontroller->SetEvent(meetingremindereventlistener);



	//prepare params used for joining meeting
	ZOOM_SDK_NAMESPACE::JoinParam joinParam;
	ZOOM_SDK_NAMESPACE::SDKError err(ZOOM_SDK_NAMESPACE::SDKERR_SERVICE_FAILED);
	joinParam.userType = ZOOM_SDK_NAMESPACE::SDK_UT_WITHOUT_LOGIN;
	ZOOM_SDK_NAMESPACE::JoinParam4WithoutLogin& withoutloginParam = joinParam.param.withoutloginuserJoin;
	// withoutloginParam.meetingNumber = 1231231234;
	withoutloginParam.meetingNumber = std::stoull(meeting_number);
	withoutloginParam.vanityID = NULL;
	withoutloginParam.userName = user_name.empty() ? "ZoomLoadBot" : user_name.c_str();
	// withoutloginParam.psw = "1";
	withoutloginParam.psw = meeting_password.c_str();
	withoutloginParam.customer_key = NULL;
	withoutloginParam.webinarToken = NULL;
	withoutloginParam.isVideoOff = !SendVideoRawData;
	withoutloginParam.isAudioOff = !SendAudioRawData;

	std::cerr << "JWT token length is " << token.size() << std::endl;
	std::cerr << "Recording token length is " << recording_token.size() << std::endl;

	//automatically set app_privilege token if it is present in config.json, or retrieved from web service
	withoutloginParam.app_privilege_token = NULL;
	if (!recording_token.size() == 0 && useRecordingTokenFromWebService==true)
	{
		withoutloginParam.app_privilege_token = recording_token.c_str();
		std::cerr << "Setting recording token" << std::endl;
	}
	else
	{
		std::cerr << "Leaving recording token as NULL" << std::endl;
	}

	if (SendVideoRawData) {

		//ensure video is turned on
		withoutloginParam.isVideoOff = false;
		//set join video to true
			ZOOM_SDK_NAMESPACE::IVideoSettingContext* pVideoContext = m_pSettingService->GetVideoSettings();
			if (pVideoContext)
			{
				SDKError autoOffErr = pVideoContext->EnableAutoTurnOffVideoWhenJoinMeeting(false);
				SDKError hdErr = pVideoContext->EnableHDVideo(true);
				SDKError originalSizeErr = pVideoContext->EnableAlwaysUseOriginalSizeVideo(true);
				SDKError autoFramingErr = pVideoContext->DisableVideoAutoFraming();
				std::cout << "EnableAutoTurnOffVideoWhenJoinMeeting(false) result: " << autoOffErr << std::endl;
				std::cout << "EnableHDVideo(true) result: " << hdErr << std::endl;
				std::cout << "EnableAlwaysUseOriginalSizeVideo(true) result: " << originalSizeErr << std::endl;
				std::cout << "DisableVideoAutoFraming() result: " << autoFramingErr << std::endl;
			}
		}
	if (SendAudioRawData) {

		ZOOM_SDK_NAMESPACE::IAudioSettingContext* pAudioContext = m_pSettingService->GetAudioSettings();
		if (pAudioContext)
		{
			//ensure auto join audio
			pAudioContext->EnableAutoJoinAudio(true);
			pAudioContext->EnableAlwaysMuteMicWhenJoinVoip(false);
			pAudioContext->SetSuppressBackgroundNoiseLevel(Suppress_BGNoise_Level_None);

		}
	}

	//attempt to join meeting
	if (m_pMeetingService)
	{
		err = m_pMeetingService->Join(joinParam);
	}
	else
	{
		std::cout << "join_meeting m_pMeetingService:Null" << std::endl;
	}

	if (ZOOM_SDK_NAMESPACE::SDKERR_SUCCESS == err)
	{
		std::cout << "join_meeting:success" << std::endl;
	}
	else
	{
		std::cout << "join_meeting:error" << std::endl;
	}

}

void LeaveMeeting()
{
	ZOOM_SDK_NAMESPACE::MeetingStatus status = ZOOM_SDK_NAMESPACE::MEETING_STATUS_FAILED;


	if (NULL == m_pMeetingService)
	{

		std::cout << "leave_meeting m_pMeetingService:Null" << std::endl;

	}
	else
	{
		status = m_pMeetingService->GetMeetingStatus();
	}

	if (status == ZOOM_SDK_NAMESPACE::MEETING_STATUS_IDLE ||
		status == ZOOM_SDK_NAMESPACE::MEETING_STATUS_ENDED ||
		status == ZOOM_SDK_NAMESPACE::MEETING_STATUS_FAILED)
	{

		std::cout << "LeaveMeeting() not in meeting " << std::endl;

	}

	if (SDKError::SDKERR_SUCCESS == m_pMeetingService->Leave(ZOOM_SDK_NAMESPACE::LEAVE_MEETING))
	{
		std::cout << "LeaveMeeting() success " << std::endl;

	}
	else
	{
		std::cout << "LeaveMeeting() error" << std::endl;

	}

}

//callback when authentication is compeleted
void OnAuthenticationComplete()
{
	JoinMeeting();
}

void AuthMeetingSDK()
{
	SDKError err(SDKError::SDKERR_SUCCESS);

	//create auth service
	if ((err = CreateAuthService(&m_pAuthService)) != SDKError::SDKERR_SUCCESS) {};
	std::cerr << "AuthService created." << std::endl;

	//Create a param to insert jwt token
	ZOOM_SDK_NAMESPACE::AuthContext param;

	//set the event listener for onauthenticationcompleted
	if ((err = m_pAuthService->SetEvent(new AuthServiceEventListener(&OnAuthenticationComplete))) != SDKError::SDKERR_SUCCESS) {};
	std::cout << "AuthServiceEventListener added." << std::endl;


	if (!token.size() == 0)
	{
		param.jwt_token = token.c_str();
		std::cerr << "AuthSDK:using token from webservice " << std::endl;
	}

	//attempt to authenticate
	ZOOM_SDK_NAMESPACE::SDKError sdkErrorResult = m_pAuthService->SDKAuth(param);

	if (ZOOM_SDK_NAMESPACE::SDKERR_SUCCESS != sdkErrorResult)
	{
		std::cerr << "AuthSDK:error " << std::endl;
	}
	else
	{
		std::cerr << "AuthSDK:send success, waiting callback " << std::endl;
	}
}



//used for non headless app

void StartMeeting()
{

	ZOOM_SDK_NAMESPACE::StartParam startParam;
	startParam.userType = ZOOM_SDK_NAMESPACE::SDK_UT_NORMALUSER;
	startParam.param.normaluserStart.vanityID = NULL;
	startParam.param.normaluserStart.customer_key = NULL;
	startParam.param.normaluserStart.isVideoOff = false;
	startParam.param.normaluserStart.isAudioOff = false;


	ZOOM_SDK_NAMESPACE::SDKError err = m_pMeetingService->Start(startParam);
	if (SDKError::SDKERR_SUCCESS == err)
	{
		std::cerr << "StartMeeting:success " << std::endl;
	}
	else
	{
		std::cerr << "StartMeeting:error " << std::endl;
	}
}


// Define a struct to hold the response data
struct ResponseData {
	std::ostringstream stream;
};

// Callback function to write response data into the stringstream
static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
	size_t totalSize = size * nmemb;
	ResponseData* response = static_cast<ResponseData*>(userp);
	response->stream.write(static_cast<const char*>(contents), totalSize);
	return totalSize;
}


std::string getJWTToken(const std::string& remote_url) {
	// Initialize libcurl
	CURL* curl = curl_easy_init();
	if (!curl) {
		throw std::runtime_error("Failed to initialize libcurl");
	}

	// Create a ResponseData object to collect the response
	ResponseData responseData;
	struct curl_slist* headers = nullptr;
	std::string responsestr;
	try {
		// Set libcurl options
		curl_easy_setopt(curl, CURLOPT_URL, remote_url.c_str());
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
		curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
		curl_easy_setopt(curl, CURLOPT_TIMEOUT, 20L);


		headers = curl_slist_append(headers, "Expect:");
		headers = curl_slist_append(headers, "Content-Type: application/json");
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

		std::string json = "{\"meetingNumber\":\"" + meeting_number + "\",\"role\":" + std::to_string(jwt_role) + "}";
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json.c_str());

		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseData);

		// Perform the HTTP request
		CURLcode res = curl_easy_perform(curl);

		if (res != CURLE_OK) {
			throw std::runtime_error(std::string("curl_easy_perform() failed: ") + curl_easy_strerror(res));
		}


		responsestr = responseData.stream.str();

		// Process the response using your existing code
		Json responses_json;
			try
			{
				responses_json = Json::parse(responsestr);
				std::cout << "config from web service received (" << responsestr.size() << " bytes)" << std::endl;
			}
		catch (Json::parse_error& ex)
		{
		}

		auto json_signature = responses_json.find("signature");
		auto json_token = responses_json.find("token");
		auto json_jwt_token = responses_json.find("jwtToken");
		auto json_jwt_token_snake = responses_json.find("jwt_token");
		auto json_recordingtoken = responses_json.find("recordingtoken");

		if (json_signature != responses_json.end() && !json_signature->is_null())
		{
			token = json_signature->get<std::string>();
		}
		else if (json_token != responses_json.end() && !json_token->is_null())
		{
			token = json_token->get<std::string>();
		}
		else if (json_jwt_token != responses_json.end() && !json_jwt_token->is_null())
		{
			token = json_jwt_token->get<std::string>();
		}
		else if (json_jwt_token_snake != responses_json.end() && !json_jwt_token_snake->is_null())
		{
			token = json_jwt_token_snake->get<std::string>();
		}

		if (useRecordingTokenFromWebService) {
			if (json_recordingtoken != responses_json.end() && !json_recordingtoken->is_null())
			{
				recording_token = json_recordingtoken->get<std::string>();
			}
		}
	}
	catch (const std::exception& e) {
		// Handle exceptions
		std::cerr << "Error: " << e.what() << std::endl;
	}

	// Cleanup libcurl
	if (headers) curl_slist_free_all(headers);
	curl_easy_cleanup(curl);
	return responsestr;
}

//std::string getJWTToken(const std::string& remote_url) {
//	httplib::Client cli(remote_url);
//
//	httplib::Params params;
//	params.emplace("meetingNumber", meeting_number);
//	params.emplace("role", 1);
//
//	auto res = cli.Post("/",params);
//	res->status;
//	res->body;
//	std::cerr << "Response: " << res->body << std::endl;
//
//	return res->body;
//}

gboolean timeout_callback(gpointer data)
{
	return TRUE;
}

//this catches a break signal, such as Ctrl + C
void my_handler(int s)
{
	printf("\nCaught signal %d\n", s);
	LeaveMeeting();
	printf("Leaving session.\n");
	CleanSDK();
	std::exit(0);
}

void initAppSettings()
{
	struct sigaction sigIntHandler;
	sigIntHandler.sa_handler = my_handler;
	sigemptyset(&sigIntHandler.sa_mask);
	sigIntHandler.sa_flags = 0;
	sigaction(SIGINT, &sigIntHandler, NULL);
	sigaction(SIGTERM, &sigIntHandler, NULL);
}

int main(int argc, char* argv[])
{

	ReadJsonSettings();

	if (useJWTTokenFromWebService) {
		std::string response = getJWTToken(remote_url);
	}
	initAppSettings();

	InitMeetingSDK();
	AuthMeetingSDK();


	loop = g_main_loop_new(NULL, FALSE);
	// add source to default context
	g_timeout_add(100, timeout_callback, loop);
	g_main_loop_run(loop);
	CleanSDK();
	return 0;
}
