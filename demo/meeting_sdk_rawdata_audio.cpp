#include "meeting_sdk_rawdata_audio.h"
// #include "external_video_source.h"
#include "rawdata/zoom_rawdata_api.h"
#include <cstddef>

// void* g_pAudioRawDataSenderThreadHandle = NULL;
bool g_stop_send_flag = false;
// ZOOM_SDK_NAMESPACE::IZoomSDKAudioRawDataSender* g_pAudioRawdataSender = NULL;
CRegressionTestRawdataAudio::CRegressionTestRawdataAudio()
{

}

CRegressionTestRawdataAudio::~CRegressionTestRawdataAudio()
{

}

void CRegressionTestRawdataAudio::DoInit()
{
	
}

void CRegressionTestRawdataAudio::DestroyRender(CRegressionTestRawdataRender& renderer)
{
	ZOOM_SDK_NAMESPACE::SDKError err = renderer.DestroyRender();
	  std::cerr << "CRegressionTestRawdataAudio::DestroyRender :" << err << std::endl;
}

void CRegressionTestRawdataAudio::CreateRender(CRegressionTestRawdataRender& renderer)
{
	ZOOM_SDK_NAMESPACE::SDKError err = renderer.CreateRender();
	  std::cerr << "CRegressionTestRawdataAudio::CreateRender :" << err << std::endl;
}

std::string CRegressionTestRawdataAudio::DoSubscribeAudio()
{
	m_pFileMixAudio = fopen("mix_audio.pcm", "wb");
	m_pFileNodeID = fopen("oneway_audio.pcm", "wb");
	m_pFileMyself = fopen("myself.pcm", "wb");


	ZOOM_SDK_NAMESPACE::SDKError err(ZOOM_SDK_NAMESPACE::SDKERR_WRONG_USAGE);
	if (ZOOM_SDK_NAMESPACE::GetAudioRawdataHelper())
	{
		err = ZOOM_SDK_NAMESPACE::GetAudioRawdataHelper()->subscribe(this);
	}
    return ConvertSDKERRORToString(err);
}

std::string CRegressionTestRawdataAudio::DoUnSubscribeAudio()
{
	ZOOM_SDK_NAMESPACE::SDKError err(ZOOM_SDK_NAMESPACE::SDKERR_WRONG_USAGE);
	if (ZOOM_SDK_NAMESPACE::GetAudioRawdataHelper())
	{
		err = ZOOM_SDK_NAMESPACE::GetAudioRawdataHelper()->unSubscribe();
	}
    return ConvertSDKERRORToString(err);
}

std::string CRegressionTestRawdataAudio::DoStartSendAudioRawData()
{
	//ZOOM_SDK_NAMESPACE::SDKError err = CZoomSDKVirtualAudioMic::GetInstance()->EnableAudioMicSource(this);
	ZOOM_SDK_NAMESPACE::SDKError err = ZOOM_SDK_NAMESPACE::SDKERR_WRONG_USAGE;
    return ConvertSDKERRORToString(err);
	
}

std::string CRegressionTestRawdataAudio::DoStopSendAudioRawData()
{
	//ZOOM_SDK_NAMESPACE::SDKError err = CZoomSDKVirtualAudioMic::GetInstance()->DisableAudioMicSource();
	ZOOM_SDK_NAMESPACE::SDKError err = ZOOM_SDK_NAMESPACE::SDKERR_WRONG_USAGE;
	return ConvertSDKERRORToString(err);
}

void CRegressionTestRawdataAudio::onMicInitialize(ZOOM_SDK_NAMESPACE::IZoomSDKAudioRawDataSender* pSender)
{
	// g_pAudioRawdataSender = pSender;
}

std::string CRegressionTestRawdataAudio::DoEnableVirtualCamera()
{
	ZOOM_SDK_NAMESPACE::SDKError err = CExternalVideoSource::GetInstance()->EnableVideoSource(this);
	return ConvertSDKERRORToString(err);
}

std::string CRegressionTestRawdataAudio::DoDisableVirtualCamera()
{
	ZOOM_SDK_NAMESPACE::SDKError err = CExternalVideoSource::GetInstance()->DisableVideoSource();
	return ConvertSDKERRORToString(err);
}

std::string CRegressionTestRawdataAudio::DoChangeSendSize()
{
	CExternalVideoSource::GetInstance()->ChangeSendSize();
}

std::string CRegressionTestRawdataAudio::DoEnableCameraPreprocess()
{
	ZOOM_SDK_NAMESPACE::SDKError err = CExternalVideoSource::GetInstance()->EnablePreprocessor();
	return ConvertSDKERRORToString(err);
}

std::string CRegressionTestRawdataAudio::DoDisableCameraPreprocess()
{
	ZOOM_SDK_NAMESPACE::SDKError err = CExternalVideoSource::GetInstance()->DisablePreprocessor();
	return ConvertSDKERRORToString(err);
}

