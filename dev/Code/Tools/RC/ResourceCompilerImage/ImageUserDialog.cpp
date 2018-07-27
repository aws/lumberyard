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
#include "ImageUserDialog.h"        // CImageUserDialog
#include "ImageObject.h"            // ImageObject
#include "resource.h"               // IDD_ ...
#include <assert.h>                 // assert()
#include "ImageCompiler.h"          // CImageCompiler
#include "ImagePreview.h"           // EPreviewMode
#include "RelativeMouseInput.h"     // CRelativeMouseInput
#include "IRCLog.h"                 // IRCLog
#include "tools.h"                  // CenterWindow()

#include "../ResourceCompiler/FunctionThread.h"

#include <vector>

#include <QMessageBox>
#include <QMetaProperty>
#include <QMouseEvent>
#include <QPainter>
#include <QThread>
#include <QWheelEvent>

#include "ui_ImageUserDialog.h"
#include "ui_ChooseResolutionDialog.h"

static const char* const s_pComboboxSeparator = "------------------------------------------------";

namespace
{
    class SimpleTextConfigSink
        : public IConfigSink
    {
    public:
        string m_text;

        virtual ~SimpleTextConfigSink()
        {
        }

        virtual void SetKeyValue(EConfigPriority ePri, const char* key, const char* value)
        {
            m_text += string(key) + "=" + value + "\r\n";
        }
    };

    class FileMaskConfigSink
        : public IConfigSink
    {
    public:
        std::vector<string> m_fileMasks;

        FileMaskConfigSink()
        {
        }

        virtual ~FileMaskConfigSink()
        {
        }

        virtual void SetKeyValue(EConfigPriority ePri, const char* key, const char* value)
        {
            if (key && StringHelpers::EqualsIgnoreCase(key, "filemasks"))
            {
                m_fileMasks.clear();

                StringHelpers::Split(value, ",", false, m_fileMasks);
            }
        }
    };
}

static volatile bool s_bDialogInitialized;

namespace Reduce
{
    enum
    {
        kMinReduce = -2
    };
    enum
    {
        kMaxReduce = 5
    };

    struct PlatformData
    {
        int platform;
        string platformName;
        string formatName;
        int reduceValue;
        uint32 width;
        uint32 height;
        bool bMark;

        PlatformData()
            : platform(-1)
            , platformName("?")
            , formatName("?")
            , reduceValue(0)
            , width(0)
            , height(0)
            , bMark(false)
        {
        }

        void set(
            int a_platform,
            const char* a_pPlatformName,
            int a_reduceValue,
            bool a_bMark)
        {
            if (!a_pPlatformName || !a_pPlatformName[0])
            {
                assert(0 && "Bad platform data");
                return;
            }
            platform = a_platform;
            platformName = a_pPlatformName;
            formatName = "?";
            reduceValue = a_reduceValue;
            width = 0;
            height = 0;
            bMark = a_bMark;
        }
    };

    static std::vector<PlatformData> s_platforms;

    static CImageCompiler* s_pImageCompiler = 0;



    static void sortArray(std::vector<PlatformData>& platforms)
    {
        struct PlatformDataSorter
        {
            bool operator()(const PlatformData& l, const PlatformData& r) const
            {
                // Let's keep order of platforms same as in rc.ini
                if (l.platform != r.platform)
                {
                    return l.platform < r.platform;
                }
                return l.platformName < r.platformName;
            }
        };

        if (platforms.size() > 1)
        {
            std::sort(platforms.begin(), platforms.end(), PlatformDataSorter());
        }
    }


    static void fillListbox(
        QWidget* hTabResolution,
        const std::vector<PlatformData>& platforms)
    {
        string tmp;

        auto hwnd = hTabResolution->findChild<QTreeWidget*>("resolutionList");
        hwnd->clear();

        for (size_t i = 0; i < platforms.size(); ++i)
        {
            const PlatformData& p = platforms[i];

            auto lvi = new QTreeWidgetItem;
            lvi->setIcon(0, QPixmap(QStringLiteral(":/reduce%1.png").arg(p.reduceValue)));
            lvi->setText(1, QString::number(p.reduceValue));
            lvi->setText(2, p.platformName.c_str());
            lvi->setText(3, QString::number(p.width));
            lvi->setText(4, QString::number(p.height));
            lvi->setText(5, p.formatName.c_str());

            hwnd->addTopLevelItem(lvi);
            lvi->setSelected(p.bMark);
        }
    }


    static int grabListboxSelection(
        QWidget* hTabResolution,
        std::vector<PlatformData>& platforms)
    {
        auto hwnd = hTabResolution->findChild<QAbstractItemView*>("resolutionList");

        int selCount = 0;
        for (int i = 0; i < (int)platforms.size(); ++i)
        {
            const QModelIndex index = hwnd->model()->index(i, 0);
            const bool bSelected = hwnd->selectionModel()->isSelected(index);
            if (bSelected)
            {
                ++selCount;
            }
            platforms[i].bMark = bSelected;
        }

        return selCount;
    }


    static void convertMapToArray(
        const CImageCompiler* pImageCompiler,
        const CImageProperties::ReduceMap& reduceMap,
        std::vector<PlatformData>& platforms)
    {
        {
            std::vector<PlatformData> tmp;
            tmp.reserve(reduceMap.size());

            for (CImageProperties::ReduceMap::const_iterator it = reduceMap.begin(); it != reduceMap.end(); ++it)
            {
                const CImageProperties::ReduceItem& ri = it->second;
                PlatformData pd;
                pd.set(ri.platformIndex, ri.platformName, ri.value, false);
                tmp.push_back(pd);
            }
            sortArray(tmp);

            // let's try to keep existing selection (.bMark)
            platforms.resize(tmp.size());
            for (size_t i = 0; i < platforms.size(); ++i)
            {
                tmp[i].bMark = platforms[i].bMark;
            }

            tmp.swap(platforms);
        }

        for (size_t i = 0; i < platforms.size(); ++i)
        {
            PlatformData& p = platforms[i];

            if (p.platform >= 0)
            {
                // It's a platform from list of known platforms

                EPixelFormat finalFormat;
                bool bFinalCubemap;
                uint32 dwFinalMipCount;
                uint32 dwFinalReduce;

                pImageCompiler->GetFinalImageInfo(p.platform, finalFormat, bFinalCubemap, dwFinalMipCount, dwFinalReduce, p.width, p.height);

                if (finalFormat < 0 || finalFormat >= ePixelFormat_Count)
                {
                    assert(0);
                    p.formatName = "?format?";
                }
                else
                {
                    const PixelFormatInfo* const pFormatInfo = CPixelFormats::GetPixelFormatInfo(finalFormat);
                    if (!pFormatInfo || !pFormatInfo->szName)
                    {
                        assert(0);
                        p.formatName = "?format?";
                    }
                    else
                    {
                        p.formatName = pFormatInfo->szName;
                    }
                }
            }
        }
    }


    static void convertArrayToMap(
        const CImageCompiler* pImageCompiler,
        const std::vector<PlatformData>& platforms,
        CImageProperties::ReduceMap& reduceMap)
    {
        string s;
        string tmp;
        for (int i = 0; i < (int)platforms.size(); ++i)
        {
            tmp = StringHelpers::Format("%s%s:%i", (i ? "," : ""), platforms[i].platformName.c_str(), platforms[i].reduceValue);
            s += tmp;
        }

        pImageCompiler->m_Props.ConvertReduceStringToMap(s, reduceMap);
    }


    static void applyChangesAndFillListbox(
        CImageCompiler* pImageCompiler,
        QWidget* hTabResolution,
        std::vector<PlatformData>& platforms)
    {
        CImageProperties::ReduceMap reduceMap;
        Reduce::convertArrayToMap(pImageCompiler, platforms, reduceMap);

        pImageCompiler->m_Props.SetReduceResolutionFile(reduceMap);

        // Recompute resolutions
        Reduce::convertMapToArray(pImageCompiler, reduceMap, platforms);

        Reduce::fillListbox(hTabResolution, platforms);


    }
} // namespace Reduce

static void critical(QWidget* parent, const QString& title, const QString& text)
{
    MainThreadExecutor::execute([&]() { QMessageBox::critical(parent, title, text); });
}

static void information(QWidget* parent, const QString& title, const QString& text)
{
    MainThreadExecutor::execute([&]() { QMessageBox::information(parent, title, text); });
}

