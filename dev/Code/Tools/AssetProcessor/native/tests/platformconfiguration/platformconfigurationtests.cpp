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

#include "native/tests/platformconfiguration/platformconfigurationtests.h"
#include "native/utilities/PlatformConfiguration.h"
#include "native/AssetManager/assetScanFolderInfo.h"
#include "native/assetprocessor.h"
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/Asset/AssetProcessorMessages.h>
#include <AzToolsFramework/AssetDatabase/AssetDatabaseConnection.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzCore/std/algorithm.h>

// make the internal calls public for the purposes of the unit test!
class UnitTestPlatformConfiguration : public AssetProcessor::PlatformConfiguration
{
    friend class GTEST_TEST_CLASS_NAME_(PlatformConfigurationUnitTests, Test_GemHandling);
    friend class GTEST_TEST_CLASS_NAME_(PlatformConfigurationUnitTests, Test_MetaFileTypes);
protected:
    bool ReadGems(QList<AssetProcessor::GemInformation>& /*gemInfoList*/) override
    {
        return true;
    }
};

PlatformConfigurationUnitTests::PlatformConfigurationUnitTests()
    : m_argc(0)
    , m_argv(0)
{
}

void PlatformConfigurationUnitTests::SetUp()
{
    using namespace AssetProcessor;
    m_qApp = new QCoreApplication(m_argc, m_argv);
    AssetProcessorTest::SetUp();
    AssetUtilities::ResetAssetRoot();
    
}

void PlatformConfigurationUnitTests::TearDown()
{
    AssetUtilities::ResetAssetRoot();
    delete m_qApp;
    AssetProcessor::AssetProcessorTest::TearDown();
}

TEST_F(PlatformConfigurationUnitTests, TestFailReadConfigFile_BadPlatform)
{
    using namespace AzToolsFramework::AssetSystem;
    using namespace AssetProcessor;

    const char* configFileName = ":/testdata/config_broken_badplatform.ini";
    UnitTestPlatformConfiguration config;
    m_absorber.Clear();
    ASSERT_FALSE(config.InitializeFromConfigFiles(configFileName));
    ASSERT_GT(m_absorber.m_numErrorsAbsorbed, 0);
}


TEST_F(PlatformConfigurationUnitTests, TestFailReadConfigFile_NoPlatform)
{
    using namespace AzToolsFramework::AssetSystem;
    using namespace AssetProcessor;

    const char* configFileName = ":/testdata/config_broken_noplatform.ini";
    UnitTestPlatformConfiguration config;
    m_absorber.Clear();
    ASSERT_FALSE(config.InitializeFromConfigFiles(configFileName));
    ASSERT_GT(m_absorber.m_numErrorsAbsorbed, 0);
}

TEST_F(PlatformConfigurationUnitTests, TestFailReadConfigFile_NoScanFolders)
{
    using namespace AzToolsFramework::AssetSystem;
    using namespace AssetProcessor;

    const char* configFileName = ":/testdata/config_broken_noscans.ini";
    UnitTestPlatformConfiguration config;
    m_absorber.Clear();
    ASSERT_FALSE(config.InitializeFromConfigFiles(configFileName));
    ASSERT_GT(m_absorber.m_numErrorsAbsorbed, 0);
}

TEST_F(PlatformConfigurationUnitTests, TestFailReadConfigFile_BrokenRecognizers)
{
    using namespace AzToolsFramework::AssetSystem;
    using namespace AssetProcessor;

    const char* configFileName = ":/testdata/config_broken_recognizers.ini";
    UnitTestPlatformConfiguration config;
    m_absorber.Clear();
    ASSERT_FALSE(config.InitializeFromConfigFiles(configFileName));
    ASSERT_GT(m_absorber.m_numErrorsAbsorbed, 0);
}

