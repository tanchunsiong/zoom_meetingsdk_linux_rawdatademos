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
#include <SDL2/SDL.h>

#include "zoom_sdk.h"
#include "auth_service_interface.h"
#include "meeting_sdk_util.h"
#include "init_auth_sdk_workflow.h"
#include "RegressionTestRawdataRender.h"
#include "meeting_sdk_rawdata_audio.h"

#include "MeetingReminderEventListener.h"

 #include "json.hpp"
 
#include <curl/curl.h>
// this needs sudo apt install libcurl4-openssl-dev (ubuntu)
// this needs yum install libcurl-devel (centos)


USING_ZOOM_SDK_NAMESPACE

using Json = nlohmann::json;
GMainLoop *loop;
std::string meeting_number, token, meeting_password, recording_token,remote_url;


CRegressionTestRawdataRender video_render_;
CRegressionTestRawdataRender share_render_;
unsigned int userID;
bool inMeeting = false;
bool isHeadless = true;
CAuthSDKWorkFlow  m_AuthSDKWorkFlow;


    gboolean timeout_callback(gpointer data)
    {
        return TRUE;
    }

    void my_handler(int s)
    {

        printf("\nCaught signal %d\n", s);
       
        //LeaveMeeting();
        printf("Leaving session.\n");
        std::exit(0);
    }
    std::string getSelfDirPath()
    {
        char dest[PATH_MAX];
        memset(dest, 0, sizeof(dest)); // readlink does not null terminate!
        if (readlink("/proc/self/exe", dest, PATH_MAX) == -1)
        {
        }

        char *tmp = strrchr(dest, '/');
        if (tmp)
            *tmp = 0;
        printf("getpath\n");
        return std::string(dest);
    }



void initAppSettings(){

    struct sigaction sigIntHandler;

        sigIntHandler.sa_handler = my_handler;
        sigemptyset(&sigIntHandler.sa_mask);
        sigIntHandler.sa_flags = 0;
        sigaction(SIGINT, &sigIntHandler, NULL);
}

static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
     printf("WriteCallback \n");

  
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    std::string response=(char*)contents;
 

    
        Json responses_json;
        try
        {
            responses_json = Json::parse(response);
            printf("config all_content: %s\n", response.c_str());
        }
        catch (Json::parse_error &ex)
        {
          
        }

        Json json_signature = responses_json["signature"];
        Json json_sdkKey = responses_json["sdkKey"];

        
          if (!json_signature.is_null())
            {
                token = json_signature.get<std::string>();
               
            }

  printf("Token in callback is: %s\n", token.c_str());
    return size * nmemb;

}



void getJWTToken(std::string remote_url){
  printf("declaring curl \n");
  CURL *curl;
  CURLcode res;
  std::string readBuffer;
  
  char *json = NULL;
  struct curl_slist *headers = NULL;
  printf("initing curl \n");
   curl = curl_easy_init();
  if(curl) {
   
     
    printf("setting remote url: %s\n", remote_url.c_str());
  
    curl_easy_setopt(curl, CURLOPT_URL, remote_url.c_str());

    //buffer size
    printf("setting buffer \n");
    curl_easy_setopt(curl, CURLOPT_BUFFERSIZE, 120000L);
    

   //temp workaround to enable SSL / HTTPS
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYSTATUS, 0);
     curl_easy_setopt(curl, CURLOPT_CAINFO, "/root/release_demo/demo/bin/cacert.pem");
    curl_easy_setopt(curl, CURLOPT_CAPATH, "/root/release_demo/demo/bin/cacert.pem");
   
   //headers
   printf("setting headers \n");
   headers = curl_slist_append(headers, "Expect:");
   headers = curl_slist_append(headers, "Content-Type: application/json");
   curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
  
 
   std::string json = "{\"meetingNumber\":\""+meeting_number+"\",\"role\":1}";
   printf("setting payload: %s\n", json.c_str());
   curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json.c_str());
    
    //callback
    printf("preparing callback \n");  
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
    
    //perform
    printf("calling remote URL \n");  
    res = curl_easy_perform(curl);
    std::cout << readBuffer << std::endl;
 
    /* Check for errors */
    if(res != CURLE_OK)
      fprintf(stderr, "curl_easy_perform() failed: %s\n",
              curl_easy_strerror(res));
 
    /* always cleanup */
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    
  }
 
}




