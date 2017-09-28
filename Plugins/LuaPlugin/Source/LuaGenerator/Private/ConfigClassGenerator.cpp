#include "ConfigClassGenerator.h"
#include "Misc/FileHelper.h"

IScriptGenerator* FConfigClassGenerator::CreateGenerator(const FConfigClass& ClassItem, const FString &OutDir)
{
	return new FConfigClassGenerator(ClassItem, OutDir);
}

FConfigClassGenerator::FConfigClassGenerator(const FConfigClass &ConfigClass, const FString &OutDir)
	: IScriptGenerator(NS_LuaGenerator::EConfigClass, OutDir)
	, m_ConfigClass(ConfigClass)
{

}

FConfigClassGenerator::~FConfigClassGenerator()
{

}

FString FConfigClassGenerator::GetKey() const
{
	return m_ConfigClass.GetClassName();
}

FString FConfigClassGenerator::GetFileName() const
{
	return FString::Printf(TEXT("%s.Script.h"), *m_ConfigClass.GetClassName());
}

FString FConfigClassGenerator::GetRegName() const
{
	return m_ConfigClass.GetRegLibName();
}

bool FConfigClassGenerator::CanExport() const
{
	return true;
}

void FConfigClassGenerator::ExportToMemory()
{
	SaveToFile();
}

void FConfigClassGenerator::SaveToFile()
{
	FString fileName = m_OutDir/ GetFileName();
	FString fileContent;
	Unity(fileContent);
	if (!FFileHelper::SaveStringToFile(fileContent, *fileName))
	{
		UE_LOG(LogLuaGenerator, Error, TEXT("Failed to save header export:%s"), *fileName);
	}
}

void FConfigClassGenerator::Unity(FString &OutStr)
{
	OutStr += EndLinePrintf(TEXT("#pragma once"));
	OutStr += EndLinePrintf(TEXT("#include \"LuaUtil.h\""));
	OutStr += m_ConfigClass.GetIncludeFilesChunk();
	OutStr += m_ConfigClass.GetFunctionsChunk();
	OutStr += m_ConfigClass.GetRegLibChunk();
}