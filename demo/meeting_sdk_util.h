#pragma once

#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "glib.h"
#include <fstream>
#include <iosfwd>
#include <iostream>
#include <set>
#include <pthread.h>
#include <mutex>

#include "zoom_sdk.h"
#include "auth_service_interface.h"
#include "meeting_service_interface.h"
#include "meeting_service_components/meeting_audio_interface.h"
#include "meeting_service_components/meeting_video_interface.h"
#include "meeting_service_components/meeting_participants_ctrl_interface.h"
#include "meeting_service_components/meeting_sharing_interface.h"


USING_ZOOM_SDK_NAMESPACE

template<class T>
class EventSinkPool
{
public:
    EventSinkPool()
    {

    }
    virtual ~EventSinkPool()
    {
        pthread_mutex_lock(&_event_sink_lock);
        _event_sink_pool.clear();
        pthread_mutex_unlock(&_event_sink_lock);
    }

    void AddListener(T* sink_)
    {
        if (sink_)
        {
            pthread_mutex_lock(&_event_sink_lock);
            _event_sink_pool.insert(sink_);
            pthread_mutex_unlock(&_event_sink_lock);
        }
    }

    void RemoveListener(T* sink_)
    {
        if (sink_)
        {
            pthread_mutex_lock(&_event_sink_lock);
            _event_sink_pool.erase(sink_);
            pthread_mutex_unlock(&_event_sink_lock);
        }
    }

public:
    pthread_mutex_t _event_sink_lock;
    std::set<T*> _event_sink_pool;
};

#define  Call_SINK_Pool(T_Name, EventSinkPool_Name, FUNC) \
{ \
    pthread_mutex_lock(&(EventSinkPool_Name._event_sink_lock)); \
    std::set<T_Name*> tmp_set; \
    tmp_set.insert(EventSinkPool_Name._event_sink_pool.begin(), EventSinkPool_Name._event_sink_pool.end()); \
    std::set<T_Name*>::iterator iter_ = tmp_set.begin(); \
    for (; tmp_set.end() != iter_; iter_++) \
    { \
        if (*iter_) \
            (*iter_)->FUNC; \
    } \
    pthread_mutex_unlock(&(EventSinkPool_Name._event_sink_lock)); \
}

class ISDKInMeetingServiceMgrEvent
	:public ZOOM_SDK_NAMESPACE::IMeetingServiceEvent,
	public ZOOM_SDK_NAMESPACE::IMeetingAudioCtrlEvent,
	public ZOOM_SDK_NAMESPACE::IMeetingParticipantsCtrlEvent,
	public ZOOM_SDK_NAMESPACE::IMeetingRecordingCtrlEvent,
	public ZOOM_SDK_NAMESPACE::IMeetingShareCtrlEvent,
	public ZOOM_SDK_NAMESPACE::IMeetingVideoCtrlEvent
{
public:
	virtual ~ISDKInMeetingServiceMgrEvent() {}
};