void CRegressionTestRawdataAudio::onInitialize(ZOOM_SDK_NAMESPACE::IZoomSDKVideoSender* sender, ZOOM_SDK_NAMESPACE::IList<ZOOM_SDK_NAMESPACE::VideoSourceCapability >* support_cap_list, ZOOM_SDK_NAMESPACE::VideoSourceCapability& suggest_cap)
{
	if (support_cap_list)
	{
		for (int i = 0; i < support_cap_list->GetCount(); i++)
		{
			ZOOM_SDK_NAMESPACE::VideoSourceCapability videoSourceCapability = support_cap_list->GetItem(i);
			char szTemp[256] = { 0 };
			snprintf(szTemp,sizeof(szTemp), "VideoSourceCapability: width = %d,  height = %d,  frame = %d", videoSourceCapability.width, videoSourceCapability.height, videoSourceCapability.frame);
		}
	}
}

void CRegressionTestRawdataAudio::onPropertyChange(ZOOM_SDK_NAMESPACE::IList<ZOOM_SDK_NAMESPACE::VideoSourceCapability >* support_cap_list, ZOOM_SDK_NAMESPACE::VideoSourceCapability suggest_cap)
{
	if (support_cap_list)
	{
		for (int i = 0; i < support_cap_list->GetCount(); i++)
		{
			ZOOM_SDK_NAMESPACE::VideoSourceCapability videoSourceCapability = support_cap_list->GetItem(i);
			char szTemp[256] = { 0 };
			snprintf(szTemp,sizeof(szTemp),"VideoSourceCapability: width = %d,  height = %d,  frame = %d", videoSourceCapability.width, videoSourceCapability.height, videoSourceCapability.frame);
		}
	}
}

void CRegressionTestRawdataAudio::onStartSend()
{
	//MessageBox(NULL, _T("onStartSend"), _T("call back"), MB_OK);
}

void CRegressionTestRawdataAudio::onStopSend()
{
	// m_pComboOnInitialize->RemoveAll();
	// m_pComboOnInitialize->SetVisible(false);
	// m_pComboOnPropertyChange->RemoveAll();
	// m_pComboOnPropertyChange->SetVisible(false);

	// MessageBox(NULL, _T("onStopSend"), _T("call back"), MB_OK);
}

void CRegressionTestRawdataAudio::onUninitialized()
{
	// m_pComboOnInitialize->RemoveAll();
	// m_pComboOnInitialize->SetVisible(false);
	// m_pComboOnPropertyChange->RemoveAll();
	// m_pComboOnPropertyChange->SetVisible(false);

	// MessageBox(NULL, _T("onUninitialized"), _T("call back"), MB_OK);
}

void CRegressionTestRawdataAudio::onMicStartSend()
{
	// if (!g_pAudioRawdataSender)
	// 	return;

	// if (!g_pAudioRawDataSenderThreadHandle)
	// {
	// 	g_stop_send_flag = false;
	// 	g_pAudioRawDataSenderThreadHandle = CreateThread(
	// 		NULL,                   // default security attributes
	// 		0,                      // use default stack size  
	// 		MyThreadFun_audio_send_raw_data,       // thread function name
	// 		NULL,          // argument to thread function 
	// 		0,                      // use default creation flags 
	// 		NULL);   // returns the thread identifier 
	// }

	// if (g_pAudioRawDataSenderThreadHandle)
	// 	ResumeThread(g_pAudioRawDataSenderThreadHandle);
}

void CRegressionTestRawdataAudio::onMicStopSend()
{
	// if (g_pAudioRawDataSenderThreadHandle)
	// 	SuspendThread(g_pAudioRawDataSenderThreadHandle);
}

void CRegressionTestRawdataAudio::onMicUninitialized()
{
	// if (g_pAudioRawDataSenderThreadHandle)
	// {
	// 	ResumeThread(g_pAudioRawDataSenderThreadHandle);
	// 	g_stop_send_flag = true;
	// 	WaitForSingleObject(g_pAudioRawDataSenderThreadHandle, INFINITE);
	// 	CloseHandle(g_pAudioRawDataSenderThreadHandle);
	// 	g_pAudioRawDataSenderThreadHandle = NULL;
	// 	g_pAudioRawdataSender = NULL;
	// }
}

void CRegressionTestRawdataAudio::onMixedAudioRawDataReceived(AudioRawData* data_)
{
	if (!data_) return;

	unsigned int nChannel = data_->GetChannelNum();
	unsigned int nSampleRate = data_->GetSampleRate();

	fwrite(data_->GetBuffer(), 1, data_->GetBufferLen(), m_pFileMixAudio);

	unsigned int channelNum = data_->GetChannelNum();
	unsigned int sampleRate = data_->GetSampleRate();
	unsigned int bufferLen = data_->GetBufferLen();
	char* buffer = data_->GetBuffer();
	const char* isCanAddRef = data_->CanAddRef() ? "true" : "false";

	char szTemp[2056] = { 0 };
	snprintf(szTemp,sizeof(szTemp), "channelNum = %d,  sampleRate = %d,  bufferLen = %d, isCanAddRef = %s,  buffer = %s",
		channelNum, sampleRate, bufferLen, isCanAddRef, buffer);


}