TEST_F(PlatformConfigurationUnitTests, TestFailReadConfigFile_Regular_Platforms)
{
    using namespace AzToolsFramework::AssetSystem;
    using namespace AssetProcessor;

    const char* configFileName = ":/testdata/config_regular.ini";
    UnitTestPlatformConfiguration config;
    m_absorber.Clear();
    ASSERT_TRUE(config.InitializeFromConfigFiles(configFileName));
    ASSERT_EQ(m_absorber.m_numErrorsAbsorbed, 0);

    // verify the data.
    ASSERT_NE(config.GetPlatformByIdentifier(AssetProcessor::CURRENT_PLATFORM), nullptr);
    ASSERT_NE(config.GetPlatformByIdentifier("es3"), nullptr);
    ASSERT_NE(config.GetPlatformByIdentifier("server"), nullptr);
    ASSERT_EQ(config.GetPlatformByIdentifier("xbone"), nullptr); // ACCEPTED_USE

    ASSERT_TRUE(config.GetPlatformByIdentifier("es3")->HasTag("mobile"));
    ASSERT_TRUE(config.GetPlatformByIdentifier("es3")->HasTag("renderer"));
    ASSERT_TRUE(config.GetPlatformByIdentifier("es3")->HasTag("android"));
    ASSERT_TRUE(config.GetPlatformByIdentifier("server")->HasTag("server"));
    ASSERT_FALSE(config.GetPlatformByIdentifier("es3")->HasTag("server"));
    ASSERT_FALSE(config.GetPlatformByIdentifier("server")->HasTag("renderer"));

}


TEST_F(PlatformConfigurationUnitTests, TestFailReadConfigFile_RegularScanfolder)
{
    using namespace AzToolsFramework::AssetSystem;
    using namespace AssetProcessor;

    const char* configFileName = ":/testdata/config_regular.ini";
    UnitTestPlatformConfiguration config;
    m_absorber.Clear();
    ASSERT_TRUE(config.InitializeFromConfigFiles(configFileName));
    ASSERT_EQ(m_absorber.m_numErrorsAbsorbed, 0);

    ASSERT_EQ(config.GetScanFolderCount(), 3); // the two, and then the one that has the same data as prior but different identifier.
    ASSERT_EQ(config.GetScanFolderAt(0).GetDisplayName(), QString("SamplesProject Scan Folder"));
    ASSERT_EQ(config.GetScanFolderAt(0).GetOutputPrefix(), QString());
    ASSERT_EQ(config.GetScanFolderAt(0).RecurseSubFolders(), true);
    ASSERT_EQ(config.GetScanFolderAt(0).GetOrder(), 0);
    // its important that this does NOT change and this makes sure the old way of doing it (case-sensitive name) persists
    ASSERT_EQ(config.GetScanFolderAt(0).GetPortableKey(), QString("from-ini-file-Game")); 
    
    ASSERT_EQ(config.GetScanFolderAt(1).GetDisplayName(), QString("FeatureTests"));
    ASSERT_EQ(config.GetScanFolderAt(1).GetOutputPrefix(), QString("featuretestsoutputfolder")); // to prove its not related to display name
    ASSERT_EQ(config.GetScanFolderAt(1).RecurseSubFolders(), false);
    ASSERT_EQ(config.GetScanFolderAt(1).GetOrder(), 5000);
    // this proves that the featuretests name is used instead of the output prefix
    ASSERT_EQ(config.GetScanFolderAt(1).GetPortableKey(), QString("from-ini-file-FeatureTests"));

    ASSERT_EQ(config.GetScanFolderAt(2).GetDisplayName(), QString("FeatureTests2"));
    ASSERT_EQ(config.GetScanFolderAt(2).GetOutputPrefix(), QString("featuretestsoutputfolder")); // to prove its not related to display name
    ASSERT_EQ(config.GetScanFolderAt(2).RecurseSubFolders(), false);
    ASSERT_EQ(config.GetScanFolderAt(2).GetOrder(), 6000);
    // this proves that the featuretests name is used instead of the output prefix
    ASSERT_EQ(config.GetScanFolderAt(2).GetPortableKey(), QString("from-ini-file-FeatureTests2"));
}

