#ifndef _EXTERNAL_VIDEO_SOURCE_H_
#define _EXTERNAL_VIDEO_SOURCE_H_
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
#include <pthread.h>
#include <SDL2/SDL.h>
#include "rawdata/rawdata_video_source_helper_interface.h"
#include "rawdata/rawdata_audio_helper_interface.h"

class CExternalVideoSource
	:public ZOOM_SDK_NAMESPACE::IZoomSDKVideoSource
	,public ZOOM_SDK_NAMESPACE::IZoomSDKPreProcessor
	//,public ZOOM_SDK_NAMESPACE::IZoomSDKAudioPreprocessor
	//,public ZOOM_SDK_NAMESPACE::IZoomSDKVirtualAudioMicEvent
{
public:
	static CExternalVideoSource* GetInstance();
	ZOOM_SDK_NAMESPACE::SDKError EnableVideoSource(ZOOM_SDK_NAMESPACE::IZoomSDKVideoSource* pEvent);
	ZOOM_SDK_NAMESPACE::SDKError DisableVideoSource();
	ZOOM_SDK_NAMESPACE::SDKError EnablePreprocessor();
	ZOOM_SDK_NAMESPACE::SDKError DisablePreprocessor();
	void ChangeSendSize();
	
	bool EnableAudioProcessor();
	bool DisableAudioProcessor();
	bool EnableVirtualMic();
	bool DisableVirtualMic();

	bool DisablePlayMeetingAudio();
	bool EnablePlayMeetingAudio();

public: //IZoomSDKVideoSource
	virtual	void onInitialize(ZOOM_SDK_NAMESPACE::IZoomSDKVideoSender* sender, ZOOM_SDK_NAMESPACE::IList<ZOOM_SDK_NAMESPACE::VideoSourceCapability >* support_cap_list, ZOOM_SDK_NAMESPACE::VideoSourceCapability& suggest_cap);
	virtual void onPropertyChange(ZOOM_SDK_NAMESPACE::IList<ZOOM_SDK_NAMESPACE::VideoSourceCapability >* support_cap_list, ZOOM_SDK_NAMESPACE::VideoSourceCapability suggest_cap);
	virtual void onStartSend();
	virtual void onStopSend();
	virtual void onUninitialized();

public: //IZoomSDKPreProcessor
	virtual void onPreProcessRawData(YUVProcessDataI420* rawData);
	ZOOM_SDK_NAMESPACE::IZoomSDKVideoSender* m_pSender = nullptr;
		int m_nDataType;
	FILE* m_process_fp = nullptr;

public:
	 void StartThread() ;
	 void ResumeThread();
	 void WaitForThread();
    void PauseThread();
	void ThreadProc();
	void SuspendThread();

//public: //IZoomSDKAudioPreprocessor
//	virtual void onAudioPreprocessDataReceived(ZOOM_SDK_NAMESPACE::IZoomSDKAudioPreprocessData* data);

//public: //IZoomSDKVirtualAudioMicEvent
//	virtual void onMicInitialize(ZOOM_SDK_NAMESPACE::IZoomSDKAudioRawDataSender* rawdata_sender);
//	virtual void onMicStartSend();
//	virtual void onMicStopSend();
//	virtual void onMicUninitialized();

private:
	CExternalVideoSource();
	~CExternalVideoSource();
	
public:
	static CExternalVideoSource* s_pVideoSource;
	pthread_t  m_hThread;
	pthread_t  m_hThreadAudio;
    //pthread_mutex_t m_mutex; // 互斥锁
    pthread_cond_t m_cond; // 条件变量
    bool m_isRunning; // 表示线程是否在运行
	bool m_isPaused; // 表示线程是否被暂停
	
	//ZOOM_SDK_NAMESPACE::IZoomSDKAudioRawDataSender* m_pAudioSender;


	ZOOM_SDK_NAMESPACE::IZoomSDKVideoSource* m_pEvent = nullptr;

};

#endif


