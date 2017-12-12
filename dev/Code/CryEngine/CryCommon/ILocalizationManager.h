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

#ifndef CRYINCLUDE_CRYCOMMON_ILOCALIZATIONMANAGER_H
#define CRYINCLUDE_CRYCOMMON_ILOCALIZATIONMANAGER_H
#pragma once

#include <CrySizer.h>
#include <AzCore/EBus/EBus.h>
//#include <platform.h> // Needed for LARGE_INTEGER (for consoles).

////////////////////////////////////////////////////////////////////////////////////////////////
// Forward declarations
////////////////////////////////////////////////////////////////////////////////////////////////

class XmlNodeRef;

//////////////////////////////////////////////////////////////////////////
// Localized strings manager interface.
//////////////////////////////////////////////////////////////////////////

// Localization Info structure
struct SLocalizedInfoGame
{
    SLocalizedInfoGame ()
        : szCharacterName(NULL)
        , bUseSubtitle(false)
    {
    }

    const char* szCharacterName;
    string sUtf8TranslatedText;

    bool bUseSubtitle;
};

struct SLocalizedAdvancesSoundEntry
{
    string  sName;
    float       fValue;
    void GetMemoryUsage(ICrySizer* pSizer) const
    {
        pSizer->AddObject(sName);
    }
};

// Localization Sound Info structure, containing sound related parameters.
struct SLocalizedSoundInfoGame
    : public SLocalizedInfoGame
{
    SLocalizedSoundInfoGame()
        : sSoundEvent(NULL)
        , fVolume(0.f)
        , fRadioRatio (0.f)
        , bIsDirectRadio(false)
        , bIsIntercepted(false)
        , nNumSoundMoods(0)
        , pSoundMoods (NULL)
        , nNumEventParameters(0)
        , pEventParameters(NULL)
    {
    }

    const char* sSoundEvent;
    float fVolume;
    float fRadioRatio;
    bool    bIsDirectRadio;
    bool    bIsIntercepted;

    // SoundMoods.
    int         nNumSoundMoods;
    SLocalizedAdvancesSoundEntry* pSoundMoods;

    // EventParameters.
    int         nNumEventParameters;
    SLocalizedAdvancesSoundEntry* pEventParameters;
};

// Localization Sound Info structure, containing sound related parameters.
struct SLocalizedInfoEditor
    : public SLocalizedInfoGame
{
    SLocalizedInfoEditor()
        : sKey(NULL)
        , sOriginalCharacterName(NULL)
        , sOriginalActorLine(NULL)
        , sUtf8TranslatedActorLine(NULL)
        , nRow(0)
    {
    }

    const char* sKey;
    const char* sOriginalCharacterName;
    const char* sOriginalActorLine;
    const char* sUtf8TranslatedActorLine;
    unsigned int nRow;
};

// Summary:
//      Interface to the Localization Manager.
struct ILocalizationManager
{
    //Platform independent language IDs. These are used to map the platform specific language codes to localization pakfiles
    //Please ensure that each entry in this enum has a corresponding entry in the PLATFORM_INDEPENDENT_LANGUAGE_NAMES array which is defined in LocalizedStringManager.cpp currently.
    enum EPlatformIndependentLanguageID
    {
        ePILID_Japanese,
        ePILID_English,
        ePILID_French,
        ePILID_Spanish,
        ePILID_German,
        ePILID_Italian,
        ePILID_Dutch,
        ePILID_Portuguese,
        ePILID_Russian,
        ePILID_Korean,
        ePILID_ChineseT,    // Traditional Chinese
        ePILID_ChineseS,    // Simplified Chinese
        ePILID_Finnish,
        ePILID_Swedish,
        ePILID_Danish,
        ePILID_Norwegian,
        ePILID_Polish,
        ePILID_Arabic,      //Test value for PS3. Not currently supported by Sony on the PS3 as a system language
        ePILID_Czech,
        ePILID_Turkish,
        ePILID_MAX_OR_INVALID   //Not a language, denotes the maximum number of languages or an unknown language
    };

