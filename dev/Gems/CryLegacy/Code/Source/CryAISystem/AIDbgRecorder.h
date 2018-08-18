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

// Description : Simple text AI debugging event recorder
//
// Notes       : Really, this class is two separate debuggers - consider splitting
//               Move the access point to gAIEnv
//               Only creates the files on files on first logging - add some kind of init



#ifndef CRYINCLUDE_CRYAISYSTEM_AIDBGRECORDER_H
#define CRYINCLUDE_CRYAISYSTEM_AIDBGRECORDER_H
#pragma once

#ifdef CRYAISYSTEM_DEBUG

// Simple text debug recorder
// Completely independent from CAIRecorder, which is more sophisticated
class CAIDbgRecorder
{
public:

    CAIDbgRecorder() {};
    ~CAIDbgRecorder() {};

    bool  IsRecording(const IAIObject* pTarget, IAIRecordable::e_AIDbgEvent event) const;
    void  Record(const IAIObject* pTarget, IAIRecordable::e_AIDbgEvent event, const char* pString) const;

protected:
    void  InitFile() const;
    void  InitFileSecondary() const;

    void  LogString(const char* pString) const;
    void  LogStringSecondary(const char* pString) const;

    // Empty indicates currently unused
    // Has to be mutable right now because it changes on first logging
    mutable string m_sFile, m_sFileSecondary;
};

#endif //CRYAISYSTEM_DEBUG

#endif // CRYINCLUDE_CRYAISYSTEM_AIDBGRECORDER_H
