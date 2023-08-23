
#include "external_video_source.h"
#include "rawdata/zoom_rawdata_api.h"
#include "setting_service_interface.h"
#include "zoom_sdk_raw_data_def.h"
#include "meeting_sdk_util.h"

CExternalVideoSource* CExternalVideoSource::s_pVideoSource = NULL;

CExternalVideoSource::CExternalVideoSource()
{
	m_hThread = NULL;
	m_hThreadAudio = NULL;
	m_pSender = NULL;
	m_nDataType = 1;
	m_process_fp = NULL;
	m_isRunning = false;
	m_isPaused = false;
	//pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
	pthread_cond_init(&m_cond, NULL);
	//m_pAudioSender = NULL;
}

CExternalVideoSource::~CExternalVideoSource()
{

    onUninitialized();
}

#define MAX_PATH    260
bool ReadFrameData(FILE* fp, int nFrameLength, char* pBuffer)
{
	if (!fp || !pBuffer)
	{
		return false;
	}

	size_t nTotalReaded=0;
	size_t nReaded=0;
	size_t nWantRead = nFrameLength;

	while(1)
	{
		nReaded = fread(pBuffer, 1, nWantRead , fp);
		if (nReaded != nWantRead)
		{	
			if (feof(fp) || ferror(fp))
			{
				return false;
			}
			else 
			{
				nTotalReaded += nReaded;
				pBuffer += nReaded;
				nWantRead = nFrameLength - nTotalReaded;		
			}
		}
		else 
		{
			return true;	
		}
	}
}

static void* MyThreadProc(void* arg) {
	CExternalVideoSource* instance = static_cast<CExternalVideoSource*>(arg);
	CExternalVideoSource* pVideoSource = instance;	
	if (!pVideoSource) return NULL;
	ZOOM_SDK_NAMESPACE::IZoomSDKVideoSender* sender = pVideoSource->m_pSender;
	if (!sender) return NULL;
    //pthread_mutex_t m_mutex = pVideoSource->m_mutex;
	 bool m_isRunning = pVideoSource->m_isRunning;
	bool m_isPaused =  pVideoSource->m_isPaused;
	int w, h;
	int nFrameLength;
	char* pFrameData;
	FILE* fp = NULL;
	char file_path[MAX_PATH] = { 0 };
	int nDataType;

	while(m_isRunning && !m_isPaused)
	{
		if (getcwd(file_path, sizeof(file_path)) == NULL) {
			perror("Error getting current directory");
			exit(EXIT_FAILURE);
		}

		size_t len = strlen(file_path);
		if (len > 0 && file_path[len - 1] != '/') {
			strcat(file_path, "/");
		}
		strcat(file_path, "/home/shawchen/release-client-5.15.x/release_demo_test/YUVFile/");
		if (pVideoSource->m_nDataType == 3)
		{
			w = 1920;
			h = 1080;
			nFrameLength = (w*h) + (w*h)/4 + (w*h)/4;
			pFrameData = new char[nFrameLength];
			strcat(file_path, "videofile_1920_1080.yuv");

		}
		else if(pVideoSource->m_nDataType == 2)
		{

			w = 1280;
			h = 720;
			nFrameLength = (w*h) + (w*h)/4 + (w*h)/4;
			pFrameData = new char[nFrameLength];
			strcat(file_path, "videofile_1280_720.yuv");

		}
		else
		{
			w = 640;
			h = 480;
			nFrameLength = (w*h) + (w*h)/4 + (w*h)/4;
			pFrameData = new char[nFrameLength];
			strcat(file_path, "videofile_640_480.yuv");
		}
		std::string file_yuv_name = "/home/shawchen/release-client-5.15.x/release_demo_test/YUVFile/videofile_640_480.yuv";
		fp = fopen(file_yuv_name.c_str(), "rb");
		
		if (!fp) 
		{	
			break;
		}

		nDataType = pVideoSource->m_nDataType;
		while (ReadFrameData(fp, nFrameLength, pFrameData))
		{
			if (pVideoSource->m_nDataType != nDataType)
				break;

			sender->sendVideoFrame(pFrameData, w, h, nFrameLength, 0);
			struct timespec sleepTime;
			sleepTime.tv_sec = 0;
			sleepTime.tv_nsec = 35000000;
			nanosleep(&sleepTime, NULL);
		}

		delete[] pFrameData;
		fclose(fp);
	}

	return NULL;
}


