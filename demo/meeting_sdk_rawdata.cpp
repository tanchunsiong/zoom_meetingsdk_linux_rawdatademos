#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <glib.h>
#include <gtkmm.h>
#include <sstream>
#include <thread>
#include <sys/syscall.h>
#include <fstream>
#include <iosfwd>
#include <iostream>
#include <SDL2/SDL.h>

#include "zoom_sdk.h"
#include "auth_service_interface.h"
#include "meeting_sdk_util.h"
#include "init_auth_sdk_workflow.h"
#include "RegressionTestRawdataRender.h"
#include "RegressionTestRawdataDemo.h"
#include "meeting_sdk_rawdata.h"

MeetingsdkRawDataUI::MeetingsdkRawDataUI()
{

}

MeetingsdkRawDataUI::~MeetingsdkRawDataUI()
{

}

void setText_view(Gtk::TextView* text_view_p)
{
    text_view = text_view_p;
}