// Class name for this application's window class.
#define WINDOW_CLASSNAME      "UserDialog"

// constructor
CImageUserDialog::CImageUserDialog()
    : m_iPreviewWidth(256)
    , m_iPreviewHeight(256)
    , m_quitRequest(QuitRequestNone)
    , m_painter(nullptr)
{
    m_pImageCompiler = 0;
    m_hTab_Resolution = 0;
    m_hWindow = 0;
    m_bDialogIsUpdating = false;
    m_hWindowThread = 0;
    m_bShowAllPresets = false;
}

// destructor
CImageUserDialog::~CImageUserDialog()
{
    assert(!m_pImageCompiler);          // should be 0 outside of DoModal call
}


bool CImageUserDialog::DoModal(CImageCompiler* inpImageCompiler)
{
    m_pImageCompiler = inpImageCompiler;
    m_eWorkerAction = WorkerActionNone;

    s_bDialogInitialized = false;

    // Start the thread that will control the window and handle the message loop.
    m_hWindowThread = FunctionThread::CreateThread(
            0,                                      //LPSECURITY_ATTRIBUTES lpThreadAttributes,
            0,                                      //SIZE_T dwStackSize,
            ThreadStart_Static,     //LPTHREAD_START_ROUTINE lpStartAddress,
            this,                                   //LPVOID lpParameter,
            0,                                      //DWORD dwCreationFlags,
            0);                                     //LPDWORD lpThreadId);

    while (!s_bDialogInitialized)
    {
        Sleep(1);
    }

    bool bSuccess = false;

    // Loop until the dialog thread has exited.
    for (;; )
    {
        // Check whether the ui thread is finished - if so, we can return.
        {
            if (!m_hWindowThread->isRunning())
            {
                bSuccess = (m_hWindowThread->returnCode() == 0);
                break;
            }
        }

        // Check whether we need to update the preview.
        WorkerAction eAction = CheckAndResetWorkerAction();

        switch (eAction)
        {
        case WorkerActionNone:
            break;

        case WorkerActionUpdatePreviewOnly:
        {
            GetDataFromDialog();
            //SetDataToDialog();        // show settings in dialog

            UpdatePreview(false);
            TriggerRepaint();

            break;
        }

        case WorkerActionUpdatePreview:
        {
            //clear progress before bar is drawn
            m_pImageCompiler->m_Progress.Start();

            {
                LOCK_MONITOR();

                m_PreviewData.bIsUpdating = true;
                if (m_PreviewData.bDelayUpdate)
                {
                    TriggerRepaint();
                    continue;
                }
            }

            GetDataFromDialog();
            GetInputExtentAndMaximumZoom();
            SetDataToDialog();          // show settings in dialog
            GetDataFromDialog();

            QPainter painter(&m_image);
            m_painter = &painter;
            Draw(&painter);
            MainThreadExecutor::execute([&]() { m_ui->widget->setPixmap(QPixmap::fromImage(m_image)); });

            UpdatePreview(true);

            {
                LOCK_MONITOR();
                m_PreviewData.bIsUpdating = false;
            }

            TriggerRepaint();
            m_painter = nullptr;
            break;
        }

        case WorkerActionGenerateOutputAndQuit:
        case WorkerActionGenerateOutput:
        {
            //Set the platform based on what the user has selected from the platform list. This will allow the user to generate and preview
            //texture compressions for any platform
            const int selCount = Reduce::grabListboxSelection(m_hTab_Resolution, Reduce::s_platforms);
            if (selCount > 0)
            {
                for (int i = 0; i < static_cast<int>(Reduce::s_platforms.size()); ++i)
                {
                    if (Reduce::s_platforms[i].bMark)
                    {
                        m_pImageCompiler->GetConvertContext()->SetPlatformIndex(i);
                    }
                }
            }

            GetDataFromDialog();
            SetDataToDialog(true);                                  // might need to update preset in UI
            GetDataFromDialog();

            bool bRet = m_pImageCompiler->RunWithProperties(true);

            if (bRet)
            {
                if (!m_pImageCompiler->UpdateAndSaveConfig())
                {
                    critical(m_hWindow, "ResourceCompiler Image Error", "Error while saving the config file (write protected?)");
                }
                else
                {
                    RCLog("Output was generated.");
                    if (eAction == WorkerActionGenerateOutputAndQuit)
                    {
                        m_quitRequest = QuitRequestNormal;
                    }
                }
            }
            else
            {
                RCLog("No Output was generated.");
            }

            //Update the progress bar and the details of the compressed texture preview window
            TriggerRepaint();
            TriggerUpdatePreview(false);
            break;
        }

        case WorkerActionSaveConfigAndQuit:
        case WorkerActionSaveConfig:
        {
            GetDataFromDialog();

            if (!m_pImageCompiler->UpdateAndSaveConfig())
            {
                critical(m_hWindow, "ResourceCompiler Image Error", "Error while saving the config file (write protected?)");
            }
            else
            {
                if (eAction == WorkerActionSaveConfigAndQuit)
                {
                    m_quitRequest = QuitRequestNormal;
                }
                else
                {
                    information(m_hWindow, "ResourceCompiler Image", "Config saved");
                }
            }
            break;
        }

        case WorkerActionPaint:
        {
            QPainter painter(&m_image);
            m_painter = &painter;
            Draw(&painter);
            m_painter = nullptr;
            MainThreadExecutor::execute([&]() { m_ui->widget->setPixmap(QPixmap::fromImage(m_image)); });
            break;
        }

        case WorkerActionQuit:
        {
            m_quitRequest = QuitRequestUserCancel;
        }
        break;
        }

        // Wait some time to avoid hogging the CPU.
        Sleep(100);
    }

    m_pImageCompiler = 0;

    return bSuccess;
}


void CImageUserDialog::CreateDialogItems()
{
    LOCK_MONITOR();

    // tab control
    {
        m_hTab_Resolution = m_ui->propTab->widget(0);

        // sub windows
        m_BumpToNormalPanel.m_pDlg = this;
        m_BumpToNormalPanel.InitDialog(m_ui->propTab->widget(2), false);
        m_BumpToNormalPanel.m_bumpStrengthValue.AddListener(this);
        m_BumpToNormalPanel.m_bumpBlurValue.AddListener(this);
        m_BumpToNormalPanel.m_bumpInvertValue.AddListener(this);

        m_AlphaAsBumpPanel.m_pDlg = this;
        m_AlphaAsBumpPanel.InitDialog(m_ui->propTab->widget(3), false);
        m_AlphaAsBumpPanel.m_bumpStrengthValue.AddListener(this);
        m_AlphaAsBumpPanel.m_bumpBlurValue.AddListener(this);
        m_AlphaAsBumpPanel.m_bumpInvertValue.AddListener(this);

        // mipmaps
        {
            m_MIPControlPanel.InitDialog(m_ui->propTab->widget(1), false);

            for (int i = 0; i < CUIMIPControlPanel::NUM_CONTROLLED_MIP_MAPS; ++i)
            {
                m_MIPControlPanel.m_MIPAlphaControl[i].GetDocument().AddListener(this);
            }
            m_MIPControlPanel.m_MIPBlurringControl.GetDocument().AddListener(this);
        }

        {
            QTreeWidget* hwnd = m_ui->resolutionList;

            hwnd->setColumnWidth(0, 18);
            hwnd->setColumnWidth(1, 23);
            hwnd->setColumnWidth(2, 51);
            hwnd->setColumnWidth(3, 40);
            hwnd->setColumnWidth(4, 40);
            hwnd->setColumnWidth(5, 73);
        }

        // make dialog visible and set active tab
        SetPropertyTab(0);
    }

    // presets
    {
        const ICfgFile* const pIni = m_pImageCompiler->m_CC.pRC->GetIniFile();
        QComboBox* hwnd = m_ui->templateCombo;

        for (int i = 0;; ++i)
        {
            const char* const pName = pIni->GetSectionName(i);
            if (!pName)
            {
                break;
            }
            if (pName[0] == 0 || pName[0] == '_')
            {
                continue;
            }

            FileMaskConfigSink sink;
            pIni->CopySectionKeysToConfig(eCP_PriorityRcIni, i, 0, &sink);
            const size_t num = sink.m_fileMasks.size();

            if (!m_FilteredPresets[""].count(pName))
            {
                m_FileOrderedPresets.push_back(pName);
            }

            m_FilteredPresets[""].insert(pName);
            for (size_t pos = 0; pos < num; ++pos)
            {
                m_FilteredPresets[sink.m_fileMasks[pos]].insert(pName);
            }

            hwnd->addItem(pName);
        }

        hwnd->setCurrentIndex(0);
    }

    m_ui->presetShowAll->setChecked(m_bShowAllPresets);

    // preview mode
    {
        m_ui->previewMode->addItem("RGB");
        m_ui->previewMode->addItem("Alpha");
        m_ui->previewMode->addItem("RGB Alpha");
        m_ui->previewMode->addItem("Debug Normals");
        m_ui->previewMode->addItem("RGB*A (HDR)");
        m_ui->previewMode->addItem("RGB*A*A (HDR)");
        m_ui->previewMode->addItem("AlphaTestBlend");
        m_ui->previewMode->addItem("AlphaTestNoBlend");
        m_ui->previewMode->addItem("mCIE2RGB");

        m_ui->previewMode->setCurrentIndex(0);     // first 0 is setting the used template
    }
}


