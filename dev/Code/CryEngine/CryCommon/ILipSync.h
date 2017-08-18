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

#ifndef CRYINCLUDE_CRYCOMMON_ILIPSYNC_H
#define CRYINCLUDE_CRYCOMMON_ILIPSYNC_H
#pragma once


struct CryCharMorphParams;

// callback interfaces
struct IDialogLoadSink
{
    // <interfuscator:shuffle>
    virtual ~IDialogLoadSink(){}
    virtual void OnDialogLoaded(struct ILipSync* pLipSync) = 0;
    virtual void OnDialogFailed(struct ILipSync* pLipSync) = 0;
    // </interfuscator:shuffle>
};

struct ILipSync
{
    // <interfuscator:shuffle>
    virtual ~ILipSync(){}
    virtual bool Init(ISystem* pSystem, IEntity* pEntity) = 0;                                            // initializes and prepares the character for lip-synching
    virtual void Release() = 0;                                                                                                           // releases all resources and deletes itself
    virtual bool LoadRandomExpressions(const char* pszExprScript, bool bRaiseError = true) = 0; // load expressions from script
    virtual bool UnloadRandomExpressions() = 0;                                                                           // release expressions
    // loads a dialog for later playback
    virtual bool LoadDialog(const char* pszFilename, int nSoundVolume, float fMinSoundRadius, float fMaxSoundRadius, float fClipDist, int nSoundFlags = 0, IScriptTable* pAITable = NULL) = 0;
    virtual bool UnloadDialog() = 0;                                                                                              // releases all resources
    virtual bool PlayDialog(bool bUnloadWhenDone = true) = 0;                                                   // plays a loaded dialog
    virtual bool StopDialog() = 0;                                                                                                    // stops (aborts) a dialog
    virtual bool DoExpression(const char* pszMorphTarget, CryCharMorphParams& MorphParams, bool bAnim = true) = 0;  // do a specific expression
    virtual bool StopExpression(const char* pszMorphTarget) = 0;                                      // stop animating the specified expression
    virtual bool Update(bool bAnimate = true) = 0;                                                                      // updates animation & stuff
    virtual void SetCallbackSink(IDialogLoadSink* pSink) = 0;                                             // set callback sink (see above)
    // </interfuscator:shuffle>
};

#endif // CRYINCLUDE_CRYCOMMON_ILIPSYNC_H
