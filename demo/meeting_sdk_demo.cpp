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
 #include "json.hpp"

#include "zoom_sdk.h"
#include "auth_service_interface.h"
#include "meeting_sdk_util.h"
#include "init_auth_sdk_workflow.h"
#include "RegressionTestRawdataRender.h"


using Json = nlohmann::json;
USING_ZOOM_SDK_NAMESPACE


GMainLoop *loop;

std::string meeting_number, token, meeting_password;
bool _inited = false;
unsigned int userID;
bool inMeeting = false;
CAuthSDKWorkFlow  m_AuthSDKWorkFlow;
CRegressionTestRawdataRender m_CRegressioRawdata;

void InitMeetingSDK()
{
    ZOOM_SDK_NAMESPACE::SDKError err(ZOOM_SDK_NAMESPACE::SDKERR_SUCCESS);
    ZOOM_SDK_NAMESPACE::InitParam initParam;
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
    
    //dreamtcs this is removed
    //initParam.obConfigOpts.optionalFeatures = true;

    err = ZOOM_SDK_NAMESPACE::InitSDK(initParam);
    if ( err != ZOOM_SDK_NAMESPACE::SDKERR_SUCCESS )
    {
         //printf("Init meetingSdk:error");
      
         std::cerr << "Init meetingSdk:error " << std::endl;
         _inited = false;
    }
    else
    {
      
        std::cerr << "Init meetingSdk:success" << std::endl;
        _inited = true;
        //printf("Init meetingSdk:success");
    }
    
    
}

void AuthMeetingSDK(std::string token)
{
   ZOOM_SDK_NAMESPACE::AuthContext param;
   param.jwt_token = token.c_str();
   if (ZOOM_SDK_NAMESPACE::SDKERR_SUCCESS != m_AuthSDKWorkFlow.Auth(param))
		{
      
			std::cerr << "AuthSDK:error " << std::endl;
		}
   else
		{
  
			std::cerr << "AuthSDK:success " << std::endl;
		}
    
}

void JoinMeeting(std::string meeting_number, std::string meeting_password)
{
    while(!SDKInterfaceWrap::GetInst().auth)
    {
    
        std::cerr << "auth is not reading ,please wait " << std::endl;
      break;
    }

    ZOOM_SDK_NAMESPACE::JoinParam joinParam;
    ZOOM_SDK_NAMESPACE::SDKError err(ZOOM_SDK_NAMESPACE::SDKERR_SERVICE_FAILED);
	joinParam.userType = ZOOM_SDK_NAMESPACE::SDK_UT_WITHOUT_LOGIN;
    ZOOM_SDK_NAMESPACE::JoinParam4WithoutLogin& withoutloginParam = joinParam.param.withoutloginuserJoin;
	withoutloginParam.meetingNumber =  std::stoull(meeting_number);
	withoutloginParam.vanityID = NULL;
	withoutloginParam.userName = "LinuxChun";
	withoutloginParam.psw = meeting_password.c_str();
	withoutloginParam.customer_key = NULL;
	withoutloginParam.webinarToken = NULL;
	withoutloginParam.isVideoOff = true;
	withoutloginParam.isAudioOff = false;
	//normalParam.isDirectShareDesktop = false;
    ZOOM_SDK_NAMESPACE::IMeetingService* m_pMeetingService = SDKInterfaceWrap::GetInst().GetMeetingService();
   do
   {
        if (m_pMeetingService)
        {
            err = m_pMeetingService->Join(joinParam);
        }
        else
        {
   
            std::cerr << "join_meeting m_pMeetingService:Null " << std::endl;
            break;
        }

        if ( ZOOM_SDK_NAMESPACE::SDKERR_SUCCESS == err )
        {
         
              std::cerr << "joinmeeting:success" << std::endl;
            
        }
        else 
        {
     
              std::cerr << "joinmeeting:errors" << std::endl;
        }
   }
   while(false);
    
}

void LeaveMeeting()
{
    ZOOM_SDK_NAMESPACE::MeetingStatus status = ZOOM_SDK_NAMESPACE::MEETING_STATUS_FAILED;
    ZOOM_SDK_NAMESPACE::IMeetingService* m_pMeetingService = SDKInterfaceWrap::GetInst().GetMeetingService();
    do
    {
        if(NULL == m_pMeetingService)
        {
        
               std::cerr << "leave_meeting m_pMeetingService:Null" << std::endl;
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
        
                std::cerr << "leave_meeting not in meeting" << std::endl;
            break;
        }

        if( SDKError::SDKERR_SUCCESS == m_pMeetingService->Leave(ZOOM_SDK_NAMESPACE::LEAVE_MEETING) )
        {
            std::cerr << "leave_meeting success" << std::endl;
            break;
        }
        else
        {
       
            std::cerr << "leave_meeting error" << std::endl;
            break;
        }
    }
    while(false);
   
}

