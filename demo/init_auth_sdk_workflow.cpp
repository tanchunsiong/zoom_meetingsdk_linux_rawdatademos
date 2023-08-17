#include "init_auth_sdk_workflow.h"
#include <cstdlib>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>


CAuthSDKWorkFlow::CAuthSDKWorkFlow()
{
	
	m_pAuthService = NULL;
}

CAuthSDKWorkFlow::~CAuthSDKWorkFlow()
{
	m_pAuthService = NULL;
	SDKInterfaceWrap::GetInst().UnListenAuthServiceEvent(this);
}

ZOOM_SDK_NAMESPACE::SDKError CAuthSDKWorkFlow::Auth(ZOOM_SDK_NAMESPACE::AuthContext& param)
{
	if (NULL == m_pAuthService)
	{
		m_pAuthService = SDKInterfaceWrap::GetInst().GetAuthService();
		SDKInterfaceWrap::GetInst().ListenAuthServiceEvent(this);
	}
	if (m_pAuthService)
	{
		ZOOM_SDK_NAMESPACE::SDKError err = m_pAuthService->SDKAuth(param);
		return err;
	}

	return ZOOM_SDK_NAMESPACE::SDKERR_UNINITIALIZE;
}

ZOOM_SDK_NAMESPACE::SDKError CAuthSDKWorkFlow::Login(const zchar_t* sso_uri_protocol)
{
	if (NULL == m_pAuthService)
	{
		m_pAuthService = SDKInterfaceWrap::GetInst().GetAuthService();
	}
	if (m_pAuthService)
	{
		ZOOM_SDK_NAMESPACE::SDKError err = ZOOM_SDK_NAMESPACE::SDKERR_SUCCESS;
		err = m_pAuthService->SSOLoginWithWebUriProtocol(sso_uri_protocol);
		return err;
	}
	return ZOOM_SDK_NAMESPACE::SDKERR_UNINITIALIZE;
}

ZOOM_SDK_NAMESPACE::SDKError CAuthSDKWorkFlow::GetSSOUrl()
{
	if (NULL == m_pAuthService)
	{
		m_pAuthService = SDKInterfaceWrap::GetInst().GetAuthService();
	}
	if (m_pAuthService)
	{
		ZOOM_SDK_NAMESPACE::SDKError err = ZOOM_SDK_NAMESPACE::SDKERR_SUCCESS;
		const char* suceess = "success";
		const char* url;
		url = m_pAuthService->GenerateSSOLoginWebURL(suceess);
		pid_t pid = fork();
    
        if (pid == 0) {
        	// Child process
        	execlp("xdg-open", "xdg-open", url, NULL);
       		_exit(1);  // This line should not be reached unless there was an error
    	} else if (pid > 0) {
        	// Parent process
        	int status;
       		waitpid(pid, &status, 0);
    	} else {
       		// Fork failed
        	// Handle error
    	}
		return err;
	}
	return ZOOM_SDK_NAMESPACE::SDKERR_UNINITIALIZE;
}