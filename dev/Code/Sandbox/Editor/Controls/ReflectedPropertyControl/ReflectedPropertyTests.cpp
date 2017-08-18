#include "stdafx.h"

#include "ReflectedPropertyTests.h"
#include "ReflectedPropertyCtrl.h"
#include "ReflectedVar.h"
#include "PropertyCtrl.h"
#include "UIEnumsDatabase.h"
#include "Particles/ParticleUIDefinition.h"
#include <AzToolsFramework/UI/PropertyEditor/ReflectedPropertyEditor.hxx>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <QDebug>

#ifdef KDAB_TEMPORARILY_REMOVED
AZ::SerializeContext* ReflectedVarTest::s_serializeContext = nullptr;
CVarBlock * ReflectedPropertyCtrlTest::s_varBlock = nullptr;
CVarBlock * ReflectedPropertyCtrlTest::s_varBlockParticle = nullptr;

template <class T> IVariable* CreateEnumVariable(const char *varName, const T &value, IVariable *parent, unsigned char dataType);

class UserCustomItemGetter : public IVariable::IGetCustomItems
{
public:
    UserCustomItemGetter(CString dialogTitle, bool useTree, bool testFailReturningItems = false) : m_dialogTitle(dialogTitle), m_useTree(useTree), m_testFailReturningItems(testFailReturningItems) 
    {
    }

    bool GetItems(IVariable* /* pVar */, std::vector<SItem>& items , CString& outDialogTitle ) override
    {
        outDialogTitle = m_dialogTitle;
        items.push_back(SItem("Car", "A red sports car"));
        items.push_back(SItem("Thunderstorm", "It's raining really hard!"));
        items.push_back(SItem("Pasta", "Farfalle"));
        items.push_back(SItem("Banjo", "A banjo for playing bluegrass music"));
        return !m_testFailReturningItems;
    }

    bool UseTree() override {return m_useTree;}
    const char* GetTreeSeparator() override {return "/";}

    private:
        bool m_useTree;
        CString m_dialogTitle;
        bool m_testFailReturningItems;
};

void AddUIEnums()
{
    // Spec settings for shadow casting lights
    string SpecString[4];
    std::vector<CString> types;
    types.push_back("Never=0");
    SpecString[0].Format("VeryHigh Spec=%d", CONFIG_VERYHIGH_SPEC);
    types.push_back(SpecString[0].c_str());
    SpecString[1].Format("High Spec=%d", CONFIG_HIGH_SPEC);
    types.push_back(SpecString[1].c_str());
    SpecString[2].Format("Medium Spec=%d", CONFIG_MEDIUM_SPEC);
    types.push_back(SpecString[2].c_str());
    SpecString[3].Format("Low Spec=%d", CONFIG_LOW_SPEC);
    types.push_back(SpecString[3].c_str());
    GetIEditor()->GetUIEnumsDatabase()->SetEnumStrings("CastShadowsTest", types);

    // Power-of-two percentages
    string percentStringPOT[5];
    types.clear();
    percentStringPOT[0].Format("Default=%d", 0);
    types.push_back(percentStringPOT[0].c_str());
    percentStringPOT[1].Format("12.5=%d", 1);
    types.push_back(percentStringPOT[1].c_str());
    percentStringPOT[2].Format("25=%d", 2);
    types.push_back(percentStringPOT[2].c_str());
    percentStringPOT[3].Format("50=%d", 3);
    types.push_back(percentStringPOT[3].c_str());
    percentStringPOT[4].Format("100=%d", 4);
    types.push_back(percentStringPOT[4].c_str());
    GetIEditor()->GetUIEnumsDatabase()->SetEnumStrings("ShadowMinResPercentTest", types);
}


