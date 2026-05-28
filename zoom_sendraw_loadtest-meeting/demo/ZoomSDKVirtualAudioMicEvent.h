//SendAudioRawData

#include <iostream>
#include <cstdint>
#include <thread>
#include "rawdata/rawdata_audio_helper_interface.h"
#include "zoom_sdk.h"
#include "zoom_sdk_raw_data_def.h"


using namespace std;
using namespace ZOOMSDK;

class ZoomSDKVirtualAudioMicEvent :
	public IZoomSDKVirtualAudioMicEvent
{

private:
	IZoomSDKAudioRawDataSender* pSender_;
	std::string audio_source_;
	int sample_rate_;
	int channels_;
	std::thread audio_thread_;
	void startAudioThread();
protected:

	/// \brief Callback for virtual audio mic to do some initialization.
/// \param pSender, You can send audio data based on this object, see \link IZoomSDKAudioRawDataSender \endlink.
	virtual void onMicInitialize(IZoomSDKAudioRawDataSender* pSender);

	/// \brief Callback for virtual audio mic can send raw data with 'pSender'.
	virtual void onMicStartSend();

	/// \brief Callback for virtual audio mic should stop send raw data.
	virtual void onMicStopSend();

	/// \brief Callback for virtual audio mic is uninitialized.
	virtual void onMicUninitialized();

public:
	~ZoomSDKVirtualAudioMicEvent();
	ZoomSDKVirtualAudioMicEvent(std::string audio_source, int sample_rate = 44100, int channels = 1);
};
