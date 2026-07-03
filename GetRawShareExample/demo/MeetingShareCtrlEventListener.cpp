#include "MeetingShareCtrlEventListener.h"
#include <cstdio>

namespace {
const char* ShareStatusName(SharingStatus status)
{
	switch (status) {
	case Sharing_Self_Send_Begin:
		return "Sharing_Self_Send_Begin";
	case Sharing_Self_Send_End:
		return "Sharing_Self_Send_End";
	case Sharing_Self_Send_Pure_Audio_Begin:
		return "Sharing_Self_Send_Pure_Audio_Begin";
	case Sharing_Self_Send_Pure_Audio_End:
		return "Sharing_Self_Send_Pure_Audio_End";
	case Sharing_Other_Share_Begin:
		return "Sharing_Other_Share_Begin";
	case Sharing_Other_Share_End:
		return "Sharing_Other_Share_End";
	case Sharing_Other_Share_Pure_Audio_Begin:
		return "Sharing_Other_Share_Pure_Audio_Begin";
	case Sharing_Other_Share_Pure_Audio_End:
		return "Sharing_Other_Share_Pure_Audio_End";
	case Sharing_View_Other_Sharing:
		return "Sharing_View_Other_Sharing";
	case Sharing_Pause:
		return "Sharing_Pause";
	case Sharing_Resume:
		return "Sharing_Resume";
	default:
		return "Unknown";
	}
}
}

MeetingShareCtrlEventListener::MeetingShareCtrlEventListener(void (*onShareSourceChanged)(ZoomSDKSharingSourceInfo shareInfo))
{
	onShareSourceChanged_ = onShareSourceChanged;
}

void MeetingShareCtrlEventListener::onSharingStatus(ZoomSDKSharingSourceInfo shareInfo)
{
	printf(
		"[GetRawShare] sharing status: userid=%u shareSourceID=%u status=%s(%d) contentType=%d firstView=%d secondView=%d optimizedVideo=%d\n",
		shareInfo.userid,
		shareInfo.shareSourceID,
		ShareStatusName(shareInfo.status),
		shareInfo.status,
		shareInfo.contentType,
		shareInfo.isShowingInFirstView,
		shareInfo.isShowingInSecondView,
		shareInfo.bEnableOptimizingVideoSharing
	);
	fflush(stdout);
	if (onShareSourceChanged_) {
		onShareSourceChanged_(shareInfo);
	}
}

void MeetingShareCtrlEventListener::onFailedToStartShare()
{
}

void MeetingShareCtrlEventListener::onLockShareStatus(bool bLocked)
{
}

void MeetingShareCtrlEventListener::onShareContentNotification(ZoomSDKSharingSourceInfo shareInfo)
{
	printf(
		"[GetRawShare] share content notification: userid=%u shareSourceID=%u status=%s(%d) contentType=%d\n",
		shareInfo.userid,
		shareInfo.shareSourceID,
		ShareStatusName(shareInfo.status),
		shareInfo.status,
		shareInfo.contentType
	);
	fflush(stdout);
	if (onShareSourceChanged_) {
		onShareSourceChanged_(shareInfo);
	}
}

void MeetingShareCtrlEventListener::onMultiShareSwitchToSingleShareNeedConfirm(IShareSwitchMultiToSingleConfirmHandler* handler_)
{
}

void MeetingShareCtrlEventListener::onShareSettingTypeChangedNotification(ShareSettingType type)
{
}

void MeetingShareCtrlEventListener::onSharedVideoEnded()
{
}

void MeetingShareCtrlEventListener::onVideoFileSharePlayError(ZoomSDKVideoFileSharePlayError error)
{
}

void MeetingShareCtrlEventListener::onOptimizingShareForVideoClipStatusChanged(ZoomSDKSharingSourceInfo shareInfo)
{
}
