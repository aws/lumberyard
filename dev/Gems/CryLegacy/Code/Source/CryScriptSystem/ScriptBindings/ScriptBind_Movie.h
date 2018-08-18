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

#ifndef CRYINCLUDE_CRYSCRIPTSYSTEM_SCRIPTBINDINGS_SCRIPTBIND_MOVIE_H
#define CRYINCLUDE_CRYSCRIPTSYSTEM_SCRIPTBINDINGS_SCRIPTBIND_MOVIE_H
#pragma once


#include "IScriptSystem.h"

//! <description>Interface to movie system.</description>
class CScriptBind_Movie
    : public CScriptableBase
{
public:
    CScriptBind_Movie(IScriptSystem* pScriptSystem, ISystem* pSystem);
    virtual ~CScriptBind_Movie();
    virtual void GetMemoryUsage(ICrySizer* pSizer) const
    {
        pSizer->AddObject(this, sizeof(*this));
    }

    //! <code>Movie.PlaySequence(sSequenceName)</code>
    //!     <param name="sSequenceName">Sequence name.</param>
    //! <description>Plays the specified sequence.</description>
    int PlaySequence(IFunctionHandler* pH, const char* sSequenceName);

    //! <code>Movie.StopSequence(sSequenceName)</code>
    //!     <param name="sSequenceName">Sequence name.</param>
    //! <description>Stops the specified sequence.</description>
    int StopSequence(IFunctionHandler* pH, const char* sSequenceName);

    //! <code>Movie.AbortSequence(sSequenceName)</code>
    //!     <param name="sSequenceName">Sequence name.</param>
    //! <description>Aborts the specified sequence.</description>
    int AbortSequence(IFunctionHandler* pH, const char* sSequenceName);

    //! <code>Movie.StopAllSequences()</code>
    //! <description>Stops all the video sequences.</description>
    int StopAllSequences(IFunctionHandler* pH);

    //! <code>Movie.StopAllCutScenes()</code>
    //! <description>Stops all the cut scenes.</description>
    int StopAllCutScenes(IFunctionHandler* pH);

    //! <code>Movie.PauseSequences()</code>
    //! <description>Pauses all the sequences.</description>
    int PauseSequences(IFunctionHandler* pH);

    //! <code>Movie.ResumeSequences()</code>
    //! <description>Resume all the sequences.</description>
    int ResumeSequences(IFunctionHandler* pH);
private:
    ISystem* m_pSystem;
    IMovieSystem* m_pMovieSystem;
};



#endif // CRYINCLUDE_CRYSCRIPTSYSTEM_SCRIPTBINDINGS_SCRIPTBIND_MOVIE_H
