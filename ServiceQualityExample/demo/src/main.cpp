#include <glib.h>
#include <signal.h>
#include <unistd.h>

#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>

#include "auth_service_interface.h"
#include "json.hpp"
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
  unsigned int poll_interval_seconds;
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
      data.value("display_name", std::string("Service Quality Demo"));
  config.poll_interval_seconds =
      data.value("poll_interval_seconds", static_cast<unsigned int>(5));
  if (config.poll_interval_seconds == 0) {
    config.poll_interval_seconds = 1;
  }
  return config;
}

const char* ResultName(SDKError result) {
  return result == SDKERR_SUCCESS ? "success" : "failed";
}

const char* QualityName(ConnectionQuality quality) {
  switch (quality) {
    case Conn_Quality_Very_Bad:
      return "very bad";
    case Conn_Quality_Bad:
      return "bad";
    case Conn_Quality_Not_Good:
      return "not good";
    case Conn_Quality_Normal:
      return "normal";
    case Conn_Quality_Good:
      return "good";
    case Conn_Quality_Excellent:
      return "excellent";
    case Conn_Quality_Unknown:
    default:
      return "unknown";
  }
}

const char* ComponentName(MeetingComponentType component) {
  switch (component) {
    case MeetingComponentType_AUDIO:
      return "audio";
    case MeetingComponentType_VIDEO:
      return "video";
    case MeetingComponentType_SHARE:
      return "share";
    case MeetingComponentType_Def:
    default:
      return "unknown";
  }
}

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
  void onMeetingStatisticsWarningNotification(StatisticsWarningType type) override;
  void onMeetingParameterNotification(
      const MeetingParameter* meeting_param) override;
  void onSuspendParticipantsActivities() override;
  void onAICompanionActiveChangeNotice(bool active) override;
  void onMeetingTopicChanged(const zchar_t* topic) override;
  void onMeetingFullToWatchLiveStream(const zchar_t* live_stream_url) override;
  void onUserNetworkStatusChanged(MeetingComponentType type,
                                  ConnectionQuality quality,
                                  unsigned int user_id,
                                  bool uplink) override;

 private:
  MeetingApp* app_;
};

