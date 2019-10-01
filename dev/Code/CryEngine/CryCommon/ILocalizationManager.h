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

#include "LocalizationManagerBus.h"
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
    : public LocalizationManagerRequestBus::Handler
{
    //Platform independent language IDs. These are used to map the platform specific language codes to localization pakfiles
    //Please ensure that each entry in this enum has a corresponding entry in the PLATFORM_INDEPENDENT_LANGUAGE_NAMES array which is defined in LocalizedStringManager.cpp currently.
    enum EPlatformIndependentLanguageID
    {
        ePILID_English_US,
        ePILID_English_GB,
        ePILID_German_DE,
        ePILID_Russian_RU,
        ePILID_Polish_PL,
        ePILID_Turkish_TR,
        ePILID_Spanish_ES,
        ePILID_Spanish_MX,
        ePILID_French_FR,
        ePILID_French_CA,
        ePILID_Italian_IT,
        ePILID_Portugese_PT,
        ePILID_Portugese_BR,
        ePILID_Japanese_JP,
        ePILID_Korean_KR,
        ePILID_Chinese_T,
        ePILID_Chinese_S,
        ePILID_Dutch_NL,
        ePILID_Finnish_FI,
        ePILID_Swedish_SE,
        ePILID_Czech_CZ,
        ePILID_Norwegian_NO,
        ePILID_Arabic_SA,
        ePILID_Danish_DK,
        ePILID_MAX_OR_INVALID,   //Not a language, denotes the maximum number of languages or an unknown language
    };

    typedef uint32 TLocalizationBitfield;

    // <interfuscator:shuffle>
    virtual ~ILocalizationManager(){}
    virtual const char* LangNameFromPILID(const ILocalizationManager::EPlatformIndependentLanguageID id) = 0;
    virtual ILocalizationManager::EPlatformIndependentLanguageID PILIDFromLangName(AZStd::string langName) = 0;
    virtual ILocalizationManager::EPlatformIndependentLanguageID GetSystemLanguage() { return ILocalizationManager::EPlatformIndependentLanguageID::ePILID_English_US; }
    virtual ILocalizationManager::TLocalizationBitfield MaskSystemLanguagesFromSupportedLocalizations(const ILocalizationManager::TLocalizationBitfield systemLanguages) = 0;
    virtual ILocalizationManager::TLocalizationBitfield IsLanguageSupported(const ILocalizationManager::EPlatformIndependentLanguageID id) = 0;
    virtual bool SetLanguage(const char* sLanguage) override { return false; }
    virtual const char* GetLanguage() override { return nullptr; }

    virtual int GetLocalizationFormat() const { return -1; }
    virtual AZStd::string GetLocalizedSubtitleFilePath(const AZStd::string& localVideoPath, const AZStd::string& subtitleFileExtension) const { return ""; }
    virtual AZStd::string GetLocalizedLocXMLFilePath(const AZStd::string& localXmlPath) const { return ""; }
    // load the descriptor file with tag information
    virtual bool InitLocalizationData(const char* sFileName, bool bReload = false) = 0;
    // request to load loca data by tag. Actual loading will happen during next level load begin event.
    virtual bool RequestLoadLocalizationDataByTag(const char* sTag) = 0;
    // direct load of loca data by tag
    virtual bool LoadLocalizationDataByTag(const char* sTag, bool bReload = false) = 0;
    virtual bool ReleaseLocalizationDataByTag(const char* sTag) = 0;

    virtual bool LoadAllLocalizationData(bool bReload = false) = 0;
    virtual bool LoadExcelXmlSpreadsheet(const char* sFileName, bool bReload = false) override { return false; }
    virtual void ReloadData() override {};

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
    virtual bool LocalizeString_ch(const char* sString, string& outLocalizedString, bool bEnglish = false) override { return false; }

    // Function to be deprecated and replaced by LocalizeString_ch to support eBus interface.
    // Use LocalizeString_ch instead.
    AZ_DEPRECATED(virtual bool LocalizeString(const char* sString, string& outLocalizedString, bool bEnglish = false), "Deprecated. Use LocalizeString_ch instead.") 
    {
        return LocalizeString_ch(sString, outLocalizedString, bEnglish);
    }

    // Summary:
    //   Same as LocalizeString( const char* sString, string& outLocalizedString, bool bEnglish=false )
    //   but at the moment this is faster.
    virtual bool LocalizeString_s(const string& sString, string& outLocalizedString, bool bEnglish = false) override { return false; }

    // Function to be deprecated and replaced by LocalizeString_s to support eBus interface.
    // Use LocalizeString_s instead.
    AZ_DEPRECATED(virtual bool LocalizeString(const string& sString, string& outLocalizedString, bool bEnglish = false), "Deprecated. Use LocalizeString_s instead.") 
    { 
        return LocalizeString_s(sString, outLocalizedString, bEnglish);
    }

    // Summary:
    virtual void LocalizeAndSubstituteInternal(AZStd::string& locString, const AZStd::vector<AZStd::string>& keys, const AZStd::vector<AZStd::string>& values) override {}
    //   Return the localized version corresponding to a label.
    // Description:
    //   A label has to start with '@' sign.
    // Parameters:
    //   sLabel              - Label to be translated, must start with '@' sign.
    //   outLocalizedString  - Localized version of the label.
    //   bEnglish            - if true, returns the always present English version of the label.
    // Returns:
    //   True if localization was successful, false otherwise.
    virtual bool LocalizeLabel(const char* sLabel, string& outLocalizedString, bool bEnglish = false) override { return false; }
    virtual bool IsLocalizedInfoFound(const char* sKey) { return false; }

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
    virtual int  GetLocalizedStringCount() override { return -1; }

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
    virtual bool GetEnglishString(const char* sKey, string& sLocalizedString) override { return false; }

    // Summary:
    //   Get Subtitle for Key or Label .
    // Parameters:
    //   sKeyOrLabel    - Key or Label to be used for subtitle lookup. Key = Label without '@' sign.
    //   outSubtitle    - Subtitle (untouched if Key/Label not found).
    //   bForceSubtitle - If true, get subtitle (sLocalized or sEnglish) even if not specified in Data file.
    // Returns:
    //   True if subtitle found (and outSubtitle filled in), false otherwise.
    virtual bool GetSubtitle(const char* sKeyOrLabel, string& outSubtitle, bool bForceSubtitle = false) override { return false; }

    // Description:
    //      These methods format outString depending on sString with ordered arguments
    //      FormatStringMessage(outString, "This is %2 and this is %1", "second", "first");
    // Arguments:
    //      outString - This is first and this is second.
    virtual void FormatStringMessage_List(string& outString, const string& sString, const char** sParams, int nParams) override {}
    virtual void FormatStringMessage(string& outString, const string& sString, const char* param1, const char* param2 = 0, const char* param3 = 0, const char* param4 = 0) override {}

    // Function to be deprecated and replaced by FormatStringMessage_List to support eBus interface.
    // Use FormatStringMessage_List instead.
    AZ_DEPRECATED(virtual void FormatStringMessage(string& outString, const string& sString, const char** sParams, int nParams), "Deprecated. Use FormatStringMessage_List instead.") 
    {
        FormatStringMessage_List(outString, sString, sParams, nParams);
    }

    virtual void LocalizeTime(time_t t, bool bMakeLocalTime, bool bShowSeconds, string& outTimeString) override {}
    virtual void LocalizeDate(time_t t, bool bMakeLocalTime, bool bShort, bool bIncludeWeekday, string& outDateString) override {}
    virtual void LocalizeDuration(int seconds, string& outDurationString) override {}
    virtual void LocalizeNumber(int number, string& outNumberString) override {}
    virtual void LocalizeNumber_Decimal(float number, int decimals, string& outNumberString) override {}

    // Summary:
    //   Returns true if the project has localization configured for use, false otherwise.
    virtual bool ProjectUsesLocalization() const override { return false; }
    // </interfuscator:shuffle>

    static ILINE TLocalizationBitfield LocalizationBitfieldFromPILID(EPlatformIndependentLanguageID pilid)
    {
        assert(pilid >= 0 && pilid < ePILID_MAX_OR_INVALID);
        return (1 << pilid);
    }
};

// Summary:
//      Simple bus that notifies listeners that the language (g_language) has changed.




#endif // CRYINCLUDE_CRYCOMMON_ILOCALIZATIONMANAGER_H
