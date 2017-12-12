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

// Description : Script Binding for MusicLogic


#ifndef CRYINCLUDE_CRYSCRIPTSYSTEM_SCRIPTBINDINGS_SCRIPTBIND_MUSICLOGIC_H
#define CRYINCLUDE_CRYSCRIPTSYSTEM_SCRIPTBINDINGS_SCRIPTBIND_MUSICLOGIC_H
#pragma once


#include <IScriptSystem.h>

namespace Audio {
    struct IMusicLogic;
}


class CScriptBind_MusicLogic
    : public CScriptableBase
{
public:
    CScriptBind_MusicLogic(IScriptSystem* const pScriptSystem, ISystem* const pSystem);
    virtual ~CScriptBind_MusicLogic();

    virtual void GetMemoryUsage(ICrySizer* pSizer) const
    {
        pSizer->AddObject(this, sizeof(*this));
    }

protected:
    //! <code>MusicLogic.SetEvent( eMusicEvent )</code>
    //!     <param name="eMusicEvent">identifier of a music event.</param>
    //! <description>
    //!     Sets an event in the music logic. Available music event ids are:
    //!     <pre>eMUSICLOGICEVENT_VEHICLE_ENTER
    //!     eMUSICLOGICEVENT_VEHICLE_LEAVE
    //!     eMUSICLOGICEVENT_WEAPON_MOUNT
    //!     eMUSICLOGICEVENT_WEAPON_UNMOUNT
    //!     eMUSICLOGICEVENT_ENEMY_SPOTTED
    //!     eMUSICLOGICEVENT_ENEMY_KILLED
    //!     eMUSICLOGICEVENT_ENEMY_HEADSHOT
    //!     eMUSICLOGICEVENT_ENEMY_OVERRUN
    //!     eMUSICLOGICEVENT_PLAYER_WOUNDED
    //!     eMUSICLOGICEVENT_MAX</pre>
    //! </description>
    int SetEvent(IFunctionHandler* pH, int eMusicEvent);

    //! <code>MusicLogic.StartLogic()</code>
    //! <description>Starts the music logic.</description>
    int StartLogic(IFunctionHandler* pH);

    //! <code>MusicLogic.StopLogic()</code>
    //! <description>Stops the music logic.</description>
    int StopLogic(IFunctionHandler* pH);

    //! <code>MusicLogic.SendEvent( eventName )</code>
    //! <description>Send an event to the music logic.</description>
    int SendEvent(IFunctionHandler* pH, const char* pEventName);

private:
    void RegisterGlobals();
    void RegisterMethods();

    IScriptSystem* m_pSS;
    Audio::IMusicLogic* m_pMusicLogic;
};

#endif // CRYINCLUDE_CRYSCRIPTSYSTEM_SCRIPTBINDINGS_SCRIPTBIND_MUSICLOGIC_H
