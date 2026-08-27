#include <glib.h>
#include <signal.h>
#include <unistd.h>

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

#include "auth_service_interface.h"
#include "json.hpp"
#include "meeting_service_components/meeting_breakout_rooms_interface_v2.h"
#include "meeting_service_interface.h"
#include "zoom_sdk.h"

using json = nlohmann::json;
using namespace ZOOMSDK;

namespace {

volatile sig_atomic_t g_stop_requested = 0;

struct Config {
  unsigned long long meeting_number;
  std::string token;
  std::string password;
  std::string display_name;
  std::string room_name;
  bool create_room;
  bool assign_first_user;
  bool start_rooms;
  unsigned int stop_after_seconds;
};

std::string ExecutableDirectory() {
  char path[4096];
  const ssize_t length = readlink("/proc/self/exe", path, sizeof(path) - 1);
  if (length <= 0) {
    throw std::runtime_error("Cannot determine the executable directory");
  }
  path[length] = '\0';

  const std::string executable(path);
  const std::string::size_type separator = executable.find_last_of('/');
  return separator == std::string::npos ? "." : executable.substr(0, separator);
}

Config LoadConfig() {
  const std::string path = ExecutableDirectory() + "/config.json";
  std::ifstream stream(path.c_str());
  if (!stream) {
    throw std::runtime_error("Cannot open " + path +
                             "; copy config.json.example to config.json");
  }

  json data;
  stream >> data;
  const std::string meeting_number =
      data.value("meeting_number", std::string());
  const std::string token = data.value("token", std::string());
  if (meeting_number.empty() || token.empty() ||
      token == "YOUR_MEETING_SDK_JWT") {
    throw std::runtime_error(
        "meeting_number and a valid token are required in config.json");
  }

  Config config;
  try {
    config.meeting_number = std::stoull(meeting_number);
  } catch (const std::exception&) {
    throw std::runtime_error("meeting_number must contain digits only");
  }
  config.token = token;
  config.password = data.value("meeting_password", std::string());
  config.display_name =
      data.value("display_name", std::string("Breakout Room Demo"));
  config.room_name =
      data.value("breakout_room_name", std::string("SDK Breakout Room"));
  config.create_room = data.value("create_room_on_join", false);
  config.assign_first_user =
      data.value("assign_first_unassigned_user", false);
  config.start_rooms = data.value("start_breakout_rooms", false);
  config.stop_after_seconds = data.value(
      "stop_breakout_rooms_after_seconds", static_cast<unsigned int>(0));
  return config;
}

const char* Safe(const zchar_t* value) {
  return value ? value : "";
}

class BreakoutManager : public IMeetingBOControllerEvent,
                        public IBOCreatorEvent,
                        public IBOAdminEvent,
                        public IBODataEvent {
 public:
  explicit BreakoutManager(const Config& config) : config_(config) {}

  ~BreakoutManager() { Detach(); }

  void Attach(IMeetingBOController* controller) {
    Detach();
    controller_ = controller;
    if (!controller_ || !controller_->SetEvent(this)) {
      std::cerr << "Unable to register the breakout-room controller event"
                << std::endl;
      controller_ = nullptr;
      return;
    }

    std::cout << "Breakout rooms enabled: " << std::boolalpha
              << controller_->IsBOEnabled()
              << ", started: " << controller_->IsBOStarted()
              << ", status: " << controller_->GetBOStatus() << std::endl;

    SetCreator(controller_->GetBOCreatorHelper());
    SetAdmin(controller_->GetBOAdminHelper());
    SetData(controller_->GetBODataHelper());
    assistant_ = controller_->GetBOAssistantHelper();
    attendee_ = controller_->GetBOAttedeeHelper();

    PrintSnapshot();
    ManageRooms();
  }

  void Detach() {
    if (stop_timer_ != 0) {
      g_source_remove(stop_timer_);
      stop_timer_ = 0;
    }
    if (creator_) {
      creator_->SetEvent(nullptr);
    }
    if (admin_) {
      admin_->SetEvent(nullptr);
    }
    if (data_) {
      data_->SetEvent(nullptr);
    }
    if (controller_) {
      controller_->SetEvent(nullptr);
    }
    controller_ = nullptr;
    creator_ = nullptr;
    admin_ = nullptr;
    assistant_ = nullptr;
    attendee_ = nullptr;
    data_ = nullptr;
  }

  void onHasCreatorRightsNotification(IBOCreator* creator) override {
    std::cout << "Breakout creator rights granted" << std::endl;
    SetCreator(creator);
    ManageRooms();
  }

  void onHasAdminRightsNotification(IBOAdmin* admin) override {
    std::cout << "Breakout admin rights granted" << std::endl;
    SetAdmin(admin);
    ManageRooms();
  }

  void onHasAssistantRightsNotification(IBOAssistant* assistant) override {
    assistant_ = assistant;
    std::cout << "Breakout assistant rights granted" << std::endl;
  }

  void onHasAttendeeRightsNotification(IBOAttendee* attendee) override {
    attendee_ = attendee;
    std::cout << "Breakout attendee rights granted" << std::endl;
  }

  void onHasDataHelperRightsNotification(IBOData* data) override {
    std::cout << "Breakout data-helper rights granted" << std::endl;
    SetData(data);
    PrintSnapshot();
    ManageRooms();
  }

  void onLostCreatorRightsNotification() override {
    if (creator_) {
      creator_->SetEvent(nullptr);
    }
    creator_ = nullptr;
    std::cout << "Breakout creator rights lost" << std::endl;
  }

  void onLostAdminRightsNotification() override {
    if (admin_) {
      admin_->SetEvent(nullptr);
    }
    admin_ = nullptr;
    std::cout << "Breakout admin rights lost" << std::endl;
  }

  void onLostAssistantRightsNotification() override {
    assistant_ = nullptr;
    std::cout << "Breakout assistant rights lost" << std::endl;
  }

  void onLostAttendeeRightsNotification() override {
    attendee_ = nullptr;
    std::cout << "Breakout attendee rights lost" << std::endl;
  }

  void onLostDataHelperRightsNotification() override {
    if (data_) {
      data_->SetEvent(nullptr);
    }
    data_ = nullptr;
    std::cout << "Breakout data-helper rights lost" << std::endl;
  }

  void onNewBroadcastMessageReceived(const zchar_t* message,
                                     unsigned int sender_id,
                                     const zchar_t* sender_name) override {
    std::cout << "Breakout broadcast from " << Safe(sender_name) << " ("
              << sender_id << "): " << Safe(message) << std::endl;
  }

  void onBOStopCountDown(unsigned int seconds) override {
    std::cout << "Breakout rooms close in " << seconds << " seconds"
              << std::endl;
  }

  void onHostInviteReturnToMainSession(
      const zchar_t* host_name,
      IReturnToMainSessionHandler* handler) override {
    std::cout << "Host " << Safe(host_name)
              << " invited this user to return to the main session"
              << std::endl;
    if (handler) {
      handler->Ignore();
    }
  }

  void onBOStatusChanged(BO_STATUS status) override {
    std::cout << "Breakout status changed: " << status << std::endl;
    PrintSnapshot();
  }

  void onBOSwitchRequestReceived(const zchar_t* room_name,
                                 const zchar_t* room_id) override {
    std::cout << "Breakout switch requested: " << Safe(room_name) << " ("
              << Safe(room_id) << ")" << std::endl;
  }

  void onBroadcastBOVoiceStatus(bool started) override {
    std::cout << "Breakout voice broadcast active: " << std::boolalpha
              << started << std::endl;
  }

  void onBOCreateSuccess(const zchar_t* room_id) override {
    std::cout << "Deprecated create notification for room " << Safe(room_id)
              << std::endl;
  }

  void OnWebPreAssignBODataDownloadStatusChanged(
      PreAssignBODataStatus status) override {
    std::cout << "Pre-assigned breakout data status: " << status << std::endl;
  }

  void OnBOOptionChanged(const BOOption&) override {
    std::cout << "Breakout options changed" << std::endl;
  }

  void onCreateBOResponse(bool success, const zchar_t* room_id) override {
    std::cout << "Create breakout room: "
              << (success ? "success" : "failed") << std::endl;
    if (success && room_id) {
      created_room_id_ = room_id;
      ManageRooms();
    }
  }

  void onRemoveBOResponse(bool success, const zchar_t* room_id) override {
    std::cout << "Remove breakout room " << Safe(room_id) << ": "
              << (success ? "success" : "failed") << std::endl;
  }

  void onUpdateBONameResponse(bool success, const zchar_t* room_id) override {
    std::cout << "Rename breakout room " << Safe(room_id) << ": "
              << (success ? "success" : "failed") << std::endl;
  }

  void onHelpRequestReceived(const zchar_t* user_id) override {
    std::cout << "Breakout help requested by user " << Safe(user_id)
              << std::endl;
  }

  void onStartBOError(BOControllerError error) override {
    std::cerr << "Start breakout rooms failed with controller error " << error
              << std::endl;
  }

  void onBOEndTimerUpdated(int remaining, bool times_up_notice) override {
    std::cout << "Breakout timer: " << remaining
              << " seconds, auto-stop=" << std::boolalpha << times_up_notice
              << std::endl;
  }

  void onStartBOResponse(bool success) override {
    std::cout << "Start breakout rooms: "
              << (success ? "success" : "failed") << std::endl;
    if (success && config_.stop_after_seconds > 0 && stop_timer_ == 0) {
      stop_timer_ =
          g_timeout_add_seconds(config_.stop_after_seconds, StopCallback, this);
    }
  }

  void onStopBOResponse(bool success) override {
    std::cout << "Stop breakout rooms: "
              << (success ? "success" : "failed") << std::endl;
  }

  void onBOInfoUpdated(const zchar_t* room_id) override {
    std::cout << "Breakout room updated: " << Safe(room_id) << std::endl;
    PrintSnapshot();
  }

  void onUnAssignedUserUpdated() override {
    std::cout << "Unassigned breakout user list updated" << std::endl;
    PrintSnapshot();
    ManageRooms();
  }

  void OnBOListInfoUpdated() override {
    std::cout << "Breakout room list updated" << std::endl;
    PrintSnapshot();
  }

 private:
  void SetCreator(IBOCreator* creator) {
    if (creator_ == creator) {
      return;
    }
    if (creator_) {
      creator_->SetEvent(nullptr);
    }
    creator_ = creator;
    if (creator_) {
      creator_->SetEvent(this);
    }
  }

  void SetAdmin(IBOAdmin* admin) {
    if (admin_ == admin) {
      return;
    }
    if (admin_) {
      admin_->SetEvent(nullptr);
    }
    admin_ = admin;
    if (admin_) {
      admin_->SetEvent(this);
    }
  }

  void SetData(IBOData* data) {
    if (data_ == data) {
      return;
    }
    if (data_) {
      data_->SetEvent(nullptr);
    }
    data_ = data;
    if (data_) {
      data_->SetEvent(this);
    }
  }

  void ManageRooms() {
    if (config_.create_room && created_room_id_.empty()) {
      if (!creator_ || create_requested_) {
        return;
      }
      create_requested_ = true;
      std::cout << "Requesting breakout room: " << config_.room_name
                << std::endl;
      if (!creator_->CreateBreakoutRoom(config_.room_name.c_str())) {
        create_requested_ = false;
        std::cerr << "CreateBreakoutRoom request was rejected" << std::endl;
      }
      return;
    }

    if (config_.assign_first_user && !assignment_complete_) {
      if (!creator_ || !data_ || created_room_id_.empty()) {
        return;
      }

      IList<const zchar_t*>* users = data_->GetUnassignedUserList();
      if (!users) {
        return;
      }
      for (int index = 0; index < users->GetCount(); ++index) {
        const zchar_t* user_id = users->GetItem(index);
        if (!user_id || data_->IsBOUserMyself(user_id)) {
          continue;
        }
        assignment_complete_ =
            creator_->AssignUserToBO(user_id, created_room_id_.c_str());
        std::cout << "Assign user " << user_id << " to room "
                  << created_room_id_ << ": "
                  << (assignment_complete_ ? "success" : "failed")
                  << std::endl;
        break;
      }
      if (!assignment_complete_) {
        return;
      }
    }

    if (config_.start_rooms && !start_requested_) {
      if (!admin_) {
        return;
      }
      if (config_.create_room && created_room_id_.empty()) {
        return;
      }
      if (config_.assign_first_user && !assignment_complete_) {
        return;
      }
      if (!admin_->CanStartBO()) {
        std::cout << "Breakout rooms are not ready to start" << std::endl;
        return;
      }
      start_requested_ = true;
      if (!admin_->StartBO()) {
        start_requested_ = false;
        std::cerr << "StartBO request was rejected" << std::endl;
      }
    }
  }

  void PrintSnapshot() {
    if (!data_) {
      return;
    }

    IList<const zchar_t*>* rooms = data_->GetBOMeetingIDList();
    std::cout << "Breakout rooms: " << (rooms ? rooms->GetCount() : 0)
              << std::endl;
    if (rooms) {
      for (int index = 0; index < rooms->GetCount(); ++index) {
        const zchar_t* room_id = rooms->GetItem(index);
        IBOMeeting* room =
            room_id ? data_->GetBOMeetingByID(room_id) : nullptr;
        IList<const zchar_t*>* users =
            room ? room->GetBOUserList() : nullptr;
        std::cout << "  " << (room ? Safe(room->GetBOName()) : "")
                  << " [" << Safe(room_id) << "], users="
                  << (users ? users->GetCount() : 0) << std::endl;
      }
    }

    IList<const zchar_t*>* unassigned = data_->GetUnassignedUserList();
    std::cout << "Unassigned users: "
              << (unassigned ? unassigned->GetCount() : 0) << std::endl;
  }

  static gboolean StopCallback(gpointer data) {
    BreakoutManager* manager = static_cast<BreakoutManager*>(data);
    manager->stop_timer_ = 0;
    if (!manager->admin_ || !manager->admin_->StopBO()) {
      std::cerr << "StopBO request was rejected" << std::endl;
    }
    return G_SOURCE_REMOVE;
  }

  Config config_;
  IMeetingBOController* controller_ = nullptr;
  IBOCreator* creator_ = nullptr;
  IBOAdmin* admin_ = nullptr;
  IBOAssistant* assistant_ = nullptr;
  IBOAttendee* attendee_ = nullptr;
  IBOData* data_ = nullptr;
  std::string created_room_id_;
  bool create_requested_ = false;
  bool assignment_complete_ = false;
  bool start_requested_ = false;
  guint stop_timer_ = 0;
};

class MeetingApp;

class AuthListener : public IAuthServiceEvent {
 public:
  explicit AuthListener(MeetingApp* app) : app_(app) {}
  void onAuthenticationReturn(AuthResult ret) override;
  void onLoginReturnWithReason(LOGINSTATUS, IAccountInfo*, LoginFailReason) override {}
  void onLogout() override {}
  void onZoomIdentityExpired() override {}
  void onZoomAuthIdentityExpired() override {}

