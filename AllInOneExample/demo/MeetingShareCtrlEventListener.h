#include "meeting_service_components/meeting_sharing_interface.h"
#include "meeting_service_components/meeting_sharing_interface.h" 
#include "zoom_sdk.h"


USING_ZOOM_SDK_NAMESPACE

class MeetingShareCtrlEventListener : public ZOOM_SDK_NAMESPACE::IMeetingShareCtrlEvent
{

public:
	MeetingShareCtrlEventListener();


	/// \brief Callback event of the changed sharing status. 
	/// \param status The values of sharing status. For more details, see \link SharingStatus \endlink enum.
	/// \param userId Sharer ID. 
	/// \remarks The userId changes according to the status value. When the status value is the Sharing_Self_Send_Begin or Sharing_Self_Send_End, the userId is the user own ID. Otherwise, the value of userId is the sharer ID.
	virtual void onSharingStatus(SharingStatus status, unsigned int userId);
	/// \brief Callback event of locked share status.
	/// \param bLocked TRUE indicates that it is locked. FALSE unlocked.
	virtual void onLockShareStatus(bool bLocked);

	/// \brief Callback event of changed sharing information.
	/// \param shareInfo Sharing information. For more details, see \link ShareInfo \endlink structure.
	virtual void onShareContentNotification(ShareInfo& shareInfo);

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

};

