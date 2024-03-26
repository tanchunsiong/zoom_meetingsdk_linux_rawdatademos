
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

//used for share event listener
#include "meeting_service_components/meeting_sharing_interface.h"
#include "MeetingShareCtrlEventListener.h"


//used for event listener
#include "MeetingParticipantsCtrlEventListener.h"
#include "MeetingRecordingCtrlEventListener.h"

//references for GetVideoRawData
#include "rawdata/rawdata_renderer_interface.h"
#include "rawdata/zoom_rawdata_api.h"

//references for GetAudioRawData
#include "meeting_service_components/meeting_recording_interface.h"

//references for GetAudioRawData && GetVideoRawData
#include "ZoomSDKRawDataPipeDelegate.h"

//references for SendVideoRawData
#include "ZoomSDKVideoSource.h"

//references for SendAudioRawData
#include "ZoomSDKVirtualAudioMicEvent.h"
#include <mutex>

//references for chatDemo
#include "MeetingChatEventListener.h"

USING_ZOOM_SDK_NAMESPACE

GMainLoop* loop;

//These are needed to readsettingsfromJSON named config.json
using Json = nlohmann::json;
std::string meeting_number, token, meeting_password, recording_token, remote_url;

//references for SendAudioRawData
std::string DEFAULT_AUDIO_SOURCE = "Big_Buck_Bunny.wav";

//references for SendVideoRawData
std::string DEFAULT_VIDEO_SOURCE = "Big_Buck_Bunny_720_10s_10MB.mp4";

//Services which are needed to initialize, authenticate and configure settings for the SDK
ZOOM_SDK_NAMESPACE::IAuthService* m_pAuthService;
ZOOM_SDK_NAMESPACE::IMeetingService* m_pMeetingService;
ZOOM_SDK_NAMESPACE::ISettingService* m_pSettingService;


//references for GetVideoRawData

IZoomSDKRenderer* videoHelper;
IMeetingRecordingController* m_pRecordController;
IMeetingParticipantsController* m_pParticipantsController;




//new renderer
ZoomSDKRawDataPipeDelegate* rawdatapipedelegate = new ZoomSDKRawDataPipeDelegate();

//references for GetAudioRawData
//ZoomSDKRawDataPipeDelegate* rawdatapipedelegate = new ZoomSDKRawDataPipeDelegate();
IZoomSDKAudioRawDataHelper* audioHelper;

//this is used to get a userID, there is no specific proper logic here. It just gets the first userID.
//userID is needed for video subscription.
unsigned int userID;



//this will fetch the JWT Token from a web service
bool useJWTTokenFromWebService = true;
//this will fetch the RecordingToken from a web service. 
bool useRecordingTokenFromWebService = true;

//this will enable or disable logic to get raw video and raw audio
//do note that this will be overwritten by config.json
bool GetVideoRawData = false;
bool GetAudioRawData = false;
bool SendVideoRawData = false;
bool SendAudioRawData = false;
bool chatDemo = false;





//this is a helper method to get the first User ID, it is just an arbitary UserID
uint32_t getUserID() {
	m_pParticipantsController = m_pMeetingService->GetMeetingParticipantsController();
	int returnvalue = m_pParticipantsController->GetParticipantsList()->GetItem(0);
	std::cout << "UserID is : " << returnvalue << std::endl;
	return returnvalue;
}

IUserInfo* getMyself() {
	m_pParticipantsController = m_pMeetingService->GetMeetingParticipantsController();
	IUserInfo* returnvalue = m_pParticipantsController->GetMySelfUser();
	//std::cout << "UserID is : " << returnvalue << std::endl;
	return returnvalue;
}

//this is a helper method to get the first User Object, it is just an arbitary User Object
IUserInfo* getFirstUserObj() {
	m_pParticipantsController = m_pMeetingService->GetMeetingParticipantsController();
	int userID = m_pParticipantsController->GetParticipantsList()->GetItem(0);
	IUserInfo* returnvalue = m_pParticipantsController->GetUserByUserID(userID);
	std::cout << "UserID is : " << returnvalue << std::endl;
	return returnvalue;
}

//this is a helper method to get the first User Object, it is just an arbitary User Object
IList<unsigned int >* getAllUserObj() {
	m_pParticipantsController = m_pMeetingService->GetMeetingParticipantsController();
	IList<unsigned int >* ParticipantsList = m_pParticipantsController->GetParticipantsList();
	std::cout << "Participant List is : " << ParticipantsList->GetCount() << std::endl;
	return ParticipantsList;
}