 private:
  MeetingApp* app_;
};

class MeetingListener : public IMeetingServiceEvent {
 public:
  explicit MeetingListener(MeetingApp* app) : app_(app) {}
  void onMeetingStatusChanged(MeetingStatus status, int result) override;
  void onMeetingStatisticsWarningNotification(StatisticsWarningType) override {}
  void onMeetingParameterNotification(const MeetingParameter*) override {}
  void onSuspendParticipantsActivities() override {}
  void onAICompanionActiveChangeNotice(bool) override {}
  void onMeetingTopicChanged(const zchar_t*) override {}
  void onMeetingFullToWatchLiveStream(const zchar_t*) override {}
  void onUserNetworkStatusChanged(MeetingComponentType,
                                  ConnectionQuality,
                                  unsigned int,
                                  bool) override {}

 private:
  MeetingApp* app_;
};

class MeetingApp {
 public:
  MeetingApp(const Config& config, GMainLoop* loop)
      : config_(config),
        loop_(loop),
        breakout_manager_(config),
        auth_listener_(this),
        meeting_listener_(this) {}

  ~MeetingApp() { Cleanup(); }

  bool Start() {
    InitParam init_param;
    init_param.strWebDomain = "https://zoom.us";
    init_param.strSupportUrl = "https://zoom.us";
    init_param.emLanguageID = LANGUAGE_English;
    init_param.enableLogByDefault = true;
    init_param.enableGenerateDump = true;

    SDKError result = InitSDK(init_param);
    if (result != SDKERR_SUCCESS) {
      std::cerr << "InitSDK failed: " << result << std::endl;
      return false;
    }
    sdk_initialized_ = true;

    result = CreateAuthService(&auth_service_);
    if (result != SDKERR_SUCCESS || !auth_service_) {
      std::cerr << "CreateAuthService failed: " << result << std::endl;
      return false;
    }
    auth_service_->SetEvent(&auth_listener_);

    AuthContext auth_context;
    auth_context.jwt_token = config_.token.c_str();
    result = auth_service_->SDKAuth(auth_context);
    if (result != SDKERR_SUCCESS) {
      std::cerr << "SDKAuth failed: " << result << std::endl;
      return false;
    }
    std::cout << "Authentication requested" << std::endl;
    return true;
  }

