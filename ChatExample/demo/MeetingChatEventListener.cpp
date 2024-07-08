//chatDemo
#include "MeetingChatEventListener.h"
#include <iostream>
#include <string>


using namespace std;
using namespace ZOOMSDK;



bool ZCharStringMatches(const zchar_t* myZCharString, const char* targetString) {
	// Convert zchar_t to a regular C-style string for comparison
	const char* myCString = myZCharString;

	// Use strcmp to compare the two C-style strings
	return strcmp(myCString, targetString) == 0;
}

MeetingChatEventListener::MeetingChatEventListener() {


}

void MeetingChatEventListener::onChatMsgNotifcation(IChatMsgInfo* chatMsg, const zchar_t* content)
{


	std::cout<<"onChatMsgNotifcation: " << chatMsg->GetSenderDisplayName() << " says " << chatMsg->GetContent() << endl;

}

void MeetingChatEventListener::onChatStatusChangedNotification(ChatStatus* status_)
{
}

void MeetingChatEventListener::onChatMsgDeleteNotification(const zchar_t* msgID, SDKChatMessageDeleteType deleteBy)
{
}

void MeetingChatEventListener::onShareMeetingChatStatusChanged(bool isStart)
{
}

void MeetingChatEventListener::onFileSendStart(ISDKFileSender* sender)
{
}

void MeetingChatEventListener::onFileReceived(ISDKFileReceiver* receiver)
{
}

void MeetingChatEventListener::onFileTransferProgress(SDKFileTransferInfo* info)
{
}
