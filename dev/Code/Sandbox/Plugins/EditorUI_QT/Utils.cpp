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

#include <EditorUI_QT_Precompiled.h>
#include "Utils.h"
#include "LibraryTreeViewItem.h"
#include "Particles/ParticleItem.h"
#include "BaseLibraryItem.h"
#include "AttributeItem.h"
#include "AttributeView.h"
#include "DockableLibraryPanel.h"

// Qt
#include <QLayout>
#include <QLayoutItem>

// EditoCore
#include <BaseLibraryManager.h>
#include <Util/UndoUtil.h>
#include "CurveEditorContent.h"
#include "FinalizingSpline.h"
#include <Include/IEditorParticleManager.h>


static std::vector<std::function<void()> > gInvokeQueue;

void Utils::invoke(std::function<void()> func)
{
    gInvokeQueue.push_back(func);
}

void Utils::runInvokeQueue()
{
    // swap so that we use a local version of the queue
    std::vector<std::function<void()> > q;
    q.swap(gInvokeQueue);

    // now run them
    for (auto & it : q)
    {
        it();
    }
}



bool IsPointInRect(const QPoint& p, const QRect& r)
{
    if (p.x() <= r.right() && p.x() >= r.left())
    {
        if (p.y() <= r.bottom() && p.y() >= r.top())
        {
            return true;
        }
    }
    return false;
}

void EditorSetTangent(ISplineInterpolator* spline, const int keyId, const SCurveEditorKey* key, bool isInTangent)
{
    ISplineInterpolator::ValueType tangentValue = { 0, 0, 0, 0 };
    if (isInTangent)
    {
        tangentValue[0] = key->m_inTangent.y / key->m_inTangent.x;
        spline->SetKeyInTangent(keyId, tangentValue);
    }
    else
    {
        tangentValue[0] = key->m_outTangent.y / key->m_outTangent.x;
        spline->SetKeyOutTangent(keyId, tangentValue);
    }
}