  void OnAuthentication(AuthResult result) {
    if (result != AUTHRET_SUCCESS) {
      std::cerr << "Authentication failed: " << result << std::endl;
      Quit();
      return;
    }

    SDKError sdk_result = CreateMeetingService(&meeting_service_);
    if (sdk_result != SDKERR_SUCCESS || !meeting_service_) {
      std::cerr << "CreateMeetingService failed: " << sdk_result << std::endl;
      Quit();
      return;
    }
    meeting_service_->SetEvent(&meeting_listener_);

    JoinParam join_param;
    join_param.userType = SDK_UT_WITHOUT_LOGIN;
    JoinParam4WithoutLogin& without_login =
        join_param.param.withoutloginuserJoin;
    without_login.meetingNumber = config_.meeting_number;
    without_login.userName = config_.display_name.c_str();
    without_login.psw = config_.password.c_str();
    without_login.vanityID = nullptr;
    without_login.customer_key = nullptr;
    without_login.webinarToken = nullptr;
    without_login.isVideoOff = false;
    without_login.isAudioOff = false;

    sdk_result = meeting_service_->Join(join_param);
    std::cout << "Join result: " << sdk_result << std::endl;
    if (sdk_result != SDKERR_SUCCESS) {
      Quit();
    }
  }

  void OnMeetingStatus(MeetingStatus status, int result) {
    std::cout << "Meeting status: " << status << ", result: " << result
              << std::endl;
    if (status == MEETING_STATUS_INMEETING) {
      breakout_manager_.Attach(
          meeting_service_->GetMeetingBOController());
    } else if (status == MEETING_STATUS_ENDED ||
               status == MEETING_STATUS_FAILED) {
      Quit();
    }
  }