void ReadJsonSettings(){
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
            catch (Json::parse_error &ex)
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
        } while (false);

        printf("directory of config file: %s\n", self_dir.c_str());
}

void InitMeetingSDK(Gtk::TextView* text_view)
{
    ZOOM_SDK_NAMESPACE::SDKError err(ZOOM_SDK_NAMESPACE::SDKERR_SUCCESS);
    ZOOM_SDK_NAMESPACE::InitParam initParam;
	// initParam.strWebDomain = "https://zoomdev.us";
	// initParam.strSupportUrl = "https://zoomdev.us";

    initParam.strWebDomain = "https://zoom.us";
	initParam.strSupportUrl = "https://zoom.us";
		//set language id
	initParam.emLanguageID = ZOOM_SDK_NAMESPACE::LANGUAGE_English;
		//change icon
	//initParam.uiWindowIconSmallID = IDI_ICON_LOGO;
	//initParam.uiWindowIconBigID = IDI_ICON_LOGO;
	//initParam.hResInstance = GetModuleHandle(NULL);
	initParam.enableLogByDefault = true;
	initParam.enableGenerateDump = true;
    // initParam.obConfigOpts.optionalFeatures = ENABLE_CUSTOMIZED_UI_FLAG;
    err = SDKInterfaceWrap::GetInst().Init(initParam);
    if ( err != ZOOM_SDK_NAMESPACE::SDKERR_SUCCESS )
    {
         //printf("Init meetingSdk:error");
         if (text_view){
            Glib::RefPtr<Gtk::TextBuffer> buffer = text_view->get_buffer();
            buffer->set_text("Init meetingSdk:error\n");
         }
         std::cerr << "Init meetingSdk:error " << std::endl;
        
    }
    else
    {
           if (text_view){
            Glib::RefPtr<Gtk::TextBuffer> buffer = text_view->get_buffer();
            buffer->set_text("Init meetingSdk:success\n");
           }
        std::cerr << "Init meetingSdk:success" << std::endl;
       
        //printf("Init meetingSdk:success");
    }
    video_render_.Init();
    
}

void CleanSDK(Gtk::TextView* text_view)
{
    ZOOM_SDK_NAMESPACE::SDKError err(ZOOM_SDK_NAMESPACE::SDKERR_SUCCESS);
    ZOOM_SDK_NAMESPACE::InitParam initParam;
    err = ZOOM_SDK_NAMESPACE::CleanUPSDK();
    if ( err != ZOOM_SDK_NAMESPACE::SDKERR_SUCCESS )
    {
         //printf("Init meetingSdk:error");
         Glib::RefPtr<Gtk::TextBuffer> buffer = text_view->get_buffer();
         buffer->set_text("CleanSDK meetingSdk:error\n");
         std::cerr << "CleanSDK meetingSdk:error " << std::endl;
         
    }
    else
    {
        Glib::RefPtr<Gtk::TextBuffer> buffer = text_view->get_buffer();
         buffer->set_text("CleanSDK meetingSdk:success\n");
        std::cerr << "CleanSDK meetingSdk:success" << std::endl;
        
        //printf("Init meetingSdk:success");
    }
    SDKInterfaceWrap::GetInst().Cleanup();
    m_AuthSDKWorkFlow.Cleanup();
}

void AuthMeetingSDK(Gtk::TextView* text_view)
{
	ZOOM_SDK_NAMESPACE::AuthContext param;
	param.jwt_token = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJhcHBLZXkiOiJUS0lMeVQ3blRTQzNPS1NiY3RGTTNBIiwiaWF0IjoxNjkyMzE1ODUzLCJleHAiOjE2OTI0ODg2NTMsInRva2VuRXhwIjoxNjkyNDg4NjUzLCJtbiI6MjMxMDIzMTMzLCJyb2xlIjoxfQ.wab_Z0X5i8KNuiXDJt9p-xFIxb5YDK7oM4JtDtjhabc";


	if (!token.size() == 0){

	param.jwt_token = token.c_str();
	}

    ZOOM_SDK_NAMESPACE::SDKError sdkErrorResult = m_AuthSDKWorkFlow.Auth(param);

     
   if (ZOOM_SDK_NAMESPACE::SDKERR_SUCCESS !=sdkErrorResult)
		{
            if (text_view){
            Glib::RefPtr<Gtk::TextBuffer> buffer = text_view->get_buffer();
            buffer->set_text("AuthSDK:error\n");
            }
			std::cerr << "AuthSDK:error " << std::endl;
		}
   else
		{
             if (text_view){
            Glib::RefPtr<Gtk::TextBuffer> buffer = text_view->get_buffer();
            buffer->set_text("AuthSDK:success\n");
			std::cerr << "AuthSDK:success " << std::endl;
             }
		}
}

