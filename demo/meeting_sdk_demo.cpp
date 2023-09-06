#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <glib.h>
#include <gtkmm.h>
#include <sstream>
#include <thread>
#include <sys/syscall.h>
#include <fstream>
#include <iosfwd>
#include <iostream>

//used to accept prompts
#include "MeetingReminderEventListener.h"
//used to listen to callbacks from meeting related matters
#include "MeetingServiceEventListener.h"
//used to listen to callbacks from authentication related matters
#include "AuthServiceEventListener.h"



//These are needed to readsettingsfromJSON named config.json
#include "json.hpp"
#include <curl/curl.h>

#include <zoom_sdk.h>
#include <auth_service_interface.h>
#include <meeting_service_interface.h>
#include <meeting_service_components/meeting_participants_ctrl_interface.h>
#include <meeting_service_components/meeting_video_interface.h>
#include <setting_service_interface.h>

//references for GetVideoRawData
#include "ZoomSDKRenderer.h"
#include <rawdata/rawdata_renderer_interface.h>
#include <rawdata/zoom_rawdata_api.h>

//references for GetAudioRawData
#include "ZoomSDKAudioRawData.h"
#include "meeting_service_components/meeting_recording_interface.h"

USING_ZOOM_SDK_NAMESPACE

std::mutex mtx;
bool jwtTokenGenerated = false;

GMainLoop* loop;

//These are needed to readsettingsfromJSON named config.json
using Json = nlohmann::json;
std::string meeting_number, token, meeting_password, recording_token, remote_url;

//Services which are needed to initialize, authenticate and configure settings for the SDK
ZOOM_SDK_NAMESPACE::IAuthService* m_pAuthService;
ZOOM_SDK_NAMESPACE::IMeetingService* m_pMeetingService;
ZOOM_SDK_NAMESPACE::ISettingService* m_pSettingService;


//references for GetVideoRawData
ZoomSDKRenderer* videoSource = new ZoomSDKRenderer();
IZoomSDKRenderer* videoHelper;
IMeetingRecordingController* m_pRecordController;
IMeetingParticipantsController* m_pParticipantsController;

//references for GetAudioRawData
ZoomSDKAudioRawData* audio_source = new ZoomSDKAudioRawData();
IZoomSDKAudioRawDataHelper* audioHelper;

unsigned int userID;

//controls for demo
bool isHeadless = true;
bool GetVideoRawData = false;
bool GetAudioRawData = true;

uint32_t getUserID() {
	m_pParticipantsController = m_pMeetingService->GetMeetingParticipantsController();
	int returnvalue = m_pParticipantsController->GetParticipantsList()->GetItem(0);
	std::cout << "UserID is : " << returnvalue << std::endl;

	return returnvalue;
}
//GetVideoRawData
void attemptToStartRawRecording() {

	std::cout << "attemptToStartRawRecording : GetMeetingRecordingController" << std::endl;
	m_pRecordController = m_pMeetingService->GetMeetingRecordingController();
	std::cout << "attemptToStartRawRecording : StartRawRecording" << std::endl;
	SDKError err1 = m_pRecordController->StartRawRecording();
	if (err1 != SDKERR_SUCCESS) {
		std::cout << "Error occurred" << std::endl;
	}

	SDKError err = createRenderer(&videoHelper, videoSource);
	if (err != SDKERR_SUCCESS) {
		std::cout << "Error occurred" << std::endl;
		//handle error
	}
	else {
		std::cout << "attemptToStartRawRecording : subscribing" << std::endl;
		videoHelper->setRawDataResolution(ZoomSDKResolution_720P);
		videoHelper->subscribe(getUserID(), RAW_DATA_TYPE_VIDEO);
	}

}

//GetAudioRawData
void attemptToStartAudioRawRecording() {
	ZOOM_SDK_NAMESPACE::IAudioSettingContext* pAudioContext = m_pSettingService->GetAudioSettings();
	if (pAudioContext)
	{
		if (pAudioContext->GetSpeakerList()->GetCount() >= 1) {
			std::cout << "Number of speakers : " << pAudioContext->GetSpeakerList()->GetCount() << std::endl;
			std::cout << "Speaker(0) name : " << pAudioContext->GetSpeakerList()->GetItem(0)->GetDeviceName() << std::endl;
		}
	}
	m_pRecordController = m_pMeetingService->GetMeetingRecordingController();

	SDKError err1 = m_pRecordController->StartRawRecording();
	if (err1 != SDKERR_SUCCESS) {
		std::cout << "Error occurred starting raw recording" << std::endl;
	}

	audioHelper = GetAudioRawdataHelper();
	if (audioHelper) {
		SDKError err = audioHelper->subscribe(audio_source);
		if (err != SDKERR_SUCCESS) {
			std::cout << "Error occurred subscribing to audio : " << err << std::endl;
		}
	}
	else {
		std::cout << "Error getting audioHelper" << std::endl;
	}


}