void CExternalVideoSource::StartThread() 
{

	if (!m_isRunning) {
		// 创建一个新线程
		m_isRunning = true;
		int result = pthread_create(&m_hThread, NULL, MyThreadProc, this);
		if (result != 0) {
			std::cerr << "Error creating thread." << std::endl;
			m_hThread = 0;
		}
		
	}

}


void CExternalVideoSource::ResumeThread() 
{
	if (m_isRunning) {
		// 唤醒线程的执行
		pthread_cond_signal(&m_cond);
	}
}

void CExternalVideoSource::WaitForThread() 
{
	if (m_hThread != 0) 
	{
		// 等待线程结束
		pthread_join(m_hThread, NULL);
		m_hThread = 0;
	}
}

void CExternalVideoSource::SuspendThread() {

        m_isPaused = true;

    }

CExternalVideoSource* CExternalVideoSource::GetInstance()
{
	if (!s_pVideoSource)
	{
		s_pVideoSource = new CExternalVideoSource;
	}
	return s_pVideoSource;
}

ZOOM_SDK_NAMESPACE::SDKError CExternalVideoSource::EnableVideoSource(ZOOM_SDK_NAMESPACE::IZoomSDKVideoSource* pEvent)
{
	ZOOM_SDK_NAMESPACE::SDKError err(ZOOM_SDK_NAMESPACE::SDKERR_WRONG_USAGE);

	ZOOM_SDK_NAMESPACE::IZoomSDKVideoSourceHelper* pVideoSourceHelper;
	pVideoSourceHelper = ZOOM_SDK_NAMESPACE::GetRawdataVideoSourceHelper();
	if (!pVideoSourceHelper)
	{
		return err;
	}
	err = pVideoSourceHelper->setExternalVideoSource(this);
	if (err != ZOOM_SDK_NAMESPACE::SDKERR_SUCCESS)
	{
		return err;
	}

	m_pEvent = pEvent;

	return err;
}

ZOOM_SDK_NAMESPACE::SDKError CExternalVideoSource::DisableVideoSource()
{
	ZOOM_SDK_NAMESPACE::SDKError err(ZOOM_SDK_NAMESPACE::SDKERR_WRONG_USAGE);
	
	ZOOM_SDK_NAMESPACE::IZoomSDKVideoSourceHelper* pVideoSourceHelper;
	pVideoSourceHelper = ZOOM_SDK_NAMESPACE::GetRawdataVideoSourceHelper();
	if (!pVideoSourceHelper)
	{
		return err;
	}

	//set external video source
	err = pVideoSourceHelper->setExternalVideoSource(NULL);

	return err;
}

ZOOM_SDK_NAMESPACE::SDKError CExternalVideoSource::EnablePreprocessor()
{
	ZOOM_SDK_NAMESPACE::SDKError err(ZOOM_SDK_NAMESPACE::SDKERR_WRONG_USAGE);

	ZOOM_SDK_NAMESPACE::IZoomSDKVideoSourceHelper* pVideoSourceHelper;
	pVideoSourceHelper = ZOOM_SDK_NAMESPACE::GetRawdataVideoSourceHelper();
	if (!pVideoSourceHelper)
	{
		return err;
	}

	//set preprocessor
	return pVideoSourceHelper->setPreProcessor(this);
}

