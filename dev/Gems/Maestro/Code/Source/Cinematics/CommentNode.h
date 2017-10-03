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

#ifndef CRYINCLUDE_CRYMOVIE_COMMENTNODE_H
#define CRYINCLUDE_CRYMOVIE_COMMENTNODE_H
#pragma once


#include "AnimNode.h"

class CCommentContext;

class CCommentNode
    : public CAnimNode
{
public:
    CCommentNode(const int id);
    static void Initialize();

    //-----------------------------------------------------------------------------
    //!
    virtual EAnimNodeType GetType() const { return eAnimNodeType_Comment; }

    //-----------------------------------------------------------------------------
    //! Overrides from CAnimNode
    virtual void Animate(SAnimContext& ac);

    virtual void CreateDefaultTracks();

    virtual void OnReset();

    virtual void Activate(bool bActivate);

    virtual void Serialize(XmlNodeRef& xmlNode, bool bLoading, bool bLoadEmptyTracks);

    //-----------------------------------------------------------------------------
    //! Overrides from IAnimNode
    virtual unsigned int GetParamCount() const;
    virtual CAnimParamType GetParamType(unsigned int nIndex) const;

protected:
    virtual bool GetParamInfoFromType(const CAnimParamType& paramId, SParamInfo& info) const;
};

#endif // CRYINCLUDE_CRYMOVIE_COMMENTNODE_H