//event callback
void onInMeeting() {


	printf("onInMeeting Invoked\n");


	//double check if you are in a meeting
	if (m_pMeetingService->GetMeetingStatus() == ZOOM_SDK_NAMESPACE::MEETING_STATUS_INMEETING) {
		printf("In Meeting Now...\n");
		IList<unsigned int>* participants = m_pMeetingService->GetMeetingParticipantsController()->GetParticipantsList();
		printf("Participants count: %d\n", participants->GetCount());

	}

	if (GetVideoRawData) {
		attemptToStartRawRecording();
	}
	if (GetAudioRawData) {
		attemptToStartAudioRawRecording();
	}

}

void onMeetingEndsQuitApp() {

}

void onMeetingJoined() {

	printf("Joining Meeting...\n");


}

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
	printf("getpath\n");
	return std::string(dest);
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

	do
	{
		Json config_json;
		try
		{
			config_json = Json::parse(buffer);
			printf("config all_content: %s\n", buffer.c_str());
		}
		catch (Json::parse_error& ex)
		{
			break;
		}

		if (config_json.is_null())
		{
			break;
		}

		Json json_meeting_number = config_json["meeting_number"];
		Json json_token = config_json["token"];
		Json json_meeting_password = config_json["meeting_password"];
		Json json_recording_token = config_json["recording_token"];
		Json json_remote_url = config_json["remote_url"];
		Json json_isHeadless = config_json["isHeadless"];

		if (!json_meeting_number.is_null())
		{
			meeting_number = json_meeting_number.get<std::string>();
			printf("config meeting_number: %s\n", meeting_number.c_str());
		}
		if (!json_token.is_null())
		{
			token = json_token.get<std::string>();
			printf("config token: %s\n", token.c_str());
		}
		if (!json_meeting_password.is_null())
		{
			meeting_password = json_meeting_password.get<std::string>();
			printf("config meeting_password: %s\n", meeting_password.c_str());
		}
		if (!json_recording_token.is_null())
		{
			recording_token = json_recording_token.get<std::string>();
			printf("config json_recording_token: %s\n", recording_token.c_str());
		}
		if (!json_remote_url.is_null())
		{
			remote_url = json_remote_url.get<std::string>();
			printf("config json_remote_url: %s\n", remote_url.c_str());
		}
		if (!json_isHeadless.is_null())
		{

			std::string stringValue = json_isHeadless.get<std::string>();
			// Convert the input string to lowercase for case-insensitive comparison
			std::transform(stringValue.begin(), stringValue.end(), stringValue.begin(), ::tolower);

			if (stringValue == "true")
			{
				std::cout << "isHeadless value is true" << std::endl;
				isHeadless = true;
			}
			else if (stringValue == "false")
			{
				std::cout << "isHeadless value is false" << std::endl;
				isHeadless = false;
			}
		}
	} while (false);

	printf("directory of config file: %s\n", self_dir.c_str());
}

void InitMeetingSDK(Gtk::TextView* text_view)