ZOOM_SDK_NAMESPACE::SDKError CExternalVideoSource::DisablePreprocessor()
{
	ZOOM_SDK_NAMESPACE::SDKError err(ZOOM_SDK_NAMESPACE::SDKERR_WRONG_USAGE);

	ZOOM_SDK_NAMESPACE::IZoomSDKVideoSourceHelper* pVideoSourceHelper;
	pVideoSourceHelper = ZOOM_SDK_NAMESPACE::GetRawdataVideoSourceHelper();
	if (!pVideoSourceHelper)
	{
		return err;
	}

	//set preprocessor
	err = pVideoSourceHelper->setPreProcessor(NULL);

	return err;
}

void CExternalVideoSource::ChangeSendSize()
{
	switch(m_nDataType)
	{
	case 3:
		{
			m_nDataType = 1;
		}
		break;
	case 2:
		{
			m_nDataType = 3;
		}
		break;
	case 1:
		{
			m_nDataType = 2;
		}
		break;
	default:
		{
			m_nDataType = 3;
		}
		break;
	}
}

bool CExternalVideoSource::EnableAudioProcessor() 
{
	//ZOOM_SDK_NAMESPACE::SDKError err;

	//ZOOM_SDK_NAMESPACE::IZoomSDKAudioRawDataHelper* pHelper;
	//pHelper = ZOOM_SDK_NAMESPACE::GetAudioRawdataHelper();
	//if (!pHelper)
	//{
	//	return false;
	//}

	//err = pHelper->setPreProcessor(this);

	//return true;

	return true;
}

bool CExternalVideoSource::DisableAudioProcessor() 
{
	//ZOOM_SDK_NAMESPACE::SDKError err;

	//ZOOM_SDK_NAMESPACE::IZoomSDKAudioRawDataHelper* pHelper;
	//pHelper = ZOOM_SDK_NAMESPACE::GetAudioRawdataHelper();
	//if (!pHelper)
	//{
	//	return false;
	//}

	//err = pHelper->setPreProcessor(NULL);

	//return true;

	return true;
}

bool CExternalVideoSource::EnableVirtualMic() 
{
	//ZOOM_SDK_NAMESPACE::SDKError err;

	//ZOOM_SDK_NAMESPACE::IZoomSDKAudioRawDataHelper* pHelper;
	//pHelper = ZOOM_SDK_NAMESPACE::GetAudioRawdataHelper();
	//if (!pHelper)
	//{
	//	return false;
	//}

	//err = pHelper->setVirtualAudioMic(this);

	//return true;

	return true;
}

bool CExternalVideoSource::DisableVirtualMic() 
{
	//ZOOM_SDK_NAMESPACE::SDKError err;

	//ZOOM_SDK_NAMESPACE::IZoomSDKAudioRawDataHelper* pHelper;
	//pHelper = ZOOM_SDK_NAMESPACE::GetAudioRawdataHelper();
	//if (!pHelper)
	//{
	//	return false;
	//}

	//err = pHelper->setVirtualAudioMic(NULL);

	//return true;

	return true;
}

bool CExternalVideoSource::DisablePlayMeetingAudio()
{
	//ZOOM_SDK_NAMESPACE::SDKError err;

	//ZOOM_SDK_NAMESPACE::IZoomSDKAudioRawDataHelper* pHelper;
	//pHelper = ZOOM_SDK_NAMESPACE::GetAudioRawdataHelper();
	//if (!pHelper)
	//{
	//	return false;
	//}

	//err = pHelper->DisablePlayMeetingAudio(true);

	//return true;

	return true;
}

bool CExternalVideoSource::EnablePlayMeetingAudio()
{
	//ZOOM_SDK_NAMESPACE::SDKError err;

	//ZOOM_SDK_NAMESPACE::IZoomSDKAudioRawDataHelper* pHelper;
	//pHelper = ZOOM_SDK_NAMESPACE::GetAudioRawdataHelper();
	//if (!pHelper)
	//{
	//	return false;
	//}

	//err = pHelper->DisablePlayMeetingAudio(false);

	//return true;

	return true;
}


#define SAMPLING_RATE 48000 
#define AUDIO_DATA_10MS_LEN ((SAMPLING_RATE * 2 * 1) / 100)

