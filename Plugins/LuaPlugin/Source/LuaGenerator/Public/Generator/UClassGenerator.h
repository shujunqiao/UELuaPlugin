#pragma once
#include "IScriptGenerator.h"
#include "BaseLuaFuncReg.h"

class FUClassGenerator : public IScriptGenerator
{
public:
	FUClassGenerator(UClass *InClass, const FString &InOutDir, const FString& HeaderFileName);
	virtual ~FUClassGenerator();

public:
	/** FBaseGenerator interface */
	virtual FString GetKey() const override { return GetClassName(); }
	virtual bool CanExport()const  override;
	virtual void ExportToMemory() override;
	virtual void SaveToFile() override;
	virtual FString GetClassName() const override;

public:
	void ExportDataMembersToMemory();
	void ExportFunctionMembersToMemory();
	void ExportExtraFuncsToMemory();

private:
	FExtraFuncMemberInfo GenerateNewExportFunction();

private:
	FString GetFileInclude();
	FString GetFileFunctionContents();
	FString GetFileRegContents();
	bool CanExportFunction(UFunction *InFunction);

private:
	UClass *m_pClass;
	FBaseFuncReg m_LuaFuncReg;
	FString m_HeaderFileName;
};
