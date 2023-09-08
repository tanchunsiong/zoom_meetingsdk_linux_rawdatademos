
#include <meeting_service_components/meeting_recording_interface.h>
#include "zoom_sdk.h"


USING_ZOOM_SDK_NAMESPACE

class MeetingRecordingCtrlEventListener :public IMeetingRecordingCtrlEvent
{

	void (*onIsGivenRecordingPermission_)();
public:
	MeetingRecordingCtrlEventListener(void (*onIsGivenRecordingPermission)());

	/// \brief Callback event that the status of my local recording changes.
	/// \param status Value of recording status. For more details, see \link RecordingStatus \endlink enum.
	virtual void onRecordingStatus(RecordingStatus status);
	/// \brief Callback event that the status of cloud recording changes.
	/// \param status Value of recording status. For more details, see \link RecordingStatus \endlink enum.
	virtual void onCloudRecordingStatus(RecordingStatus status);

	/// \brief Callback event that the recording authority changes.
	/// \param bCanRec TRUE indicates to enable to record.
	virtual void onRecordPrivilegeChanged(bool bCanRec);

	/// \brief Callback event that the status of request local recording privilege.
	/// \param status Value of request local recording privilege status. For more details, see \link RequestLocalRecordingStatus \endlink enum.
	virtual void onLocalRecordingPrivilegeRequestStatus(RequestLocalRecordingStatus status);


	/// \brief Callback event when a user requests local recording privilege.
	/// \param handler A pointer to the IRequestLocalRecordingPrivilegeHandler. For more details, see \link IRequestLocalRecordingPrivilegeHandler \endlink.
	virtual void onLocalRecordingPrivilegeRequested(IRequestLocalRecordingPrivilegeHandler* handler);




};