void CRegressionTestRawdataAudio::onOneWayAudioRawDataReceived(AudioRawData* data_, uint32_t node_id)
{
	if (!data_)
		return;

	ZOOM_SDK_NAMESPACE::IUserInfo* pMyInfo = SDKInterfaceWrap::GetInst().GetMeetingService()->GetMeetingParticipantsController()->GetMySelfUser();
	if (!pMyInfo)
	{
		return;
	}
	unsigned int myselfUserID = pMyInfo->GetUserID();
	if (node_id == myselfUserID)
	{
		fwrite(data_->GetBuffer(), 1, data_->GetBufferLen(), m_pFileMyself);
	}
	else
	{
		fwrite(data_->GetBuffer(), 1, data_->GetBufferLen(), m_pFileNodeID);
	}

	unsigned int channelNum = data_->GetChannelNum();
	unsigned int sampleRate = data_->GetSampleRate();
	unsigned int bufferLen = data_->GetBufferLen();
	char* buffer = data_->GetBuffer();
	const char* isCanAddRef = data_->CanAddRef() ?  "true" : "false";

	char szTemp[1028] = { 0 };
	snprintf(szTemp,sizeof(szTemp), "channelNum = %d,  sampleRate = %d,  bufferLen = %d, isCanAddRef = %s,  buffer = %s",
		channelNum, sampleRate, bufferLen, isCanAddRef, buffer);

}

ZOOM_SDK_NAMESPACE::ZoomSDKResolution CRegressionTestRawdataAudio::GetCurrentSelectedResolution()
{
	ZOOM_SDK_NAMESPACE::ZoomSDKResolution sdkResolution = ZOOM_SDK_NAMESPACE::ZoomSDKResolution_360P;

	std::string strResolution;
	if (strResolution == "90P")
	{
		sdkResolution = ZOOM_SDK_NAMESPACE::ZoomSDKResolution_90P;
	}
	else if (strResolution == "180P")
	{
		sdkResolution = ZOOM_SDK_NAMESPACE::ZoomSDKResolution_180P;
	}
	else if (strResolution =="360P")
	{
		sdkResolution = ZOOM_SDK_NAMESPACE::ZoomSDKResolution_360P;
	}
	else if (strResolution == "720P")
	{
		sdkResolution = ZOOM_SDK_NAMESPACE::ZoomSDKResolution_720P;
	}
	else if (strResolution == "1080P")
	{
		sdkResolution = ZOOM_SDK_NAMESPACE::ZoomSDKResolution_1080P;
	}

	return sdkResolution;
}

std::string CRegressionTestRawdataAudio::ConvertResolutionToString(const ZOOM_SDK_NAMESPACE::ZoomSDKResolution& resolution)
{
	std::string sResolution;

	switch (resolution)
	{
	case ZOOM_SDK_NAMESPACE::ZoomSDKResolution_90P:
		sResolution = "ZoomSDKResolution_90P";
		break;
	case ZOOM_SDK_NAMESPACE::ZoomSDKResolution_180P:
		sResolution = "ZoomSDKResolution_180P";
		break;
	case ZOOM_SDK_NAMESPACE::ZoomSDKResolution_360P:
		sResolution = "ZoomSDKResolution_360P";
		break;
	case ZOOM_SDK_NAMESPACE::ZoomSDKResolution_720P:
		sResolution = "ZoomSDKResolution_720P";
		break;
	case ZOOM_SDK_NAMESPACE::ZoomSDKResolution_1080P:
		sResolution = "ZoomSDKResolution_1080P";
		break;
	case ZOOM_SDK_NAMESPACE::ZoomSDKResolution_NoUse:
		sResolution = "ZoomSDKResolution_NoUse";
		break;
	default:
		break;
	}

	return sResolution;
}

std::string CRegressionTestRawdataAudio::ConvertRawDataTypeToString(const ZOOM_SDK_NAMESPACE::ZoomSDKRawDataType& type)
{
	std::string sRawDataType;

	switch (type)
	{
	case ZOOM_SDK_NAMESPACE::RAW_DATA_TYPE_VIDEO:
		sRawDataType = "raw_data_type_video";
		break;
	case ZOOM_SDK_NAMESPACE::RAW_DATA_TYPE_SHARE:
		sRawDataType = "raw_data_type_share";
		break;
	default:
		break;
	}

	return sRawDataType;
}