    typedef uint32 TLocalizationBitfield;

    // <interfuscator:shuffle>
    virtual ~ILocalizationManager(){}
    virtual const char* LangNameFromPILID(const ILocalizationManager::EPlatformIndependentLanguageID id) = 0;
    virtual ILocalizationManager::TLocalizationBitfield MaskSystemLanguagesFromSupportedLocalizations(const ILocalizationManager::TLocalizationBitfield systemLanguages) = 0;
    virtual ILocalizationManager::TLocalizationBitfield IsLanguageSupported(const ILocalizationManager::EPlatformIndependentLanguageID id) = 0;
    virtual bool SetLanguage(const char* sLanguage) = 0;
    virtual const char* GetLanguage() = 0;

    // load the descriptor file with tag information
    virtual bool InitLocalizationData(const char* sFileName, bool bReload = false) = 0;
    // request to load loca data by tag. Actual loading will happen during next level load begin event.
    virtual bool RequestLoadLocalizationDataByTag(const char* sTag) = 0;
    // direct load of loca data by tag
    virtual bool LoadLocalizationDataByTag(const char* sTag, bool bReload = false) = 0;
    virtual bool ReleaseLocalizationDataByTag(const char* sTag) = 0;

    virtual bool LoadExcelXmlSpreadsheet(const char* sFileName, bool bReload = false) = 0;
    virtual void ReloadData() = 0;

    // Summary:
    //   Free localization data.
    virtual void FreeData() = 0;

    // Summary:
    //   Translate a string into the currently selected language.
    // Description:
    //   Processes the input string and translates all labels contained into the currently selected language.
    // Parameters:
    //   sString             - String to be translated.
    //   outLocalizedString  - Translated version of the string.
    //   bEnglish            - if true, translates the string into the always present English language.
    // Returns:
    //   true if localization was successful, false otherwise
    virtual bool LocalizeString(const char* sString, string& outLocalizedString, bool bEnglish = false) = 0;

    // Summary:
    //   Same as LocalizeString( const char* sString, string& outLocalizedString, bool bEnglish=false )
    //   but at the moment this is faster.
    virtual bool LocalizeString(const string& sString, string& outLocalizedString, bool bEnglish = false) = 0;

    // Summary:
    //   Return the localized version corresponding to a label.
    // Description:
    //   A label has to start with '@' sign.
    // Parameters:
    //   sLabel              - Label to be translated, must start with '@' sign.
    //   outLocalizedString  - Localized version of the label.
    //   bEnglish            - if true, returns the always present English version of the label.
    // Returns:
    //   True if localization was successful, false otherwise.
    virtual bool LocalizeLabel(const char* sLabel, string& outLocalizedString, bool bEnglish = false) = 0;

    // Summary:
    //   Get localization info structure corresponding to a key (key=label without the '@' sign).
    // Parameters:
    //   sKey    - Key to be looked up. Key = Label without '@' sign.
    //   outGameInfo - Reference to localization info structure to be filled in.
    //  Returns:
    //    True if info for key was found, false otherwise.
    virtual bool GetLocalizedInfoByKey(const char* sKey, SLocalizedInfoGame& outGameInfo) = 0;

    // Summary:
    //   Get the sound localization info structure corresponding to a key.
    // Parameters:
    //   sKey         - Key to be looked up. Key = Label without '@' sign.
    //   outSoundInfo - reference to sound info structure to be filled in
    //                                  pSoundMoods requires nNumSoundMoods-times allocated memory
    //                                  on return nNumSoundMoods will hold how many SoundsMood entries are needed
    //                                  pEventParameters requires nNumEventParameters-times allocated memory
    //                                  on return nNumEventParameters will hold how many EventParameter entries are needed
    //                                  Passing 0 in the Num fields will make the query ignore checking for allocated memory
    // Returns:
    //   True if successful, false otherwise (key not found, or not enough memory provided to write additional info).
    virtual bool GetLocalizedInfoByKey(const char* sKey, SLocalizedSoundInfoGame* pOutSoundInfo) = 0;

