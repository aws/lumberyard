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

#include "UICommon.h"
#include "Tools/Shape/CubeEditor.h"

class QPushButton;
class QComboBox;
class QCheckBox;

class CubeEditorPanel
    : public QWidget
    , public ICubeEditorPanel
{
public:
    CubeEditorPanel(CubeEditor* pCubeEditor);

    void Done() override;
    BrushFloat GetCubeSize() const override;
    bool IsSidesMerged() const override;
    bool IsAddButtonChecked() const override;
    bool IsRemoveButtonChecked() const override;
    bool IsPaintButtonChecked() const override;
    int  GetSubMatID() const override;
    void SetSubMatID(int nID) const override;
    bool SetMaterial(CMaterial* pMaterial) override;
    void SelectPrevBrush() override;
    void SelectNextBrush() override;
    void UpdateSubMaterialComboBox() override;

protected:

    void SelectEditMode(CD::ECubeEditorMode editMode);

    CD::ECubeEditorMode m_CheckedButton;

    CubeEditor* m_pCubeEditor;
    std::vector<BrushFloat> m_CubeSizeList;

    QPushButton* m_pAddButton;
    QPushButton* m_pRemoveButton;
    QPushButton* m_pPaintButton;

    QComboBox* m_pBrushSizeComboBox;
    QComboBox* m_pSubMatIdComboBox;
    QCheckBox* m_pMergeSideCheckBox;
};