CVarBlock* ReflectedPropertyCtrlTest::GenerateTestData()
{
    CVarBlock *varBlock = new CVarBlock;
    CVariableArray *table = new CVariableArray;
    table->SetName("Key Properties");
    varBlock->AddVariable(table);

    //Test ePropertyBool, ePropertyInt, ePropertyFloat, ePropertyString, ePropertyAngle
    CreateVariable("Min", 100, table);
    CreateVariable("Angle", 3.14f, table, IVariable::DT_ANGLE); //should be 180 degrees in editor
    CreateVariable("Scale", 2.5f, table);
    CreateVariable("Enabled", true, table);
    CreateVariable("First Name", CString("John"), table);

    //ePropertyVector, ePropertyVector2, ePropertyVector4
    CreateVariable("Vector 2", Vec2(10, 20), table);
    CreateVariable("Vector 3", Vec3(10, 20, 30), table);
    CreateVariable("Vector 4", Vec4(10, 20, 30, 40), table);

    // enums representing strings (ePropertySelection)
    CVariableEnum<CString> *format = new CVariableEnum<CString>;
    format->SetName("Output Format");
    format->SetEnumList(NULL);
    format->AddEnumItem("jpg", "jpg");
    format->AddEnumItem("bmp", "bmp");
    format->AddEnumItem("tga", "tga");
    format->AddEnumItem("hdr", "hdr");
    format->SetDisplayValue("tga");
    table->AddVariable(format);

    //Test enums representing ints (ePropertySelection)
    CVariableEnum<int> *bufferToCapture = new CVariableEnum<int>;
    format->SetName("Buffer(s) to capture");
    bufferToCapture->SetEnumList(NULL);
    bufferToCapture->AddEnumItem("Just frame", 0);
    bufferToCapture->AddEnumItem("Frame & miscs", 1);
    bufferToCapture->AddEnumItem("Stereo", 2);
    bufferToCapture->SetDisplayValue("Stereo");
    table->AddVariable(bufferToCapture);
    format->SetName("Buffer(s) to capture");

    //Test enums implmented via the UIEnumsDatabase.
    //These have a PropertyType corresponding to their underlying data type (string, float, int) but are edited via an enum lookup. 
    AddUIEnums();
    IVariable * pShadowMinRes = CreateVariable("CastShadowsTest", 1, table, IVariable::DT_UIENUM);
    IVariable * pCastShadowVar = CreateVariable("ShadowMinResPercentTest", 12.5f, table, IVariable::DT_UIENUM);
    pShadowMinRes->SetFlags(pShadowMinRes->GetFlags() | IVariable::UI_UNSORTED);
    pCastShadowVar->SetFlags(pCastShadowVar->GetFlags() | IVariable::UI_UNSORTED);

    //ePropertyAnimation
    IVariable *animation = CreateVariable("Animation", CString("animation_string"), table, IVariable::DT_ANIMATION);

    //ePropertyColor as Vec3 and COLORREF
    CreateVariable("Fade Color", Vec3(1.0, .5, 0), table, IVariable::DT_COLOR);
    COLORREF col = QColor(200, 100, 10);
    CreateVariable("COLORREF Color", (int)col, table, IVariable::DT_COLOR);

    //ePropertyTexture
    CreateVariable("Texture", CString("super/duper/really/extra/long/file/path/to/texture"), table, IVariable::DT_TEXTURE);

    //ePropertyAudioTrigger
    CreateVariable("Audio Trigger", CString("mute_all"), table, IVariable::DT_AUDIO_TRIGGER);

    //ePropertyFile
    CreateVariable("File", CString("c:/file/path/to/file.txt"), table, IVariable::DT_FILE);

    //Test for odd case of setting the variable type of an array to something other than an array so it can hold a value plus children.
    CVariableArray *textureArray = new CVariableArray;
    textureArray->SetName("Diffuse");
    textureArray->SetDataType(IVariable::DT_TEXTURE);
    textureArray->Set("materials/gloss.tif");
    table->AddVariable(textureArray);
    CreateVariable<bool>("IsProjectedTexGen", true, textureArray, IVariable::DT_BOOLEAN);

    CVariableEnum<CString> *texType= new CVariableEnum<CString>;
    texType->SetName("Texture Type");
    texType->SetEnumList(NULL);
    texType->AddEnumItem("2D", "2D");
    texType->AddEnumItem("3D", "3D");
    texType->SetDisplayValue("2D");
    textureArray->AddVariable(texType);

    CreateVariable("Shader", CString("shader"), table, IVariable::DT_SHADER);
    CreateVariable("Material", CString("TEST_MAT"), table, IVariable::DT_MATERIAL);
    CreateVariable("AI Behavior", CString("behave"), table, IVariable::DT_AI_BEHAVIOR);
    CreateVariable("AI Anchor", CString("TEST_ANCHOR"), table, IVariable::DT_AI_ANCHOR);
#ifdef USE_DEPRECATED_AI_CHARACTER_SYSTEM
    CreateVariable("AI Character", CString("character"), table, IVariable::DT_AI_CHARACTER);
#endif

    //Smart Object Properties
    CreateVariable("SO Class", CString(""), table, IVariable::DT_SOCLASS);
    CreateVariable("SO Classes", CString(""), table, IVariable::DT_SOCLASSES);
    CreateVariable("SO State", CString(""), table, IVariable::DT_SOSTATE);
    CreateVariable("SO States", CString(""), table, IVariable::DT_SOSTATES);
    CreateVariable("SO State Pattern", CString(""), table, IVariable::DT_SOSTATEPATTERN);
    CreateVariable("SO Action", CString(""), table, IVariable::DT_SOACTION);
    CreateVariable("SO Helper", CString(""), table, IVariable::DT_SOHELPER);
    CreateVariable("SO Nav Helper", CString(""), table, IVariable::DT_SONAVHELPER);
    CreateVariable("SO Anim Helper", CString(""), table, IVariable::DT_SOANIMHELPER);
    CreateVariable("SO Event", CString(""), table, IVariable::DT_SOEVENT);
    CreateVariable("SO Template", CString(""), table, IVariable::DT_SOTEMPLATE);

    CreateVariable("Equip", CString(""), table, IVariable::DT_EQUIP);
    CreateVariable("Reverb Preset", CString(""), table, IVariable::DT_REVERBPRESET);
    CreateVariable("Custom Action", CString(""), table, IVariable::DT_CUSTOMACTION);
    CreateVariable("Game Token", CString(""), table, IVariable::DT_GAMETOKEN);
    CreateVariable("Mission Obj", CString(""), table, IVariable::DT_MISSIONOBJ);
    CreateVariable("Sequence", CString(""), table, IVariable::DT_SEQUENCE);
    CreateVariable("Sequence ID", CString(""), table, IVariable::DT_SEQUENCE_ID);
    CreateVariable("Local String", CString(""), table, IVariable::DT_LOCAL_STRING);
    CreateVariable("Light Animation", CString(""), table, IVariable::DT_LIGHT_ANIMATION);
    CreateVariable("Particle Effect", CString(""), table, IVariable::DT_PARTICLE_EFFECT);
    CreateVariable("Flare", CString(""), table, IVariable::DT_FLARE);

    //test custom UserItems
    CreateVariable("User", CString("User - No Tree"), table, IVariable::DT_USERITEMCB)->SetUserData(new UserCustomItemGetter("Items (no tree)", false));
    CreateVariable("User", CString("User - Tree"), table, IVariable::DT_USERITEMCB)->SetUserData(new UserCustomItemGetter("Items (with tree)", true));
    //This will test IVariable::IGetCustomItems::GetItems() returning false (should not show dialog) 
    CreateVariable("User", CString("User GetItems Fail"), table, IVariable::DT_USERITEMCB)->SetUserData(new UserCustomItemGetter("GetItems Failure", false, true));

    //AI Waves and AI Territory
    IVariable *territory = CreateEnumVariable("AI Territory", CString(""), table, IVariable::DT_AITERRITORY);
    IVariable *wave = CreateEnumVariable("AI Wave", CString(""), table, IVariable::DT_AIWAVE);

    //AIPFPropertiesList and AIEntityClasses
    CreateVariable("AI PFPropertiesList", CString("abc,def,g"), table, IVariable::DT_AI_PFPROPERTIESLIST);
    CreateVariable("AI Entity Classes", CString(""), table, IVariable::DT_AIENTITYCLASSES);

    return varBlock;
}