{
	ZOOM_SDK_NAMESPACE::SDKError err(ZOOM_SDK_NAMESPACE::SDKERR_SUCCESS);
	ZOOM_SDK_NAMESPACE::InitParam initParam;

	// set domain
	initParam.strWebDomain = "https://zoom.us";
	initParam.strSupportUrl = "https://zoom.us";

	// set language id
	initParam.emLanguageID = ZOOM_SDK_NAMESPACE::LANGUAGE_English;

	// change icon
	// initParam.uiWindowIconSmallID = IDI_ICON_LOGO;
	// initParam.uiWindowIconBigID = IDI_ICON_LOGO;
	// initParam.hResInstance = GetModuleHandle(NULL);

	//set logging perferences
	initParam.enableLogByDefault = true;
	initParam.enableGenerateDump = true;

	// initParam.obConfigOpts.optionalFeatures = ENABLE_CUSTOMIZED_UI_FLAG;


	err = ZOOM_SDK_NAMESPACE::InitSDK(initParam);
	if (err != ZOOM_SDK_NAMESPACE::SDKERR_SUCCESS)
	{

		if (text_view)
		{
			Glib::RefPtr<Gtk::TextBuffer> buffer = text_view->get_buffer();
			buffer->set_text("Init meetingSdk:error\n");
		}
		std::cerr << "Init meetingSdk:error " << std::endl;
	}
	else
	{
		if (text_view)
		{
			Glib::RefPtr<Gtk::TextBuffer> buffer = text_view->get_buffer();
			buffer->set_text("Init meetingSdk:success\n");
		}
		std::cerr << "Init meetingSdk:success" << std::endl;


	}

}

void CleanSDK(Gtk::TextView* text_view)
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

	err = ZOOM_SDK_NAMESPACE::CleanUPSDK();
	if (err != ZOOM_SDK_NAMESPACE::SDKERR_SUCCESS)
	{
		// printf("Init meetingSdk:error");
		if (text_view) {
			Glib::RefPtr<Gtk::TextBuffer> buffer = text_view->get_buffer();
			buffer->set_text("CleanSDK meetingSdk:error\n");
		}
		std::cerr << "CleanSDK meetingSdk:error " << std::endl;
	}
	else
	{
		if (text_view) {
			Glib::RefPtr<Gtk::TextBuffer> buffer = text_view->get_buffer();
			buffer->set_text("CleanSDK meetingSdk:success\n");
		}
		std::cerr << "CleanSDK meetingSdk:success" << std::endl;


	}


}



void JoinMeeting(Gtk::TextView* text_view, Gtk::TextView* text_view_userid, Gtk::Entry* entryA)
{
	std::cerr << "Joining Meeting" << std::endl;
	SDKError err2(SDKError::SDKERR_SUCCESS);


	//try to create the meetingservice object, this object will be used to join the meeting
	if ((err2 = CreateMeetingService(&m_pMeetingService)) != SDKError::SDKERR_SUCCESS) {};
	std::cerr << "MeetingService created." << std::endl;

	//before joining a meeting, create the setting service
	CreateSettingService(&m_pSettingService);
	std::cerr << "Settingservice created." << std::endl;

	if (GetAudioRawData) {
		//set join audio to true
		ZOOM_SDK_NAMESPACE::IAudioSettingContext* pAudioContext = m_pSettingService->GetAudioSettings();
		if (pAudioContext)
		{
			pAudioContext->EnableAutoJoinAudio(true);
		}
	}

	// Set the event listener
	m_pMeetingService->SetEvent(new MeetingServiceEventListener(&onMeetingJoined, &onMeetingEndsQuitApp, &onInMeeting));



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

	std::cerr << "JWT token is " << token << std::endl;
	std::cerr << "Recording token is " << recording_token << std::endl;

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

	withoutloginParam.isVideoOff = false;
	withoutloginParam.isAudioOff = false;

	if (GetAudioRawData) {

	}

	// set prompt handler here
	IMeetingReminderController* meetingremindercontroller = m_pMeetingService->GetMeetingReminderController();
	MeetingReminderEventListener* meetingremindereventlistener = new MeetingReminderEventListener();
	meetingremindercontroller->SetEvent(meetingremindereventlistener);

	//attempt to join meeting
	do
	{
		if (m_pMeetingService)
		{
			err = m_pMeetingService->Join(joinParam);
		}
		else
		{
			if (text_view)
			{
				Glib::RefPtr<Gtk::TextBuffer> buffer = text_view->get_buffer();
				buffer->set_text("join_meeting m_pMeetingService:Null\n");
			}
			break;
		}

		if (ZOOM_SDK_NAMESPACE::SDKERR_SUCCESS == err)
		{
			if (text_view)
			{
				Glib::RefPtr<Gtk::TextBuffer> buffer = text_view->get_buffer();
				buffer->set_text("joinmeeting:success\n");
			}
		}
		else
		{
			if (text_view)
			{
				Glib::RefPtr<Gtk::TextBuffer> buffer = text_view->get_buffer();
				buffer->set_text("joinmeeting:errors\n");
			}
		}
	} while (false);
}

