#include "ClassGenerator.h"
#include "UnrealType.h"
#include "GeneratorDefine.h"
#include "Templates/Casts.h"
#include "Misc/FileHelper.h"
#include "UObjectIterator.h"

void FClassGeneratorConfig::Init()
{
	FString ConfigFilePath = NS_LuaGenerator::ProjectPath / NS_LuaGenerator::LuaConfigFileRelativePath;
	GConfig->GetArray(NS_LuaGenerator::NotSupportClassSection, NS_LuaGenerator::NotSupportClassKey, m_NotSuportClasses, ConfigFilePath);
}

bool FClassGeneratorConfig::CanExport(const FString &InClassName)
{
	return !m_NotSuportClasses.Contains(InClassName);
}

IScriptGenerator* FClassGenerator::CreateGenerator(UObject *InObj, const FString &OutDir)
{
	UClass *pClass = Cast<UClass>(InObj);

	if (pClass)
	{
		return new FClassGenerator(pClass, OutDir);
	}
	else
	{
		UE_LOG(LogLuaGenerator, Error, TEXT("FClassGenerator::CreateGenerator error"));
		return nullptr;
	}
}

FClassGenerator::FClassGenerator(UClass *InClass, const FString &InOutDir)
{
	m_FileName.Empty();
	m_FileContent.Empty();
	m_FunctionNames.Empty();

	m_pClass = InClass;
	m_OutDir = InOutDir;
	m_FileName = InClass->GetName() + NS_LuaGenerator::ClassScriptHeaderSuffix; 
}

FClassGenerator::~FClassGenerator()
{

}

bool FClassGenerator::CanExport()
{
	return m_ClassConfig.CanExport(m_pClass->GetName());
}

void FClassGenerator::Export()
{
	GenerateScriptHeader(m_FileContent);
	GenerateFunctions(m_FileContent);
	GenerateRegister(m_FileContent);
	GenerateScriptTail(m_FileContent);
	SaveToFile();
}

void FClassGenerator::SaveToFile()
{
	FString fileName = m_OutDir/m_pClass->GetName() + NS_LuaGenerator::ClassScriptHeaderSuffix;
	if (!FFileHelper::SaveStringToFile(m_FileContent, *fileName))
	{
		UE_LOG(LogLuaGenerator, Error, TEXT("Failed to save header export:%s"), *fileName);
	}
}

void FClassGenerator::GenerateScriptHeader(FString &OutStr)
{
	OutStr += EndLinPrintf(TEXT("#pragma once"));
	OutStr += EndLinPrintf(TEXT("PRAGMA_DISABLE_DEPRECATION_WARNINGS"));
	OutStr += EndLinPrintf(TEXT(""));
}

void FClassGenerator::GenerateFunctions(FString &OutStr)
{
	for (TFieldIterator<UFunction> FuncIt(m_pClass); FuncIt; ++FuncIt)
	{
		if (CanExportFunction(*FuncIt))
		{
			AddFunctionToRegister(*FuncIt);
			GenerateSingleFunction(*FuncIt, OutStr);
		}
	}
}

void FClassGenerator::GenerateSingleFunction(UFunction *InFunction, FString &OutStr)
{
	OutStr += EndLinPrintf(TEXT(""));
	OutStr += EndLinPrintf(TEXT("static int32 %s_%s(lua_State* L)"), *m_pClass->GetName(), *InFunction->GetFName().ToString());
	OutStr += EndLinPrintf(TEXT("{"));
	GenerateFunctionParams(InFunction, OutStr);
	OutStr += EndLinPrintf(TEXT("}"));
}

void FClassGenerator::AddFunctionToRegister(UFunction *InFunction)
{
	m_FunctionNames.Add(InFunction->GetName());
}

void FClassGenerator::GenerateFunctionParam(UProperty *InParam, int32 InIndex, FString &OutStr)
{
	FString propertyType = GetPropertyType(InParam, CPPF_ArgumentOrReturnValue);
	UE_LOG(LogLuaGenerator, Error, TEXT("GetPropertyType type:%s"), *propertyType);

	if (InParam->IsA(UIntProperty::StaticClass()) ||
		InParam->IsA(UUInt32Property::StaticClass()) ||
		InParam->IsA(UInt64Property::StaticClass()) ||
		InParam->IsA(UUInt16Property::StaticClass()))
	{
		UE_LOG(LogLuaGenerator, Error, TEXT("Function param type:%s"), TEXT("int32"));
	}
	else if (InParam->IsA(UFloatProperty::StaticClass()))
	{
		UE_LOG(LogLuaGenerator, Error, TEXT("Function param type:%s"), TEXT("float"));
	}
	else if (InParam->IsA(UStrProperty::StaticClass()))
	{
		UE_LOG(LogLuaGenerator, Error, TEXT("Function param type:%s"), TEXT("FString"));
	}
	else if (InParam->IsA(UNameProperty::StaticClass()))
	{
		UE_LOG(LogLuaGenerator, Error, TEXT("Function param type:%s"), TEXT("FName"));
	}
	else if (InParam->IsA(UBoolProperty::StaticClass()))
	{
		UE_LOG(LogLuaGenerator, Error, TEXT("Function param type:%s"), TEXT("bool"));
	}
	else if (InParam->IsA(UStructProperty::StaticClass()))
	{
		UStructProperty* StructProp = CastChecked<UStructProperty>(InParam);
		UE_LOG(LogLuaGenerator, Error, TEXT("Function param type:%s"), *StructProp->Struct->GetName());
	}
	else if (InParam->IsA(UClassProperty::StaticClass()))
	{
		//UClassProperty* StructProp = CastChecked<UClassProperty>(InParam);
		//UE_LOG(LogLuaGenerator, Error, TEXT("Function param type:%s"), StructProp->Struct->GetName());
	}
	else if (InParam->IsA(UObjectPropertyBase::StaticClass()))
	{
		//FString::Printf(TEXT("(%s)(lua_touserdata"), *GetPropertyTypeCPP(InParam, CPPF_ArgumentOrReturnValue), ParamIndex);
	}
}

