#pragma once
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

#include "RegressionTestRawdataRender.h"
#include "rawdata/rawdata_audio_helper_interface.h"
#include "rawdata/rawdata_video_source_helper_interface.h"
#include "external_video_source.h"
#include "meeting_sdk_util.h"

class CRenderWndUI;
class CRegressionTestRawdataAudio :
	                                     public ZOOM_SDK_NAMESPACE::IZoomSDKAudioRawDataDelegate,
										 public ZOOM_SDK_NAMESPACE::IZoomSDKVideoSource,
	                                     public ZOOM_SDK_NAMESPACE::IZoomSDKVirtualAudioMicEvent
{
public:
	CRegressionTestRawdataAudio();
	~CRegressionTestRawdataAudio();

	void UpdateParticipantsList(ZOOM_SDK_NAMESPACE::IList<unsigned int >* pParticipantsList);

public:
	virtual void DoInit();

public: //IZoomSDKAudioRawDataDelegate
	virtual void onMixedAudioRawDataReceived(AudioRawData* data_);
	virtual void onOneWayAudioRawDataReceived(AudioRawData* data_, uint32_t node_id);

public: // IZoomSDKVirtualAudioMicEvent
	virtual void onMicInitialize(ZOOM_SDK_NAMESPACE::IZoomSDKAudioRawDataSender* pSender);
	virtual void onMicStartSend();
	virtual void onMicStopSend();
	virtual void onMicUninitialized();
public: //IZoomSDKVideoSource
	virtual	void onInitialize(ZOOM_SDK_NAMESPACE::IZoomSDKVideoSender* sender, ZOOM_SDK_NAMESPACE::IList<ZOOM_SDK_NAMESPACE::VideoSourceCapability >* support_cap_list, ZOOM_SDK_NAMESPACE::VideoSourceCapability& suggest_cap);
	virtual void onPropertyChange(ZOOM_SDK_NAMESPACE::IList<ZOOM_SDK_NAMESPACE::VideoSourceCapability >* support_cap_list, ZOOM_SDK_NAMESPACE::VideoSourceCapability suggest_cap);
	virtual void onStartSend();
	virtual void onStopSend();
	virtual void onUninitialized();
public:
	std::string DoEnableVirtualCamera();
	std::string DoDisableVirtualCamera();
    std::string DoChangeSendSize();
	std::string DoEnableCameraPreprocess();
	std::string DoDisableCameraPreprocess();
public:
    std::string DoSubscribeAudio();
	std::string DoUnSubscribeAudio();
    std::string DoStartSendAudioRawData();
	std::string DoStopSendAudioRawData();
    std::string DoStartSharePureAudioRawdata();
	std::string DoEnableShareAudioRawdata();
	std::string DoDisableShareAudioRawdata();
    std::string DoHasRawdataLicense();

protected:
    
	void Subscribe(CRegressionTestRawdataRender& renderer,CRenderWndUI* pWnd, ZOOM_SDK_NAMESPACE::ZoomSDKRawDataType type);
	void GetRenderInfo(const CRegressionTestRawdataRender& renderer);
	void DestroyRender(CRegressionTestRawdataRender& renderer);
	void CreateRender(CRegressionTestRawdataRender& renderer);
	unsigned int GetCurrentSelectedUserID();
	ZOOM_SDK_NAMESPACE::ZoomSDKResolution GetCurrentSelectedResolution();
	std::string ConvertResolutionToString(const ZOOM_SDK_NAMESPACE::ZoomSDKResolution& resolution);
	std::string ConvertRawDataTypeToString(const ZOOM_SDK_NAMESPACE::ZoomSDKRawDataType& type);
    
    FILE* m_pFileMixAudio = nullptr;
	FILE* m_pFileNodeID = nullptr;
	FILE* m_pFileMyself = nullptr;
};
static std::string ConvertSDKERRORToString(ZOOM_SDK_NAMESPACE::SDKError type){
	std::string sdk_err_string;

	switch (type)
	{
	case ZOOM_SDK_NAMESPACE::SDKERR_SUCCESS:
		sdk_err_string = "SDKERR_SUCCESS";
		break;
	case ZOOM_SDK_NAMESPACE::SDKERR_NO_IMPL:
		sdk_err_string = "SDKERR_NO_IMPL";
		break;
    case ZOOM_SDK_NAMESPACE::SDKERR_WRONG_USAGE:
		sdk_err_string = "SDKERR_WRONG_USAGE";
		break;
    case ZOOM_SDK_NAMESPACE::SDKERR_INVALID_PARAMETER:
		sdk_err_string = "SDKERR_INVALID_PARAMETER";
		break;
    case ZOOM_SDK_NAMESPACE::SDKERR_MODULE_LOAD_FAILED:
		sdk_err_string = "SDKERR_MODULE_LOAD_FAILED";
		break;
    case ZOOM_SDK_NAMESPACE::SDKERR_MEMORY_FAILED:
		sdk_err_string = "SDKERR_MEMORY_FAILED";
		break;
    case ZOOM_SDK_NAMESPACE::SDKERR_UNINITIALIZE:
		sdk_err_string = "SDKERR_UNINITIALIZE";
		break;
    case ZOOM_SDK_NAMESPACE::SDKERR_SERVICE_FAILED:
		sdk_err_string = "SDKERR_SERVICE_FAILED";
		break;
    case ZOOM_SDK_NAMESPACE::SDKERR_UNAUTHENTICATION:
		sdk_err_string = "SDKERR_UNAUTHENTICATION";
		break;
    case ZOOM_SDK_NAMESPACE::SDKERR_NORECORDINGINPROCESS:
		sdk_err_string = "SDKERR_NORECORDINGINPROCESS";
		break;
    case ZOOM_SDK_NAMESPACE::SDKERR_TRANSCODER_NOFOUND:
		sdk_err_string = "SDKERR_TRANSCODER_NOFOUND";
		break;
    case ZOOM_SDK_NAMESPACE::SDKERR_VIDEO_NOTREADY:
		sdk_err_string = "SDKERR_VIDEO_NOTREADY";
		break;
    case ZOOM_SDK_NAMESPACE::SDKERR_NO_PERMISSION:
		sdk_err_string = "SDKERR_NO_PERMISSION";
		break;
    case ZOOM_SDK_NAMESPACE::SDKERR_UNKNOWN:
		sdk_err_string = "SDKERR_UNKNOWN";
		break;
    case ZOOM_SDK_NAMESPACE::SDKERR_OTHER_SDK_INSTANCE_RUNNING:
		sdk_err_string = "SDKERR_OTHER_SDK_INSTANCE_RUNNING";
		break;
    case ZOOM_SDK_NAMESPACE::SDKERR_INTERNAL_ERROR:
		sdk_err_string = "SDKERR_INTERNAL_ERROR";
		break;
    case ZOOM_SDK_NAMESPACE::SDKERR_NO_AUDIODEVICE_ISFOUND:
		sdk_err_string = "SDKERR_NO_AUDIODEVICE_ISFOUND";
		break;
    case ZOOM_SDK_NAMESPACE::SDKERR_NO_VIDEODEVICE_ISFOUND:
		sdk_err_string = "SDKERR_NO_VIDEODEVICE_ISFOUND";
		break;
    case ZOOM_SDK_NAMESPACE::SDKERR_TOO_FREQUENT_CALL:
		sdk_err_string = "SDKERR_TOO_FREQUENT_CALL";
		break;
    case ZOOM_SDK_NAMESPACE::SDKERR_FAIL_ASSIGN_USER_PRIVILEGE:
		sdk_err_string = "SDKERR_FAIL_ASSIGN_USER_PRIVILEGE";
		break;
    case ZOOM_SDK_NAMESPACE::SDKERR_MEETING_DONT_SUPPORT_FEATURE:
		sdk_err_string = "SDKERR_MEETING_DONT_SUPPORT_FEATURE";
		break;
    case ZOOM_SDK_NAMESPACE::SDKERR_MEETING_NOT_SHARE_SENDER:
		sdk_err_string = "SDKERR_MEETING_NOT_SHARE_SENDER";
		break;
    case ZOOM_SDK_NAMESPACE::SDKERR_MEETING_YOU_HAVE_NO_SHARE:
		sdk_err_string = "SDKERR_MEETING_YOU_HAVE_NO_SHARE";
		break;
    case ZOOM_SDK_NAMESPACE::SDKERR_MEETING_VIEWTYPE_PARAMETER_IS_WRONG:
		sdk_err_string = "SDKERR_MEETING_VIEWTYPE_PARAMETER_IS_WRONG";
		break;
    case ZOOM_SDK_NAMESPACE::SDKERR_MEETING_ANNOTATION_IS_OFF:
		sdk_err_string = "SDKERR_MEETING_ANNOTATION_IS_OFF";
		break;
    case ZOOM_SDK_NAMESPACE::SDKERR_SETTING_OS_DONT_SUPPORT:
		sdk_err_string = "SDKERR_SETTING_OS_DONT_SUPPORT";
		break;
    case ZOOM_SDK_NAMESPACE::SDKERR_EMAIL_LOGIN_IS_DISABLED:
		sdk_err_string = "SDKERR_EMAIL_LOGIN_IS_DISABLED";
		break;
    case ZOOM_SDK_NAMESPACE::SDKERR_HARDWARE_NOT_MEET_FOR_VB:
		sdk_err_string = "SDKERR_HARDWARE_NOT_MEET_FOR_VB";
		break;
    case ZOOM_SDK_NAMESPACE::SDKERR_NEED_USER_CONFIRM_RECORD_DISCLAIMER:
		sdk_err_string = "SDKERR_NEED_USER_CONFIRM_RECORD_DISCLAIMER";
		break;
    case ZOOM_SDK_NAMESPACE::SDKERR_NO_SHARE_DATA:
		sdk_err_string = "SDKERR_NO_SHARE_DATA";
		break;
    case ZOOM_SDK_NAMESPACE::SDKERR_SHARE_CANNOT_SUBSCRIBE_MYSELF:
		sdk_err_string = "SDKERR_SHARE_CANNOT_SUBSCRIBE_MYSELF";
		break;
    case ZOOM_SDK_NAMESPACE::SDKERR_NOT_IN_MEETING:
		sdk_err_string = "SDKERR_NOT_IN_MEETING";
		break;
	default:
		break;
	}

	return sdk_err_string;
}