class MeetingApp {
 public:
  MeetingApp(const Config& config, GMainLoop* loop)
      : config_(config),
        loop_(loop),
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
    std::cout << "Join: " << ResultName(sdk_result) << " (" << sdk_result
              << ")" << std::endl;
    if (sdk_result != SDKERR_SUCCESS) {
      Quit();
    }
  }

  void OnMeetingStatus(MeetingStatus status, int result) {
    std::cout << "Meeting status: " << status << ", result: " << result
              << std::endl;
    if (status == MEETING_STATUS_INMEETING && quality_timer_ == 0) {
      std::cout << "Service-quality monitoring started" << std::endl;
      PollQuality();
      quality_timer_ =
          g_timeout_add_seconds(config_.poll_interval_seconds, PollCallback, this);
    }

    if (status == MEETING_STATUS_ENDED || status == MEETING_STATUS_FAILED) {
      Quit();
    }
  }

  void PollQuality() {
    if (!meeting_service_ ||
        meeting_service_->GetMeetingStatus() != MEETING_STATUS_INMEETING) {
      return;
    }

    std::cout << "\nConnection quality"
              << " | audio up="
              << QualityName(meeting_service_->GetAudioConnQuality(true))
              << " down="
              << QualityName(meeting_service_->GetAudioConnQuality(false))
              << " | video up="
              << QualityName(meeting_service_->GetVideoConnQuality(true))
              << " down="
              << QualityName(meeting_service_->GetVideoConnQuality(false))
              << " | share up="
              << QualityName(meeting_service_->GetSharingConnQuality(true))
              << " down="
              << QualityName(meeting_service_->GetSharingConnQuality(false))
              << std::endl;

    MeetingAudioStatisticInfo audio;
    SDKError result =
        meeting_service_->GetMeetingAudioStatisticInfo(audio);
    if (result == SDKERR_SUCCESS) {
      std::cout << "Audio send: " << audio.sendBandwidth << " kbps, "
                << audio.sendRTT << " ms RTT, " << audio.sendJitter
                << " ms jitter, " << audio.sendPacketLossAvg
                << "% avg loss | recv: " << audio.recvBandwidth << " kbps, "
                << audio.recvRTT << " ms RTT, " << audio.recvJitter
                << " ms jitter, " << audio.recvPacketLossAvg << "% avg loss"
                << std::endl;
    } else {
      std::cerr << "GetMeetingAudioStatisticInfo failed: " << result
                << std::endl;
    }

    MeetingASVStatisticInfo video;
    result = meeting_service_->GetMeetingVideoStatisticInfo(video);
    PrintVisualStats("Video", result, video);

    MeetingASVStatisticInfo share;
    result = meeting_service_->GetMeetingShareStatisticInfo(share);
    PrintVisualStats("Share", result, share);
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
  static gboolean PollCallback(gpointer data) {
    static_cast<MeetingApp*>(data)->PollQuality();
    return G_SOURCE_CONTINUE;
  }

  static void PrintVisualStats(const char* name,
                               SDKError result,
                               const MeetingASVStatisticInfo& stats) {
    if (result != SDKERR_SUCCESS) {
      std::cerr << "GetMeeting" << name << "StatisticInfo failed: " << result
                << std::endl;
      return;
    }

    const unsigned int send_width =
        static_cast<unsigned int>(stats.sendResolution) & 0xffff;
    const unsigned int send_height =
        (static_cast<unsigned int>(stats.sendResolution) >> 16) & 0xffff;
    const unsigned int recv_width =
        static_cast<unsigned int>(stats.recvResolution) & 0xffff;
    const unsigned int recv_height =
        (static_cast<unsigned int>(stats.recvResolution) >> 16) & 0xffff;

    std::cout << name << " send: " << stats.sendBandwidth << " kbps, "
              << stats.sendFps << " fps, " << stats.sendRTT << " ms RTT, "
              << send_width << "x" << send_height << ", "
              << stats.sendPacketLossAvg << "% avg loss | recv: "
              << stats.recvBandwidth << " kbps, " << stats.recvFps << " fps, "
              << stats.recvRTT << " ms RTT, " << recv_width << "x"
              << recv_height << ", "
              << stats.recvPacketLossAvg << "% avg loss" << std::endl;
  }

  void Quit() {
    if (loop_ && g_main_loop_is_running(loop_)) {
      g_main_loop_quit(loop_);
    }
  }

  void Cleanup() {
    if (quality_timer_ != 0) {
      g_source_remove(quality_timer_);
      quality_timer_ = 0;
    }
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
  IAuthService* auth_service_ = nullptr;
  IMeetingService* meeting_service_ = nullptr;
  AuthListener auth_listener_;
  MeetingListener meeting_listener_;
  guint quality_timer_ = 0;
  bool sdk_initialized_ = false;
};

void AuthListener::onAuthenticationReturn(AuthResult ret) {
  app_->OnAuthentication(ret);
}

void MeetingListener::onMeetingStatusChanged(MeetingStatus status, int result) {
  app_->OnMeetingStatus(status, result);
}

void MeetingListener::onMeetingStatisticsWarningNotification(
    StatisticsWarningType type) {
  std::cout << "Meeting statistics warning: " << type << std::endl;
}

void MeetingListener::onMeetingParameterNotification(
    const MeetingParameter* meeting_param) {
  if (meeting_param) {
    std::cout << "Meeting parameters are available" << std::endl;
  }
}

void MeetingListener::onSuspendParticipantsActivities() {
  std::cout << "Participant activities were suspended" << std::endl;
}

void MeetingListener::onAICompanionActiveChangeNotice(bool active) {
  std::cout << "AI Companion active: " << std::boolalpha << active << std::endl;
}

void MeetingListener::onMeetingTopicChanged(const zchar_t* topic) {
  std::cout << "Meeting topic changed: " << (topic ? topic : "") << std::endl;
}

void MeetingListener::onMeetingFullToWatchLiveStream(
    const zchar_t* live_stream_url) {
  std::cout << "Meeting is full; live stream available: "
            << (live_stream_url ? live_stream_url : "") << std::endl;
}

void MeetingListener::onUserNetworkStatusChanged(MeetingComponentType type,
                                                 ConnectionQuality quality,
                                                 unsigned int user_id,
                                                 bool uplink) {
  std::cout << "Network status: user=" << user_id
            << ", component=" << ComponentName(type)
            << ", direction=" << (uplink ? "uplink" : "downlink")
            << ", quality=" << QualityName(quality) << std::endl;
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