//check if you have permission to start raw recording
void CheckAndStartRawRecording(bool isVideo, bool isAudio) {

	if (isVideo || isAudio) {
		m_pRecordController = m_pMeetingService->GetMeetingRecordingController();
		SDKError err2 = m_pMeetingService->GetMeetingRecordingController()->CanStartRawRecording();

		if (err2 == SDKERR_SUCCESS) {
			SDKError err1 = m_pRecordController->StartRawRecording();
			if (err1 != SDKERR_SUCCESS) {
				std::cout << "Error occurred starting raw recording" << std::endl;
			}
			else {

				//GetVideoRawData
				if (isVideo) {


					//get only for 1 user
					//IUserInfo* p = getFirstUserObj();
					//createRenderer(&videoHelper, rawdatapipedelegate);
					//rawdatapipedelegate->SubScribeUser(p, videoHelper);

					//get for all users
					IList<unsigned int >* pList = getAllUserObj();
					

				

					for (int i = 0; i < pList->GetCount(); ++i) {
						unsigned int userID = pList->GetItem(i);

						// Do something with userID
						m_pParticipantsController = m_pMeetingService->GetMeetingParticipantsController();
						IUserInfo* p = m_pParticipantsController->GetUserByUserID(userID);
						if (p != getMyself()) {
							cout << "User ID: " << userID << endl;
							rawdatapipedelegate = new ZoomSDKRawDataPipeDelegate();
							createRenderer(&videoHelper, rawdatapipedelegate);
							rawdatapipedelegate->SubScribeUser(p, videoHelper);

							//use the same delegate to subscribe audio
							if (isAudio) {
								audioHelper = GetAudioRawdataHelper();
								if (audioHelper) {
									SDKError err = audioHelper->subscribe(rawdatapipedelegate);
									if (err != SDKERR_SUCCESS) {
										std::cout << "Error occurred subscribing to audio : " << err << std::endl;
									}
								}
								else {
									std::cout << "Error getting audioHelper" << std::endl;
								}
							}

						}
						else {
							cout << "Skipping self User ID : " << userID << endl;
						}
					}


				}
				//GetAudioRawData
				if (isAudio && !isVideo) {
					audioHelper = GetAudioRawdataHelper();
					if (audioHelper) {
						SDKError err = audioHelper->subscribe(rawdatapipedelegate);
						if (err != SDKERR_SUCCESS) {
							std::cout << "Error occurred subscribing to audio : " << err << std::endl;
						}
					}
					else {
						std::cout << "Error getting audioHelper" << std::endl;
					}
				}
			}
		}
		else {
			std::cout << "Cannot start raw recording: no permissions yet, need host, co-host, or recording privilege" << std::endl;
		}
	}
}

//check if you meet the requirements to send raw data
void CheckAndStartRawSending(bool isVideo, bool isAudio) {


	//SendVideoRawData
	if (isVideo) {

		ZoomSDKVideoSource* virtual_camera_video_source = new ZoomSDKVideoSource(DEFAULT_VIDEO_SOURCE);
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
		ZoomSDKVirtualAudioMicEvent* audio_source = new ZoomSDKVirtualAudioMicEvent(DEFAULT_AUDIO_SOURCE);
		IZoomSDKAudioRawDataHelper* audioHelper = GetAudioRawdataHelper();
		if (audioHelper) {
			SDKError err = audioHelper->setExternalAudioSource(audio_source);
		}
	}


}


//callback when given host permission
void onIsHost() {
	printf("Is host now...\n");
	CheckAndStartRawRecording(GetVideoRawData, GetAudioRawData);

}

//callback when given cohost permission
void onIsCoHost() {
	printf("Is co-host now...\n");
	CheckAndStartRawRecording(GetVideoRawData, GetAudioRawData);

}

//callback when given recording permission
void onIsGivenRecordingPermission() {
	printf("Is given recording permissions now...\n");
	CheckAndStartRawRecording(GetVideoRawData, GetAudioRawData);

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

		m_pMeetingService->GetMeetingChatController()->SetEvent(new MeetingChatEventListener(&turnOnSendVideoAndAudio, &turnOffSendVideoandAudio));
	}

	//first attempt to start raw recording  / sending, upon successfully joined and achieved "in-meeting" state.

	CheckAndStartRawRecording(GetVideoRawData, GetAudioRawData);
	CheckAndStartRawSending(SendVideoRawData, SendAudioRawData);
}

