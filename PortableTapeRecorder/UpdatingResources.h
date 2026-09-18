//////////////////////////////////////
//UpdatingResources.h
//copyright 2006 by Ryadovoy
///////////////////////////////////////
#ifndef __UPDATING_RESOURCES_H_
#define __UPDATING_RESOURCES_H_
 
//#include "stdafx.h"
#include <vector>
#include <windows.h>
 
//error codes for ResourceUpdater class
#define ERR_CANNOT_OPEN_EXE_FILE        9004L
#define ERR_CANNOT_UPDATE_RESOURCE      9005L
#define ERR_CANNOT_LOAD_ICON_FROM_FILE  9006L
#define ERR_CANNOT_SAVE_CHANGES         9007L
///////////////////////////////////////
//class ResourceUpdater change icon resource in executable file.
//Creating resource from .ico file and set it as IDR_MAINFRAME icon
class ResourceUpdater{
public:
    void SetExecutableFileName(LPCSTR strExecutableFileName);
    void SetIconFileName(LPCSTR strIconFileName);
    LONG UpdateResources();
private:
    LPCSTR     m_strExecutableFileName;
    LPCSTR     m_strIconFileName;
};
 
#endif //__UPDATING_RESOURCES_H_