template <class T>
IVariable* ReflectedPropertyCtrlTest::CreateVariable(const char *varName, const T &value, IVariable *parent, unsigned char dataType)
{
    CVariable<T> *var = new CVariable<T>();
    if (varName)
        var->SetName(varName);
    var->SetDataType(dataType);
    var->Set(value);
    parent->AddVariable(var);
    return var;
}


template <class T>
IVariable* CreateEnumVariable(const char *varName, const T &value, IVariable *parent, unsigned char dataType)
{
    CVariableEnum<T> *var = new CVariableEnum<T>();
    if (varName)
        var->SetName(varName);
    var->SetDataType(dataType);
    var->Set(value);
    parent->AddVariable(var);
    return var;
}

void ReflectedPropertyCtrlTest::OnVariableUpdated(IVariable *pVar)
{
    qDebug() << "Variable changed " << pVar->GetName() << " = " << pVar->GetDisplayValue();
}

void ReflectedPropertyCtrlTest::Test(QWidget *parent /*= nullptr*/)
{
    if (!s_varBlock)
        s_varBlock = GenerateTestData();

    ReflectedPropertyControl *ctrl = new ReflectedPropertyControl(parent);
    ctrl->Setup(true, 200);
    ctrl->SetUpdateCallback(functor(ReflectedPropertyCtrlTest::OnVariableUpdated));
    ctrl->AddVarBlock(s_varBlock);
    ctrl->resize(500, 600);
    ctrl->show();

    QPropertyCtrl *oldControl = new QPropertyCtrl(nullptr);
    oldControl->m_props.AddVarBlock(s_varBlock);
    oldControl->resize(500, 600);
    oldControl->show();

    //For testing ePropertyColorCurve and ePropertyFloatCurve the easiest way to create the correct IVariables
    //is to use CParticleUIDefinition which contains a bunch of stuff including a float curve and color curve.
    if (!s_varBlockParticle)
    {
        CParticleUIDefinition *pParticleUI = new CParticleUIDefinition();
        s_varBlockParticle = pParticleUI->CreateVars();
    }
    ReflectedPropertyControl *ctrlSpline = new ReflectedPropertyControl(parent);
    ctrlSpline->Setup(true, 200);
    ctrlSpline->SetUpdateCallback(functor(ReflectedPropertyCtrlTest::OnVariableUpdated));
    ctrlSpline->AddVarBlock(s_varBlockParticle);
    ctrlSpline->resize(500, 600);
    ctrlSpline->show();

    QPropertyCtrl *oldControlSpline = new QPropertyCtrl(nullptr);
    oldControlSpline->m_props.AddVarBlock(s_varBlockParticle);
    oldControlSpline->resize(500, 600);
    oldControlSpline->show();
}