TEST_F(PlatformConfigurationUnitTests, TestFailReadConfigFile_RegularScanfolderPlatformSpecific)
{
    using namespace AzToolsFramework::AssetSystem;
    using namespace AssetProcessor;

    const char* configFileName = ":/testdata/config_regular_platform_scanfolder.ini";
    UnitTestPlatformConfiguration config;
    m_absorber.Clear();
    ASSERT_TRUE(config.InitializeFromConfigFiles(configFileName));
    ASSERT_EQ(m_absorber.m_numErrorsAbsorbed, 0);

    ASSERT_EQ(config.GetScanFolderCount(), 5);
    ASSERT_EQ(config.GetScanFolderAt(0).GetDisplayName(), QString("gameoutput"));
    AZStd::vector<AssetBuilderSDK::PlatformInfo>& platforms = config.GetScanFolderAt(0).GetPlatforms();
    ASSERT_EQ(platforms.size(), 4);
    ASSERT_TRUE(AZStd::find(platforms.begin(), platforms.end(), AssetBuilderSDK::PlatformInfo(AssetProcessor::CURRENT_PLATFORM, AZStd::unordered_set<AZStd::string>{})) != platforms.end());
    ASSERT_TRUE(AZStd::find(platforms.begin(), platforms.end(), AssetBuilderSDK::PlatformInfo("es3", AZStd::unordered_set<AZStd::string>{})) != platforms.end());
    ASSERT_TRUE(AZStd::find(platforms.begin(), platforms.end(), AssetBuilderSDK::PlatformInfo("ios", AZStd::unordered_set<AZStd::string>{})) != platforms.end());
    ASSERT_TRUE(AZStd::find(platforms.begin(), platforms.end(), AssetBuilderSDK::PlatformInfo("server", AZStd::unordered_set<AZStd::string>{})) != platforms.end());

    ASSERT_EQ(config.GetScanFolderAt(1).GetDisplayName(), QString("editoroutput"));
    platforms = config.GetScanFolderAt(1).GetPlatforms();
    ASSERT_EQ(platforms.size(), 2);
    ASSERT_TRUE(AZStd::find(platforms.begin(), platforms.end(), AssetBuilderSDK::PlatformInfo(AssetProcessor::CURRENT_PLATFORM, AZStd::unordered_set<AZStd::string>{})) != platforms.end());
    ASSERT_TRUE(AZStd::find(platforms.begin(), platforms.end(), AssetBuilderSDK::PlatformInfo("es3", AZStd::unordered_set<AZStd::string>{})) != platforms.end());

    ASSERT_EQ(config.GetScanFolderAt(2).GetDisplayName(), QString("folder1output"));
    platforms = config.GetScanFolderAt(2).GetPlatforms();
    ASSERT_EQ(platforms.size(), 1);
    ASSERT_TRUE(AZStd::find(platforms.begin(), platforms.end(), AssetBuilderSDK::PlatformInfo("es3", AZStd::unordered_set<AZStd::string>{})) != platforms.end());

    ASSERT_EQ(config.GetScanFolderAt(3).GetDisplayName(), QString("folder2output"));
    platforms = config.GetScanFolderAt(3).GetPlatforms();
    ASSERT_EQ(platforms.size(), 3);
    ASSERT_TRUE(AZStd::find(platforms.begin(), platforms.end(), AssetBuilderSDK::PlatformInfo(AssetProcessor::CURRENT_PLATFORM, AZStd::unordered_set<AZStd::string>{})) != platforms.end());
    ASSERT_TRUE(AZStd::find(platforms.begin(), platforms.end(), AssetBuilderSDK::PlatformInfo("ios", AZStd::unordered_set<AZStd::string>{})) != platforms.end());
    ASSERT_TRUE(AZStd::find(platforms.begin(), platforms.end(), AssetBuilderSDK::PlatformInfo("server", AZStd::unordered_set<AZStd::string>{})) != platforms.end());
    
    ASSERT_EQ(config.GetScanFolderAt(4).GetDisplayName(), QString("folder3output"));
    platforms = config.GetScanFolderAt(4).GetPlatforms();
    ASSERT_EQ(platforms.size(), 0);
}