void LeaveMeeting(Gtk::TextView* text_view)
{
	ZOOM_SDK_NAMESPACE::MeetingStatus status = ZOOM_SDK_NAMESPACE::MEETING_STATUS_FAILED;

	do
	{
		if (NULL == m_pMeetingService)
		{
			if (text_view) {
				Glib::RefPtr<Gtk::TextBuffer> buffer = text_view->get_buffer();
				buffer->set_text("leave_meeting m_pMeetingService:Null\n");
			}
			break;
		}
		else
		{
			status = m_pMeetingService->GetMeetingStatus();
		}

		if (status == ZOOM_SDK_NAMESPACE::MEETING_STATUS_IDLE ||
			status == ZOOM_SDK_NAMESPACE::MEETING_STATUS_ENDED ||
			status == ZOOM_SDK_NAMESPACE::MEETING_STATUS_FAILED)
		{
			if (text_view) {
				Glib::RefPtr<Gtk::TextBuffer> buffer = text_view->get_buffer();
				buffer->set_text("leave_meeting not in meeting\n");
			}
			break;
		}

		if (SDKError::SDKERR_SUCCESS == m_pMeetingService->Leave(ZOOM_SDK_NAMESPACE::LEAVE_MEETING))
		{
			if (text_view) {
				Glib::RefPtr<Gtk::TextBuffer> buffer = text_view->get_buffer();
				buffer->set_text("leave_meeting success\n");
			}
			break;
		}
		else
		{
			if (text_view) {
				Glib::RefPtr<Gtk::TextBuffer> buffer = text_view->get_buffer();
				buffer->set_text("leave_meeting error\n");
			}
			break;
		}
	} while (false);
}

// dreamtcs
void OnAuthenticationComplete()
{
	if (isHeadless) {
		JoinMeeting(nullptr, nullptr, nullptr);
	}
}

void AuthMeetingSDK(Gtk::TextView* text_view)
{
	SDKError err(SDKError::SDKERR_SUCCESS);

	//create auth service
	if ((err = CreateAuthService(&m_pAuthService)) != SDKError::SDKERR_SUCCESS) {};
	std::cerr << "AuthService created." << std::endl;

	//Create a param to put in jwt token
	ZOOM_SDK_NAMESPACE::AuthContext param;

	//set the event listener for onauthenticationcompleted
	if ((err = m_pAuthService->SetEvent(new AuthServiceEventListener(&OnAuthenticationComplete))) != SDKError::SDKERR_SUCCESS) {};
	std::cout << "AuthServiceEventListener added." << std::endl;


	if (!token.size() == 0)
	{
		param.jwt_token = token.c_str();
		std::cerr << "AuthSDK:success " << std::endl;
	}

	//attempt to authenticate
	ZOOM_SDK_NAMESPACE::SDKError sdkErrorResult = m_pAuthService->SDKAuth(param);

	if (ZOOM_SDK_NAMESPACE::SDKERR_SUCCESS != sdkErrorResult)
	{
		if (text_view)
		{
			Glib::RefPtr<Gtk::TextBuffer> buffer = text_view->get_buffer();
			buffer->set_text("AuthSDK:error\n");
		}
		std::cerr << "AuthSDK:error " << std::endl;
	}
	else
	{
		if (text_view)
		{
			Glib::RefPtr<Gtk::TextBuffer> buffer = text_view->get_buffer();
			buffer->set_text("AuthSDK:success\n");
			std::cerr << "AuthSDK:success " << std::endl;
		}
	}
}

//Logging in  might be irrelavanet for a headless app
void Login(Gtk::TextView* text_view, Gtk::Entry* entryA)
{

	std::string text = "ssologintokenhere";
	const char* token = text.c_str();



	//dreamtcs not tested yet
	if (m_pAuthService)
	{
		ZOOM_SDK_NAMESPACE::SDKError err = ZOOM_SDK_NAMESPACE::SDKERR_SUCCESS;
		err = m_pAuthService->SSOLoginWithWebUriProtocol(token);

		if (ZOOM_SDK_NAMESPACE::SDKERR_SUCCESS != err)
		{

			std::cerr << "Login:error " << std::endl;

			return;
		}
		else
		{

			std::cerr << "Login:success " << std::endl;
		}
	}



}

void gen_token()
{
	//m_AuthSDKWorkFlow.GetSSOUrl();
}