void RGB2Yuv420p(char* destination, char* rgb, int width, int height)
{
	size_t image_size = width * height;
	size_t upos = image_size;
	size_t vpos = upos + upos / 4;
	size_t i = 0;

	for (size_t line = 0; line < height; ++line)
	{
		if (!(line % 2))
		{
			for (size_t x = 0; x < width; x += 2)
			{
				uint8_t r = rgb[3 * i];
				uint8_t g = rgb[3 * i + 1];
				uint8_t b = rgb[3 * i + 2];

				destination[i++] = ((66 * r + 129 * g + 25 * b) >> 8) + 16;

				destination[upos++] = ((-38 * r + -74 * g + 112 * b) >> 8) + 128;
				destination[vpos++] = ((112 * r + -94 * g + -18 * b) >> 8) + 128;

				r = rgb[3 * i];
				g = rgb[3 * i + 1];
				b = rgb[3 * i + 2];

				destination[i++] = ((66 * r + 129 * g + 25 * b) >> 8) + 16;
			}
		}
		else
		{
			for (size_t x = 0; x < width; x += 1)
			{
				uint8_t r = rgb[3 * i];
				uint8_t g = rgb[3 * i + 1];
				uint8_t b = rgb[3 * i + 2];

				destination[i++] = ((66 * r + 129 * g + 25 * b) >> 8) + 16;
			}
		}
	}
}


//IZoomSDKVideoSource
void CExternalVideoSource::onInitialize(ZOOM_SDK_NAMESPACE::IZoomSDKVideoSender* sender, ZOOM_SDK_NAMESPACE::IList<ZOOM_SDK_NAMESPACE::VideoSourceCapability >* support_cap_list, ZOOM_SDK_NAMESPACE::VideoSourceCapability& suggest_cap)
{
	if (!sender || !support_cap_list) 
	{
		return;
	}
	m_pSender = sender;

	//capability
	char szDbg[512] = {0};
	ZOOM_SDK_NAMESPACE::VideoSourceCapability cap;
	unsigned int nCount = support_cap_list->GetCount();	

	for(unsigned int i=0; i<nCount; i++)
	{
		cap = support_cap_list->GetItem(i);

		snprintf(szDbg,sizeof(szDbg),"############### %d: frame=%d, Height=%d, width=%d\r\n", i, cap.frame, cap.height, cap.width);
		
	}

	snprintf(szDbg, sizeof(szDbg),"############### suggest_cap: frame=%d, Height=%d, width=%d\r\n", suggest_cap.frame, suggest_cap.height, suggest_cap.width);
	

	if (m_pEvent)
		m_pEvent->onInitialize(sender, support_cap_list, suggest_cap);
}

void CExternalVideoSource::onPropertyChange(ZOOM_SDK_NAMESPACE::IList<ZOOM_SDK_NAMESPACE::VideoSourceCapability >* support_cap_list, ZOOM_SDK_NAMESPACE::VideoSourceCapability suggest_cap)
{
	if (!support_cap_list) 
	{
		return;
	}

	char szDbg[512] = {0};
	ZOOM_SDK_NAMESPACE::VideoSourceCapability cap;
	unsigned int nCount = support_cap_list->GetCount();

	//OutputDebugString(_T("############### onPropertyChange\r\n"));
	for (unsigned int i = 0; i < nCount; i++)
	{
		cap = support_cap_list->GetItem(i);

		snprintf(szDbg, sizeof(szDbg),"############### %d: frame=%d, Height=%d, width=%d\r\n", i, cap.frame, cap.height, cap.width);
		//OutputDebugString(szDbg);
	}

	snprintf(szDbg,sizeof(szDbg), "\r\n ############### suggest_cap: frame=%d, Height=%d, width=%d\r\n", suggest_cap.frame, suggest_cap.height, suggest_cap.width);
	//OutputDebugString(szDbg);

	if (suggest_cap.width == 1920 && suggest_cap.height == 1080)
	{
		m_nDataType = 3;
	}
	else if (suggest_cap.width == 1280 && suggest_cap.height == 720) 
	{
		m_nDataType = 2;
	}
	else 
	{
		m_nDataType = 1;
	}

	if (m_pEvent)
		m_pEvent->onPropertyChange(support_cap_list, suggest_cap);
}

 static void* StartThreadHelper(void* arg) {
        return MyThreadProc(arg);
}




