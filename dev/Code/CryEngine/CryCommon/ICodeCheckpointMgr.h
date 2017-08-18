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

// Description : Interface to track code checkpoint registration


#ifndef CRYINCLUDE_CRYCOMMON_ICODECHECKPOINTMGR_H
#define CRYINCLUDE_CRYCOMMON_ICODECHECKPOINTMGR_H
#pragma once


// Forward declarations
class CCodeCheckpoint;

// Not for release
#ifndef _RELEASE

    #define CODECHECKPOINT_ENABLED

#endif

/// Interface for a global manager of code coverage checkpoints. Must be thread safe.
struct ICodeCheckpointMgr
{
    // <interfuscator:shuffle>
    virtual ~ICodeCheckpointMgr(){}
    /// Used by code checkpoints to register themselves with the manager.
    virtual void RegisterCheckpoint(CCodeCheckpoint* pCheckpoint) = 0;

    /// Performs a (possibly) expensive lookup by name for a given checkpoint index. Returns index to blank record if not found.
    virtual size_t GetCheckpointIndex(const char* name) = 0;

    /// Performs a cheap lookup by direct index, will return NULL if checkpoint has not yet been registered.
    virtual const CCodeCheckpoint* GetCheckpoint(size_t checkpointIdx) const = 0;

    /// Performs cheap lookup for checkpoint name, will always return a valid name for a valid input index.
    virtual const char* GetCheckPointName(size_t checkpointIdx) const = 0;

    /// Returns the total number of checkpoints
    virtual size_t GetTotalCount() const = 0;

    /// Returns the total number of checkpoints that have been hit at least once.
    virtual size_t GetTotalEncountered() const = 0;

    /// Frees this instance from memory
    virtual void Release() = 0;
    // </interfuscator:shuffle>
};

/// Inline helper class used for registration and update of code checkpoint state.
class CCodeCheckpoint
{
public:
    CCodeCheckpoint(const char* name)
        : m_hitCount()
        , m_name(name)
    {
        ICodeCheckpointMgr* pMgr = gEnv->pCodeCheckpointMgr;

        if (pMgr)
        {
            pMgr->RegisterCheckpoint(this);
        }
    }

    ILINE void Hit()    { ++m_hitCount; }
    void Reset()            { m_hitCount = 0; }

    uint32 HitCount() const     { return m_hitCount; }
    const char* Name() const    { return m_name; }

private:
    uint32          m_hitCount;     /// How many times has this been hit (uint32 gives 49 days @ 1000 hits/second)
    const char* m_name;             /// Name for the checkpoint. Should be descriptive and unique but not function name.
};

/**
* The checkpoint macro, resolves to nothing on release builds.
* Best usage will include redefining the macro to prefix the name with the module.
* Do not use method names automatically - if code is moved, renamed, copy/pasted,
* it's more useful to preserve the label.
*/
#ifdef CODECHECKPOINT_ENABLED

    #define CODECHECKPOINT(x)                       \
    do                                              \
    {                                               \
        static CCodeCheckpoint s_checkpoint##x(#x); \
        s_checkpoint##x.Hit();                      \
    }                                               \
    while (false)

#else

    #define CODECHECKPOINT(x) ((void)(0))

#endif  // !defined CODECHECKPOINT_ENABLED

#endif // CRYINCLUDE_CRYCOMMON_ICODECHECKPOINTMGR_H
