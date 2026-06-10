#include "MeetingShareCtrlEventListener.h"
#include <cstdio>


MeetingShareCtrlEventListener::MeetingShareCtrlEventListener()
{
	
}

void MeetingShareCtrlEventListener::onSharingStatus(ZoomSDKSharingSourceInfo shareInfo)
{
	printf("Participant : %d status is %d\n", shareInfo.userid, shareInfo.status);
}

void MeetingShareCtrlEventListener::onFailedToStartShare()
{
}

void MeetingShareCtrlEventListener::onLockShareStatus(bool bLocked)
{
}

void MeetingShareCtrlEventListener::onShareContentNotification(ZoomSDKSharingSourceInfo shareInfo)
{
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

