#include "meeting_service_components/meeting_reminder_ctrl_interface.h" // Assuming this header includes the required definitions
#include "zoom_sdk.h"




class MeetingReminderEventListener : public ZOOM_SDK_NAMESPACE::IMeetingReminderEvent
{
public:
    MeetingReminderEventListener();

  
   virtual void onReminderNotify(ZOOMSDK::IMeetingReminderContent* content, ZOOMSDK::IMeetingReminderHandler* handle);
    
};