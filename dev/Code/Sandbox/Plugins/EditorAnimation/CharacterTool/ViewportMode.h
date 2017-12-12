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

#include "../../EditorCommon/QViewportConsumer.h"
#include <vector>

namespace Serialization
{
    class IArchive;
    struct SStruct;
}

struct ICharacterInstance;
class QToolBar;
class QPropertyTree;

namespace CharacterTool
{
    using std::vector;
    class CharacterToolForm;
    class CharacterDocument;
    class TransformPanel;
    struct System;
    struct ExplorerEntry;

    struct SModeContext
    {
        System* system;
        CharacterToolForm* window;
        CharacterDocument* document;
        ICharacterInstance* character;
        TransformPanel* transformPanel;
        QToolBar* toolbar;
        std::vector<QPropertyTree*> layerPropertyTrees;
    };

    struct IViewportMode
        : public QViewportConsumer
    {
        virtual ~IViewportMode() {}
        virtual void Serialize(Serialization::IArchive& ar) {}
        virtual void EnterMode(const SModeContext& context) {}
        virtual void LeaveMode() {}

        virtual void GetPropertySelection(vector<const void*>* selectedItems) const {}
        virtual void SetPropertySelection(const vector<const void*>& items) {}
    };
}