void CImageUserDialog::SelectPreset(const string& presetName)
{
    string filename = m_pImageCompiler->m_CC.config->GetAsString("overwritefilename", m_pImageCompiler->m_CC.sourceFileNameOnly, m_pImageCompiler->m_CC.sourceFileNameOnly);
    filename = PathHelpers::RemoveExtension(filename);

    QComboBox* combo = m_ui->templateCombo;
    combo->clear();
    int presetCount = 0;
    int presetIndex = -1;
    std::set<string> matchingPresets;
    std::set<string>::const_iterator p;
    std::list<string>::const_iterator l;

    // Collect names of all matching presets
    std::map<string, std::set<string> >::const_iterator s;
    for (s = m_FilteredPresets.begin(); s != m_FilteredPresets.end(); ++s)
    {
        const string& suffix = s->first;
        if (StringHelpers::MatchesWildcardsIgnoreCase(filename, suffix))
        {
            for (p = s->second.begin(); p != s->second.end(); ++p)
            {
                matchingPresets.insert(*p);
            }
        }
    }

    // Add names of all matching presets to the combobox
    for (p = matchingPresets.begin(); p != matchingPresets.end(); ++p)
    {
        // preset name matches
        if (StringHelpers::EqualsIgnoreCase(*p, presetName))
        {
            presetIndex = presetCount;
        }

        combo->addItem(p->c_str());
        ++presetCount;
    }

    if (!presetCount || presetIndex == -1 || m_bShowAllPresets)
    {
        const bool bShowAllPresets = !presetCount || m_bShowAllPresets;

        // Add a separator between matching and non-matching names of presets
        if (presetCount)
        {
            combo->addItem(s_pComboboxSeparator);
            ++presetCount;
        }

        // Add all non-matching names of presets to the combobox
        for (l = m_FileOrderedPresets.begin(); l != m_FileOrderedPresets.end(); ++l)
        {
            if (!matchingPresets.count(*l))
            {
                // preset name matches
                if (StringHelpers::EqualsIgnoreCase(*l, presetName))
                {
                    presetIndex = presetCount;
                }

                // show only the preset which is in the file, in case we don't show all of them already
                if (presetIndex != presetCount && !bShowAllPresets)
                {
                    continue;
                }

                combo->addItem(l->c_str());
                ++presetCount;
            }
        }
    }

    combo->setCurrentIndex(presetIndex);
}


void CImageUserDialog::SetPreviewModes(const CImageProperties& props, const bool bInitalUpdate)
{
    EPreviewMode modes[ePM_Num];
    int modeCount = 0;

    const EPreviewMode eRgbkMode = (props.GetRGBKCompression() == 1) ? ePM_RGBLinearK : ePM_RGBSquareK;

    if (props.GetRGBKCompression() > 0)
    {
        modes[modeCount++] = eRgbkMode;
        modes[modeCount++] = ePM_RawRGB;
        modes[modeCount++] = ePM_KKK;
    }
    else if (props.GetColorModel() == CImageProperties::eColorModel_CIE)
    {
        modes[modeCount++] = ePM_RGB;
        modes[modeCount++] = ePM_RawCIE;
    }
    else if (props.GetColorModel() == CImageProperties::eColorModel_IRB)
    {
        modes[modeCount++] = ePM_RGB;
        modes[modeCount++] = ePM_RawIRB;
    }
    else if (props.GetColorModel() == CImageProperties::eColorModel_YCbCr)
    {
        modes[modeCount++] = ePM_RGB;
        modes[modeCount++] = ePM_RawYCbCr;
    }
    else if (props.GetColorModel() == CImageProperties::eColorModel_YFbFr)
    {
        modes[modeCount++] = ePM_RGB;
        modes[modeCount++] = ePM_RawYFbFr;
    }
    else
    {
        modes[modeCount++] = ePM_RGB;
    }

    if (props.GetRGBKCompression() == 0)
    {
        modes[modeCount++] = ePM_AAA;
        modes[modeCount++] = ePM_RGBA;
        modes[modeCount++] = ePM_AlphaTestBlend;
        modes[modeCount++] = ePM_AlphaTestNoBlend;
    }

    if (props.GetMipRenormalize())
    {
        modes[modeCount++] = ePM_NormalLength;
    }

    QComboBox* combo = m_ui->previewMode;
    combo->clear();

    for (int i = 0; i < modeCount; ++i)
    {
        combo->addItem(CImagePreview::GetPreviewModeChoiceText(modes[i]));
    }

    const EPreviewMode curMode =
        (bInitalUpdate && props.GetRGBKCompression() > 0)
        ? eRgbkMode
        : GetPreviewMode(props, combo);

    int selection = -1;
    for (int i = 0; i < modeCount; ++i)
    {
        if (modes[i] == curMode)
        {
            selection = i;
            break;
        }
    }

    combo->setCurrentIndex(selection < 0 ? 0 : selection);
}


string CImageUserDialog::GetPreset(QWidget* control)
{
    const QString text = control->property(control->metaObject()->userProperty().name()).toString();
    if (!StringHelpers::Equals(s_pComboboxSeparator, text.toUtf8().data()))
    {
        return text.toUtf8().data();
    }

    return "";
}


EPreviewMode CImageUserDialog::GetPreviewMode(const CImageProperties& props, QWidget* control)
{
    const QString text = control->property(control->metaObject()->userProperty().name()).toString();
    if (!StringHelpers::Equals(s_pComboboxSeparator, text.toUtf8().data()))
    {
        for (int i = 0; i < ePM_Num; ++i)
        {
            const EPreviewMode pm = (EPreviewMode)i;
            if (!QString::compare(text, CImagePreview::GetPreviewModeChoiceText(pm), Qt::CaseInsensitive))
            {
                return pm;
            }
        }
    }

    return props.m_ePreviewModeOriginal;
}


EPreviewMode CImageUserDialog::GetOriginalPreviewMode(EPreviewMode processed)
{
    switch (processed)
    {
    // output only cases
    case ePM_RGB:
    case ePM_RGBLinearK:
    case ePM_RGBSquareK:
    case ePM_KKK:
    case ePM_RawRGB:
    case ePM_RawCIE:
    case ePM_RawIRB:
    case ePM_RawYCbCr:
    case ePM_RawYFbFr:
        return ePM_RGB;
    default:
        break;
    }

    return processed;
}


