/*
	This source file is part of Rigs of Rods
	Copyright 2005-2012 Pierre-Michel Ricordel
	Copyright 2007-2012 Thomas Fischer
	Copyright 2013-2014 Petr Ohlidal

	For more information, see http://www.rigsofrods.com/

	Rigs of Rods is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License version 3, as
	published by the Free Software Foundation.

	Rigs of Rods is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Rigs of Rods. If not, see <http://www.gnu.org/licenses/>.
*/

/** 
	@file   ErrorUtils.cpp
	@author Thomas Fischer
	@date   3rd October 2009
*/

#include "ErrorUtils.h"

#include "Application.h"


#include <OgrePrerequisites.h>

#define _L

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
#include <windows.h>
#include <shlobj.h>
#include <shellapi.h> // for ShellExecuteW
#endif

#ifndef NOOGRE
#endif //NOOGRE

using namespace Ogre;

int ErrorUtils::ShowError(Ogre::UTFString title, Ogre::UTFString err)
{
	return ErrorUtils::ShowMsgBox(title, err, 0);
}

int ErrorUtils::ShowInfo(Ogre::UTFString title, Ogre::UTFString err)
{
	return ErrorUtils::ShowMsgBox(title, err, 1);
}

int ErrorUtils::ShowMsgBox(Ogre::UTFString title, Ogre::UTFString err, int type)
{
	// we might call the ErrorUtils::ShowMsgBox without having ogre created yet!
	//LOG("message box: " + title + ": " + err);
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
	int mtype = MB_ICONERROR;
	if (type == 1) mtype = MB_ICONINFORMATION;
	MessageBoxW( NULL, err.asWStr_c_str(), title.asWStr_c_str(), MB_OK | mtype | MB_TOPMOST);
#elif OGRE_PLATFORM == OGRE_PLATFORM_LINUX
	printf("\n\n%s: %s\n\n", title.asUTF8_c_str(), err.asUTF8_c_str());
#elif OGRE_PLATFORM == OGRE_PLATFORM_APPLE
	printf("\n\n%s: %s\n\n", title.asUTF8_c_str(), err.asUTF8_c_str());
	//CFOptionFlags flgs;
	//CFUserNotificationDisplayAlert(0, kCFUserNotificationStopAlertLevel, NULL, NULL, NULL, T("A network error occured"), T("Bad server port."), NULL, NULL, NULL, &flgs);
#endif
	return 0;
}

bool storederror = false;
Ogre::UTFString stored_title, stored_err, stored_url;
int ErrorUtils::ShowOgreWebError(Ogre::UTFString title, Ogre::UTFString err, Ogre::UTFString url)
{
#ifndef NOOGRE

	storederror = true;
	stored_title = title;
	stored_err = err;
	stored_url = url;
	
#if OGRE_PLATFORM == OGRE_PLATFORM_LINUX
	printf("\n\n%s: %s / url: %s\n\n", title.asUTF8_c_str(), err.asUTF8_c_str(), url.asUTF8_c_str());
#endif	
	return 0;
#else
	return ErrorUtils::ShowWebError(Ogre::UTFString("Rigs of Rods: ") + title, err, url);
#endif //NOOGRE
}

void ErrorUtils::ShowStoredOgreWebErrors()
{
	if (!storederror) return;
	ErrorUtils::ShowErrorDialog(stored_title, stored_err, stored_url);
}

int ErrorUtils::ShowErrorDialog(Ogre::UTFString title, Ogre::UTFString err, Ogre::UTFString url)
{
    // NO logmanager use, because it could be that its not initialized yet!
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
    UINT flags = MB_OK | MB_ICONERROR | MB_TOPMOST | MB_SYSTEMMODAL | MB_SETFOREGROUND;
    int Response = MessageBoxW( NULL, err.asWStr_c_str(), title.asWStr_c_str(), flags);
#else
    printf("\n\n%s: %s / url: %s\n\n", title.asUTF8_c_str(), err.asUTF8_c_str(), url.asUTF8_c_str());
#endif
    return 0;
}