void getuserID(Gtk::TextView* text_view, Gtk::TextView* text_view_userid, Gtk::Entry* entryA)
{

	if (m_pMeetingService->GetMeetingStatus() == MEETING_STATUS_INMEETING)
	{
		Glib::RefPtr<Gtk::TextBuffer> buffer = text_view->get_buffer();
		buffer->set_text("getuserID is not inMeeting ,please wait\n");
		return;
	}
	ZOOM_SDK_NAMESPACE::IList<unsigned int>* pParticipantsList = m_pMeetingService->GetMeetingParticipantsController()->GetParticipantsList();
	unsigned int myUserID = 0;
	if (pParticipantsList == NULL || (pParticipantsList && pParticipantsList->GetCount() == 0))
	{

		ZOOM_SDK_NAMESPACE::IUserInfo* pMyInfo = m_pMeetingService->GetMeetingParticipantsController()->GetMySelfUser();
		if (pMyInfo)
		{
			myUserID = pMyInfo->GetUserID();
			userID = myUserID;
		}
		return;
	}

	std::string username_id = "";
	for (int i = 0; i < pParticipantsList->GetCount(); i++)
	{
		unsigned int user_id = pParticipantsList->GetItem(i);
		std::string sUserName = m_pMeetingService->GetMeetingParticipantsController()->GetUserByUserID(user_id)->GetUserName();
		username_id += " userName: " + sUserName + "userId: " + std::to_string(user_id);
	}
	Glib::RefPtr<Gtk::TextBuffer> buffer_userid = text_view->get_buffer();
	buffer_userid->set_text(username_id);
}
void StartMeeting(Gtk::TextView* text_view, Gtk::TextView* text_view_userid)
{
	//if (!SDKInterfaceWrap::GetInst().login)
	//{
	//	Glib::RefPtr<Gtk::TextBuffer> buffer = text_view->get_buffer();
	//	buffer->set_text("login is not reading ,please wait\n");
	//	return;
	//}

	ZOOM_SDK_NAMESPACE::StartParam startParam;
	startParam.userType = ZOOM_SDK_NAMESPACE::SDK_UT_NORMALUSER;
	startParam.param.normaluserStart.vanityID = NULL;
	startParam.param.normaluserStart.customer_key = NULL;
	startParam.param.normaluserStart.isVideoOff = false;
	startParam.param.normaluserStart.isAudioOff = false;


	ZOOM_SDK_NAMESPACE::SDKError err = m_pMeetingService->Start(startParam);
	if (SDKError::SDKERR_SUCCESS == err)
	{
		Glib::RefPtr<Gtk::TextBuffer> buffer = text_view->get_buffer();
		buffer->set_text("StartMeeting success\n");
	}
	else
	{
		Glib::RefPtr<Gtk::TextBuffer> buffer = text_view->get_buffer();
		buffer->set_text("StartMeeting error\n");
	}
}

void mute_unmute_video(Gtk::TextView* text_view)
{
	ZOOM_SDK_NAMESPACE::SDKError err(ZOOM_SDK_NAMESPACE::SDKERR_SUCCESS);
	ZOOM_SDK_NAMESPACE::MeetingStatus status = ZOOM_SDK_NAMESPACE::MEETING_STATUS_FAILED;

	ZOOM_SDK_NAMESPACE::IMeetingVideoController* pVideoCtrl = m_pMeetingService->GetMeetingVideoController();
	if (pVideoCtrl == NULL)
	{
		Glib::RefPtr<Gtk::TextBuffer> buffer = text_view->get_buffer();
		buffer->set_text("pVideoCtrl is null\n");
		return;
	}

	ZOOM_SDK_NAMESPACE::IMeetingParticipantsController* pUserCtrl = m_pMeetingService->GetMeetingParticipantsController();
	if (!pUserCtrl)
		return;

	ZOOM_SDK_NAMESPACE::IUserInfo* pUserMe = pUserCtrl->GetMySelfUser();
	if (!pUserMe)
		return;

	//toggle between video on and off
	if (pUserMe->IsVideoOn())
	{
		err = pVideoCtrl->MuteVideo();
	}
	else
	{
		err = pVideoCtrl->UnmuteVideo();
	}

	if (SDKError::SDKERR_SUCCESS == err)
	{
		Glib::RefPtr<Gtk::TextBuffer> buffer = text_view->get_buffer();
		buffer->set_text("mute_unmute_video success\n");
	}
	else
	{
		Glib::RefPtr<Gtk::TextBuffer> buffer = text_view->get_buffer();
		buffer->set_text("mute_unmute_video error\n");
	}
}