//on meeting ended, typically by host, do something here. it is possible to reuse this SDK instance
void onMeetingEndsQuitApp() {
	// CleanSDK();
	//std::exit(0);
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
template<typename T>
bool processJsonValue(const Json& json, const std::string& key, T& value) {
	if (!json[key].is_null()) {
		value = json[key].get<std::string>();
		printf("config %s: %s\n", key.c_str(), value.c_str());
		return true;
	}
	return false;
}

// Function to parse and process JSON value as a boolean
bool processJsonBoolean(const Json& json, const std::string& key, bool& value) {
	if (!json[key].is_null()) {
		std::string stringValue = json[key].get<std::string>();
		std::transform(stringValue.begin(), stringValue.end(), stringValue.begin(), ::tolower);
		value = (stringValue == "true");
		printf("%s value is %s\n", key.c_str(), (value ? "true" : "false"));
		return true;
	}
	return false;
}


void ReadJsonSettings()
{


	std::string self_dir = getSelfDirPath();
	printf("self path: %s\n", self_dir.c_str());
	self_dir.append("/config.json");

	std::ifstream t(self_dir.c_str());
	t.seekg(0, std::ios::end);
	size_t size = t.tellg();
	std::string buffer(size, ' ');
	t.seekg(0);
	t.read(&buffer[0], size);

	Json config_json;
	try {
		config_json = Json::parse(buffer);
		printf("values of configuration from localfile: %s\n", buffer.c_str());
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


		processJsonBoolean(config_json, "useJWTTokenFromWebService", useJWTTokenFromWebService);
		processJsonBoolean(config_json, "useRecordingTokenFromWebService", useRecordingTokenFromWebService);
		processJsonBoolean(config_json, "GetVideoRawData", GetVideoRawData);
		processJsonBoolean(config_json, "GetAudioRawData", GetAudioRawData);
		processJsonBoolean(config_json, "SendVideoRawData", SendVideoRawData);
		processJsonBoolean(config_json, "SendAudioRawData", SendAudioRawData);
		// Additional processing or handling of parsed values can be done here
	}


	std::string bin_dir1 = getSelfDirPath();
	if (SendAudioRawData) {
		DEFAULT_AUDIO_SOURCE = bin_dir1.append("/").append(DEFAULT_AUDIO_SOURCE);
	}
	std::string bin_dir2 = getSelfDirPath();
	if (SendVideoRawData) {
		DEFAULT_VIDEO_SOURCE = bin_dir2.append("/").append(DEFAULT_VIDEO_SOURCE);
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
	if (videoHelper) {
		videoHelper->unSubscribe();
	}
	if (audioHelper) {
		audioHelper->unSubscribe();
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

	// Set the event listener for host, co-host 
	m_pParticipantsController = m_pMeetingService->GetMeetingParticipantsController();
	m_pParticipantsController->SetEvent(new MeetingParticipantsCtrlEventListener(&onIsHost, &onIsCoHost));

	// Set the event listener for recording privilege status
	m_pRecordController = m_pMeetingService->GetMeetingRecordingController();
	m_pRecordController->SetEvent(new MeetingRecordingCtrlEventListener(&onIsGivenRecordingPermission));


	// set event listnener for prompt handler 
	IMeetingReminderController* meetingremindercontroller = m_pMeetingService->GetMeetingReminderController();
	MeetingReminderEventListener* meetingremindereventlistener = new MeetingReminderEventListener();
	meetingremindercontroller->SetEvent(meetingremindereventlistener);

	//set event for Share Controller
	IMeetingShareController* meetingShareController = m_pMeetingService->GetMeetingShareController();
	SDKError shareEventError = meetingShareController->SetEvent(new MeetingShareCtrlEventListener());

	//prepare params used for joining meeting
	ZOOM_SDK_NAMESPACE::JoinParam joinParam;
	ZOOM_SDK_NAMESPACE::SDKError err(ZOOM_SDK_NAMESPACE::SDKERR_SERVICE_FAILED);
	joinParam.userType = ZOOM_SDK_NAMESPACE::SDK_UT_WITHOUT_LOGIN;
	ZOOM_SDK_NAMESPACE::JoinParam4WithoutLogin& withoutloginParam = joinParam.param.withoutloginuserJoin;
	// withoutloginParam.meetingNumber = 1231231234;
	withoutloginParam.meetingNumber = std::stoull(meeting_number);
	withoutloginParam.vanityID = NULL;
	withoutloginParam.userName = "LinuxChun";
	// withoutloginParam.psw = "1";
	withoutloginParam.psw = meeting_password.c_str();
	withoutloginParam.customer_key = NULL;
	withoutloginParam.webinarToken = NULL;
	withoutloginParam.isVideoOff = false;
	withoutloginParam.isAudioOff = false;

	std::cerr << "JWT token is " << token << std::endl;
	std::cerr << "Recording token is " << recording_token << std::endl;

	//automatically set app_privilege token if it is present in config.json, or retrieved from web service
	withoutloginParam.app_privilege_token = NULL;
	if (!recording_token.size() == 0)
	{
		withoutloginParam.app_privilege_token = recording_token.c_str();
		std::cerr << "Setting recording token" << std::endl;
	}
	else
	{
		std::cerr << "Leaving recording token as NULL" << std::endl;
	}

	if (GetAudioRawData) {
		//set join audio to true
		ZOOM_SDK_NAMESPACE::IAudioSettingContext* pAudioContext = m_pSettingService->GetAudioSettings();
		if (pAudioContext)
		{
			//ensure auto join audio
			pAudioContext->EnableAutoJoinAudio(true);
		}
	}
	if (SendVideoRawData) {

		//ensure video is turned on
		withoutloginParam.isVideoOff = false;
		//set join video to true
		ZOOM_SDK_NAMESPACE::IVideoSettingContext* pVideoContext = m_pSettingService->GetVideoSettings();
		if (pVideoContext)
		{
			pVideoContext->EnableAutoTurnOffVideoWhenJoinMeeting(false);
		}
	}
	if (SendAudioRawData) {

		ZOOM_SDK_NAMESPACE::IAudioSettingContext* pAudioContext = m_pSettingService->GetAudioSettings();
		if (pAudioContext)
		{
			//ensure auto join audio
			pAudioContext->EnableAutoJoinAudio(true);
			pAudioContext->EnableAlwaysMuteMicWhenJoinVoip(true);
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
	try {
		// Set libcurl options
		curl_easy_setopt(curl, CURLOPT_URL, remote_url.c_str());
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYSTATUS, 0L);


		headers = curl_slist_append(headers, "Expect:");
		headers = curl_slist_append(headers, "Content-Type: application/json");
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

		std::string json = "{\"meetingNumber\":\"" + meeting_number + "\",\"role\":1}";
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json.c_str());

		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseData);

		// Perform the HTTP request
		CURLcode res = curl_easy_perform(curl);

		if (res != CURLE_OK) {
			throw std::runtime_error(std::string("curl_easy_perform() failed: ") + curl_easy_strerror(res));
		}


		std::string responsestr = responseData.stream.str();

		// Process the response using your existing code
		Json responses_json;
		try
		{
			responses_json = Json::parse(responsestr);
			printf("config from web service: %s\n", responsestr.c_str());
		}
		catch (Json::parse_error& ex)
		{
		}

		Json json_signature = responses_json["signature"];
		Json json_sdkKey = responses_json["sdkKey"];
		Json json_recordingtoken = responses_json["recordingtoken"];

		if (!json_signature.is_null())
		{
			token = json_signature.get<std::string>();
		}

		if (useRecordingTokenFromWebService) {
			if (!json_recordingtoken.is_null())
			{
				recording_token = json_recordingtoken.get<std::string>();
			}
		}
		// Return the response data as a string
		return responsestr;
	}
	catch (const std::exception& e) {
		// Handle exceptions
		std::cerr << "Error: " << e.what() << std::endl;
		// Return an empty string or handle the error as needed
		return "";
	}

	// Cleanup libcurl
	curl_slist_free_all(headers);
	curl_easy_cleanup(curl);
	return "";
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

	if (useJWTTokenFromWebService) {
		std::string response = getJWTToken(remote_url);

	}

	//InitMeetingSDK();
	//AuthMeetingSDK();

	std::exit(0);
}

void initAppSettings()
{
	struct sigaction sigIntHandler;
	sigIntHandler.sa_handler = my_handler;
	sigemptyset(&sigIntHandler.sa_mask);
	sigIntHandler.sa_flags = 0;
	sigaction(SIGINT, &sigIntHandler, NULL);
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
	return 0;
}