void Login()
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
}

void gen_okken()
{
   m_AuthSDKWorkFlow.GetSSOUrl();
}

void getuserID()
{
   if(!SDKInterfaceWrap::GetInst().inMeeting)
    {

         std::cerr << "getuserID is not inMeeting ,please wait" << std::endl;
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
 
       std::cerr << username_id << std::endl;
}
void StartMeeting()
{
    if(!SDKInterfaceWrap::GetInst().login)
    {
 
          std::cerr << "login is not reading ,please wait" << std::endl;
        return;
    }

    ZOOM_SDK_NAMESPACE::StartParam startParam;
	startParam.userType = ZOOM_SDK_NAMESPACE::SDK_UT_NORMALUSER;
	startParam.param.normaluserStart.vanityID = NULL;
	startParam.param.normaluserStart.customer_key = NULL;
	startParam.param.normaluserStart.isVideoOff = false;
	startParam.param.normaluserStart.isAudioOff = false;
	//startParam.param.normaluserStart.isDirectShareDesktop = false;
    ZOOM_SDK_NAMESPACE::IMeetingService* m_pMeetingService = SDKInterfaceWrap::GetInst().GetMeetingService();
    ZOOM_SDK_NAMESPACE::SDKError err = m_pMeetingService->Start(startParam);
   if( SDKError::SDKERR_SUCCESS == err)
   {
       
           std::cerr << "StartMeeting success" << std::endl;
   }
   else
   {
   
           std::cerr << "StartMeeting error" << std::endl;
   }
}

void subscribe()
{
    m_CRegressioRawdata.Init();
    userID = std::stoi("userid");
    ZOOM_SDK_NAMESPACE::SDKError err = m_CRegressioRawdata.subscribe(userID, ZOOM_SDK_NAMESPACE::ZoomSDKRawDataType::RAW_DATA_TYPE_VIDEO);
    if( SDKError::SDKERR_SUCCESS == err)
    {
        
           std::cerr << "subscribe success" << std::endl;
    }
    else
    {
     
           std::cerr << "subscribe error" << std::endl;
    }
}


void mute_unmute_video()
{
    ZOOM_SDK_NAMESPACE::SDKError err(ZOOM_SDK_NAMESPACE::SDKERR_SUCCESS);
    ZOOM_SDK_NAMESPACE::MeetingStatus status = ZOOM_SDK_NAMESPACE::MEETING_STATUS_FAILED;
    ZOOM_SDK_NAMESPACE::IMeetingService* m_pMeetingService = SDKInterfaceWrap::GetInst().GetMeetingService();
    ZOOM_SDK_NAMESPACE::IMeetingVideoController* pVideoCtrl = m_pMeetingService->GetMeetingVideoController();
    if (pVideoCtrl == NULL)
    {
      
        std::cerr << "pVideoCtrl is null" << std::endl;
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
     
         std::cerr << "mute_unmute_video success" << std::endl;
    }
    else
    {
      
         std::cerr << "mute_unmute_video error" << std::endl;
    }

}

// int main(int argc, char* argv[])
// {
    
//     //joinVideoSDKSession(session_name, session_psw, session_token);
//     // 初始化GTKmm应用程序
//     auto app = Gtk::Application::create(argc, argv);

//     // 创建主窗口
//     Gtk::Window window;
//     window.set_default_size(600, 400);  // 设置默认大小为300x200像素
//     window.set_title("meetingsdk Demo");

//     // 创建垂直布局容器
//     Gtk::Box box(Gtk::ORIENTATION_VERTICAL);
//     window.add(box);
    
//     //水平容器
//     Gtk::Box* hbox = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL));

//     // 创建文本框
//     Gtk::ScrolledWindow scrolled_window;
//     Gtk::TextView text_view;
//     Gtk::TextView text_view_userid;
//     scrolled_window.add(text_view);
//     scrolled_window.add(text_view_userid);
//     box.pack_start(scrolled_window);
    
//     // 创建输入框
//     Gtk::Entry entryA ;
//     hbox->pack_start(entryA);

//     Gtk::Button* buttongen_token = Gtk::manage(new Gtk::Button("gen_token"));
//     hbox->pack_start(*buttongen_token, Gtk::PACK_SHRINK);
//     buttongen_token->signal_clicked().connect([](){
//         gen_okken();
//     });

//     // 将水平布局容器添加到垂直布局容器中
//     box.pack_start(*hbox, Gtk::PACK_SHRINK);


//     // 创建按钮 a
//     Gtk::Button button_a("Init sdk");
//     button_a.set_size_request(100, 50);
//     button_a.signal_clicked().connect(sigc::bind(sigc::ptr_fun(InitMeetingSDK), &text_view));
//     box.pack_start(button_a);

//     // 创建按钮 b
//     Gtk::Button button_b("auth sdk");
//     button_b.set_size_request(100, 50);
//     button_b.signal_clicked().connect(sigc::bind(sigc::ptr_fun(AuthMeetingSDK), &text_view));
//     box.pack_start(button_b);

//     // 创建按钮 c
//     Gtk::Button button_c("join meeting");
//     button_c.set_size_request(100, 50);
//     button_c.signal_clicked().connect(sigc::bind(sigc::ptr_fun(JoinMeeting), &text_view,&text_view_userid));
//     box.pack_start(button_c);

//     // 创建按钮 d
//     Gtk::Button button_d("leave meeting");
//     button_d.set_size_request(100, 50);
//     button_d.signal_clicked().connect(sigc::bind(sigc::ptr_fun(LeaveMeeting), &text_view));
//     box.pack_start(button_d);
   
//    // 创建按钮 e
//     Gtk::Button button_e("start meeting");
//     button_e.set_size_request(100, 50);
//     button_e.signal_clicked().connect(sigc::bind(sigc::ptr_fun(StartMeeting), &text_view,&text_view_userid));
//     box.pack_start(button_e);

//     // 创建按钮 f
//     Gtk::Button button_f("login");
//     button_f.set_size_request(100, 50);
//     button_f.signal_clicked().connect(sigc::bind(sigc::ptr_fun(Login), &text_view,&entryA));
//     box.pack_start(button_f);
    
//     // 创建按钮 g
//     Gtk::Button button_g("subscribe");
//     button_g.set_size_request(100, 50);
//     button_g.signal_clicked().connect(sigc::bind(sigc::ptr_fun(subscribe), &text_view,&entryA));
//     box.pack_start(button_g);

//     // 创建按钮 h
//     Gtk::Button button_h("getuser_ID");
//     button_h.set_size_request(100, 50);
//     button_h.signal_clicked().connect(sigc::bind(sigc::ptr_fun(getuserID), &text_view,&text_view_userid,&entryA));
//     box.pack_start(button_h);

//     // 创建按钮 i
//     Gtk::Button button_i("mute_unmute_video");
//     button_i.set_size_request(100, 50);
//     button_i.signal_clicked().connect(sigc::bind(sigc::ptr_fun(mute_unmute_video), &text_view));
//     box.pack_start(button_i);
   
   
    
//     // 显示所有控件
//     // button_a.show();
//     // button_b.show();
//     // button_c.show();
//     // button_d.show();
//     // button_e.show();
//     // button_f.show();
//     // scrolled_window.show();
//     // text_view.show();
//     // box.show();
//     window.show_all();

//     // 获取并打印线程ID
//     std::ostringstream oss;
//     oss << "Thread ID: " << syscall(SYS_gettid) << std::endl;
//     Glib::RefPtr<Gtk::TextBuffer> buffer = text_view.get_buffer();
//     buffer->insert(buffer->end(), oss.str());

//     // 显示窗口和运行主循环
//     return app->run(window);
// }
    gboolean timeout_callback(gpointer data)
    {
        return TRUE;
    }

    void my_handler(int s)
    {

        printf("\nCaught signal %d\n", s);
        JoinMeeting(meeting_number,meeting_password);
        //LeaveMeeting();
        //printf("Leaving session.\n");
        //std::exit(0);
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
    int main(int argc, char *argv[])
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
        } while (false);

        if (meeting_number.size() == 0 || token.size() == 0)
        {
            return 0;
        }

        //init
        InitMeetingSDK();
        //auth
        AuthMeetingSDK(token);
        //join
         printf("begin to join: %s\n", self_dir.c_str());
		JoinMeeting(meeting_number,meeting_password);
   

        struct sigaction sigIntHandler;

        sigIntHandler.sa_handler = my_handler;
        sigemptyset(&sigIntHandler.sa_mask);
        sigIntHandler.sa_flags = 0;

        sigaction(SIGINT, &sigIntHandler, NULL);

        loop = g_main_loop_new(NULL, FALSE);

        // add source to default context
        g_timeout_add(100, timeout_callback, loop);
        g_main_loop_run(loop);
        return 0;
    }