void mute_unmute_audio(Gtk::TextView* text_view, Gtk::Entry* entryA)
{
	ZOOM_SDK_NAMESPACE::SDKError err(ZOOM_SDK_NAMESPACE::SDKERR_SUCCESS);
	ZOOM_SDK_NAMESPACE::MeetingStatus status = ZOOM_SDK_NAMESPACE::MEETING_STATUS_FAILED;

	ZOOM_SDK_NAMESPACE::IMeetingAudioController* pAudioCtrl = m_pMeetingService->GetMeetingAudioController();
	if (pAudioCtrl == NULL)
	{
		Glib::RefPtr<Gtk::TextBuffer> buffer = text_view->get_buffer();
		buffer->set_text("pVideoCtrl is null\n");
		return;
	}

	ZOOM_SDK_NAMESPACE::IMeetingParticipantsController* pUserCtrl = m_pMeetingService->GetMeetingParticipantsController();
	if (!pUserCtrl)
		return;

	ZOOM_SDK_NAMESPACE::IUserInfo* pUserMe = pUserCtrl->GetMySelfUser();
	if (!pUserMe)
		return;
	std::string text = entryA->get_text();
	const char* userid = text.c_str();
	int num = std::atoi(userid);

	//toggle between audio on and audio off
	if (!pUserMe->IsAudioMuted())
	{
		err = pAudioCtrl->MuteAudio(num);
	}
	else
	{
		err = pAudioCtrl->UnMuteAudio(num);
	}

	if (SDKError::SDKERR_SUCCESS == err)
	{
		Glib::RefPtr<Gtk::TextBuffer> buffer = text_view->get_buffer();
		buffer->set_text("mute_unmute_video success\n");
	}
	else
	{
		Glib::RefPtr<Gtk::TextBuffer> buffer = text_view->get_buffer();
		buffer->set_text("mute_unmute_video error\n");
	}
}

////////////////////////////////////////////////raw_data//////////////////////////////////////////////

void send_raw_video(Gtk::TextView* text_view)
{




}
void send_raw_audio(Gtk::TextView* text_view)
{


}

void request_recording_permissions(Gtk::TextView* text_view)
{

	std::cerr << "request_recording_permissions: " << std::endl;


	ZOOM_SDK_NAMESPACE::IMeetingRecordingController* m_pMeetingRecorder = m_pMeetingService->GetMeetingRecordingController();

	ZOOM_SDK_NAMESPACE::SDKError err = m_pMeetingRecorder->RequestLocalRecordingPrivilege();
	if (SDKError::SDKERR_SUCCESS == err)
	{
		Glib::RefPtr<Gtk::TextBuffer> buffer = text_view->get_buffer();
		buffer->set_text("request_recording_permissions success\n");
	}
	else
	{
		Glib::RefPtr<Gtk::TextBuffer> buffer = text_view->get_buffer();
		buffer->set_text("request_recording_permissions error\n");
	}
}



void subscribe_video(Gtk::TextView* text_view, Gtk::Entry* entryA)
{

}

void un_subscribe_video(Gtk::TextView* text_view)
{

}



// dreamtcs


static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp)
{
	printf("WriteCallback \n");

	((std::string*)userp)->append((char*)contents, size * nmemb);
	std::string response = (char*)contents;

	Json responses_json;
	try
	{
		responses_json = Json::parse(response);
		printf("config all_content: %s\n", response.c_str());
	}
	catch (Json::parse_error& ex)
	{
	}

	Json json_signature = responses_json["signature"];
	Json json_sdkKey = responses_json["sdkKey"];

	if (!json_signature.is_null())
	{
		token = json_signature.get<std::string>();
	}

	printf("Token in callback is: %s\n", token.c_str());
	std::lock_guard<std::mutex> lock(mtx);
	jwtTokenGenerated = true;

	return size * nmemb;
}