void JoinMeeting(Gtk::TextView* text_view,Gtk::TextView* text_view_userid,Gtk::Entry* entryA)
{
      std::cerr << "Joining Meeting"  << std::endl;
    if(!SDKInterfaceWrap::GetInst().auth)
    { 
        if (text_view){
            Glib::RefPtr<Gtk::TextBuffer> buffer = text_view->get_buffer();
            buffer->set_text("auth is not reading ,please wait\n");
        }
        return;
    }

    ZOOM_SDK_NAMESPACE::JoinParam joinParam;
    ZOOM_SDK_NAMESPACE::SDKError err(ZOOM_SDK_NAMESPACE::SDKERR_SERVICE_FAILED);
	joinParam.userType = ZOOM_SDK_NAMESPACE::SDK_UT_WITHOUT_LOGIN;
    ZOOM_SDK_NAMESPACE::JoinParam4WithoutLogin& withoutloginParam = joinParam.param.withoutloginuserJoin;
    // withoutloginParam.meetingNumber = 2713378095;
    // withoutloginParam.psw = "1";
	withoutloginParam.meetingNumber =  std::stoull(meeting_number);
	withoutloginParam.vanityID = NULL;
	withoutloginParam.userName = "LinuxChun";
	withoutloginParam.psw = meeting_password.c_str();
	withoutloginParam.customer_key = NULL;
	withoutloginParam.webinarToken = NULL;
      std::cerr << "JWT token is " << token << std::endl;
	  std::cerr << "Recording token is " << recording_token << std::endl;
   
    withoutloginParam.app_privilege_token=NULL;
	if (!recording_token.size()==0){
		withoutloginParam.app_privilege_token=recording_token.c_str();
		  std::cerr << "Setting recording token"  << std::endl;
	}
	else{
    std::cerr << "Leaving recording token as NULL"  << std::endl;

	}

	withoutloginParam.isVideoOff = true;
	withoutloginParam.isAudioOff = true;
	//normalParam.isDirectShareDesktop = false;
    ZOOM_SDK_NAMESPACE::IMeetingService* m_pMeetingService = SDKInterfaceWrap::GetInst().GetMeetingService();



    //set prompt handler here
    IMeetingReminderController* meetingremindercontroller = m_pMeetingService->GetMeetingReminderController();
    MeetingReminderEventListener* meetingremindereventlistener = new MeetingReminderEventListener();
    meetingremindercontroller->SetEvent(meetingremindereventlistener);
    
   do
   {
        if (m_pMeetingService)
        {
            err = m_pMeetingService->Join(joinParam);
        }
        else
        {
            if (text_view){
                Glib::RefPtr<Gtk::TextBuffer> buffer = text_view->get_buffer();
                buffer->set_text("join_meeting m_pMeetingService:Null\n");
            }
            break;
        }

        if ( ZOOM_SDK_NAMESPACE::SDKERR_SUCCESS == err )
        {
                if (text_view){
                    Glib::RefPtr<Gtk::TextBuffer> buffer = text_view->get_buffer();
                    buffer->set_text("joinmeeting:success\n");
                }
        }
        else 
        {    if (text_view){
                    Glib::RefPtr<Gtk::TextBuffer> buffer = text_view->get_buffer();
                    buffer->set_text("joinmeeting:errors\n");
             }
        }
   }
   while(false);
    
}

void LeaveMeeting(Gtk::TextView* text_view)
{
    ZOOM_SDK_NAMESPACE::MeetingStatus status = ZOOM_SDK_NAMESPACE::MEETING_STATUS_FAILED;
    ZOOM_SDK_NAMESPACE::IMeetingService* m_pMeetingService = SDKInterfaceWrap::GetInst().GetMeetingService();
    do
    {
        if(NULL == m_pMeetingService)
        {
            Glib::RefPtr<Gtk::TextBuffer> buffer = text_view->get_buffer();
            buffer->set_text("leave_meeting m_pMeetingService:Null\n");
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
            Glib::RefPtr<Gtk::TextBuffer> buffer = text_view->get_buffer();
            buffer->set_text("leave_meeting not in meeting\n");
            break;
        }

        if( SDKError::SDKERR_SUCCESS == m_pMeetingService->Leave(ZOOM_SDK_NAMESPACE::LEAVE_MEETING) )
        {
            Glib::RefPtr<Gtk::TextBuffer> buffer = text_view->get_buffer();
            buffer->set_text("leave_meeting success\n");
            break;
        }
        else
        {
            Glib::RefPtr<Gtk::TextBuffer> buffer = text_view->get_buffer();
            buffer->set_text("leave_meeting error\n");
            break;
        }
    }
    while(false);
   
}

