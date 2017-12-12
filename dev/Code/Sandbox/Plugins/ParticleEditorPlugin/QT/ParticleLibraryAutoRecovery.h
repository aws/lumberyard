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
#pragma once

#include <qstringlist.h>
#include <qobject.h>

class CBaseLibraryManager;
class QTimer;

class ParticleLibraryAutoRecovery
    : public QObject
{
public:
    explicit ParticleLibraryAutoRecovery();
    ~ParticleLibraryAutoRecovery();

    bool AttemptRecovery();
    void Discard();
private:
    void Save();
    void Load();
    QStringList GetBackupNames();
    bool HasExisting();
    void ResetTimer();

private:
    CBaseLibraryManager* m_pLibraryManager;
    QTimer* m_Timer;
};