class SDKInterfaceWrap
	: public ZOOM_SDK_NAMESPACE::IAuthServiceEvent
	,public ZOOM_SDK_NAMESPACE::IMeetingServiceEvent
	,public ZOOM_SDK_NAMESPACE::IMeetingAudioCtrlEvent
	,public ZOOM_SDK_NAMESPACE::IMeetingVideoCtrlEvent
	,public ZOOM_SDK_NAMESPACE::IMeetingParticipantsCtrlEvent
{
public:
	static SDKInterfaceWrap& GetInst();
	//bool IsSelectCustomizedUIMode() {return _customiezed_ui_mode;};
	ZOOM_SDK_NAMESPACE::SDKError  Init(ZOOM_SDK_NAMESPACE::InitParam& param_);
	ZOOM_SDK_NAMESPACE::SDKError  Cleanup();
	ZOOM_SDK_NAMESPACE::INetworkConnectionHelper* GetNetworkConnectionHelper();

	ZOOM_SDK_NAMESPACE::IAuthService* GetAuthService();
	void ListenAuthServiceEvent(ZOOM_SDK_NAMESPACE::IAuthServiceEvent* event_);// set event
	void UnListenAuthServiceEvent(ZOOM_SDK_NAMESPACE::IAuthServiceEvent* event_);//remove event
	ZOOM_SDK_NAMESPACE::IMeetingService* GetMeetingService();
	void ListenMeetingServiceEvent(ZOOM_SDK_NAMESPACE::IMeetingServiceEvent* event_);
	void UnListenMeetingServiceEvent(ZOOM_SDK_NAMESPACE::IMeetingServiceEvent* event_);

	ZOOM_SDK_NAMESPACE::ISettingService* GetSettingService();
	const ZOOM_SDK_NAMESPACE::IZoomLastError* GetLastError();

	// void ListenChatServiceMgrEvent(ZOOM_SDK_NAMESPACE::IMeetingChatCtrlEvent* event_);
	// void UnListenChatServiceMgrEvent(ZOOM_SDK_NAMESPACE::IMeetingChatCtrlEvent* event_);

	void ListenInMeetingServiceMgrEvent(ISDKInMeetingServiceMgrEvent* event_);
	void UnListenInMeetingServiceMgrEvent(ISDKInMeetingServiceMgrEvent* event_);
	// void ListenCloseCaptionEvent(ZOOM_SDK_NAMESPACE::IClosedCaptionControllerEvent* event_);
	// void UnListenCloseCaptionEvent(ZOOM_SDK_NAMESPACE::IClosedCaptionControllerEvent* event_);

	void ListenMeetingVideoCtrEvent(ZOOM_SDK_NAMESPACE::IMeetingVideoCtrlEvent* event_);
	void UnListenMeetingVideoCtrEvent(ZOOM_SDK_NAMESPACE::IMeetingVideoCtrlEvent* event_);

	void ListenMeetingParticipantsCtrEvent(ZOOM_SDK_NAMESPACE::IMeetingParticipantsCtrlEvent* event_);
	void UnListenMeetingParticipantsCtrEvent(ZOOM_SDK_NAMESPACE::IMeetingParticipantsCtrlEvent* event_);

	//ZOOM_SDK_NAMESPACE::IMeetingChatController* GetMeetingChatController();
	ZOOM_SDK_NAMESPACE::IMeetingParticipantsController* GetMeetingParticipantsController();	
	//ZOOM_SDK_NAMESPACE::IClosedCaptionController* GetCloseCaptionController();
	//ZOOM_SDK_NAMESPACE::ICustomizedAnnotationController* SDKInterfaceWrap::GetCustomizedAnnotationController(ZOOM_SDK_NAMESPACE::ICustomizedShareRender* pShareRender = NULL);
	//ZOOM_SDK_NAMESPACE::IMeetingRemoteController* GetMeetingRemoteController();
	ZOOM_SDK_NAMESPACE::IMeetingVideoController* GetMeetingVideoController();
	
	
	//auth service
	virtual void onAuthenticationReturn(ZOOM_SDK_NAMESPACE::AuthResult ret)
	{
		Call_SINK_Pool(ZOOM_SDK_NAMESPACE::IAuthServiceEvent, _auth_service_event_pool, onAuthenticationReturn(ret));
		if (ret == AUTHRET_SUCCESS)
		{
            std::cerr << "onAuthenticationReturn success" << std::endl;
			auth = true;
			
		}
		else
		{
        	std::cerr << "onAuthenticationReturn false" << std::endl;
		}
		
	}

	virtual void onLoginReturnWithReason(ZOOM_SDK_NAMESPACE::LOGINSTATUS ret, ZOOM_SDK_NAMESPACE::IAccountInfo* pAccountInfo, ZOOM_SDK_NAMESPACE::LoginFailReason reason) 
	{
		Call_SINK_Pool(ZOOM_SDK_NAMESPACE::IAuthServiceEvent, _auth_service_event_pool, onLoginReturnWithReason(ret, pAccountInfo, reason))
		if(ret ==  ZOOM_SDK_NAMESPACE::LOGINSTATUS::LOGIN_SUCCESS)
		{
			login = true;
		}
	}

	virtual void onLogout()
	{
		//Call_SINK_Pool(ZOOM_SDK_NAMESPACE::IAuthServiceEvent, _auth_service_event_pool, onLogout())
	}
	virtual void onZoomIdentityExpired()
	{
		//Call_SINK_Pool(ZOOM_SDK_NAMESPACE::IAuthServiceEvent, _auth_service_event_pool, onZoomIdentityExpired())
	}
	virtual void onZoomAuthIdentityExpired()
	{
		//Call_SINK_Pool(ZOOM_SDK_NAMESPACE::IAuthServiceEvent, _auth_service_event_pool, onZoomAuthIdentityExpired())
	}
	
	// virtual void onNotificationServiceStatus(ZOOM_SDK_NAMESPACE::SDKNotificationServiceStatus status)
	// {
	// 	//Call_SINK_Pool(ZOOM_SDK_NAMESPACE::IAuthServiceEvent, _auth_service_event_pool, onNotificationServiceStatus(status))
	// }
	
	//meeting service
	virtual void onMeetingStatusChanged(ZOOM_SDK_NAMESPACE::MeetingStatus status, int iResult = 0)
	{
		//Call_SINK_Pool(ISDKInMeetingServiceMgrEvent, _sdk_inmeeting_service_mgr_event_pool, onMeetingStatusChanged(status, iResult))
		//Call_SINK_Pool(ZOOM_SDK_NAMESPACE::IMeetingServiceEvent, _meeting_service_event_pool, onMeetingStatusChanged(status, iResult))
		ZOOM_SDK_NAMESPACE::MeetingStatus meetingstatus = status;
		std::cerr << "meeting_sdk_util::onMeetingStatusChanged:" << status << std::endl;
		if (ZOOM_SDK_NAMESPACE::MeetingStatus::MEETING_STATUS_INMEETING == status)
		{
			inMeeting = true;
		}
	}
	virtual void onMeetingStatisticsWarningNotification(ZOOM_SDK_NAMESPACE::StatisticsWarningType type)
	{
		//Call_SINK_Pool(ZOOM_SDK_NAMESPACE::IMeetingServiceEvent, _meeting_service_event_pool, onMeetingStatisticsWarningNotification(type))
		//Call_SINK_Pool(ISDKInMeetingServiceMgrEvent, _sdk_inmeeting_service_mgr_event_pool, onMeetingStatisticsWarningNotification(type))
	}

	virtual void onMeetingParameterNotification(const ZOOM_SDK_NAMESPACE::MeetingParameter* meeting_param)
	{
		//Call_SINK_Pool(ZOOM_SDK_NAMESPACE::IMeetingServiceEvent, _meeting_service_event_pool, onMeetingParameterNotification(meeting_param))
		//Call_SINK_Pool(ISDKInMeetingServiceMgrEvent, _sdk_inmeeting_service_mgr_event_pool, onMeetingParameterNotification(meeting_param))
	}

	virtual void onSuspendParticipantsActivities()
	{
		//Call_SINK_Pool(ZOOM_SDK_NAMESPACE::IMeetingServiceEvent, _meeting_service_event_pool, onSuspendParticipantsActivities())
		//Call_SINK_Pool(ISDKInMeetingServiceMgrEvent, _sdk_inmeeting_service_mgr_event_pool, onSuspendParticipantsActivities())
	}

	//IMeetingAudioCtrlEvent
	virtual void onUserAudioStatusChange(ZOOM_SDK_NAMESPACE::IList<ZOOM_SDK_NAMESPACE::IUserAudioStatus* >* lstAudioStatusChange, const zchar_t* strAudioStatusList = NULL)
	{
		//Call_SINK_Pool(ISDKInMeetingServiceMgrEvent, _sdk_inmeeting_service_mgr_event_pool, onUserAudioStatusChange(lstAudioStatusChange, strAudioStatusList))	
	}
	virtual void onUserActiveAudioChange(ZOOM_SDK_NAMESPACE::IList<unsigned int >* plstActiveAudio)
	{
		//Call_SINK_Pool(ISDKInMeetingServiceMgrEvent, _sdk_inmeeting_service_mgr_event_pool, onUserActiveAudioChange(plstActiveAudio))
	}
	virtual void onHostRequestStartAudio(ZOOM_SDK_NAMESPACE::IRequestStartAudioHandler* handler_)
	{
		//Call_SINK_Pool(ISDKInMeetingServiceMgrEvent, _sdk_inmeeting_service_mgr_event_pool, onHostRequestStartAudio(handler_))
	}

	//IMeetingChatCtrlEvent,
	// virtual void onChatMsgNotifcation(ZOOM_SDK_NAMESPACE::IChatMsgInfo* chatMsg, const zchar_t* content = NULL)
	// {
	// 	//Call_SINK_Pool(ZOOM_SDK_NAMESPACE::IMeetingChatCtrlEvent, _chat_servic_event_pool, onChatMsgNotifcation(chatMsg, content))
	// }
	// virtual void onChatStautsChangedNotification(ZOOM_SDK_NAMESPACE::ChatStatus* status_)
	// {
	// 	//Call_SINK_Pool(ZOOM_SDK_NAMESPACE::IMeetingChatCtrlEvent, _chat_servic_event_pool, onChatStautsChangedNotification(status_))
	// }
	// virtual void onChatMsgDeleteNotification(const zchar_t* msgID, ZOOM_SDK_NAMESPACE::SDKChatMessageDeleteType deleteBy)
	// {
	// 	//Call_SINK_Pool(ZOOM_SDK_NAMESPACE::IMeetingChatCtrlEvent, _chat_servic_event_pool, onChatMsgDeleteNotification(msgID, deleteBy))
	// }
	virtual void onShareMeetingChatStatusChanged(bool isStart) 
	{
		//Call_SINK_Pool(ZOOM_SDK_NAMESPACE::IMeetingChatCtrlEvent, _chat_servic_event_pool, onShareMeetingChatStatusChanged(isStart))
	}

	//IMeetingParticipantsCtrlEvent
	virtual void onUserJoin(ZOOM_SDK_NAMESPACE::IList<unsigned int >* lstUserID, const zchar_t* strUserList = NULL)
	{
		//Call_SINK_Pool(ISDKInMeetingServiceMgrEvent, _sdk_inmeeting_service_mgr_event_pool, onUserJoin(lstUserID, strUserList))
		//Call_SINK_Pool(ZOOM_SDK_NAMESPACE::IMeetingParticipantsCtrlEvent, _meeting_participants_ctr_event_pool, onUserJoin(lstUserID, strUserList))
		std::cerr << "onMeetingStatusChanged" << std::endl;
	}
	virtual void onUserLeft(ZOOM_SDK_NAMESPACE::IList<unsigned int >* lstUserID, const zchar_t* strUserList = NULL)
	{
		//Call_SINK_Pool(ISDKInMeetingServiceMgrEvent, _sdk_inmeeting_service_mgr_event_pool, onUserLeft(lstUserID, strUserList))
		//Call_SINK_Pool(ZOOM_SDK_NAMESPACE::IMeetingParticipantsCtrlEvent, _meeting_participants_ctr_event_pool, onUserLeft(lstUserID, strUserList))
	}
	virtual void onHostChangeNotification(unsigned int userId)
	{
		//Call_SINK_Pool(ISDKInMeetingServiceMgrEvent, _sdk_inmeeting_service_mgr_event_pool, onHostChangeNotification(userId))
		//Call_SINK_Pool(ZOOM_SDK_NAMESPACE::IMeetingParticipantsCtrlEvent, _meeting_participants_ctr_event_pool, onHostChangeNotification(userId))
	}
	virtual void onLowOrRaiseHandStatusChanged(bool bLow, unsigned int userid)
	{
		//Call_SINK_Pool(ISDKInMeetingServiceMgrEvent, _sdk_inmeeting_service_mgr_event_pool, onLowOrRaiseHandStatusChanged(bLow, userid))
		//Call_SINK_Pool(ZOOM_SDK_NAMESPACE::IMeetingParticipantsCtrlEvent, _meeting_participants_ctr_event_pool, onLowOrRaiseHandStatusChanged(bLow, userid))
	}
	virtual void onUserNamesChanged(ZOOM_SDK_NAMESPACE::IList<unsigned int>* pList)
	{
		//Call_SINK_Pool(ISDKInMeetingServiceMgrEvent, _sdk_inmeeting_service_mgr_event_pool, onUserNamesChanged(pList))
	}
	virtual void onCoHostChangeNotification(unsigned int userId, bool isCoHost)
	{
		//Call_SINK_Pool(ISDKInMeetingServiceMgrEvent, _sdk_inmeeting_service_mgr_event_pool, onCoHostChangeNotification(userId, isCoHost))
		//Call_SINK_Pool(ZOOM_SDK_NAMESPACE::IMeetingParticipantsCtrlEvent, _meeting_participants_ctr_event_pool, onCoHostChangeNotification(userId, isCoHost))
	}
	virtual void onInvalidReclaimHostkey() {}
	virtual void onAllHandsLowered() {}
	virtual void onLocalRecordingStatusChanged(unsigned int user_id, ZOOM_SDK_NAMESPACE::RecordingStatus status) {}
	
	virtual void onAllowParticipantsRenameNotification(bool bAllow)
	{
		//Call_SINK_Pool(ISDKInMeetingServiceMgrEvent, _sdk_inmeeting_service_mgr_event_pool, onAllowParticipantsRenameNotification(bAllow))
		//Call_SINK_Pool(ZOOM_SDK_NAMESPACE::IMeetingParticipantsCtrlEvent, _meeting_participants_ctr_event_pool, onAllowParticipantsRenameNotification(bAllow))
	}

	virtual void onAllowParticipantsUnmuteSelfNotification(bool bAllow)
	{
		//Call_SINK_Pool(ISDKInMeetingServiceMgrEvent, _sdk_inmeeting_service_mgr_event_pool, onAllowParticipantsUnmuteSelfNotification(bAllow))
		//Call_SINK_Pool(ZOOM_SDK_NAMESPACE::IMeetingParticipantsCtrlEvent, _meeting_participants_ctr_event_pool, onAllowParticipantsUnmuteSelfNotification(bAllow))
	}

	virtual void onAllowParticipantsStartVideoNotification(bool bAllow)
	{
		//Call_SINK_Pool(ISDKInMeetingServiceMgrEvent, _sdk_inmeeting_service_mgr_event_pool, onAllowParticipantsStartVideoNotification(bAllow))
		//Call_SINK_Pool(ZOOM_SDK_NAMESPACE::IMeetingParticipantsCtrlEvent, _meeting_participants_ctr_event_pool, onAllowParticipantsStartVideoNotification(bAllow))
	}

	virtual void onAllowParticipantsShareWhiteBoardNotification(bool bAllow)
	{
		//Call_SINK_Pool(ISDKInMeetingServiceMgrEvent, _sdk_inmeeting_service_mgr_event_pool, onAllowParticipantsShareWhiteBoardNotification(bAllow))
		//Call_SINK_Pool(ZOOM_SDK_NAMESPACE::IMeetingParticipantsCtrlEvent, _meeting_participants_ctr_event_pool, onAllowParticipantsShareWhiteBoardNotification(bAllow))
	}
	
	virtual void onRequestLocalRecordingPrivilegeChanged(LocalRecordingRequestPrivilegeStatus status) {}
	
	virtual void onInMeetingUserAvatarPathUpdated(unsigned int userID) {}
	//IMeetingRecordingCtrlEvent
	virtual void onRecording2MP4Done(bool bsuccess, int iResult, const zchar_t* szPath)
	{
		//Call_SINK_Pool(ISDKInMeetingServiceMgrEvent, _sdk_inmeeting_service_mgr_event_pool, onRecording2MP4Done(bsuccess, iResult, szPath))
	}
	virtual void onRecording2MP4Processing(int iPercentage)
	{
		//Call_SINK_Pool(ISDKInMeetingServiceMgrEvent, _sdk_inmeeting_service_mgr_event_pool, onRecording2MP4Processing(iPercentage))
	}
	virtual void onRecordingStatus(ZOOM_SDK_NAMESPACE::RecordingStatus status)
	{
		//Call_SINK_Pool(ISDKInMeetingServiceMgrEvent, _sdk_inmeeting_service_mgr_event_pool, onRecordingStatus(status))
	}
	virtual void onCloudRecordingStatus(ZOOM_SDK_NAMESPACE::RecordingStatus status)
	{
		//Call_SINK_Pool(ISDKInMeetingServiceMgrEvent, _sdk_inmeeting_service_mgr_event_pool, onCloudRecordingStatus(status))
	}
	virtual void onRecordPriviligeChanged(bool bCanRec)
	{
		//Call_SINK_Pool(ISDKInMeetingServiceMgrEvent, _sdk_inmeeting_service_mgr_event_pool, onRecordPriviligeChanged(bCanRec))
	}
	// virtual void onCustomizedLocalRecordingSourceNotification(ZOOM_SDK_NAMESPACE::ICustomizedLocalRecordingLayoutHelper* layout_helper)
	// {
	// 	//Call_SINK_Pool(ISDKInMeetingServiceMgrEvent, _sdk_inmeeting_service_mgr_event_pool, onCustomizedLocalRecordingSourceNotification(layout_helper))
	// }

	//IMeetingRemoteCtrlEvent
	// virtual void onRemoteControlStatus(ZOOM_SDK_NAMESPACE::RemoteControlStatus status, unsigned int userId)
	// {
	// 	//Call_SINK_Pool(ISDKInMeetingServiceMgrEvent, _sdk_inmeeting_service_mgr_event_pool, onRemoteControlStatus(status, userId))
	// }

	//IMeetingShareCtrlEvent
	virtual void onSharingStatus(ZOOM_SDK_NAMESPACE::SharingStatus status, unsigned int userId)
	{
		//Call_SINK_Pool(ISDKInMeetingServiceMgrEvent, _sdk_inmeeting_service_mgr_event_pool, onSharingStatus(status, userId))
	}
	virtual void onLockShareStatus(bool bLocked)
	{
		//Call_SINK_Pool(ISDKInMeetingServiceMgrEvent, _sdk_inmeeting_service_mgr_event_pool, onLockShareStatus(bLocked))
	}
	virtual void onShareContentNotification(ZOOM_SDK_NAMESPACE::ShareInfo& shareInfo)
	{
		//Call_SINK_Pool(ISDKInMeetingServiceMgrEvent, _sdk_inmeeting_service_mgr_event_pool, onShareContentNotification(shareInfo))
	}

	//IMeetingVideoCtrlEvent
	virtual void onUserVideoStatusChange(unsigned int userId, ZOOM_SDK_NAMESPACE::VideoStatus status)
	{
		//Call_SINK_Pool(ISDKInMeetingServiceMgrEvent, _sdk_inmeeting_service_mgr_event_pool, onUserVideoStatusChange(userId, status))
		//Call_SINK_Pool(IMeetingVideoCtrlEvent, _meeting_video_ctrl_event_pool, onUserVideoStatusChange(userId, status))
	}
	virtual void onSpotlightedUserListChangeNotification(ZOOM_SDK_NAMESPACE::IList<unsigned int >* lstSpotlightedUserID){}
	virtual void onHostRequestStartVideo(ZOOM_SDK_NAMESPACE::IRequestStartVideoHandler* handler_)
	{
		//Call_SINK_Pool(ISDKInMeetingServiceMgrEvent, _sdk_inmeeting_service_mgr_event_pool, onHostRequestStartVideo(handler_))
		//Call_SINK_Pool(IMeetingVideoCtrlEvent, _meeting_video_ctrl_event_pool, onHostRequestStartVideo(handler_))
	}
	virtual void onActiveSpeakerVideoUserChanged(unsigned int userid)
	{
		//Call_SINK_Pool(ISDKInMeetingServiceMgrEvent, _sdk_inmeeting_service_mgr_event_pool, onActiveSpeakerVideoUserChanged(userid))
		//Call_SINK_Pool(IMeetingVideoCtrlEvent, _meeting_video_ctrl_event_pool, onActiveSpeakerVideoUserChanged(userid))
	}
	virtual void onActiveVideoUserChanged(unsigned int userid)
	{
		//Call_SINK_Pool(ISDKInMeetingServiceMgrEvent, _sdk_inmeeting_service_mgr_event_pool, onActiveVideoUserChanged(userid))
		//Call_SINK_Pool(IMeetingVideoCtrlEvent, _meeting_video_ctrl_event_pool, onActiveVideoUserChanged(userid))
	}
	virtual void onHostVideoOrderUpdated(ZOOM_SDK_NAMESPACE::IList<unsigned int >* orderList) {}
	virtual void onFollowHostVideoOrderChanged(bool bFollow) {}
	virtual void onLocalVideoOrderUpdated(ZOOM_SDK_NAMESPACE::IList<unsigned int >* localOrderList) {}
	virtual void onUserVideoQualityChanged(ZOOM_SDK_NAMESPACE::VideoConnectionQuality quality, unsigned int userid) {}

	//IMeetingUIControllerEvent
	virtual void onInviteBtnClicked(bool& bHandled)
	{
		//Call_SINK_Pool(ISDKInMeetingServiceMgrEvent, _sdk_inmeeting_service_mgr_event_pool, onInviteBtnClicked(bHandled))
	}
	virtual void onStartShareBtnClicked()
	{
		//Call_SINK_Pool(ISDKInMeetingServiceMgrEvent, _sdk_inmeeting_service_mgr_event_pool, onStartShareBtnClicked())
	}
	virtual void onEndMeetingBtnClicked()
	{
		//Call_SINK_Pool(ISDKInMeetingServiceMgrEvent, _sdk_inmeeting_service_mgr_event_pool, onEndMeetingBtnClicked())
	}
	virtual void onParticipantListBtnClicked()
	{
		//Call_SINK_Pool(ISDKInMeetingServiceMgrEvent, _sdk_inmeeting_service_mgr_event_pool, onParticipantListBtnClicked())
	}
	virtual void onCustomLiveStreamMenuClicked()
	{
		//Call_SINK_Pool(ISDKInMeetingServiceMgrEvent, _sdk_inmeeting_service_mgr_event_pool, onCustomLiveStreamMenuClicked())
	}
	virtual void onZoomInviteDialogFailed()
	{
		//Call_SINK_Pool(ISDKInMeetingServiceMgrEvent, _sdk_inmeeting_service_mgr_event_pool, onZoomInviteDialogFailed())
	}
	virtual void onCCBTNClicked()
	{
		//Call_SINK_Pool(ISDKInMeetingServiceMgrEvent, _sdk_inmeeting_service_mgr_event_pool, onCCBTNClicked())
	}

	// virtual void onAudioBtnClicked(ZOOM_SDK_NAMESPACE::AudioBtnClickedCallbackInfo info)
	// {
	// 	//Call_SINK_Pool(ISDKInMeetingServiceMgrEvent, _sdk_inmeeting_service_mgr_event_pool, onAudioBtnClicked(info))
	// }

	virtual void onAudioMenuBtnClicked()
	{
		//Call_SINK_Pool(ISDKInMeetingServiceMgrEvent, _sdk_inmeeting_service_mgr_event_pool, onAudioMenuBtnClicked())
	}

	//ITestAudioDeviceHelperEvent
	// virtual void OnMicSpkVolumnChanged(unsigned int MicVolume, unsigned int SpkVolume)
	// {
	// 	//Call_SINK_Pool(ISDKInMeetingServiceMgrEvent, _sdk_inmeeting_service_mgr_event_pool, OnMicSpkVolumnChanged(MicVolume, SpkVolume))
	// }
	// virtual void OnNoAudioDeviceIsUseful(bool bMicOrSpk)
	// {
	// 	//Call_SINK_Pool(ISDKInMeetingServiceMgrEvent, _sdk_inmeeting_service_mgr_event_pool, OnNoAudioDeviceIsUseful(bMicOrSpk))
	// }
	// virtual void OnTestMicStatusChanged(ZOOM_SDK_NAMESPACE::SDK_TESTMIC_STATUS status,bool& bHandled)
	// {
	// 	//Call_SINK_Pool(ISDKInMeetingServiceMgrEvent, _sdk_inmeeting_service_mgr_event_pool, OnTestMicStatusChanged(status, bHandled))
	// }
	// virtual void OnSelectedAudioDeviceIsChanged()
	// {
	// 	//Call_SINK_Pool(ISDKInMeetingServiceMgrEvent, _sdk_inmeeting_service_mgr_event_pool, OnSelectedAudioDeviceIsChanged())
	// }
	
	// //ITestVideoDeviceHelperEvent
	// virtual void OnNoVideoDeviceIsUseful()
	// {
	// 	//Call_SINK_Pool(ISDKInMeetingServiceMgrEvent, _sdk_inmeeting_service_mgr_event_pool, OnNoVideoDeviceIsUseful())
	// }
	// virtual void OnSelectedVideoDeviceIsChanged()
	// {
	// 	//Call_SINK_Pool(ISDKInMeetingServiceMgrEvent, _sdk_inmeeting_service_mgr_event_pool, OnSelectedVideoDeviceIsChanged())
	// }
	// virtual void OnNoWindowToShowPreview()
	// {
	// 	//Call_SINK_Pool(ISDKInMeetingServiceMgrEvent, _sdk_inmeeting_service_mgr_event_pool, OnNoWindowToShowPreview())
	// }

	//ICloseCaptionControllerEvent
	virtual void onAssignedToSendCC(bool bAssigned)
	{
		//Call_SINK_Pool(ZOOM_SDK_NAMESPACE::IClosedCaptionControllerEvent, _close_caption_event_pool, onAssignedToSendCC(bAssigned))
	}
	virtual void onClosedCaptionMsgReceived(const zchar_t* ccMsg, unsigned int sender_id, time_t time)
	{
		//Call_SINK_Pool(ZOOM_SDK_NAMESPACE::IClosedCaptionControllerEvent, _close_caption_event_pool, onClosedCaptionMsgReceived(ccMsg, sender_id,time))
	}
	// virtual void onLiveTranscriptionMsgInfoReceived(ZOOM_SDK_NAMESPACE::ILiveTranscriptionMessageInfo* messageInfo)
	// {
	// 	//Call_SINK_Pool(ZOOM_SDK_NAMESPACE::IClosedCaptionControllerEvent, _close_caption_event_pool, onLiveTranscriptionMsgInfoReceived(messageInfo))
	// }
	// virtual void onOriginalLanguageMsgReceived(ZOOM_SDK_NAMESPACE::ILiveTranscriptionMessageInfo* messageInfo)
	// {
	// 	//Call_SINK_Pool(ZOOM_SDK_NAMESPACE::IClosedCaptionControllerEvent, _close_caption_event_pool, onOriginalLanguageMsgReceived(messageInfo))
	// }
	// virtual void onLiveTranscriptionStatus(ZOOM_SDK_NAMESPACE::SDKLiveTranscriptionStatus status) {}
	// virtual void onLiveTranscriptionMsgReceived(const zchar_t* ltMsg, unsigned int speaker_id, ZOOM_SDK_NAMESPACE::SDKLiveTranscriptionOperationType type) {}
	// virtual void onLiveTranscriptionMsgError(ZOOM_SDK_NAMESPACE::ILiveTranscriptionLanguage* spokenLanguage, ZOOM_SDK_NAMESPACE::ILiveTranscriptionLanguage* transcriptLanguage) {}
	virtual void onRequestForLiveTranscriptReceived(unsigned int requester_id, bool bAnonymous) {}
	virtual void onRequestLiveTranscriptionStatusChange(bool bEnabled){}
	virtual void onCaptionStatusChanged(bool bEnabled) {}
    bool auth = false;
	bool inMeeting = false;
	bool login = false;
private: 
	SDKInterfaceWrap();
	virtual ~SDKInterfaceWrap();
	EventSinkPool<ZOOM_SDK_NAMESPACE::IAuthServiceEvent > _auth_service_event_pool;
	EventSinkPool<ZOOM_SDK_NAMESPACE::IMeetingServiceEvent > _meeting_service_event_pool;
	EventSinkPool<ISDKInMeetingServiceMgrEvent > _sdk_inmeeting_service_mgr_event_pool;
	// EventSinkPool<ZOOM_SDK_NAMESPACE::IMeetingChatCtrlEvent> _chat_servic_event_pool;
	// EventSinkPool<ZOOM_SDK_NAMESPACE::IClosedCaptionControllerEvent> _close_caption_event_pool;
	EventSinkPool<ZOOM_SDK_NAMESPACE::IMeetingVideoCtrlEvent> _meeting_video_ctrl_event_pool;
	EventSinkPool<ZOOM_SDK_NAMESPACE::IMeetingParticipantsCtrlEvent> _meeting_participants_ctr_event_pool;

	ZOOM_SDK_NAMESPACE::IAuthService* _auth_service;
	ZOOM_SDK_NAMESPACE::IMeetingService* _meeting_service;
	ZOOM_SDK_NAMESPACE::INetworkConnectionHelper* _network_connection_helper;
	ZOOM_SDK_NAMESPACE::ISettingService* _setting_service;
	
	const ZOOM_SDK_NAMESPACE::IZoomLastError* _last_error;
	
	bool _inited;
	// ZOOM_SDK_NAMESPACE::IMeetingChatController* _meeting_chat_ctrl;
	ZOOM_SDK_NAMESPACE::IMeetingParticipantsController* _meeting_participants_ctrl;
	// ZOOM_SDK_NAMESPACE::IClosedCaptionController* _close_caption_ctrl;
	//ZOOM_SDK_NAMESPACE::ICustomizedAnnotationController* _customer_annotation_ctrl;
	bool _customiezed_ui_mode;
	ZOOM_SDK_NAMESPACE::IMeetingVideoController* _meeting_video_ctrl;
};