TEST_F(PlatformConfigurationUnitTests, TestFailReadConfigFile_RegularExcludes)
{
    using namespace AzToolsFramework::AssetSystem;
    using namespace AssetProcessor;

    const char* configFileName = ":/testdata/config_regular.ini";
    UnitTestPlatformConfiguration config;
    m_absorber.Clear();
    ASSERT_TRUE(config.InitializeFromConfigFiles(configFileName));
    ASSERT_EQ(m_absorber.m_numErrorsAbsorbed, 0);

    ASSERT_TRUE(config.IsFileExcluded("blahblah/$tmp_01.test"));
    ASSERT_FALSE(config.IsFileExcluded("blahblah/tmp_01.test"));

    ASSERT_TRUE(config.IsFileExcluded("blahblah/Levels/blahblah_hold/whatever.test"));
    ASSERT_FALSE(config.IsFileExcluded("blahblah/Levels/blahblahhold/whatever.test"));
}

TEST_F(PlatformConfigurationUnitTests, TestFailReadConfigFile_Recognizers)
{
    using namespace AzToolsFramework::AssetSystem;
    using namespace AssetProcessor;

    const char* configFileName = ":/testdata/config_regular.ini";
    UnitTestPlatformConfiguration config;
    m_absorber.Clear();
    ASSERT_TRUE(config.InitializeFromConfigFiles(configFileName));
    ASSERT_EQ(m_absorber.m_numErrorsAbsorbed, 0);

    const AssetProcessor::RecognizerContainer& recogs = config.GetAssetRecognizerContainer();

    ASSERT_EQ(recogs.size(), 6);

    ASSERT_TRUE(recogs.contains("i_caf"));
    ASSERT_EQ(recogs["i_caf"].m_patternMatcher.GetBuilderPattern().m_pattern, "*.i_caf");
    ASSERT_EQ(recogs["i_caf"].m_patternMatcher.GetBuilderPattern().m_type, AssetBuilderSDK::AssetBuilderPattern::Wildcard);
    ASSERT_EQ(recogs["i_caf"].m_platformSpecs.size(), 2);
    ASSERT_TRUE(recogs["i_caf"].m_platformSpecs.contains("es3"));
    ASSERT_TRUE(recogs["i_caf"].m_platformSpecs.contains(AssetProcessor::CURRENT_PLATFORM));
    ASSERT_FALSE(recogs["i_caf"].m_platformSpecs.contains("server")); // server has been set to skip.
    ASSERT_EQ(recogs["i_caf"].m_platformSpecs["es3"].m_extraRCParams, "mobile");
    ASSERT_EQ(recogs["i_caf"].m_platformSpecs[AssetProcessor::CURRENT_PLATFORM].m_extraRCParams, "defaultparams");

    ASSERT_TRUE(recogs.contains("caf"));
    ASSERT_TRUE(recogs["caf"].m_platformSpecs.contains("es3"));
    ASSERT_TRUE(recogs["caf"].m_platformSpecs.contains("server"));
    ASSERT_TRUE(recogs["caf"].m_platformSpecs.contains(CURRENT_PLATFORM));
    ASSERT_EQ(recogs["caf"].m_platformSpecs.size(), 3);
    ASSERT_EQ(recogs["caf"].m_platformSpecs["es3"].m_extraRCParams, "rendererparams");
    ASSERT_EQ(recogs["caf"].m_platformSpecs[CURRENT_PLATFORM].m_extraRCParams, "rendererparams");
    ASSERT_EQ(recogs["caf"].m_platformSpecs["server"].m_extraRCParams, "copy");

    ASSERT_TRUE(recogs.contains("mov"));
    ASSERT_TRUE(recogs["mov"].m_platformSpecs.contains("es3"));
    ASSERT_TRUE(recogs["mov"].m_platformSpecs.contains("server"));
    ASSERT_TRUE(recogs["mov"].m_platformSpecs.contains(CURRENT_PLATFORM));
    ASSERT_EQ(recogs["mov"].m_platformSpecs.size(), 3);
    ASSERT_EQ(recogs["mov"].m_platformSpecs["es3"].m_extraRCParams, "platformspecificoverride");
    ASSERT_EQ(recogs["mov"].m_platformSpecs[CURRENT_PLATFORM].m_extraRCParams, "rendererparams");
    ASSERT_EQ(recogs["mov"].m_platformSpecs["server"].m_extraRCParams, "copy");

    // the "rend" test makes sure that even if you dont specify 'params' its still there by default for all enabled platforms.
    // (but platforms can override it)
    ASSERT_TRUE(recogs.contains("rend"));
    ASSERT_TRUE(recogs["rend"].m_platformSpecs.contains(CURRENT_PLATFORM));
    ASSERT_TRUE(recogs["rend"].m_platformSpecs.contains("es3"));
    ASSERT_TRUE(recogs["rend"].m_platformSpecs.contains("server"));
    ASSERT_FALSE(recogs["rend"].m_platformSpecs.contains("osx_gl")); // this is not an enabled platform and should not be there.
    ASSERT_EQ(recogs["rend"].m_platformSpecs.size(), 3);
    ASSERT_EQ(recogs["rend"].m_platformSpecs[CURRENT_PLATFORM].m_extraRCParams, "rendererparams");
    ASSERT_EQ(recogs["rend"].m_platformSpecs["es3"].m_extraRCParams, "rendererparams");
    ASSERT_EQ(recogs["rend"].m_platformSpecs["server"].m_extraRCParams, ""); // default if not specified is empty string
   
    ASSERT_TRUE(recogs.contains("alldefault"));
    ASSERT_TRUE(recogs["alldefault"].m_platformSpecs.contains(CURRENT_PLATFORM));
    ASSERT_TRUE(recogs["alldefault"].m_platformSpecs.contains("es3"));
    ASSERT_TRUE(recogs["alldefault"].m_platformSpecs.contains("server"));
    ASSERT_FALSE(recogs["alldefault"].m_platformSpecs.contains("osx_gl")); // this is not an enabled platform and should not be there.
    ASSERT_EQ(recogs["alldefault"].m_platformSpecs.size(), 3);
    ASSERT_EQ(recogs["alldefault"].m_platformSpecs[CURRENT_PLATFORM].m_extraRCParams, "");
    ASSERT_EQ(recogs["alldefault"].m_platformSpecs["es3"].m_extraRCParams, "");
    ASSERT_EQ(recogs["alldefault"].m_platformSpecs["server"].m_extraRCParams, "");

    ASSERT_TRUE(recogs.contains("skipallbutone"));
    ASSERT_FALSE(recogs["skipallbutone"].m_platformSpecs.contains(CURRENT_PLATFORM));
    ASSERT_FALSE(recogs["skipallbutone"].m_platformSpecs.contains("es3"));
    ASSERT_TRUE(recogs["skipallbutone"].m_platformSpecs.contains("server")); // server is only one enabled (set to copy)
    ASSERT_EQ(recogs["skipallbutone"].m_platformSpecs.size(), 1);
    ASSERT_EQ(recogs["skipallbutone"].m_platformSpecs["server"].m_extraRCParams, "copy");
}