void CImageUserDialog::SetDataToDialog(const bool bInitalUpdate)
{
    m_bDialogIsUpdating = true;

    if (QThread::currentThread() != qApp->thread())
    {
        MainThreadExecutor::execute([&]() {SetDataToDialog(bInitalUpdate); });
        return;
    }

    // preview part in main window -------------------------------------
    m_ui->previewOn->setChecked(m_pImageCompiler->m_Props.m_bPreview);

    m_ui->previewFiltered->setChecked(m_pImageCompiler->m_Props.m_bPreviewFiltered);

    m_ui->previewTiled->setChecked(m_pImageCompiler->m_Props.m_bPreviewTiled);

    SetPreviewModes(m_pImageCompiler->m_Props, bInitalUpdate);

    m_ui->previewModeText->setText(CImagePreview::GetPreviewModeDescription(GetPreviewMode(m_pImageCompiler->m_Props, m_ui->previewModeText)));

    // main window -----------------------------------------------------
    m_ui->presetShowAll->setChecked(m_bShowAllPresets);

    SelectPreset(m_pImageCompiler->m_Props.GetPreset());

    m_BumpToNormalPanel.SetDataToDialog(m_pImageCompiler->m_Props.m_BumpToNormal, bInitalUpdate);
    m_AlphaAsBumpPanel.SetDataToDialog(m_pImageCompiler->m_Props.m_AlphaAsBump, bInitalUpdate);
    m_MIPControlPanel.SetDataToDialog(m_pImageCompiler->m_CC.config, m_pImageCompiler->m_Props, bInitalUpdate);

    // sub window: resolution ------------------------------------------

    m_ui->suppressEngineReduce->setChecked(m_pImageCompiler->m_Props.GetSupressEngineReduce());
    m_ui->suppressEngineReduce->setEnabled(!m_pImageCompiler->m_CC.config->HasKey("ser", eCP_PriorityAll & (~eCP_PriorityFile)));

    {
        CImageProperties::ReduceMap reduceMap;
        m_pImageCompiler->m_Props.GetReduceResolutionFile(reduceMap);

        Reduce::convertMapToArray(m_pImageCompiler, reduceMap, Reduce::s_platforms);
        Reduce::applyChangesAndFillListbox(m_pImageCompiler, m_hTab_Resolution, Reduce::s_platforms);
    }

    // -----------------------------------------------------------------

    m_bDialogIsUpdating = false;
}

bool CImageUserDialog::PreviewGenerationCanceled()
{
    LOCK_MONITOR();

    //check whether interface thread has been closed (window closed)
    {
        if (m_hWindowThread->returnCode() != STILL_ACTIVE)
        {
            return true;
        }
    }

    // Tiago: commented out because we have too many preview window updates
    //ZoomPreviewUpdateCheck();

    const bool bCancel = (m_eWorkerAction != WorkerActionNone && m_eWorkerAction != WorkerActionPaint && m_eWorkerAction != WorkerActionUpdatePreview);

    return bCancel;
}

void CImageUserDialog::ZoomPreviewUpdateCheck()
{
    bool bUpdatePreview = false;

    //if currently updating preview data and the zoom is reduced so MIPs are not needed, cancel processing and trigger update
    if (m_PreviewData.bIsUpdating)
    {
        if (m_PreviewData.iCurMaxScale64 > m_PreviewData.iScale64 && m_PreviewData.iScale64 < DEFAULT_SCALE64)
        {
            bUpdatePreview = true;
        }
    }

    //if zoomed out from partial view, trigger update
    if (m_PreviewData.iCurMaxScale64 > m_PreviewData.iScale64 && m_PreviewData.iCurMaxScale64 > m_PreviewData.iInitialScale64)
    {
        bUpdatePreview = true;
    }

    //if zoomed in beyond previous measures, preview data must be updated (to calculate skipped MIPs)
    if (m_PreviewData.iCurMaxScale64 < m_PreviewData.iScale64 && m_PreviewData.iCurMaxScale64 < DEFAULT_SCALE64)
    {
        bUpdatePreview = true;
    }

    if (bUpdatePreview)
    {
        TriggerUpdatePreview(false);
    }
}

void CImageUserDialog::UpdateProgressBar(float fProgress)
{
    if (PreviewGenerationCanceled())
    {
        return;
    }

    assert(fProgress >= 0);
    if (fProgress > 1.0f)
    {
        fProgress = 1.0f;
    }

    QString inszTxt = QStringLiteral("%1%").arg(fProgress * 100.0f, 3, 'f', 0);

    QRect rectProgressBorder;
    rectProgressBorder.setLeft(m_iPreviewWidth + 9);
    rectProgressBorder.setTop(m_iPreviewHeight - 14);
    rectProgressBorder.setRight(rectProgressBorder.left() + m_iPreviewWidth - 1);
    rectProgressBorder.setBottom(m_iPreviewHeight - 1);

    QRect rectProgressBar = rectProgressBorder.adjusted(1, 1, -1, -1);

    QRect rectProgressCount;
    rectProgressCount.setLeft(m_iPreviewWidth + 9);
    rectProgressCount.setTop(rectProgressBorder.top() - 1);
    rectProgressCount.setRight(rectProgressCount.left() + m_iPreviewWidth);
    rectProgressCount.setBottom(rectProgressCount.top() + 18 - 2);

    bool hasPainter = m_painter != nullptr;
    if (!hasPainter)
    {
        m_painter = new QPainter(&m_image);
    }

    m_painter->fillRect(rectProgressBorder, QPalette().shadow());
    m_painter->fillRect(rectProgressBar, QPalette().dark());
    rectProgressBar.setWidth((int)((m_iPreviewWidth - 2) * fProgress));
    m_painter->fillRect(rectProgressBar, QPalette().light());

    m_painter->drawText(rectProgressCount, Qt::AlignCenter, inszTxt);

    if (!hasPainter)
    {
        delete m_painter;
    }

}

void CImageUserDialog::GetDataFromDialog()
{
    if (QThread::currentThread() != qApp->thread())
    {
        MainThreadExecutor::execute([&]() {GetDataFromDialog(); });
        return;
    }
    // preview part in main window
    //m_pImageCompiler->m_Props.SetToDefault(false);

    m_pImageCompiler->m_Props.m_bPreview = m_ui->previewOn->isChecked();

    m_pImageCompiler->m_Props.m_bPreviewFiltered = m_ui->previewFiltered->isChecked();

    m_pImageCompiler->m_Props.m_bPreviewTiled = m_ui->previewTiled->isChecked();

    m_pImageCompiler->m_Props.m_ePreviewModeProcessed = GetPreviewMode(m_pImageCompiler->m_Props, m_ui->previewMode);
    m_pImageCompiler->m_Props.m_ePreviewModeOriginal = GetOriginalPreviewMode(m_pImageCompiler->m_Props.m_ePreviewModeProcessed);

    m_pImageCompiler->m_Props.SetPreset(GetPreset(m_ui->templateCombo));

    // Resolution tab
    {
        m_pImageCompiler->m_Props.SetSupressEngineReduce(m_ui->suppressEngineReduce->isChecked());

        CImageProperties::ReduceMap reduceMap;
        Reduce::convertArrayToMap(m_pImageCompiler, Reduce::s_platforms, reduceMap);
        m_pImageCompiler->m_Props.SetReduceResolutionFile(reduceMap);
    }

    m_BumpToNormalPanel.GetDataFromDialog(m_pImageCompiler->m_Props.m_BumpToNormal);
    m_AlphaAsBumpPanel.GetDataFromDialog(m_pImageCompiler->m_Props.m_AlphaAsBump);
    m_MIPControlPanel.GetDataFromDialog(m_pImageCompiler->m_Props);
}



void CImageUserDialog::UpdateWindowTitle()
{
    LOCK_MONITOR();

    assert(m_pImageCompiler);

    string zoom;
    zoom.Format("  Zoom:%d%%", (100 * m_PreviewData.iScale64) / 64);

    const string filename = m_pImageCompiler->m_CC.config->GetAsString("overwritefilename", m_pImageCompiler->m_CC.sourceFileNameOnly, m_pImageCompiler->m_CC.sourceFileNameOnly);

    const string title = filename + zoom;

    m_hWindow->setProperty("windowTitle", QString(title.c_str()));
}