void CExternalVideoSource::onStartSend()
{
	if (!m_hThread)
	{
		StartThread();
	}else{
		ResumeThread();
	}

	if (m_pEvent)
		m_pEvent->onStartSend();

}

void CExternalVideoSource::onStopSend()
{
	if (m_hThread)
	{
		SuspendThread();
	}

	if (m_pEvent)
		m_pEvent->onStopSend();
}

void CExternalVideoSource::onUninitialized()
{
	//OutputDebugString(_T("############### onUninitialized\r\n"));
	if (m_hThread)
	{
		m_isRunning = false;
		m_hThread = 0;
		m_nDataType = 1;
		m_isPaused = false;
		//pthread_mutex_destroy(&m_mutex);
		pthread_cond_destroy(&m_cond);
	}

	if (m_pEvent)
		m_pEvent->onUninitialized();

	m_pEvent = nullptr;
}

void CExternalVideoSource::onPreProcessRawData(YUVProcessDataI420* rawData)
{
	if (!rawData) return;

	char szDbg[512] = {0};
	// OutputDebugString(szDbg);
	snprintf(szDbg,sizeof(szDbg), "################## onPreProcessRawData(), w=%d, h=%d, IsLimitedI420=%d, ratation=%d, YStride=%d, UStride=%d, VStride=%d\r\n", 
					rawData->GetWidth(), 
					rawData->GetHeight(), 
					rawData->IsLimitedI420(), 
					rawData->GetRotation(),
					rawData->GetYStride(),
					rawData->GetUStride(),
					rawData->GetVStride()
					);
	//OutputDebugString(szDbg);

	//get y/u/v data from file
	static char* y=NULL, *u=NULL, *v=NULL;
	if (!y && !u && !v)
	{
		int nBufSize = 640*480;
		size_t nRead;

		char file_path[MAX_PATH] = { 0 };
		if (getcwd(file_path, sizeof(file_path)) == NULL) {
				perror("Error getting current directory");
				exit(EXIT_FAILURE);
			}

		size_t len = strlen(file_path);
		if (len > 0 && file_path[len - 1] != '/') {
			strcat(file_path, "/");
		}

		strcat(file_path, "/home/shawchen/release-client-5.15.x/release_demo_test/YUVFile/preProcessor.yuv");
		
		FILE* fp = fopen(file_path, "rb");
		if (!fp) return;

		y = new char[nBufSize];
		nRead = fread(y, 1, nBufSize, fp);
		u = new char[nBufSize/4];
		nRead = fread(u, 1, nBufSize/4, fp);
		v = new char[nBufSize/4];
		nRead = fread(v, 1, nBufSize/4, fp);
	}

	//fill to raw_data
	char* y_raw_data, *u_raw_data, *v_raw_data;
	char* y_temp=y, *u_temp=u, *v_temp=v;
	for (int i=0; i<480; i++, y_temp+=640)
	{	//Y
		y_raw_data = rawData->GetYBuffer(i);
		if(y_raw_data)
			memcpy(y_raw_data, y_temp, 640);
	}

	for (int i=0; i<480/2; i++, u_temp+=640/2)
	{	//U
		u_raw_data = rawData->GetUBuffer(i);
		if(u_raw_data)
			memcpy(u_raw_data, u_temp, 640/2);
	}

	for (int i=0; i<480/2; i++, v_temp+=640/2)
	{	//V
		v_raw_data = rawData->GetVBuffer(i);
		if(v_raw_data)
			memcpy(v_raw_data, v_temp, 640/2);
	}
}
