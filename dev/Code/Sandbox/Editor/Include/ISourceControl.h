/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#ifndef CRYINCLUDE_EDITOR_INCLUDE_ISOURCECONTROL_H
#define CRYINCLUDE_EDITOR_INCLUDE_ISOURCECONTROL_H
#pragma once

#include <map>

#include <AzToolsFramework/SourceControl/SourceControlAPI.h>

#include "IEditorClassFactory.h" 

// Source control status of item.
enum ESccFileAttributes
{
    SCC_FILE_ATTRIBUTE_INVALID      = 0x0000, // File is not found.
    SCC_FILE_ATTRIBUTE_NORMAL       = 0x0001, // Normal file on disk.
    SCC_FILE_ATTRIBUTE_READONLY     = 0x0002, // Read only files cannot be modified at all, either read only file not under source control or file in packfile.
    SCC_FILE_ATTRIBUTE_INPAK        = 0x0004, // File is inside pack file.
    SCC_FILE_ATTRIBUTE_MANAGED      = 0x0008, // File is managed under source control.
    SCC_FILE_ATTRIBUTE_CHECKEDOUT   = 0x0010, // File is under source control and is checked out.
    SCC_FILE_ATTRIBUTE_BYANOTHER    = 0x0020, // File is under source control and is checked out by another user.
    SCC_FILE_ATTRIBUTE_FOLDER       = 0x0040, // Managed folder.
    SCC_FILE_ATTRIBUTE_LOCKEDBYANOTHER = 0x0080, // File is under source control and is checked out and locked by another user.
    SCC_FILE_ATTRIBUTE_NOTATHEAD    = 0x0100, // File is under source control and is not the latest version of the file
    SCC_FILE_ATTRIBUTE_ADD          = 0x0200, // File is under source control and is marked for add
    SCC_FILE_ATTRIBUTE_MOVED        = 0x0400, // File is under source control and is marked for move/add
};

// Source control flags
enum ESccFlags
{
    NONE                      = 0,
    GETLATEST_REVERT          = 1 << 0, // don't revert when perform GetLatestVersion() on opened files
    GETLATEST_ONLY_CHECK      = 1 << 1, // don't actually get latest version of the file, check if scc has more recent version
    ADD_WITHOUT_SUBMIT        = 1 << 2, // add the files to the default changelist
    ADD_CHANGELIST            = 1 << 3, // add a changelist with the description
    ADD_AS_BINARY_FILE        = 1 << 4, // add the files as binary files
    DELETE_WITHOUT_SUBMIT     = 1 << 5, // mark for delete and don't submit
    RENAME_WITHOUT_SUBMIT     = 1 << 6, // mark for move and don't submit
    RENAME_WITHOUT_REVERT     = 1 << 7, // mark for move and don't revert changes
    DELETE_MOVED_FILES        = 1 << 8, // make sure moved files are not left on disk
};

// Lock status of an item in source control
enum ESccLockStatus
{
    SCC_LOCK_STATUS_UNLOCKED,
    SCC_LOCK_STATUS_LOCKED_BY_OTHERS,
    SCC_LOCK_STATUS_LOCKED_BY_US,
};

//////////////////////////////////////////////////////////////////////////
// Description
//    This interface provide access to the source control functionality.
//////////////////////////////////////////////////////////////////////////
struct ISourceControl
    : public IUnknown
{
    DEFINE_UUID(0x1D391E8C, 0xA124, 0x46bb, 0x80, 0x8D, 0x9B, 0xCA, 0x15, 0x5B, 0xCA, 0xFD)

    // Description:
    //    Returns attributes of the file.
    // Return:
    //    Combination of flags from ESccFileAttributes enumeration.
    virtual uint32 GetFileAttributes(const char* filename) = 0;

    // Source Control State
    enum ConnectivityState
    {
        Connected = 0,
        BadConfiguration,
        Disconnected_Retrying,
        Disconnected,
    };

    using SourceControlState = AzToolsFramework::SourceControlState;

    typedef std::function<void(ConnectivityState)> connectivityChangedFunction;

    virtual bool DoesChangeListExist(const char* pDesc, char* changeid, int nLen) = 0;
    virtual bool CreateChangeList(const char* pDesc, char* changeid, int nLen) = 0;
    virtual bool Add(const char* filename, const char* desc = 0, int nFlags = 0, char* changelistId = NULL) = 0;
    virtual bool CheckOut(const char* filename, int nFlags = 0, char* changelistId = NULL) = 0;
    virtual bool Rename(const char* filename, const char* newfilename, const char* desc = 0, int nFlags = 0) = 0;
    virtual bool GetLatestVersion(const char* filename, int nFlags = 0) = 0;
    //function to register callback
    virtual uint RegisterCallback(connectivityChangedFunction func) = 0;
    //function to unregister callback
    virtual void UnRegisterCallback(uint handle) = 0;
    //function to enable/disable source control
    virtual void SetSourceControlState(SourceControlState state) = 0;
    virtual ConnectivityState GetConnectivityState() = 0;

    // GetOtherUser - Get other user name who edit file filename
    // outUser: output char buffer, provided by user, 64 - recommended size of this buffer
    // nOutUserSize: size of outUser buffer, 64 is recommended
    virtual bool GetOtherUser(const char* filename, char* outUser, int nOutUserSize) = 0;

    // Show settings dialog
    virtual void ShowSettings() = 0;

    //////////////////////////////////////////////////////////////////////////
    // IUnknown
    //////////////////////////////////////////////////////////////////////////
    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject) { return E_NOINTERFACE; };
    virtual ULONG STDMETHODCALLTYPE AddRef() { return 0; };
    virtual ULONG STDMETHODCALLTYPE Release() { return 0; };
    //////////////////////////////////////////////////////////////////////////
};

#endif // CRYINCLUDE_EDITOR_INCLUDE_ISOURCECONTROL_H