//
void CImageUserDialog::Draw(QPainter* painter)
{
    LOCK_MONITOR();

    int iGap = 4;
    QRect rec(0, 0, m_iPreviewWidth, m_iPreviewHeight);

    bool bPreviewCMConverted = m_pImageCompiler->m_Props.GetCubemap();
    bool bPreviewToGamma = m_pImageCompiler->m_Props.GetColorSpace().second != CImageProperties::eOutputColorSpace_Linear;
    bool bPreviewStretched = m_pImageCompiler->m_Props.GetConfigAsBool("previewstretched", true, true);

    // left side = original
    const ImageObject* original = m_pImageCompiler->GetInputImage();
    if (!original)
    {
        m_ImagePreview.PrintTo(painter, rec, "Load failed", "(format is not supported by driver)");
        return;
    }

    // original may change shape
    if (bPreviewCMConverted)
    {
        ImageToProcess image(original->CopyImage());
        image.ConvertProbe(m_pImageCompiler->m_Props.GetPowOf2());
        original = image.get();
        image.forget();
    }

    // Q: Why can't original image be gamma-corrected?
    // Q: Why can't original image be RGBK?
    m_ImagePreview.AssignImage(original);
    m_ImagePreview.AssignPreset(&m_pImageCompiler->m_Props, false, true);

    if (m_pImageCompiler->m_Props.GetPowOf2() && !original->HasPowerOfTwoSizes())
    {
        char str[256];
        sprintf_s(str, "(%dx%d)", original->GetWidth(0), original->GetHeight(0));
        m_ImagePreview.PrintTo(painter, rec, "Image size is not power of two", str);
        return;
    }

    if (!m_ImagePreview.BlitTo(painter, rec, m_PreviewData.fShiftX, m_PreviewData.fShiftY, m_PreviewData.iScale64))
    {
        painter->fillRect(rec, m_hWindow->palette().brush(QPalette::Window));
    }

    rec.moveLeft(m_iPreviewWidth + iGap * 2 + 1);

    // right side = destination
    if (m_PreviewData.bIsUpdating)
    {
        UpdateProgressBar(m_pImageCompiler->m_Progress.Get());
    }
    else if (m_pImageCompiler->m_Props.m_bPreview)
    {
        const ImageObject* destination = m_pImageCompiler->GetFinalImage();
        if (!destination)
        {
            m_ImagePreview.PrintTo(painter, rec, "Conversion failed", "(check rc_log.log for details)");
            return;
        }

        // Q: Why can't destination image be RGBA?
        m_ImagePreview.AssignImage(destination);
        m_ImagePreview.AssignPreset(&m_pImageCompiler->m_Props, bPreviewToGamma, false);

        // Make it stretch to the same size as the original image,
        // this can happen if the destination platform has limitations
        // on the highest mipmap-level.
        if (bPreviewStretched)
        {
            m_ImagePreview.AssignScale(original);
        }

        if (!m_ImagePreview.BlitTo(painter, rec, m_PreviewData.fOldShiftX, m_PreviewData.fOldShiftY, m_PreviewData.iScale64))
        {
            painter->fillRect(rec, m_hWindow->palette().brush(QPalette::Window));
        }
    }
    else
    {
        m_ImagePreview.PrintTo(painter, rec, "Preview is disabled", "(leftmost checkbox in Preview pane)");
    }
}

void CImageUserDialog::MouseMessage(Qt::MouseButtons indwButtons, const int iniRelX, const int iniRelY, int iniRelZ)
{
    // early out if no processing required because no mouse buttons are currently and previously pressed and not zooming
    if (indwButtons == Qt::NoButton && !m_PreviewData.dwMouseButtonsOld && !iniRelZ)
    {
        return;
    }

    assert(m_pImageCompiler);
    const ImageObject* original = m_pImageCompiler->GetInputImage();

    uint32 iWidth = 0;
    uint32 iHeight = 0;
    uint32 iMips = 0;

    if (original)
    {
        original->GetExtentTransformed(iWidth, iHeight, iMips, m_pImageCompiler->m_Props);
    }

    float fOldShiftX = m_PreviewData.fShiftX;
    float fOldShiftY = m_PreviewData.fShiftY;
    int iOldScale64 = m_PreviewData.iScale64;

    m_PreviewData.fShiftX -= iniRelX * (64.0f / m_PreviewData.iScale64 / iWidth);
    m_PreviewData.fShiftY -= iniRelY * (64.0f / m_PreviewData.iScale64 / iHeight);

    while (iniRelZ)
    {
        if (iniRelZ > 0)
        {
            m_PreviewData.iScale64 /= 2;
            iniRelZ -= WHEEL_DELTA;
        }
        else
        {
            m_PreviewData.iScale64 *= 2;
            iniRelZ += WHEEL_DELTA;
        }
    }

    if (m_PreviewData.iScale64 < 1)
    {
        m_PreviewData.iScale64 = 1;
    }
    if (m_PreviewData.iScale64 > 16 * 16 * 4)
    {
        m_PreviewData.iScale64 = 16 * 16 * 4;
    }

    bool bNoMovementPossible = false;
    if (original)
    {
        m_ImagePreview.AssignImage(original);

        bNoMovementPossible = m_ImagePreview.ClampBlitOffset(m_iPreviewWidth, m_iPreviewHeight, m_PreviewData.fShiftX, m_PreviewData.fShiftY, m_PreviewData.iScale64);
    }

    LOCK_MONITOR();

    m_PreviewData.bDelayUpdate = false;

    const bool bMousePOld    = (m_PreviewData.dwMouseButtonsOld & Qt::LeftButton) || (m_PreviewData.dwMouseButtonsOld & Qt::RightButton);
    const bool bMousePNow    = (indwButtons                     & Qt::LeftButton) || (indwButtons                     & Qt::RightButton);
    const bool bChangedShift = (fOldShiftX != m_PreviewData.fShiftX || fOldShiftY != m_PreviewData.fShiftY);
    const bool bChangedScale = (iOldScale64 != m_PreviewData.iScale64);

    if (bChangedShift && !bMousePOld && !m_PreviewData.bIsUpdating)
    {
        m_PreviewData.fOldShiftX = m_PreviewData.fShiftX;
        m_PreviewData.fOldShiftY = m_PreviewData.fShiftY;
    }

    if (bChangedShift || bChangedScale)
    {
        UpdateWindowTitle();            // Zoom:%d

        // update window
        TriggerRepaint();

        if (m_PreviewData.bIsUpdating)
        {
            TriggerUpdatePreview(false);
        }
        else
        {
            ZoomPreviewUpdateCheck();
        }

        if (bMousePOld)
        {
            m_PreviewData.bDelayUpdate = true;
        }
    }

    if (bMousePOld && !bMousePNow && (m_PreviewData.iScale64 > m_PreviewData.iInitialScale64))
    {
        float deltaX = fabsf(m_PreviewData.fOldShiftX - m_PreviewData.fShiftX);
        float deltaY = fabsf(m_PreviewData.fOldShiftY - m_PreviewData.fShiftY);

        if (deltaX > 0.0001f || deltaY > 0.0001f)
        {
            TriggerUpdatePreview(false);
        }

        //if (m_PreviewData.fOldShiftX!=m_PreviewData.fShiftX || m_PreviewData.fOldShiftX!=m_PreviewData.fShiftY)
        //  TriggerUpdatePreview(false);
    }

    m_PreviewData.dwMouseButtonsOld = indwButtons;
}



void CImageUserDialog::UpdatePreview(const bool bFullConvert)
{
    if (QThread::currentThread() != qApp->thread())
    {
        MainThreadExecutor::execute([&]() {UpdatePreview(bFullConvert); });
        return;
    }
    m_PreviewData.iCurMaxScale64 = m_PreviewData.iScale64;

    // preview images
    {
        QRect rect = m_hWindow->rect();

        rect.setBottom(m_iPreviewHeight);

        bool bErase = false;
        if (bFullConvert)
        {
            SetDataToDialog(true);                                                              // might need to update preset in UI
            bErase = m_pImageCompiler->RunWithProperties(false);  // do image conversion
        }

        //InvalidateRect(m_hWindow,&rect,bErase);               // don't erase background
    }

    // info text left
    {
        string sLeftInfo = m_pImageCompiler->GetInfoStringUI(true);

        m_ui->leftPreviewInfo->setText(sLeftInfo.c_str());
    }

    // info text right
    if (!PreviewGenerationCanceled())
    {
        string sRightInfo = m_pImageCompiler->GetInfoStringUI(false);

        m_ui->rightPreviewInfo->setText(sRightInfo.c_str());

        m_PreviewData.fOldShiftX = m_PreviewData.fShiftX;
        m_PreviewData.fOldShiftY = m_PreviewData.fShiftY;
    }
}


void CImageUserDialog::SetPropertyTab(const int iniNo)
{
    if (QThread::currentThread() != qApp->thread())
    {
        MainThreadExecutor::execute([&]() {SetPropertyTab(iniNo); });
        return;
    }
    LOCK_MONITOR();

    assert(iniNo >= 0 && iniNo <= 3);
    m_ui->propTab->setCurrentIndex(iniNo);
    m_ui->propTab->currentWidget()->setFocus();
}

