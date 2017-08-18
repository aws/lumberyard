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

#include "stdafx.h"
#include "SelectAnimationDialog.h"
#include "ICryAnimation.h"

//////////////////////////////////////////////////////////////////////////
CSelectAnimationDialog::CSelectAnimationDialog(QWidget* pParent)
:	CGenericSelectItemDialog(pParent),
	m_pCharacterInstance(0)
{
	setWindowTitle(tr("Select Animation"));
}

//////////////////////////////////////////////////////////////////////////
/* virtual */ void
CSelectAnimationDialog::OnInitDialog()
{
	SetMode(eMODE_TREE);
	ShowDescription(true);
	__super::OnInitDialog();
}

//////////////////////////////////////////////////////////////////////////
/* virtual */ void
CSelectAnimationDialog::GetItems(std::vector<SItem>& outItems)
{
	IAnimationSet* pAnimSet = (m_pCharacterInstance ? m_pCharacterInstance->GetIAnimationSet() : 0);

	for (int animIndex = 0, animCount = (pAnimSet ? pAnimSet->GetAnimationCount() : 0); animIndex < animCount; ++animIndex)
	{
		const char* animName = (pAnimSet ? pAnimSet->GetNameByAnimID(animIndex) : 0);
		if (animName && *animName)
		{
			string animNameString = animName;
			string::size_type underscorePosition = animNameString.find("_");
			if (underscorePosition == 0)
				underscorePosition = animNameString.find("_", underscorePosition + 1);
			string category = (underscorePosition != string::npos ? animNameString.substr(0, underscorePosition) : "");

			SItem item;
			item.name = (!category.empty() ? category + "/" : "");
			item.name += animNameString;
			outItems.push_back(item);
		}
	}
}

//////////////////////////////////////////////////////////////////////////
/* virtual */ void CSelectAnimationDialog::ItemSelected()
{
}

//////////////////////////////////////////////////////////////////////////
void CSelectAnimationDialog::SetCharacterInstance(ICharacterInstance* pCharacterInstance)
{
	m_pCharacterInstance = pCharacterInstance;
}

//////////////////////////////////////////////////////////////////////////
QString CSelectAnimationDialog::GetSelectedItem()
{
	int slashPos = m_selectedItem.indexOf("/");
	return (slashPos != -1 ? m_selectedItem.right(m_selectedItem.length() - slashPos - 1) : m_selectedItem);
}

#include <SelectAnimationDialog.moc>