TEST_F(PlatformConfigurationUnitTests, TestFailReadConfigFile_Overrides)
{
    using namespace AzToolsFramework::AssetSystem;
    using namespace AssetProcessor;

    const char* configFileName = ":/testdata/config_regular.ini";
    const char* overrideConfigFileName = ":/testdata/config_overrides.ini";
    UnitTestPlatformConfiguration config;
    m_absorber.Clear();
    ASSERT_TRUE(config.InitializeFromConfigFiles(configFileName, overrideConfigFileName));
    ASSERT_EQ(m_absorber.m_numErrorsAbsorbed, 0);

    const AssetProcessor::RecognizerContainer& recogs = config.GetAssetRecognizerContainer();

    // note that the override config DISABLES the server platform - and this in turn disables the "server only" compile rule called skipallbutone

    // verify the data.
    ASSERT_NE(config.GetPlatformByIdentifier(AssetProcessor::CURRENT_PLATFORM), nullptr);
    ASSERT_NE(config.GetPlatformByIdentifier("es3"), nullptr);
    ASSERT_NE(config.GetPlatformByIdentifier("ps4"), nullptr); // ACCEPTED_USE
    // this override swaps server with ps4 in that it turns ON ps4, turns off server // ACCEPTED_USE
    ASSERT_EQ(config.GetPlatformByIdentifier("xbone"), nullptr); // ACCEPTED_USE
    ASSERT_EQ(config.GetPlatformByIdentifier("server"), nullptr); // this should be off due to overrides

    // there is a rule which only output on server, so that rule should be omitted
    ASSERT_EQ(recogs.size(), 5); // so theres 5 instead of 6.
    ASSERT_FALSE(recogs.contains("skipallbutone")); // this is the rule that had only a server.

    ASSERT_TRUE(recogs.contains("i_caf"));
    ASSERT_EQ(recogs["i_caf"].m_patternMatcher.GetBuilderPattern().m_pattern, "*.i_caf");
    ASSERT_EQ(recogs["i_caf"].m_patternMatcher.GetBuilderPattern().m_type, AssetBuilderSDK::AssetBuilderPattern::Wildcard);
    ASSERT_EQ(recogs["i_caf"].m_platformSpecs.size(), 3);
    ASSERT_TRUE(recogs["i_caf"].m_platformSpecs.contains("es3"));
    ASSERT_TRUE(recogs["i_caf"].m_platformSpecs.contains("ps4")); // ACCEPTED_USE
    ASSERT_TRUE(recogs["i_caf"].m_platformSpecs.contains(AssetProcessor::CURRENT_PLATFORM));
    ASSERT_FALSE(recogs["i_caf"].m_platformSpecs.contains("server")); // server has been set to skip.
    ASSERT_EQ(recogs["i_caf"].m_platformSpecs["es3"].m_extraRCParams, "mobile");
    ASSERT_EQ(recogs["i_caf"].m_platformSpecs[AssetProcessor::CURRENT_PLATFORM].m_extraRCParams, "defaultparams");
    ASSERT_EQ(recogs["i_caf"].m_platformSpecs["ps4"].m_extraRCParams, "copy"); // ACCEPTED_USE


}