int CImageUserDialog::GetPreviewReduceResolution(uint32 dwWidth, uint32 dwHeight)
{
    const int factor = m_PreviewData.iCurMaxScale64 << m_pImageCompiler->m_Props.GetRequestedResolutionReduce(dwWidth, dwHeight);

    if (factor >= DEFAULT_SCALE64)
    {
        return 0;
    }

    return m_pImageCompiler->m_Props.GetMipMaps() ? ((int)(log((float)DEFAULT_SCALE64) / LN2) - (int)(log((float)factor) / LN2)) : 0;
}

QRect CImageUserDialog::GetPreviewRectangle(const uint32 mipLevel)
{
    QRect rect;

    int iWidth = m_PreviewData.iOrigWidth >> mipLevel;
    if (iWidth < 1)
    {
        iWidth = 1;
    }
    int iHeight = m_PreviewData.iOrigHeight >> mipLevel;
    if (iHeight < 1)
    {
        iHeight = 1;
    }
    const int iScale = m_PreviewData.iCurMaxScale64 << mipLevel;

    float fCoordinateShiftX = 0.5f;
    float fCoordinateShiftY = 0.5f;
    if (m_PreviewData.iCurMaxScale64 > m_PreviewData.iInitialScale64)
    {
        float fScaleShiftX = 1.0f;
        float fScaleShiftY = 1.0f;
        if (iWidth != iHeight)
        {
            if (iWidth < iHeight)
            {
                fScaleShiftX = (float)(iHeight / iWidth);
            }
            else
            {
                fScaleShiftY = (float)(iWidth / iHeight);
            }
        }

        const float fTemp = m_PreviewData.iInitialScale64 / (float)m_PreviewData.iCurMaxScale64;
        fCoordinateShiftX *= fTemp * fScaleShiftX;
        fCoordinateShiftY *= fTemp * fScaleShiftY;
    }

    const int iTemp = (MAX(m_iPreviewWidth, m_iPreviewHeight) * DEFAULT_SCALE64) / iScale;

    rect.setLeft((long)((m_PreviewData.fShiftX - fCoordinateShiftX) * iWidth));
    rect.setRight(rect.left() + iTemp);
    rect.setRight((rect.right() > iWidth) ? iWidth : rect.right());
    rect.setTop((long)((m_PreviewData.fShiftY - fCoordinateShiftY) * iHeight));
    rect.setBottom(rect.top() + iTemp);
    rect.setBottom((rect.bottom() > iHeight) ? iHeight : rect.bottom());

    //additional border pixels for mipmap generation
    const int maxBorderSize = 16;
    rect.setLeft(rect.left() - MIN(maxBorderSize, rect.left()));
    rect.setRight(rect.right() + MIN(maxBorderSize, iWidth - rect.right()));
    rect.setTop(rect.top() - MIN(maxBorderSize, rect.top()));
    rect.setBottom(rect.bottom() + MIN(maxBorderSize, iHeight - rect.bottom()));

    // expand to include full 4x4 blocks
    rect.setLeft(rect.left() & (~0x3));
    rect.setTop(rect.top() & (~0x3));
    rect.setRight((rect.right() + 3) & (~0x3));
    rect.setBottom((rect.bottom() + 3) & (~0x3));

    return rect;
}

void CImageUserDialog::GetInputExtentAndMaximumZoom()
{
    uint32 iTexWidth = 0;
    uint32 iTexHeight = 0;
    uint32 iTexMips = 0;
    int iScale64 = DEFAULT_SCALE64;

    if (m_pImageCompiler->GetInputImage())
    {
        m_pImageCompiler->GetInputImage()->GetExtentTransformed(iTexWidth, iTexHeight, iTexMips, m_pImageCompiler->m_Props);
    }

    m_PreviewData.iOrigWidth  = iTexWidth;
    m_PreviewData.iOrigHeight = iTexHeight;

    while ((iTexWidth > 256 || iTexHeight > 256) && iScale64 != 1)
    {
        iTexWidth >>= 1;
        iTexHeight >>= 1;

        iScale64 >>= 1;
    }

    m_PreviewData.iCurMaxScale64  = iScale64;
    m_PreviewData.iInitialScale64 = iScale64;
}

#ifndef Q_OS_WIN
#define WM_CLOSE   0x10
#define WM_COMMAND 0x111
#endif

void CImageUserDialog::WndProc(int uMsg, int wParam, int lParam)
{
    switch (uMsg)
    {
    case WM_COMMAND:                                                //
    {
        int wID = LOWORD(wParam);
        switch (wID)
        {
        // TODO: Move generation to main thread.
        case IDC_GENERATEOUTPUT:
            TriggerGenerateOutput(false);
            return;
        case IDC_ABOUT:
        {
            char buffer[2048];
            m_pImageCompiler->m_CC.pRC->GetGenericInfo(buffer, sizeof(buffer), "\r\n");
            QMessageBox::information(m_hWindow, "About", buffer);
        }
            return;
        case IDOK:
            TriggerSaveConfig(true);
            return;

        case IDCANCEL:
            //                      PostQuitMessage(0);
            TriggerCancel();
            return;

        case IDC_ZOOMIN:
            MouseMessage(0, 0, 0, -WHEEL_DELTA);
            return;

        case IDC_ZOOMOUT:
            MouseMessage(0, 0, 0, WHEEL_DELTA);
            return;

        case IDC_BTN_SHOWPRESETINFO:
        {
            auto hWnd2 = m_ui->templateCombo;
            const QString presetName = hWnd2->currentText();
            if (presetName.isEmpty())
            {
                assert(0);
                return;
            }

            const CImageCompiler* const pIC = m_pImageCompiler;
            const int sectionIndex = pIC->m_CC.pRC->GetIniFile()->FindSection(presetName.toUtf8().data());

            if (sectionIndex < 0)
            {
                assert(0);
                return;
            }

            SimpleTextConfigSink sink;
            pIC->m_CC.pRC->GetIniFile()->CopySectionKeysToConfig(eCP_PriorityRcIni, sectionIndex, 0, &sink);

            const QString title = QString("Preset: ") + presetName;
            QMessageBox::information(m_hWindow, title, sink.m_text.c_str());
            return;
        }

        case IDC_RESOLUTION_BTN_CHOOSE:
        case IDC_RESOLUTION_BTN_HIGHER:
        case IDC_RESOLUTION_BTN_LOWER:
        {
            const int selCount = Reduce::grabListboxSelection(m_hTab_Resolution, Reduce::s_platforms);
            if (selCount <= 0)
            {
                QMessageBox::warning(m_hWindow, "RC", "You must select at least one row in the list first.");
                return;
            }

            if (wID == IDC_RESOLUTION_BTN_CHOOSE)
            {
                Reduce::s_pImageCompiler = m_pImageCompiler;
                ChooseResolutionDialog dialog(m_hWindow);
                Reduce::s_pImageCompiler = 0;
                QObject::connect(&dialog, &ChooseResolutionDialog::resolutionSelected, &dialog, [&](int res)
                {
                    assert(res >= Reduce::kMinReduce);
                    assert(res <= Reduce::kMaxReduce);
                    for (int i = 0; i < (int)Reduce::s_platforms.size(); ++i)
                    {
                        if (Reduce::s_platforms[i].bMark)
                        {
                            Reduce::s_platforms[i].reduceValue = res;
                        }
                    }
                });
                dialog.exec();
            }
            else
            {
                const int add = (wID == IDC_RESOLUTION_BTN_HIGHER) ? -1 : +1;
                for (int i = 0; i < (int)Reduce::s_platforms.size(); ++i)
                {
                    if (Reduce::s_platforms[i].bMark)
                    {
                        int newValue = Reduce::s_platforms[i].reduceValue + add;
                        Util::clampMinMax(newValue, (int)Reduce::kMinReduce, (int)Reduce::kMaxReduce);
                        Reduce::s_platforms[i].reduceValue = newValue;
                    }
                }
            }

            Reduce::applyChangesAndFillListbox(m_pImageCompiler, m_hTab_Resolution, Reduce::s_platforms);

            TriggerUpdatePreview(true);
            return;
        }

        case IDC_RESOLUTION_BTN_DELPLATFORM:
        {
            const int selCount = Reduce::grabListboxSelection(m_hTab_Resolution, Reduce::s_platforms);
            if (selCount <= 0)
            {
                QMessageBox::warning(m_hWindow, "RC", "You must select at least one row in the list first.");
                return;
            }

            for (int i = 0; i < (int)Reduce::s_platforms.size(); ++i)
            {
                if (Reduce::s_platforms[i].bMark && Reduce::s_platforms[i].platform >= 0)
                {
                    QMessageBox::warning(m_hWindow, "RC", "You must select at least one row in the list first.");
                    return;
                }
            }

            for (int i = 0; i < (int)Reduce::s_platforms.size(); ++i)
            {
                if (Reduce::s_platforms[i].bMark)
                {
                    Reduce::s_platforms.erase(Reduce::s_platforms.begin() + i);
                    --i;
                }
            }

            Reduce::applyChangesAndFillListbox(m_pImageCompiler, m_hTab_Resolution, Reduce::s_platforms);

            TriggerUpdatePreview(true);
            return;
        }

        // check box change, update all
        case IDC_PRESETSHOWALL:                                         // preset-filtering has changed
            // refill and select the preset
        {
            auto hwnd2 = m_ui->presetShowAll;
            m_bShowAllPresets = hwnd2->isChecked();
            SelectPreset(m_pImageCompiler->m_Props.GetPreset());
            return;
        }

        // combo box change, update all
        case IDC_TEMPLATECOMBO:                                             // preset has changed
        case IDC_MIPMAPS:                                   // mipmaps have changed
        case IDC_MIPMETHOD:                                 // mipmap filtering method has changed
        case IDC_MIPOP:                                     // mipmap operation method has changed
            // prevent refresh when opening the drop-out
            Reduce::grabListboxSelection(m_hTab_Resolution, Reduce::s_platforms);
            TriggerUpdatePreview(true);
            return;

        // check box change, update all
        case IDC_SUPRESSENGINEREDUCE:                       // image flags have changed
        case IDC_MAINTAINALPHACOVERAGE:                     // maintainalphacoverage has changed
        case IDC_NM_BUMPINVERT:                             // bump map invert has changed
            Reduce::grabListboxSelection(m_hTab_Resolution, Reduce::s_platforms);
            TriggerUpdatePreview(true);
            return;

        case IDC_PREVIEWMODE:                                                   // preview-mode has changed
            // prevent refresh when opening the drop-out
            Reduce::grabListboxSelection(m_hTab_Resolution, Reduce::s_platforms);
            TriggerUpdatePreview(false);
            return;

        // preview changes
        case IDC_PREVIEWON:
        case IDC_PREVIEWTILED:
        case IDC_PREVIEWFILTERED:
            Reduce::grabListboxSelection(m_hTab_Resolution, Reduce::s_platforms);
            TriggerUpdatePreview(false);
            return;
        }
    }
        return;

    case WM_CLOSE:
    {
        LockHandle lh = Lock();
        m_quitRequest = QuitRequestUserCancel;
    }
        return;
    }
}