void getJWTToken(std::string remote_url)
{
	printf("declaring curl \n");
	CURL* curl;
	CURLcode res;
	std::string readBuffer;

	char* json = NULL;
	struct curl_slist* headers = NULL;
	printf("initing curl \n");
	curl = curl_easy_init();
	if (curl)
	{

		printf("setting remote url: %s\n", remote_url.c_str());

		curl_easy_setopt(curl, CURLOPT_URL, remote_url.c_str());

		// buffer size
		printf("setting buffer \n");
		curl_easy_setopt(curl, CURLOPT_BUFFERSIZE, 120000L);

		// temp workaround to enable SSL / HTTPS
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYSTATUS, 0);
		curl_easy_setopt(curl, CURLOPT_CAINFO, "/root/release_demo/demo/bin/cacert.pem");
		curl_easy_setopt(curl, CURLOPT_CAPATH, "/root/release_demo/demo/bin/cacert.pem");

		// headers
		printf("setting headers \n");
		headers = curl_slist_append(headers, "Expect:");
		headers = curl_slist_append(headers, "Content-Type: application/json");
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

		std::string json = "{\"meetingNumber\":\"" + meeting_number + "\",\"role\":1}";
		printf("setting payload: %s\n", json.c_str());
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json.c_str());

		// callback
		printf("preparing callback \n");
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

		// perform
		printf("calling remote URL \n");
		res = curl_easy_perform(curl);
		std::cout << readBuffer << std::endl;

		/* Check for errors */
		if (res != CURLE_OK)
			fprintf(stderr, "curl_easy_perform() failed: %s\n",
				curl_easy_strerror(res));

		/* always cleanup */
		curl_slist_free_all(headers);
		curl_easy_cleanup(curl);
	}
}



gboolean timeout_callback(gpointer data)
{
	return TRUE;
}

