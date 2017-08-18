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

#pragma once
#ifndef CRYINCLUDE_EDITOR_MISSIONSCRIPT_H
#define CRYINCLUDE_EDITOR_MISSIONSCRIPT_H

class CMissionScript
{
private:
    QString m_sFilename;
    QStringList m_methods;
    QStringList m_events;
public:
    CMissionScript();
    virtual ~CMissionScript();

    void SetScriptFile(const QString& file);
    bool Load();
    void Edit();

    //! Call on reset of mission.
    void OnReset();

    //! Get Lua filename.
    const QString& GetFilename() { return m_sFilename; }

    //////////////////////////////////////////////////////////////////////////
    int GetMethodCount() { return m_methods.size(); }
    const QString& GetMethod(int i) { return m_methods[i]; }

    //////////////////////////////////////////////////////////////////////////
    int GetEventCount() { return m_events.size(); }
    const QString& GetEvent(int i) { return m_events[i]; }
};
#endif // CRYINCLUDE_EDITOR_MISSIONSCRIPT_H