  void RequestStop() {
    if (meeting_service_ &&
        meeting_service_->GetMeetingStatus() == MEETING_STATUS_INMEETING) {
      const SDKError result = meeting_service_->Leave(LEAVE_MEETING);
      if (result != SDKERR_SUCCESS) {
        std::cerr << "Leave failed: " << result << std::endl;
      }
    }
    Quit();
  }

 private:
  void Quit() {
    if (loop_ && g_main_loop_is_running(loop_)) {
      g_main_loop_quit(loop_);
    }
  }

  void Cleanup() {
    breakout_manager_.Detach();
    if (meeting_service_) {
      meeting_service_->SetEvent(nullptr);
      DestroyMeetingService(meeting_service_);
      meeting_service_ = nullptr;
    }
    if (auth_service_) {
      auth_service_->SetEvent(nullptr);
      DestroyAuthService(auth_service_);
      auth_service_ = nullptr;
    }
    if (sdk_initialized_) {
      CleanUPSDK();
      sdk_initialized_ = false;
    }
  }

  Config config_;
  GMainLoop* loop_;
  BreakoutManager breakout_manager_;
  IAuthService* auth_service_ = nullptr;
  IMeetingService* meeting_service_ = nullptr;
  AuthListener auth_listener_;
  MeetingListener meeting_listener_;
  bool sdk_initialized_ = false;
};

void AuthListener::onAuthenticationReturn(AuthResult ret) {
  app_->OnAuthentication(ret);
}

void MeetingListener::onMeetingStatusChanged(MeetingStatus status, int result) {
  app_->OnMeetingStatus(status, result);
}

void HandleSignal(int) {
  g_stop_requested = 1;
}

gboolean CheckSignal(gpointer data) {
  if (!g_stop_requested) {
    return G_SOURCE_CONTINUE;
  }
  static_cast<MeetingApp*>(data)->RequestStop();
  return G_SOURCE_REMOVE;
}

}  // namespace

int main() {
  try {
    const Config config = LoadConfig();
    GMainLoop* loop = g_main_loop_new(nullptr, FALSE);
    MeetingApp app(config, loop);

    signal(SIGINT, HandleSignal);
    signal(SIGTERM, HandleSignal);
    g_timeout_add(200, CheckSignal, &app);

    if (!app.Start()) {
      g_main_loop_unref(loop);
      return EXIT_FAILURE;
    }

    g_main_loop_run(loop);
    g_main_loop_unref(loop);
    return EXIT_SUCCESS;
  } catch (const std::exception& error) {
    std::cerr << "Error: " << error.what() << std::endl;
    return EXIT_FAILURE;
  }
}