void my_handler(int s)
{

	printf("\nCaught signal %d\n", s);
	LeaveMeeting(nullptr);
	printf("Leaving session.\n");
	CleanSDK(nullptr);
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

	if (!isHeadless)
	{

		// 初始化GTKmm应用程序
		auto app = Gtk::Application::create(argc, argv);

		Gtk::Window window;
		window.set_default_size(600, 500);
		window.set_title("meetingsdk Demo");
		Gtk::Paned paned;
		window.add(paned);
		// 创建垂直布局容器
		Gtk::Box box(Gtk::ORIENTATION_VERTICAL);
		paned.add(box);

		// 水平容器
		Gtk::Box* hbox = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL));

		// 创建文本框
		Gtk::ScrolledWindow scrolled_window;
		Gtk::TextView text_view;
		Gtk::TextView text_view_userid;
		scrolled_window.add(text_view);
		scrolled_window.add(text_view_userid);
		box.pack_start(scrolled_window);

		// 创建文本框
		Gtk::ScrolledWindow scrolled_window_r;
		Gtk::TextView text_view_r;
		Gtk::TextView text_view_userid_r;
		scrolled_window.add(text_view_r);
		scrolled_window.add(text_view_userid_r);
		// 创建输入框
		Gtk::Entry entryA;
		hbox->pack_start(entryA);

		Gtk::Button* buttongen_token = Gtk::manage(new Gtk::Button("gen_token"));
		hbox->pack_start(*buttongen_token, Gtk::PACK_SHRINK);
		buttongen_token->signal_clicked().connect([]()
			{ gen_token(); });

		// 将水平布局容器添加到垂直布局容器中
		box.pack_start(*hbox, Gtk::PACK_SHRINK);

		// 创建按钮 a
		Gtk::Button button_a("Init sdk");
		button_a.set_size_request(100, 50);
		button_a.signal_clicked().connect(sigc::bind(sigc::ptr_fun(InitMeetingSDK), &text_view));
		box.pack_start(button_a);

		// 创建按钮 b
		Gtk::Button button_b("auth sdk");
		button_b.set_size_request(100, 50);
		button_b.signal_clicked().connect(sigc::bind(sigc::ptr_fun(AuthMeetingSDK), &text_view));
		box.pack_start(button_b);

		// 创建按钮 c
		Gtk::Button button_c("join meeting");
		button_c.set_size_request(100, 50);
		button_c.signal_clicked().connect(sigc::bind(sigc::ptr_fun(JoinMeeting), &text_view, &text_view_userid, &entryA));
		box.pack_start(button_c);

		// 创建按钮 d
		Gtk::Button button_d("leave meeting");
		button_d.set_size_request(100, 50);
		button_d.signal_clicked().connect(sigc::bind(sigc::ptr_fun(LeaveMeeting), &text_view));
		box.pack_start(button_d);

		// 创建按钮 e
		Gtk::Button button_e("start meeting");
		button_e.set_size_request(100, 50);
		button_e.signal_clicked().connect(sigc::bind(sigc::ptr_fun(StartMeeting), &text_view, &text_view_userid));
		box.pack_start(button_e);

		// 创建按钮 f
		Gtk::Button button_f("login");
		button_f.set_size_request(100, 50);
		button_f.signal_clicked().connect(sigc::bind(sigc::ptr_fun(Login), &text_view, &entryA));
		box.pack_start(button_f);

		// 创建按钮 h
		Gtk::Button button_h("getuser_ID");
		button_h.set_size_request(100, 50);
		button_h.signal_clicked().connect(sigc::bind(sigc::ptr_fun(getuserID), &text_view, &text_view_userid, &entryA));
		box.pack_start(button_h);

		// 创建按钮 i
		Gtk::Button button_i("mute_unmute_video");
		button_i.set_size_request(100, 50);
		button_i.signal_clicked().connect(sigc::bind(sigc::ptr_fun(mute_unmute_video), &text_view));
		box.pack_start(button_i);

		// 创建按钮 j
		Gtk::Button button_j("clean_sdk");
		button_j.set_size_request(100, 50);
		button_j.signal_clicked().connect(sigc::bind(sigc::ptr_fun(CleanSDK), &text_view));
		box.pack_start(button_j);

		// 创建按钮 v
		Gtk::Button button_v("mute_unmute_audio");
		button_v.set_size_request(100, 50);
		button_v.signal_clicked().connect(sigc::bind(sigc::ptr_fun(mute_unmute_audio), &text_view, &entryA));
		box.pack_start(button_v);

		/////////////////////////////////////////////raw_data////////////////////////////////////////////////////

		Gtk::Button button_r_c("request_recording_permissions (not useful for linux msdk to get raw data access)");
		button_r_c.set_size_request(100, 50);
		button_r_c.signal_clicked().connect(sigc::bind(sigc::ptr_fun(request_recording_permissions), &text_view_r));
		box.pack_start(button_r_c);


		Gtk::Button button_r_a("subscribe_video");
		button_r_a.set_size_request(100, 50);
		button_r_a.signal_clicked().connect(sigc::bind(sigc::ptr_fun(subscribe_video), &text_view_r, &entryA));
		box.pack_start(button_r_a);

		Gtk::Button button_r_b("un_subscribe_video");
		button_r_b.set_size_request(100, 50);
		button_r_b.signal_clicked().connect(sigc::bind(sigc::ptr_fun(un_subscribe_video), &text_view_r));
		box.pack_start(button_r_b);

		/////////////////////////////////////////////send raw_data////////////////////////////////////////////////////

		Gtk::Button button_s_a("Send Raw Video)");
		button_s_a.set_size_request(100, 50);
		button_s_a.signal_clicked().connect(sigc::bind(sigc::ptr_fun(send_raw_video), &text_view_r));
		box.pack_start(button_s_a);

		Gtk::Button button_s_b("Send Raw Audio");
		button_s_b.set_size_request(100, 50);
		button_s_b.signal_clicked().connect(sigc::bind(sigc::ptr_fun(send_raw_audio), &text_view_r));
		box.pack_start(button_s_b);

		window.show_all();

		// 获取并打印线程ID
		std::ostringstream oss;
		oss << "Thread ID: " << syscall(SYS_gettid) << std::endl;
		Glib::RefPtr<Gtk::TextBuffer> buffer = text_view.get_buffer();
		buffer->insert(buffer->end(), oss.str());
		return app->run(window);
	}
	else
	{



		std::thread tokenThread(getJWTToken, remote_url);
		tokenThread.join();

		// getJWTToken(remote_url);



		if (jwtTokenGenerated)
		{
			InitMeetingSDK(nullptr);
			AuthMeetingSDK(nullptr);
		}
		else
		{
			std::cout << "JWT token generation failed." << std::endl;
		}



		initAppSettings();

		loop = g_main_loop_new(NULL, FALSE);

		// add source to default context
		g_timeout_add(100, timeout_callback, loop);
		g_main_loop_run(loop);

		return 0;
	}
}
