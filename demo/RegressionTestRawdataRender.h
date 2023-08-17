#pragma once

#include "rawdata/rawdata_renderer_interface.h"
#include "zoom_sdk_raw_data_def.h"
#include <mutex>

class CRegressionTestRawdataRender : public ZOOM_SDK_NAMESPACE::IZoomSDKRendererDelegate
{
public:
	CRegressionTestRawdataRender();
	~CRegressionTestRawdataRender();

	void Init();

	ZOOM_SDK_NAMESPACE::SDKError subscribe(unsigned int nUserID, ZOOM_SDK_NAMESPACE::ZoomSDKRawDataType type);
	ZOOM_SDK_NAMESPACE::SDKError unSubscribe();
	ZOOM_SDK_NAMESPACE::SDKError SetResolution(ZOOM_SDK_NAMESPACE::ZoomSDKResolution resolution);
	ZOOM_SDK_NAMESPACE::SDKError CreateRender();
	ZOOM_SDK_NAMESPACE::SDKError DestroyRender();
	void GetInfo(ZOOM_SDK_NAMESPACE::ZoomSDKResolution& resolution, ZOOM_SDK_NAMESPACE::ZoomSDKRawDataType& type, unsigned int& userID) const;

	// void OnSize(RECT rect);
public: 
	// IZoomSDKRendererDelegate
	void onRendererBeDestroyed();
	void onRawDataFrameReceived(YUVRawDataI420* data);
	void onRawDataStatusChanged(RawDataStatus status);
private:
	int m_dataWidth = 0;
	int m_dataHeight = 0;
	ZOOM_SDK_NAMESPACE::IZoomSDKRenderer* m_pSDKRenderer = nullptr;
	bool m_bInited = false;
	std::recursive_mutex data_lock_;
};
