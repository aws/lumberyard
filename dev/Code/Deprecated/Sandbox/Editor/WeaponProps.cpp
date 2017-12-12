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
#include "WeaponProps.h"

#include "CryEditDoc.h"
#include "Mission.h"

// CWeaponProps dialog

IMPLEMENT_DYNAMIC(CWeaponProps, CPropertyPage)
CWeaponProps::CWeaponProps()
	: CPropertyPage(CWeaponProps::IDD)
{
	m_title.LoadString(IDS_WEAPONS);
	m_psp.pszTitle = "Weapons";
#if _MFC_VER >= 0x0700
	m_psp.pszHeaderTitle = "Weapons";
	m_psp.pszHeaderSubTitle = "Weapons";
#endif //MFC7
}

CWeaponProps::~CWeaponProps()
{
}

void CWeaponProps::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_AVAILABLE, m_availableWeapons);
	DDX_Control(pDX, IDC_USED, m_usedWeapons);
}


BEGIN_MESSAGE_MAP(CWeaponProps, CPropertyPage)
	ON_BN_CLICKED(IDC_ADD, OnBnClickedAdd)
	ON_BN_CLICKED(IDC_REMOVE, OnBnClickedRemove)
END_MESSAGE_MAP()


// CWeaponProps message handlers

//////////////////////////////////////////////////////////////////////////
BOOL CWeaponProps::OnInitDialog()
{
	CPropertyPage::OnInitDialog();

	m_psp.pszTitle = "Weapons";
#if _MFC_VER >= 0x0700 // MFC 7.0 
	m_psp.pszHeaderTitle = "Weapons";
	m_psp.pszHeaderSubTitle = "Weapons";
#endif //MFC7

	CMission *mission = GetIEditor()->GetDocument()->GetCurrentMission();
	std::vector<CString> usedWeapons;
	mission->GetUsedWeapons( usedWeapons );

	for (int i = 0; i < usedWeapons.size(); i++)
	{
		m_usedWeapons.AddString(usedWeapons[i]);
		int id = m_availableWeapons.FindStringExact(-1,usedWeapons[i]);
		if (id != LB_ERR)
		{
			m_availableWeapons.DeleteString(id);
		}
	}


	CRect rc;
	GetDlgItem(IDC_AMMUNITION)->GetWindowRect( rc );
	ScreenToClient( rc );
	m_propWnd.Create( WS_CHILD|WS_VISIBLE|WS_BORDER,rc,this );

	XmlNodeRef templ = GetIEditor()->FindTemplate( "WeaponAmmo" );
	if (templ)
	{
		m_node = templ->clone();
		XmlNodeRef ammo = mission->GetWeaponsAmmo();
	
		XmlNodeRef xmlTemplate;
		xmlTemplate = m_node;
		CXmlTemplate::GetValues( xmlTemplate,ammo );
		
		m_propWnd.CreateItems( xmlTemplate );
		m_propWnd.SetUpdateCallback( functor(*this,&CWeaponProps::OnPropertyChanged) );
	}

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

//////////////////////////////////////////////////////////////////////////
void CWeaponProps::OnOK()
{
	CMission *mission = GetIEditor()->GetDocument()->GetCurrentMission();
	XmlNodeRef ammo = mission->GetWeaponsAmmo();
	if (m_node != 0 && ammo != 0)
		CXmlTemplate::SetValues( m_node,ammo );

	std::vector<CString> usedWeapons;
	usedWeapons.clear();
	for (int i = 0; i < m_usedWeapons.GetCount(); i++)
	{
		CString str;
		m_usedWeapons.GetText(i,str);
		usedWeapons.push_back(str);
	}
	mission->SetUsedWeapons(usedWeapons);

	CPropertyPage::OnOK();
}

//////////////////////////////////////////////////////////////////////////
BOOL CWeaponProps::OnQueryCancel()
{
	// TODO: Add your specialized code here and/or call the base class

	return CPropertyPage::OnQueryCancel();
}

//////////////////////////////////////////////////////////////////////////
void CWeaponProps::OnReset()
{
	// TODO: Add your specialized code here and/or call the base class

	CPropertyPage::OnReset();
}

//////////////////////////////////////////////////////////////////////////
void CWeaponProps::OnCancel()
{
	// TODO: Add your specialized code here and/or call the base class

	CPropertyPage::OnCancel();
}

//////////////////////////////////////////////////////////////////////////
void CWeaponProps::OnBnClickedAdd()
{
	int i = 0;
	while (i < m_availableWeapons.GetCount())
	{
		if (m_availableWeapons.GetSel(i))
		{
			CString str;
			m_availableWeapons.GetText( i,str );
			m_availableWeapons.DeleteString(i);
			m_usedWeapons.AddString(str);
			SetModified();
		}
		else
			i++;
	}
}

//////////////////////////////////////////////////////////////////////////
void CWeaponProps::OnBnClickedRemove()
{
	int i = 0;
	while (i < m_usedWeapons.GetCount())
	{
		if (m_usedWeapons.GetSel(i))
		{
			CString str;
			m_usedWeapons.GetText( i,str );
			m_usedWeapons.DeleteString(i);
			m_availableWeapons.AddString(str);
			SetModified();
		}
		else
			i++;
	}
}

//////////////////////////////////////////////////////////////////////////
void CWeaponProps::OnPropertyChanged( XmlNodeRef node )
{
	SetModified();
}