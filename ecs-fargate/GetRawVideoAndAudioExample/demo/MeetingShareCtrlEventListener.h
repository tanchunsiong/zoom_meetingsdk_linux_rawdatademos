#include "meeting_service_components/meeting_sharing_interface.h"
#include "meeting_service_components/meeting_sharing_interface.h" 
#include "zoom_sdk.h"


USING_ZOOM_SDK_NAMESPACE

class MeetingShareCtrlEventListener : public ZOOM_SDK_NAMESPACE::IMeetingShareCtrlEvent
{

public:
	MeetingShareCtrlEventListener();


	/// \brief Callback event of the changed sharing status. 
	/// \param shareInfo Sharing information.
	virtual void onSharingStatus(ZoomSDKSharingSourceInfo shareInfo);

	virtual void onFailedToStartShare();
	/// \brief Callback event of locked share status.
	/// \param bLocked TRUE indicates that it is locked. FALSE unlocked.
	virtual void onLockShareStatus(bool bLocked);

	/// \brief Callback event of changed sharing information.
	/// \param shareInfo Sharing information.
	virtual void onShareContentNotification(ZoomSDKSharingSourceInfo shareInfo);

	/// \brief Callback event of switching multi-participants share to one participant share.
	/// \param handler_ An object pointer used by user to complete all the related operations. For more details, see \link IShareSwitchMultiToSingleConfirmHandler \endlink.
	virtual void onMultiShareSwitchToSingleShareNeedConfirm(IShareSwitchMultiToSingleConfirmHandler* handler_);

	/// \brief Callback event of sharing setting type changed.
	/// \param type Sharing setting type. For more details, see \link ShareSettingType \endlink structure.
	virtual void onShareSettingTypeChangedNotification(ShareSettingType type);

	/// \brief Callback event of the shared video's playback has completed.
	virtual void onSharedVideoEnded();

	/// \brief Callback event of the video file playback error.
	/// \param error The error type. For more details, see \link ZoomSDKVideoFileSharePlayError \endlink structure.
	virtual void onVideoFileSharePlayError(ZoomSDKVideoFileSharePlayError error);;

	virtual void onOptimizingShareForVideoClipStatusChanged(ZoomSDKSharingSourceInfo shareInfo);

};
