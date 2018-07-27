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

#include "StdAfx.h"
#include "ViewManager.h"
#include "SetVectorDlg.h"
#include "ui_SetVectorDlg.h"
#include "Objects/EntityObject.h"
#include "Include/ITransformManipulator.h"
#include "MainWindow.h"

#include "MathConversion.h"

/////////////////////////////////////////////////////////////////////////////
// CSetVectorDlg dialog


CSetVectorDlg::CSetVectorDlg(QWidget* pParent /*=NULL*/)
    : QDialog(pParent)
    , m_ui(new Ui::SetVectorDlg)
{
    m_ui->setupUi(this);

    OnInitDialog();

    connect(m_ui->buttonOk, &QPushButton::clicked, this, &CSetVectorDlg::accept);
    connect(m_ui->buttonCancel, &QPushButton::clicked, this, &CSetVectorDlg::reject);
}

CSetVectorDlg::~CSetVectorDlg()
{
}

/////////////////////////////////////////////////////////////////////////////
// CSetVectorDlg message handlers


void CSetVectorDlg::OnInitDialog()
{
    QString editModeString;
    int emode = GetIEditor()->GetEditMode();

    if (emode == eEditModeMove)
    {
        editModeString = tr("Position");
    }
    else if (emode == eEditModeRotate)
    {
        editModeString = tr("Rotation");
    }
    else if (emode == eEditModeScale)
    {
        editModeString = tr("Scale");
    }

    m_ui->label->setText(tr("Enter %1 here:").arg(editModeString));

    currentVec = GetVectorFromEditor();
    m_ui->edit->setText(QStringLiteral("%1, %2, %3").arg(currentVec.x, 2, 'f', 2).arg(currentVec.y, 2, 'f', 2).arg(currentVec.z, 2, 'f', 2));
}

void CSetVectorDlg::accept()
{
    Vec3 newVec = GetVectorFromText();
    SetVector(newVec);

    if (GetIEditor()->GetEditMode() == eEditModeMove && currentVec.GetDistance(newVec) > 10.0f)
    {
        MainWindow::instance()->GetActionManager()->GetAction(ID_GOTO_SELECTED)->trigger();
    }
    QDialog::accept();
}

Vec3 CSetVectorDlg::GetVectorFromEditor()
{
    Vec3 v;
    int emode = GetIEditor()->GetEditMode();
    CBaseObject* obj = GetIEditor()->GetSelectedObject();
    RefCoordSys coordSys = GetIEditor()->GetReferenceCoordSys();
    bool bWorldSpace = GetIEditor()->GetReferenceCoordSys() == COORDS_WORLD;

    if (obj)
    {
        v = obj->GetWorldPos();
    }

    if (emode == eEditModeMove)
    {
        if (obj)
        {
            if (bWorldSpace)
            {
                v = obj->GetWorldTM().GetTranslation();
            }
            else
            {
                v = obj->GetPos();
            }
        }
    }
    if (emode == eEditModeRotate)
    {
        if (obj)
        {
			Quat qrot;
			if (bWorldSpace)
			{
				AffineParts ap;
				ap.SpectralDecompose(obj->GetWorldTM());
				qrot = ap.rot;
			}
			else
			{
				qrot = obj->GetRotation();
			}

			v = AZVec3ToLYVec3(AZ::ConvertQuaternionToEulerDegrees(LYQuaternionToAZQuaternion(qrot)));
        }
    }
    if (emode == eEditModeScale)
    {
        if (obj)
        {
            if (bWorldSpace)
            {
                AffineParts ap;
                ap.SpectralDecompose(obj->GetWorldTM());
                v = ap.scale;
            }
            else
            {
                v = obj->GetScale();
            }
        }
    }
    return v;
}

Vec3 CSetVectorDlg::GetVectorFromText()
{
    float vec[3] = {0, 0, 0};

    QString m_sPos = m_ui->edit->text();
    const QStringList parts = m_sPos.split(QRegularExpression("[\\s,;\\t]"), QString::SkipEmptyParts);
    for (int k = 0; k < parts.count(); ++k)
    {
        vec[k] = parts[k].toDouble();
    }

    return Vec3(vec[0], vec[1], vec[2]);
}

void CSetVectorDlg::SetVector(const Vec3& v)
{
    int emode = GetIEditor()->GetEditMode();
    if (emode != eEditModeMove && emode != eEditModeRotate && emode != eEditModeScale)
    {
        return;
    }

    int referenceCoordSys = GetIEditor()->GetReferenceCoordSys();

    CBaseObject* obj = GetIEditor()->GetSelectedObject();

    Matrix34 tm;
    AffineParts ap;
    if (obj)
    {
        tm = obj->GetWorldTM();
        ap.SpectralDecompose(tm);
    }

    if (emode == eEditModeMove)
    {
        if (obj)
        {
            if (referenceCoordSys == COORDS_WORLD)
            {
                tm.SetTranslation(v);
                obj->SetWorldTM(tm);
            }
            else
            {
                obj->SetPos(v);
            }
        }
    }
    if (emode == eEditModeRotate)
    {
        if (obj)
        {
			Quat qrot = AZQuaternionToLYQuaternion(AZ::ConvertEulerDegreesToQuaternion(LYVec3ToAZVec3(v)));
            if (referenceCoordSys == COORDS_WORLD)
            {
                tm = Matrix34::Create(ap.scale, qrot, ap.pos);
                obj->SetWorldTM(tm);
            }
            else
            {
                obj->SetRotation(qrot);
            }
        }
        else
        {
            GetIEditor()->GetSelection()->Rotate((Ang3)v, referenceCoordSys);
        }
    }
    if (emode == eEditModeScale)
    {
        if (v.x == 0 || v.y == 0 || v.z == 0)
        {
            return;
        }

        if (obj)
        {
            if (referenceCoordSys == COORDS_WORLD)
            {
                tm = Matrix34::Create(v, ap.rot, ap.pos);
                obj->SetWorldTM(tm);
            }
            else
            {
                obj->SetScale(v);
            }
        }
        else
        {
            GetIEditor()->GetSelection()->Scale(v, referenceCoordSys);
        }
    }
}

#include <SetVectorDlg.moc>