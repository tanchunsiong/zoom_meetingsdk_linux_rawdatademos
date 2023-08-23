#include "meeting_sdk_util.h"


class CAuthSDKWorkFlow : public ZOOM_SDK_NAMESPACE::IAuthServiceEvent
{
public:

 

	CAuthSDKWorkFlow();
	virtual ~CAuthSDKWorkFlow();

	ZOOM_SDK_NAMESPACE::SDKError Auth(ZOOM_SDK_NAMESPACE::AuthContext& param);
	ZOOM_SDK_NAMESPACE::SDKError Login(const zchar_t* sso_uri_protocol);
	ZOOM_SDK_NAMESPACE::SDKError GetSSOUrl();
	void Cleanup(){}
	//void SetUIEvent(CAuthSDKWorkFlowUIEvent* event_);
	virtual void onAuthenticationReturn(AuthResult ret){}
	virtual void onLoginReturnWithReason(ZOOM_SDK_NAMESPACE::LOGINSTATUS ret, ZOOM_SDK_NAMESPACE::IAccountInfo* pAccountInfo, ZOOM_SDK_NAMESPACE::LoginFailReason reason) {}
	virtual void onLogout(){};
	virtual void onZoomIdentityExpired(){};
	virtual void onZoomAuthIdentityExpired() {}
	// virtual void onNotificationServiceStatus(ZOOM_SDK_NAMESPACE::SDKNotificationServiceStatus status) {}
    
protected:
	//CAuthSDKWorkFlowUIEvent* m_pAuthUIEvent;
	ZOOM_SDK_NAMESPACE::IAuthService* m_pAuthService;
};