void FClassGenerator::GenerateFunctionParams(UFunction *InFunction, FString &OutStr)
{
	int32 index = 0;
	for (TFieldIterator<UProperty> ParamIt(InFunction); ParamIt; ++ParamIt, ++index)
	{
		GenerateFunctionParam(*ParamIt, index, OutStr);
	}
}

void FClassGenerator::GeneratorCheckParamsNumCode(UFunction *InFunction, FString &OutStr)
{

}

FString FClassGenerator::GetPropertyType(UProperty *Property, uint32 PortFlags/*=0*/)
{
	static FString EnumDecl(TEXT("enum "));
	static FString StructDecl(TEXT("struct "));
	static FString ClassDecl(TEXT("class "));
	static FString TEnumAsByteDecl(TEXT("TEnumAsByte<enum "));
	static FString TSubclassOfDecl(TEXT("TSubclassOf<class "));

	FString PropertyType = Property->GetCPPType(NULL, PortFlags);
	if (Property->IsA(UArrayProperty::StaticClass()))
	{
		auto PropertyArr = Cast<UArrayProperty>(Property);
		FString inerTypeCpp = GetPropertyType(PropertyArr->Inner, CPPF_ArgumentOrReturnValue);
		if (inerTypeCpp == "EObjectTypeQuery")
			inerTypeCpp = "TEnumAsByte<EObjectTypeQuery> ";
		PropertyType = FString::Printf(TEXT("TArray<%s>"), *inerTypeCpp);
	}
	// Strip any forward declaration keywords
	if (PropertyType.StartsWith(EnumDecl) || PropertyType.StartsWith(StructDecl) || PropertyType.StartsWith(ClassDecl))
	{
		int FirstSpaceIndex = PropertyType.Find(TEXT(" "));
		PropertyType = PropertyType.Mid(FirstSpaceIndex + 1);
	}
	else if (PropertyType.StartsWith(TEnumAsByteDecl))
	{
		int FirstSpaceIndex = PropertyType.Find(TEXT(" "));
		PropertyType = TEXT("TEnumAsByte<") + PropertyType.Mid(FirstSpaceIndex + 1);
	}
	else if (PropertyType.StartsWith(TSubclassOfDecl))
	{
		int FirstSpaceIndex = PropertyType.Find(TEXT(" "));
		PropertyType = TEXT("TSubclassOf<") + PropertyType.Mid(FirstSpaceIndex + 1);
	}
	return PropertyType;
}

void FClassGenerator::GenerateRegister(FString &OutStr)
{
	OutStr += EndLinPrintf(TEXT(""));
	OutStr += EndLinPrintf(TEXT("static const luaL_Reg %s_Lib[] ="), *m_pClass->GetName());
	OutStr += EndLinPrintf(TEXT("{"));

	for (const FString &FunctionName : m_FunctionNames)
	{
		GenerateRegisterItem(FunctionName, OutStr);
	}

	OutStr += EndLinPrintf(TEXT("\t{ NULL, NULL }"));
	OutStr += EndLinPrintf(TEXT("};"));
}

void FClassGenerator::GenerateRegisterItem(const FString &InFunctionName, FString &OutStr)
{
	OutStr += EndLinPrintf(TEXT("\t{ \"%s\", %s},"), *InFunctionName, *GenerateRegisterFuncName(*InFunctionName, *m_pClass->GetName()) );
}

FString FClassGenerator::GenerateRegisterFuncName(const FString &InFunctionName, const FString &ClassName)
{
	return ClassName + TEXT("_") + InFunctionName;
}

bool FClassGenerator::CanExportFunction(UFunction *InFunction)
{
	if ( (InFunction->FunctionFlags&FUNC_Public) && (InFunction->FunctionFlags & FUNC_RequiredAPI))
	{
		return true;
	}
	else
	{
		return false;
	}
}

void FClassGenerator::GenerateScriptTail(FString &OutStr)
{

	OutStr += EndLinPrintf(TEXT(""));
	OutStr += EndLinPrintf(TEXT("PRAGMA_ENABLE_DEPRECATION_WARNINGS"));
}

FClassGeneratorConfig FClassGenerator::m_ClassConfig;