CPropertyContainer* ReflectedVarTest::GenerateTestData()
{
    CPropertyContainer *container = new CPropertyContainer("Properties");
    container->AddProperty(new CReflectedVarString("Level Name", "Don't Die"));
    container->AddProperty(new CReflectedVarFloat("Scale", .75f));
    container->AddProperty(new CReflectedVarFloat("Rotation", 3.14159));
    container->AddProperty(new CReflectedVarInt("Min", 10));
    container->AddProperty(new CReflectedVarInt("Max", 1000));
    container->AddProperty(new CReflectedVarBool("Enabled", false));
    container->AddProperty(new CReflectedVarColor("FadeColor", AZ::Vector3(1.0f, 0.5f, 0.0f)));

    CReflectedVarVector3 *vector = new CReflectedVarVector3("Pos", AZ::Vector3(-10, 0, 2.75));
    vector->m_minVal = -10;
    vector->m_maxVal = 10;
    vector->m_stepSize = .22;
    container->AddProperty(vector);

    CPropertyContainer *inner = new CPropertyContainer("Nested Test");
    container->AddProperty(new CReflectedVarBool("Bold", true));
    inner->AddProperty(new CReflectedVarInt("width", 800));
    inner->AddProperty(new CReflectedVarInt("height", 600));
    container->AddProperty(inner);

    CReflectedVarEnum<int> *enumInt = new CReflectedVarEnum<int>("Size");
    enumInt->addEnum(12, "Small");
    enumInt->addEnum(16, "Medium");
    enumInt->addEnum(31, "Large");
    container->AddProperty(enumInt);

    CReflectedVarEnum<AZStd::string> *enumString = new CReflectedVarEnum<AZStd::string>("Country");
    enumString->addEnum("US", "United States");
    enumString->addEnum("DK", "Denmark");
    enumString->addEnum("CA", "Canada");
    container->AddProperty(enumString);

    CReflectedVarAnimation *anim = new CReflectedVarAnimation("Animation");
    anim->m_animation = "animtion_ID";
    anim->m_entityID = AZ::EntityId(10101);
    container->AddProperty(anim);

    CReflectedVarResource *texture = new CReflectedVarResource("Texture");
    texture->m_path = "some_texture.dds";
    texture->m_propertyType = ePropertyTexture;
    container->AddProperty(texture);

    CReflectedVarResource *audioTrigger = new CReflectedVarResource("Audio trigger");
    audioTrigger->m_path = "/file/path/to/audio_trigger";
    audioTrigger->m_propertyType = ePropertyAudioTrigger;
    container->AddProperty(audioTrigger);

    container->AddProperty(new CReflectedVarGenericProperty(ePropertyAiAnchor, "AI Anchor Test", "abc"));
    return container;
}

void ReflectedVarTest::ShowPropertyEditor(CPropertyContainer *data, const QString &title, bool addEachVarAsInstance, QWidget *parent /*= nullptr*/)
{
    AzToolsFramework::ReflectedPropertyEditor *propEditor = new AzToolsFramework::ReflectedPropertyEditor(parent);
    propEditor->setWindowTitle(title);
    propEditor->Setup(s_serializeContext, nullptr, true, 250);
    if (addEachVarAsInstance)
    {
        const AZStd::vector<CReflectedVar*> properties = data->GetProperties();
        for (int i = 0; i < properties.size(); ++i)
        {
            CReflectedVar *var = properties[i];
            const AZ::Uuid& classId = AZ::SerializeTypeInfo<CReflectedVar>::GetUuid(var);
            propEditor->AddInstance(var, classId);
        }
    }
    else
    {
        propEditor->AddInstance(data);
    }
    propEditor->resize(450, 600);
    propEditor->InvalidateAll();
    propEditor->ExpandAll();
    propEditor->show();
}

void ReflectedVarTest::Test()
{
    EBUS_EVENT_RESULT(s_serializeContext, AZ::ComponentApplicationBus, GetSerializeContext);
    AZ_Assert(s_serializeContext, "Serialization context not available");
    ReflectedVarInit::setupReflection(s_serializeContext);
    RegisterReflectedVarHandlers();
    CPropertyContainer *data = GenerateTestData();
    ShowPropertyEditor(data, "Add Container", false);
    //This does not work!
//	showPropertyEditor(data, "Add Elements", true);
}
#endif