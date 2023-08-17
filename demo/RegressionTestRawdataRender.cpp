#include "RegressionTestRawdataRender.h"
#include "rawdata/zoom_rawdata_api.h"
#include <iostream>
#include <fstream>
#include <string>

CRegressionTestRawdataRender::CRegressionTestRawdataRender()
{
}

CRegressionTestRawdataRender::~CRegressionTestRawdataRender()
{

}

void CRegressionTestRawdataRender::Init()
{
	if (m_bInited)
		return;


	ZOOM_SDK_NAMESPACE::SDKError err = ZOOM_SDK_NAMESPACE::createRenderer(&m_pSDKRenderer, this);
	if (err != ZOOM_SDK_NAMESPACE::SDKERR_SUCCESS)
		return;

	m_bInited = true;
}

ZOOM_SDK_NAMESPACE::SDKError CRegressionTestRawdataRender::CreateRender()
{
	if (m_pSDKRenderer)
		return ZOOM_SDK_NAMESPACE::SDKERR_WRONG_USAGE;

	return ZOOM_SDK_NAMESPACE::createRenderer(&m_pSDKRenderer, this);
}

ZOOM_SDK_NAMESPACE::SDKError CRegressionTestRawdataRender::DestroyRender()
{
	ZOOM_SDK_NAMESPACE::SDKError err(ZOOM_SDK_NAMESPACE::SDKERR_WRONG_USAGE);
	if (m_pSDKRenderer)
		err = ZOOM_SDK_NAMESPACE::destroyRenderer(m_pSDKRenderer);

	return err;
}


ZOOM_SDK_NAMESPACE::SDKError CRegressionTestRawdataRender::subscribe(unsigned int nUserID, ZOOM_SDK_NAMESPACE::ZoomSDKRawDataType type)
{
	ZOOM_SDK_NAMESPACE::SDKError err(ZOOM_SDK_NAMESPACE::SDKERR_WRONG_USAGE);
	if (m_pSDKRenderer)
	{
		err = m_pSDKRenderer->subscribe(nUserID, type);
		if (err != ZOOM_SDK_NAMESPACE::SDKERR_SUCCESS)
			return err;
	}

	return err;
}

ZOOM_SDK_NAMESPACE::SDKError CRegressionTestRawdataRender::unSubscribe()
{
	ZOOM_SDK_NAMESPACE::SDKError err(ZOOM_SDK_NAMESPACE::SDKERR_WRONG_USAGE);
	if (m_pSDKRenderer)
	{
		err = m_pSDKRenderer->unSubscribe();
		if (err != ZOOM_SDK_NAMESPACE::SDKERR_SUCCESS)
			return err;
		m_dataWidth = 0;
		m_dataHeight = 0;
	}

	return err;
}

ZOOM_SDK_NAMESPACE::SDKError CRegressionTestRawdataRender::SetResolution(ZOOM_SDK_NAMESPACE::ZoomSDKResolution resolution)
{
	ZOOM_SDK_NAMESPACE::SDKError err(ZOOM_SDK_NAMESPACE::SDKERR_WRONG_USAGE);
	if (m_pSDKRenderer)
	{
		err = m_pSDKRenderer->setRawDataResolution(resolution);
	}

	return err;
}

void CRegressionTestRawdataRender::GetInfo(ZOOM_SDK_NAMESPACE::ZoomSDKResolution& resolution, ZOOM_SDK_NAMESPACE::ZoomSDKRawDataType& type, unsigned int& userID) const
{
	if (!m_pSDKRenderer)
		return;

	resolution = m_pSDKRenderer->getResolution();
	type = m_pSDKRenderer->getRawDataType();
	userID = m_pSDKRenderer->getUserId();
}

// void CRegressionTestRawdataRender::OnSize(RECT rect)
// {
// 	std::lock_guard<std::recursive_mutex> guard(data_lock_);
// 	m_rectDst = { rect.left,rect.top,rect.right - rect.left,rect.bottom - rect.top };
// }

void CRegressionTestRawdataRender::onRendererBeDestroyed()
{
	m_dataWidth = 0;
	m_dataHeight = 0;
	m_pSDKRenderer = nullptr;
}

void CRegressionTestRawdataRender::onRawDataFrameReceived(YUVRawDataI420* data)
{

}

void CRegressionTestRawdataRender::onRawDataStatusChanged(RawDataStatus status)
{
}