TEST_F(PlatformConfigurationUnitTests, Test_GemHandling)
{
    UnitTestPlatformConfiguration config;
    QTemporaryDir tempEngineRoot;
    QDir tempPath(tempEngineRoot.path());
    UnitTestUtils::CreateDummyFile(tempPath.absoluteFilePath("bootstrap.cfg"), QString("sys_game_folder=SamplesProject\n"));
    AssetUtilities::ResetAssetRoot();

    QDir computedEngineRoot;
    ASSERT_TRUE(AssetUtilities::ComputeAssetRoot(computedEngineRoot, &tempPath));
    ASSERT_TRUE(!computedEngineRoot.absolutePath().isEmpty());
    ASSERT_TRUE(tempPath.absolutePath() == computedEngineRoot.absolutePath());

    // create ONE of the two files - they are optional, but the paths to them should always be checked and generated.
    ASSERT_TRUE(UnitTestUtils::CreateDummyFile(tempPath.absoluteFilePath("Gems/LyShine/AssetProcessorGemConfig.ini"), ";nothing to see here"));

    // note that it is expected that the gems system gives us absolute paths.
    QList<AssetProcessor::GemInformation> fakeGems;
    fakeGems.push_back({ "0fefab3f13364722b2eab3b96ce2bf20", tempPath.absoluteFilePath("Gems/LyShine").toUtf8().constData(), "Gems/LyShine", "LyShine", true, false });// true = pretend this is a game gem.
    fakeGems.push_back({ "ff06785f7145416b9d46fde39098cb0c", tempPath.absoluteFilePath("Gems/LmbrCentral/v2").toUtf8().constData(), "Gems/LmbrCentral/v2", "LmbrCentral", false, false });

    // reading gems via the Gems System is already to be tested in the actual Gems API tests.
    // to avoid trying to load those DLLs we avoid calling the acutal ReadGems function
    config.AddGemScanFolders(fakeGems);
    QStringList allConfigFilesList;
    bool foundGameGem = false;
    // it is intentional that you always get sent back the list of all available config files to read from 
    config.AddGemConfigFiles(fakeGems, allConfigFilesList, foundGameGem);

    ASSERT_TRUE(allConfigFilesList.size() == 2);
    // note the opposite order here.  This ensures the Game Gem is at the end.
    ASSERT_TRUE(allConfigFilesList[0] == tempPath.absoluteFilePath("Gems/LmbrCentral/v2/AssetProcessorGemConfig.ini"));
    ASSERT_TRUE(allConfigFilesList[1] == tempPath.absoluteFilePath("Gems/LyShine/AssetProcessorGemConfig.ini"));

    ASSERT_TRUE(foundGameGem);

    QString expectedScanFolder = tempPath.absoluteFilePath("Gems/LyShine/Assets");

    AssetUtilities::ResetAssetRoot();

    ASSERT_TRUE(config.GetScanFolderCount() == 4);
    ASSERT_TRUE(config.GetScanFolderAt(0).IsRoot() == false);
    ASSERT_TRUE(config.GetScanFolderAt(0).GetOutputPrefix().isEmpty());
    ASSERT_TRUE(config.GetScanFolderAt(0).RecurseSubFolders());
    // the first one is a game gem, so its order should be above 1 but below 100.
    ASSERT_TRUE((config.GetScanFolderAt(0).GetOrder() >= 1) && (config.GetScanFolderAt(0).GetOrder() <= 100));
    ASSERT_TRUE(config.GetScanFolderAt(0).ScanPath().compare(expectedScanFolder, Qt::CaseInsensitive) == 0);

    // for each gem, there are currently 2 scanfolders, in that order.  The first one is the gem assets folder, with no output prefix
    // the second one is the one that exists only for the "gem.json file", is root, non recursive.

    expectedScanFolder = tempPath.absoluteFilePath("Gems/LyShine");
    QString expectedGemOutputPrefix = QString("Gems/LyShine").toLower();
    ASSERT_TRUE(config.GetScanFolderAt(1).IsRoot() == true);
    ASSERT_TRUE(config.GetScanFolderAt(1).GetOutputPrefix() == expectedGemOutputPrefix);
    ASSERT_TRUE(!config.GetScanFolderAt(1).RecurseSubFolders());

    // the root for a game gem should have similar orderp riority.
    ASSERT_TRUE((config.GetScanFolderAt(1).GetOrder() >= 1) && (config.GetScanFolderAt(0).GetOrder() <= 100));
    ASSERT_TRUE(config.GetScanFolderAt(1).ScanPath().compare(expectedScanFolder, Qt::CaseInsensitive) == 0);

    expectedScanFolder = tempPath.absoluteFilePath("Gems/LmbrCentral/v2/Assets");
    ASSERT_TRUE(config.GetScanFolderAt(2).IsRoot() == false);
    ASSERT_TRUE(config.GetScanFolderAt(2).GetOutputPrefix().isEmpty());
    ASSERT_TRUE(config.GetScanFolderAt(2).RecurseSubFolders());
    ASSERT_TRUE(config.GetScanFolderAt(2).GetOrder() > config.GetScanFolderAt(0).GetOrder());
    ASSERT_TRUE(config.GetScanFolderAt(2).ScanPath().compare(expectedScanFolder, Qt::CaseInsensitive) == 0);

    // also test the lmbrcentral gem folder.
    expectedScanFolder = tempPath.absoluteFilePath("Gems/LmbrCentral/v2");
    expectedGemOutputPrefix = QString("Gems/LmbrCentral/v2").toLower();

    ASSERT_TRUE(config.GetScanFolderAt(3).IsRoot() == true);
    ASSERT_TRUE(config.GetScanFolderAt(3).GetOutputPrefix() == expectedGemOutputPrefix);
    ASSERT_TRUE(!config.GetScanFolderAt(3).RecurseSubFolders());

    // anything thats not a game gem should have a much higher order.
    ASSERT_TRUE(config.GetScanFolderAt(3).GetOrder() >= 100);
    ASSERT_TRUE(config.GetScanFolderAt(3).ScanPath().compare(expectedScanFolder, Qt::CaseInsensitive) == 0);
}

TEST_F(PlatformConfigurationUnitTests, Test_MetaFileTypes)
{
    UnitTestPlatformConfiguration config;

    config.AddMetaDataType("xxxx", "");
    config.AddMetaDataType("yyyy", "zzzz");
    ASSERT_TRUE(config.MetaDataFileTypesCount() == 2);
    ASSERT_TRUE(QString::compare(config.GetMetaDataFileTypeAt(1).first, "yyyy", Qt::CaseInsensitive) == 0);
    ASSERT_TRUE(QString::compare(config.GetMetaDataFileTypeAt(1).second, "zzzz", Qt::CaseInsensitive) == 0);
}