void Login(Gtk::TextView* text_view,Gtk::Entry* entryA)
{

    std::string text ="zaktokenhere";
    const char* token = text.c_str();
    if (ZOOM_SDK_NAMESPACE::SDKERR_SUCCESS != m_AuthSDKWorkFlow.Login(token))
		{

			std::cerr << "Login:error " << std::endl;

            return;
		}
   else
		{

			std::cerr << "Login:success " << std::endl;
		}
    // std::string text = entryA->get_text();
    // const char* token = text.c_str();
    // if (ZOOM_SDK_NAMESPACE::SDKERR_SUCCESS != m_AuthSDKWorkFlow.Login(token))
    // {
    //     Glib::RefPtr<Gtk::TextBuffer> buffer = text_view->get_buffer();
    //     buffer->set_text("Login:error\n");
    //     std::cerr << "Login:error " << std::endl;
    //     return;
    // }
    // else
    // {
    //     Glib::RefPtr<Gtk::TextBuffer> buffer = text_view->get_buffer();
    //     buffer->set_text("Login:success\n");
    //     std::cerr << "Login:success " << std::endl;
    // }
}

void gen_okken()
{
   m_AuthSDKWorkFlow.GetSSOUrl();
}

void getuserID(Gtk::TextView* text_view,Gtk::TextView* text_view_userid,Gtk::Entry* entryA)
{
   if(!SDKInterfaceWrap::GetInst().inMeeting)
    {
        Glib::RefPtr<Gtk::TextBuffer> buffer = text_view->get_buffer();
        buffer->set_text("getuserID is not inMeeting ,please wait\n");
        return;
    }
    ZOOM_SDK_NAMESPACE::IList<unsigned int >* pParticipantsList = SDKInterfaceWrap::GetInst().GetMeetingService()->GetMeetingParticipantsController()->GetParticipantsList();
	unsigned int myUserID = 0;
    if (pParticipantsList == NULL || (pParticipantsList && pParticipantsList->GetCount() == 0))
    {
        ZOOM_SDK_NAMESPACE::IUserInfo* pMyInfo = SDKInterfaceWrap::GetInst().GetMeetingService()->GetMeetingParticipantsController()->GetMySelfUser();
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
			std::string sUserName = SDKInterfaceWrap::GetInst().GetMeetingService()->GetMeetingParticipantsController()->GetUserByUserID(user_id)->GetUserName();
            username_id += "userName:  " + sUserName + "userId:  " +std::to_string(user_id);
            
		}
    Glib::RefPtr<Gtk::TextBuffer> buffer_userid = text_view->get_buffer();
    buffer_userid->set_text(username_id);
}
void StartMeeting(Gtk::TextView* text_view,Gtk::TextView* text_view_userid)
{
    if(!SDKInterfaceWrap::GetInst().login)
    {
        Glib::RefPtr<Gtk::TextBuffer> buffer = text_view->get_buffer();
        buffer->set_text("login is not reading ,please wait\n");
        return;
    }

    ZOOM_SDK_NAMESPACE::StartParam startParam;
	startParam.userType = ZOOM_SDK_NAMESPACE::SDK_UT_NORMALUSER;
	startParam.param.normaluserStart.vanityID = NULL;
	startParam.param.normaluserStart.customer_key = NULL;
	startParam.param.normaluserStart.isVideoOff = false;
	startParam.param.normaluserStart.isAudioOff = true;
	//startParam.param.normaluserStart.isDirectShareDesktop = false;
    ZOOM_SDK_NAMESPACE::IMeetingService* m_pMeetingService = SDKInterfaceWrap::GetInst().GetMeetingService();
    ZOOM_SDK_NAMESPACE::SDKError err = m_pMeetingService->Start(startParam);
   if( SDKError::SDKERR_SUCCESS == err)
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
    ZOOM_SDK_NAMESPACE::IMeetingService* m_pMeetingService = SDKInterfaceWrap::GetInst().GetMeetingService();
    ZOOM_SDK_NAMESPACE::IMeetingVideoController* pVideoCtrl = m_pMeetingService->GetMeetingVideoController();
    if (pVideoCtrl == NULL)
    {
        Glib::RefPtr<Gtk::TextBuffer> buffer = text_view->get_buffer();
        buffer->set_text("pVideoCtrl is null\n");
        return;
    }
	
    ZOOM_SDK_NAMESPACE::IMeetingParticipantsController* pUserCtrl = m_pMeetingService->GetMeetingParticipantsController();
    if(!pUserCtrl)
        return;

    ZOOM_SDK_NAMESPACE::IUserInfo* pUserMe = pUserCtrl->GetMySelfUser();
    if(!pUserMe)
        return;
    
    if(pUserMe->IsVideoOn())
    {
        err = pVideoCtrl->MuteVideo();
    }
    else
    {
        err = pVideoCtrl->UnmuteVideo();
    }

    if( SDKError::SDKERR_SUCCESS == err)
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

void mute_unmute_audio(Gtk::TextView* text_view,Gtk::Entry* entryA)
{
    ZOOM_SDK_NAMESPACE::SDKError err(ZOOM_SDK_NAMESPACE::SDKERR_SUCCESS);
    ZOOM_SDK_NAMESPACE::MeetingStatus status = ZOOM_SDK_NAMESPACE::MEETING_STATUS_FAILED;
    ZOOM_SDK_NAMESPACE::IMeetingService* m_pMeetingService = SDKInterfaceWrap::GetInst().GetMeetingService();
    ZOOM_SDK_NAMESPACE::IMeetingAudioController* pAudioCtrl = m_pMeetingService->GetMeetingAudioController();
    if (pAudioCtrl == NULL)
    {
        Glib::RefPtr<Gtk::TextBuffer> buffer = text_view->get_buffer();
        buffer->set_text("pVideoCtrl is null\n");
        return;
    }
	
    ZOOM_SDK_NAMESPACE::IMeetingParticipantsController* pUserCtrl = m_pMeetingService->GetMeetingParticipantsController();
    if(!pUserCtrl)
        return;

    ZOOM_SDK_NAMESPACE::IUserInfo* pUserMe = pUserCtrl->GetMySelfUser();
    if(!pUserMe)
        return;
    std::string text = entryA->get_text();
    const char* userid = text.c_str();
    int num = std::atoi(userid);
    if(!pUserMe->IsAudioMuted())
    {
        err = pAudioCtrl->MuteAudio(num);
    }
    else
    {
        err = pAudioCtrl->UnMuteAudio(num);
    }

    if( SDKError::SDKERR_SUCCESS == err)
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
void  subscribe_video(Gtk::TextView* text_view,Gtk::Entry* entryA)
{
    video_render_.CreateRender();
    int user_ID = std::stoi(entryA->get_text());
    ZOOM_SDK_NAMESPACE::SDKError err = video_render_.subscribe(user_ID, ZOOM_SDK_NAMESPACE::ZoomSDKRawDataType::RAW_DATA_TYPE_VIDEO);
    if( SDKError::SDKERR_SUCCESS == err)
    {
        Glib::RefPtr<Gtk::TextBuffer> buffer = text_view->get_buffer();
        buffer->set_text("subscribe_video success\n");
    }
    else
    {
        Glib::RefPtr<Gtk::TextBuffer> buffer = text_view->get_buffer();
        buffer->set_text("subscribe_video error\n");
    }
}

void un_subscribe_video(Gtk::TextView* text_view)
{
    ZOOM_SDK_NAMESPACE::SDKError err = video_render_.unSubscribe();
    if( SDKError::SDKERR_SUCCESS == err)
    {
        Glib::RefPtr<Gtk::TextBuffer> buffer = text_view->get_buffer();
        buffer->set_text("un_subscribe_video success\n");
    }
    else
    {
        Glib::RefPtr<Gtk::TextBuffer> buffer = text_view->get_buffer();
        buffer->set_text("un_subscribe_video error\n");
    }
}

//dreamtcs
  static void OnAuthenticationComplete(bool success){
    if (success){
        JoinMeeting(nullptr,nullptr,nullptr);
    }
  }


int main(int argc, char* argv[])
{

   ReadJsonSettings();

    if (!isHeadless){

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

        //水平容器
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
        Gtk::Entry entryA ;
        hbox->pack_start(entryA);

        Gtk::Button* buttongen_token = Gtk::manage(new Gtk::Button("gen_token"));
        hbox->pack_start(*buttongen_token, Gtk::PACK_SHRINK);
        buttongen_token->signal_clicked().connect([](){
            gen_okken();
        });

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
        button_c.signal_clicked().connect(sigc::bind(sigc::ptr_fun(JoinMeeting), &text_view,&text_view_userid,&entryA));
        box.pack_start(button_c);

        // 创建按钮 d
        Gtk::Button button_d("leave meeting");
        button_d.set_size_request(100, 50);
        button_d.signal_clicked().connect(sigc::bind(sigc::ptr_fun(LeaveMeeting), &text_view));
        box.pack_start(button_d);
    
    // 创建按钮 e
        Gtk::Button button_e("start meeting");
        button_e.set_size_request(100, 50);
        button_e.signal_clicked().connect(sigc::bind(sigc::ptr_fun(StartMeeting), &text_view,&text_view_userid));
        box.pack_start(button_e);

        // 创建按钮 f
        Gtk::Button button_f("login");
        button_f.set_size_request(100, 50);
        button_f.signal_clicked().connect(sigc::bind(sigc::ptr_fun(Login), &text_view,&entryA));
        box.pack_start(button_f);
        

        // 创建按钮 h
        Gtk::Button button_h("getuser_ID");
        button_h.set_size_request(100, 50);
        button_h.signal_clicked().connect(sigc::bind(sigc::ptr_fun(getuserID), &text_view,&text_view_userid,&entryA));
        box.pack_start(button_h);

        // 创建按钮 i
        Gtk::Button button_i("mute_unmute_video");
        button_i.set_size_request(100, 50);
        button_i.signal_clicked().connect(sigc::bind(sigc::ptr_fun(mute_unmute_video), &text_view));
        box.pack_start(button_i);

        // 创建按钮 j
        Gtk::Button button_j("clearn_sdk");
        button_j.set_size_request(100, 50);
        button_j.signal_clicked().connect(sigc::bind(sigc::ptr_fun(CleanSDK), &text_view));
        box.pack_start(button_j);

        // 创建按钮 v
        Gtk::Button button_v("mute_unmute_audio");
        button_v.set_size_request(100, 50);
        button_v.signal_clicked().connect(sigc::bind(sigc::ptr_fun(mute_unmute_audio), &text_view,&entryA));
        box.pack_start(button_v);


        /////////////////////////////////////////////raw_data////////////////////////////////////////////////////
        // 创建按钮 a
        Gtk::Button button_r_a("subscribe_video");
        button_r_a.set_size_request(100, 50);
        button_r_a.signal_clicked().connect(sigc::bind(sigc::ptr_fun(subscribe_video), &text_view_r, &entryA));
        box.pack_start(button_r_a);

        // 创建按钮 a
        Gtk::Button button_r_b("un_subscribe_video");
        button_r_b.set_size_request(100, 50);
        button_r_b.signal_clicked().connect(sigc::bind(sigc::ptr_fun(un_subscribe_video), &text_view_r));
        box.pack_start(button_r_b);

        window.show_all();

        //获取并打印线程ID
        std::ostringstream oss;
        oss << "Thread ID: " << syscall(SYS_gettid) << std::endl;
        Glib::RefPtr<Gtk::TextBuffer> buffer = text_view.get_buffer();
        buffer->insert(buffer->end(), oss.str());
        return app->run(window);
    
    }
    else{
        //this does not work for centos at the moment, seg fault for SSL connections
         getJWTToken(remote_url);
        //init
        InitMeetingSDK(nullptr);
        
        SDKInterfaceWrap::SetAuthCompleteCallback(OnAuthenticationComplete);
        
      
       
    
        //auth
        AuthMeetingSDK(nullptr);
    
     
    

        initAppSettings();

    

        loop = g_main_loop_new(NULL, FALSE);

        // add source to default context
        g_timeout_add(100, timeout_callback, loop);
        g_main_loop_run(loop);
          usleep(2000000); 
    
        JoinMeeting(nullptr,nullptr,nullptr);
        return 0;
    }
}



