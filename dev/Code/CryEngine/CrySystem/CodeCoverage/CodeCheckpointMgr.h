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

// Description : A central class to track code checkpoint registration


#ifndef CRYINCLUDE_CRYSYSTEM_CODECOVERAGE_CODECHECKPOINTMGR_H
#define CRYINCLUDE_CRYSYSTEM_CODECOVERAGE_CODECHECKPOINTMGR_H
#pragma once


#include <ICodeCheckpointMgr.h>

/// The global manager of code coverage checkpoints. Must be thread safe.
class CCodeCheckpointMgr
    : public ICodeCheckpointMgr
{
public:

    CCodeCheckpointMgr();
    virtual ~CCodeCheckpointMgr();

    // ICodeCheckpointMgr implementation

    /// Used by code checkpoints to register themselves with the manager.
    virtual void RegisterCheckpoint(CCodeCheckpoint* pCheckpoint);

    /// Looks up a checkpoint by name. If not found it bookmarks that record and returns an index to it.
    virtual size_t GetCheckpointIndex(const char* name);

    /// Performs a cheap lookup by index, will return NULL if checkpoint has not yet been registered.
    virtual const CCodeCheckpoint* GetCheckpoint(size_t checkpointIdx) const;

    /// Performs cheap lookup for checkpoint name, will always return a valid name for a valid input index.
    virtual const char* GetCheckPointName(size_t checkpointIdx) const;

    /// Returns the total number of checkpoints
    virtual size_t GetTotalCount() const;

    /// Returns the total number of checkpoints that have been hit at least once.
    virtual size_t GetTotalEncountered() const;

    /// Frees this instance from memory
    virtual void Release();

private:
    /// Returns index for record matching name or ~0 if not found.
    size_t FindRecordByName(const char* name) const;

private:
    struct CheckpointRecord
    {
        CCodeCheckpoint*    m_pCheckpoint;  /// The checkpoint if registered otherwise NULL
        const char*             m_name;                 /// The name of the checkpoint (owned if m_pCheckpoint is NULL)
    };

    mutable CryCriticalSection m_critSection;   /// Used to protect access to m_checkpoints from multiple threads

    typedef std::vector<CheckpointRecord> TCheckpointVector;
    TCheckpointVector m_checkpoints;                    /// Set of checkpoints either registered or requested via indices
};

#endif // CRYINCLUDE_CRYSYSTEM_CODECOVERAGE_CODECHECKPOINTMGR_H