    // Summary:
    //   Return number of localization entries.
    virtual int  GetLocalizedStringCount() = 0;

    // Summary:
    //   Get the localization info structure at index nIndex.
    // Parameters:
    //   nIndex  - Index.
    //   outEditorInfo - Reference to localization info structure to be filled in.
    // Returns:
    //   True if successful, false otherwise (out of bounds).
    virtual bool GetLocalizedInfoByIndex(int nIndex, SLocalizedInfoEditor& outEditorInfo) = 0;

    // Summary:
    //   Get the localization info structure at index nIndex.
    // Parameters:
    //   nIndex  - Index.
    //   outGameInfo - Reference to localization info structure to be filled in.
    // Returns:
    //   True if successful, false otherwise (out of bounds).
    virtual bool GetLocalizedInfoByIndex(int nIndex, SLocalizedInfoGame& outGameInfo) = 0;

    // Summary:
    //   Get the english localization info structure corresponding to a key.
    // Parameters:
    //   sKey         - Key to be looked up. Key = Label without '@' sign.
    //   sLocalizedString - Corresponding english language string.
    // Returns:
    //   True if successful, false otherwise (key not found).
    virtual bool GetEnglishString(const char* sKey, string& sLocalizedString) = 0;

    // Summary:
    //   Get Subtitle for Key or Label .
    // Parameters:
    //   sKeyOrLabel    - Key or Label to be used for subtitle lookup. Key = Label without '@' sign.
    //   outSubtitle    - Subtitle (untouched if Key/Label not found).
    //   bForceSubtitle - If true, get subtitle (sLocalized or sEnglish) even if not specified in Data file.
    // Returns:
    //   True if subtitle found (and outSubtitle filled in), false otherwise.
    virtual bool GetSubtitle(const char* sKeyOrLabel, string& outSubtitle, bool bForceSubtitle = false) = 0;

    // Description:
    //      These methods format outString depending on sString with ordered arguments
    //      FormatStringMessage(outString, "This is %2 and this is %1", "second", "first");
    // Arguments:
    //      outString - This is first and this is second.
    virtual void FormatStringMessage(string& outString, const string& sString, const char** sParams, int nParams) = 0;
    virtual void FormatStringMessage(string& outString, const string& sString, const char* param1, const char* param2 = 0, const char* param3 = 0, const char* param4 = 0) = 0;

    virtual void LocalizeTime(time_t t, bool bMakeLocalTime, bool bShowSeconds, string& outTimeString) = 0;
    virtual void LocalizeDate(time_t t, bool bMakeLocalTime, bool bShort, bool bIncludeWeekday, string& outDateString) = 0;
    virtual void LocalizeDuration(int seconds, string& outDurationString) = 0;
    virtual void LocalizeNumber(int number, string& outNumberString) = 0;
    virtual void LocalizeNumber(float number, int decimals, string& outNumberString) = 0;

    // Summary:
    //   Returns true if the project has localization configured for use, false otherwise.
    virtual bool ProjectUsesLocalization() const = 0;
    // </interfuscator:shuffle>

    static ILINE TLocalizationBitfield LocalizationBitfieldFromPILID(EPlatformIndependentLanguageID pilid)
    {
        assert(pilid >= 0 && pilid < ePILID_MAX_OR_INVALID);
        return (1 << pilid);
    }
};

// Summary:
//      Simple bus that notifies listeners that the language (g_language) has changed.
class LanguageChangeNotification
    : public AZ::EBusTraits
{
public:
    static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;

    virtual ~LanguageChangeNotification() = default;

    virtual void LanguageChanged() = 0;
};

using LanguageChangeNotificationBus = AZ::EBus<LanguageChangeNotification>;

#endif // CRYINCLUDE_CRYCOMMON_ILOCALIZATIONMANAGER_H
