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

// Description : Special tag point for comment.


#include "StdAfx.h"
#include "TagComment.h"

//////////////////////////////////////////////////////////////////////////
// CBase implementation.
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
CTagComment::CTagComment()
{
    SetColor(QColor(255, 160, 0));

    m_entityClass = "Comment";
}

//////////////////////////////////////////////////////////////////////////
void CTagComment::Serialize(CObjectArchive& ar)
{
    CEntityObject::Serialize(ar);

    // 141022 - for backward compatibility, check "Comment" attribute in the xml node
    // , retrieve all old CTagComment data, and write it on the properties.
    XmlNodeRef xmlNode = ar.node;
    if (ar.bLoading)
    {
        // Loading
        QString commentText;
        xmlNode->getAttr("Comment", commentText);
        if (!commentText.isEmpty())
        {
            // set script properties with old CTagComment values.
            CVarBlock* pProperties = GetProperties();
            if (pProperties)
            {
                IVariable* pVar = pProperties->FindVariable("Text");
                if (pVar)
                {
                    pVar->Set(commentText);
                }

                float fSize = 0;
                bool bHidden = false;
                float fMaxDistance = 0;
                int nCharsPerLine = 0;
                bool bFixed = false;
                Vec3 diffuseColor = Vec3(0, 0, 0);

                pVar = pProperties->FindVariable("fSize");
                if (pVar && xmlNode->getAttr("Size", fSize))
                {
                    pVar->Set(fSize);
                }

                pVar = pProperties->FindVariable("bHidden");
                if (pVar && xmlNode->getAttr("Hidden", bHidden))
                {
                    pVar->Set(bHidden);
                }

                pVar = pProperties->FindVariable("fMaxDist");
                if (pVar && xmlNode->getAttr("MaxDistance", fMaxDistance))
                {
                    pVar->Set(fMaxDistance);
                }

                pVar = pProperties->FindVariable("nCharsPerLine");
                if (pVar && xmlNode->getAttr("CharsPerLine", nCharsPerLine))
                {
                    pVar->Set(nCharsPerLine);
                }

                pVar = pProperties->FindVariable("bFixed");
                if (pVar && xmlNode->getAttr("Fixed", bFixed))
                {
                    pVar->Set(bFixed);
                }

                pVar = pProperties->FindVariable("clrDiffuse");
                if (pVar && xmlNode->getAttr("Color", diffuseColor))
                {
                    pVar->Set(diffuseColor);
                }
            }
        }
    }
    else
    {
        // Saving.
        // Old "Comment" attribute will be gone once it's saved, and no special treatment is needed.
    }
}

#include <Objects/TagComment.moc>