unsigned int CImageUserDialog::ThreadStart_Static(void* pThis)
{
    MainThreadExecutor::execute([pThis]() {static_cast<CImageUserDialog*>(pThis)->ThreadStart(); });
    return 0;
}

DWORD CImageUserDialog::ThreadStart()
{
    // zoom out if image is too big
    GetInputExtentAndMaximumZoom();

    m_PreviewData.iScale64 = m_PreviewData.iInitialScale64;

    m_ui.reset(new Ui::ImageUserDialog);
    m_hWindow = new QWidget(nullptr, Qt::MSWindowsFixedSizeDialogHint);
    auto eventFilter = new EventFilter(m_hWindow);
    m_ui->setupUi(m_hWindow);
    m_image = QImage(m_ui->widget->size(), QImage::Format_RGBA8888);

    auto activated = static_cast<void(QComboBox::*)(int)>(&QComboBox::activated);

    QObject::connect(m_ui->generateOutput, &QPushButton::clicked, m_hWindow, [&]() { WndProc(WM_COMMAND, IDC_GENERATEOUTPUT); });
    QObject::connect(m_ui->about, &QPushButton::clicked, m_hWindow, [&]() { WndProc(WM_COMMAND, IDC_ABOUT); });
    QObject::connect(m_ui->buttonBox, &QDialogButtonBox::accepted, m_hWindow, [&]() { WndProc(WM_COMMAND, IDOK); });
    QObject::connect(m_ui->buttonBox, &QDialogButtonBox::rejected, m_hWindow, [&]() { WndProc(WM_COMMAND, IDCANCEL); });
    QObject::connect(m_ui->zoomIn, &QPushButton::clicked, m_hWindow, [&]() { WndProc(WM_COMMAND, IDC_ZOOMIN); });
    QObject::connect(m_ui->zoomOut, &QPushButton::clicked, m_hWindow, [&]() { WndProc(WM_COMMAND, IDC_ZOOMOUT); });
    QObject::connect(m_ui->showPresetInfo, &QPushButton::clicked, m_hWindow, [&]() { WndProc(WM_COMMAND, IDC_BTN_SHOWPRESETINFO); });
    QObject::connect(m_ui->resolutionChoose, &QPushButton::clicked, m_hWindow, [&]() { WndProc(WM_COMMAND, IDC_RESOLUTION_BTN_CHOOSE); });
    QObject::connect(m_ui->resolutionHigher, &QPushButton::clicked, m_hWindow, [&]() { WndProc(WM_COMMAND, IDC_RESOLUTION_BTN_HIGHER); });
    QObject::connect(m_ui->resolutionLower, &QPushButton::clicked, m_hWindow, [&]() { WndProc(WM_COMMAND, IDC_RESOLUTION_BTN_LOWER); });
    QObject::connect(m_ui->resolutionDelPlatform, &QPushButton::clicked, m_hWindow, [&]() { WndProc(WM_COMMAND, IDC_RESOLUTION_BTN_DELPLATFORM); });
    QObject::connect(m_ui->presetShowAll, &QCheckBox::clicked, m_hWindow, [&]() { WndProc(WM_COMMAND, IDC_PRESETSHOWALL); });
    QObject::connect(m_ui->templateCombo, activated, m_hWindow, [&]() { WndProc(WM_COMMAND, IDC_TEMPLATECOMBO); });
    QObject::connect(m_ui->mipMaps, activated, m_hWindow, [&]() { WndProc(WM_COMMAND, IDC_MIPMAPS); });
    QObject::connect(m_ui->mipMethod, activated, m_hWindow, [&]() { WndProc(WM_COMMAND, IDC_MIPMETHOD); });
    QObject::connect(m_ui->mipOp, activated, m_hWindow, [&]() { WndProc(WM_COMMAND, IDC_MIPOP); });
    QObject::connect(m_ui->suppressEngineReduce, &QCheckBox::clicked, m_hWindow, [&]() { WndProc(WM_COMMAND, IDC_SUPRESSENGINEREDUCE); });
    QObject::connect(m_ui->maintainAlphaCoverage, &QCheckBox::clicked, m_hWindow, [&]() { WndProc(WM_COMMAND, IDC_MAINTAINALPHACOVERAGE); });
    QObject::connect(m_ui->nmBumpInvert_RGB, &QCheckBox::clicked, m_hWindow, [&]() { WndProc(WM_COMMAND, IDC_NM_BUMPINVERT); });
    QObject::connect(m_ui->nmBumpInvert_Alpha, &QCheckBox::clicked, m_hWindow, [&]() { WndProc(WM_COMMAND, IDC_NM_BUMPINVERT); });
    QObject::connect(m_ui->previewMode, activated, m_hWindow, [&]() { WndProc(WM_COMMAND, IDC_PREVIEWMODE); });
    QObject::connect(m_ui->previewOn, &QCheckBox::clicked, m_hWindow, [&]() { WndProc(WM_COMMAND, IDC_PREVIEWON); });
    QObject::connect(m_ui->previewTiled, &QCheckBox::clicked, m_hWindow, [&]() { WndProc(WM_COMMAND, IDC_PREVIEWTILED); });
    QObject::connect(m_ui->previewFiltered, &QCheckBox::clicked, m_hWindow, [&]() { WndProc(WM_COMMAND, IDC_PREVIEWFILTERED); });
    QObject::connect(eventFilter, &EventFilter::eventReceived, m_hWindow, [&](QEvent* event)
    {
        if (event->type() == QEvent::Close)
        {
            WndProc(WM_CLOSE);
            event->accept();
        }
    });

    eventFilter = new EventFilter(m_ui->widget);
    QObject::connect(eventFilter, &EventFilter::eventReceived, eventFilter, [&](QEvent* event)
    {
        switch (event->type())
        {
        case QEvent::MouseButtonPress:
        case QEvent::MouseButtonRelease:
        case QEvent::MouseMove:
        {
            QMouseEvent* e = static_cast<QMouseEvent*>(event);
            int relX, relY;
            LOCK_MONITOR();
            switch (event->type())
            {
            case QEvent::MouseButtonPress:
                m_RelMouse.OnButtonDown(m_ui->widget);
                break;
            case QEvent::MouseButtonRelease:
                m_RelMouse.OnButtonUp();
                MouseMessage(e->buttons(), 0, 0, 0);
                break;
            case QEvent::MouseMove:
                m_RelMouse.OnMouseMove(m_ui->widget, e->buttons() != Qt::NoButton, relX, relY);
                MouseMessage(e->buttons(), relX, relY, 0);
                break;
            }
            e->setAccepted(true);
            break;
        }
        case QEvent::Wheel:
        {
            QWheelEvent* e = static_cast<QWheelEvent*>(event);
            MouseMessage(0, 0, 0, -e->pixelDelta().y());
            e->setAccepted(true);
            break;
        }
        default:
            break;
        }
    });

    CreateDialogItems();

    SetDataToDialog(true);

    UpdateWindowTitle();
    TriggerUpdatePreview(true);

    // bringing the dialog to the top (otherwise, if a key is in pressed state
    // during launching RC from Photoshop, the dialog will be occluded
    // by the Photoshop's window)
    m_hWindow->setWindowFlags(m_hWindow->windowFlags() | Qt::WindowStaysOnTopHint);
    m_hWindow->setWindowFlags(m_hWindow->windowFlags() & ~Qt::WindowStaysOnTopHint);
    m_hWindow->show();
    m_hWindow->raise();

    // update preview and warning subtitles
    {
        string sLeftInfo = m_pImageCompiler->GetInfoStringUI(true);

        m_ui->leftPreviewInfo->setText(sLeftInfo.c_str());
        m_ui->rightPreviewInfo->clear();
        m_ui->warningLine->clear();
    }

    s_bDialogInitialized = true;

    // message loop
    while (m_quitRequest == QuitRequestNone)
    {
        qApp->processEvents();
    }

    delete m_hWindow;
    m_hWindow = 0;

    return (m_quitRequest == QuitRequestNormal) ? 0 : 1;
}

