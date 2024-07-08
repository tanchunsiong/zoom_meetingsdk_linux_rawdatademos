#include "MeetingShareCtrlEventListener.h"
#include <cstdio>


MeetingShareCtrlEventListener::MeetingShareCtrlEventListener()
{
	
}

void MeetingShareCtrlEventListener::onSharingStatus(SharingStatus status, unsigned int userId)
{

	printf("Participant : %d status is %d\n", userId, status);
}

void MeetingShareCtrlEventListener::onLockShareStatus(bool bLocked)
{
}

void MeetingShareCtrlEventListener::onShareContentNotification(ShareInfo& shareInfo)
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