void CImageUserDialog::TriggerUpdatePreview(bool bFull)
{
    LOCK_MONITOR();

    m_pImageCompiler->m_Props.m_bPreview = m_ui->previewOn->isChecked();
    if (m_pImageCompiler->m_Props.m_bPreview)
    {
        m_eWorkerAction = (bFull ? WorkerActionUpdatePreview : WorkerActionUpdatePreviewOnly);
    }
    else
    {
        if (!m_bDialogIsUpdating)
        {
            GetDataFromDialog();
            SetDataToDialog();
            GetDataFromDialog();
        }
    }
}

void CImageUserDialog::TriggerGenerateOutput(bool bQuit)
{
    LOCK_MONITOR();

    if (bQuit)
    {
        m_eWorkerAction = WorkerActionGenerateOutputAndQuit;
    }
    else
    {
        m_eWorkerAction = WorkerActionGenerateOutput;
    }
}

void CImageUserDialog::TriggerSaveConfig(bool bQuit)
{
    LOCK_MONITOR();

    if (bQuit)
    {
        m_eWorkerAction = WorkerActionSaveConfigAndQuit;
    }
    else
    {
        m_eWorkerAction = WorkerActionSaveConfig;
    }
}

void CImageUserDialog::TriggerRepaint()
{
    LOCK_MONITOR();

    if (m_eWorkerAction == WorkerActionNone)
    {
        m_eWorkerAction = WorkerActionPaint;
    }
}

CImageUserDialog::WorkerAction CImageUserDialog::CheckAndResetWorkerAction()
{
    LOCK_MONITOR();

    WorkerAction eAction = m_eWorkerAction;
    m_eWorkerAction = WorkerActionNone;
    return eAction;
}

void CImageUserDialog::OnTinyDocumentChanged(TinyDocument<float>* pDocument)
{
    TriggerUpdatePreview(true);
}

void CImageUserDialog::OnTinyDocumentChanged(TinyDocument<bool>* pDocument)
{
    TriggerUpdatePreview(true);
}

void CImageUserDialog::TriggerCancel()
{
    LOCK_MONITOR();

    m_eWorkerAction = WorkerActionQuit;
}

void MainThreadExecutor::execute(AZStd::function<void()> f)
{
    static MainThreadExecutor e;
    QMetaObject::invokeMethod(&e, "executeInternal", QThread::currentThread() == e.thread() ? Qt::DirectConnection : Qt::BlockingQueuedConnection, Q_ARG(void*, &f));
}

MainThreadExecutor::MainThreadExecutor()
{
    moveToThread(qApp->thread());
}

void MainThreadExecutor::executeInternal(void* function)
{
    auto f = reinterpret_cast<AZStd::function<void()>*>(function);
    if (f)
    {
        (*f)();
    }
}

EventFilter::EventFilter(QObject* target)
    : QObject(target)
{
    target->installEventFilter(this);
}

bool EventFilter::eventFilter(QObject* watched, QEvent* event)
{
    if (watched = parent())
    {
        emit eventReceived(event);
    }
    return QObject::eventFilter(watched, event);
}

ChooseResolutionDialog::ChooseResolutionDialog(QWidget* parent)
    : QDialog(parent)
    , m_ui(new Ui::ChooseResolutionDialog)
{
    m_ui->setupUi(this);

    connect(m_ui->button_2, &QPushButton::clicked, this, [&]() { emit resolutionSelected(-2); });
    connect(m_ui->button_1, &QPushButton::clicked, this, [&]() { emit resolutionSelected(-1); });
    connect(m_ui->button0, &QPushButton::clicked, this, [&]() { emit resolutionSelected(0); });
    connect(m_ui->button1, &QPushButton::clicked, this, [&]() { emit resolutionSelected(1); });
    connect(m_ui->button2, &QPushButton::clicked, this, [&]() { emit resolutionSelected(2); });
    connect(m_ui->button3, &QPushButton::clicked, this, [&]() { emit resolutionSelected(3); });
    connect(m_ui->button4, &QPushButton::clicked, this, [&]() { emit resolutionSelected(4); });
    connect(m_ui->button5, &QPushButton::clicked, this, [&]() { emit resolutionSelected(5); });

    connect(this, &ChooseResolutionDialog::resolutionSelected, this, &QDialog::accept);

    // Let's find what resolutions we will have for all possible "reduce" values [-2; 5]
    // We will use GetFinalImageInfo() which requires platform specifide. For that we can
    // use arbitrary platform from the list of available platforms, because usually
    // resulting width and height do not depend on platform (it's not exactly true
    // for platforms having some specific "min" and "max" texture sizes, but showing
    // proper per-platform resolutions for our multi-select list of platforms is 10x
    // more complicated).

    CImageProperties::ReduceMap savedReduceMap;
    Reduce::s_pImageCompiler->m_Props.GetReduceResolutionFile(savedReduceMap);

    for (int i = Reduce::kMinReduce; i <= Reduce::kMaxReduce; ++i)
    {
        QString str = QString::number(i);

        {
            CImageProperties::ReduceMap workReduceMap;
            Reduce::s_pImageCompiler->m_Props.ConvertReduceStringToMap(str.toUtf8().data(), workReduceMap);
            Reduce::s_pImageCompiler->m_Props.SetReduceResolutionFile(workReduceMap);
        }

        const int arbitraryPlatform = 0;

        EPixelFormat finalFormat;
        bool bFinalCubemap;
        uint32 dwFinalMipCount;
        uint32 dwFinalReduce;
        uint32 dwWidth;
        uint32 dwHeight;

        Reduce::s_pImageCompiler->GetFinalImageInfo(arbitraryPlatform, finalFormat, bFinalCubemap, dwFinalMipCount, dwFinalReduce, dwWidth, dwHeight);

        str = QStringLiteral("%1 x %2  (%3)").arg(dwWidth).arg(dwHeight).arg(i);

        QString buttonName = QStringLiteral("button%1").arg(i).replace("-", "_");
        findChild<QPushButton*>(buttonName)->setText(str);
    }

    Reduce::s_pImageCompiler->m_Props.SetReduceResolutionFile(savedReduceMap);
}

#include "ImageUserDialog.moc"