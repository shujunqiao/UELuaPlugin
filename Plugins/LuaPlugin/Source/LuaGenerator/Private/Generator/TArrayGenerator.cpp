#include "Generator/TArrayGenerator.h"
#include "CoreUObject.h"
#include "GeneratorDefine.h"
#include "LuaConfigManager.h"
using namespace NS_LuaGenerator;

IScriptGenerator* FTArrayGenerator::CreateGenerator( UProperty *pProperty, const FString &InOutDir)
{
	return new FTArrayGenerator(pProperty, InOutDir);
}

FTArrayGenerator::FTArrayGenerator(UProperty *pProperty, const FString &InOutDir)
	: IScriptGenerator(E_GeneratorType::ETArray, InOutDir)
{
	UArrayProperty *pArrayProperty = Cast<UArrayProperty>(pProperty);
	Init(pArrayProperty);
}

FTArrayGenerator::~FTArrayGenerator()
{

}

FString FTArrayGenerator::GetKey() const
{
	return m_TArrayInfo.PureType;
}

FString FTArrayGenerator::GetFileName() const
{
	return GetClassName() + g_LuaConfigManager->ClassScriptHeaderSuffix;
}

FString FTArrayGenerator::GetRegName() const
{
	return FString::Printf(TEXT("%s_Lib"), *GetClassName());
}

bool FTArrayGenerator::CanExport() const
{
	return m_bSupportElement;
}

void FTArrayGenerator::ExportToMemory()
{
	m_LuaFuncReg.AddExtraFuncMember(ExtraNew());
	m_LuaFuncReg.AddExtraFuncMember(ExtraDestory());
	m_LuaFuncReg.AddExtraFuncMember(ExtraNum());
	m_LuaFuncReg.AddExtraFuncMember(ExtraAdd());
	m_LuaFuncReg.AddExtraFuncMember(ExtraGet());
	m_LuaFuncReg.AddExtraFuncMember(ExtraSet());
	m_LuaFuncReg.AddExtraFuncMember(ExtraEmpty());
	m_LuaFuncReg.AddExtraFuncMember(ExtraCopy());
	m_LuaFuncReg.AddExtraFuncMember(ExtraRemoveAt());
}

void FTArrayGenerator::SaveToFile()
{
	FString FileContent;
	FString FilePathName = m_OutDir / GetFileName();
	FileContent += GetFileHeader();
	FileContent += m_LuaFuncReg.GetFuncContents();
	FileContent += m_LuaFuncReg.GetRegLibContents();
	FileContent += GetFileTail();

	if (!FFileHelper::SaveStringToFile(FileContent, *FilePathName))
	{
		UE_LOG(LogLuaGenerator, Error, TEXT("Failed to save header export:%s"), *GetFileName());
	}
}

FString FTArrayGenerator::GetClassName() const
{
	return FString::Printf(TEXT("%s"), *m_ClassName);
}

void FTArrayGenerator::GetParentNames(TArray<FString> &OutParentNames) const
{

}

FExtraFuncMemberInfo FTArrayGenerator::ExtraNew()
{
	FExtraFuncMemberInfo ExtraInfo;
	ExtraInfo.funcName = "New";
	FString &funcBody = ExtraInfo.funcBody;
	funcBody += EndLinePrintf(TEXT("\t%s *pTArray = new %s;"), *m_TArrayInfo.PureType, *m_TArrayInfo.PureType);
	funcBody += EndLinePrintf(TEXT("\tFLuaUtil::Push(InLuaState, FLuaClassType<%s*>(pTArray, \"%s\"));"), *m_TArrayInfo.PureType, *m_TArrayInfo.PureType);
	funcBody += EndLinePrintf(TEXT("\treturn 1;"));
	return ExtraInfo;
}

FExtraFuncMemberInfo FTArrayGenerator::ExtraDestory()
{
	FExtraFuncMemberInfo ExtraInfo;
	ExtraInfo.funcName = "Destory";
	FString &funcBody = ExtraInfo.funcBody;
	funcBody += EndLinePrintf(TEXT("\t%s *pTArray = FLuaUtil::TouserData<%s*>(InLuaState, 1, \"%s\");"), *m_TArrayInfo.PureType, *m_TArrayInfo.PureType, *m_TArrayInfo.PureType);
	funcBody += EndLinePrintf(TEXT("\treturn 0;"));
	return ExtraInfo;
}

FExtraFuncMemberInfo FTArrayGenerator::ExtraNum()
{
	FExtraFuncMemberInfo ExtraInfo;
	ExtraInfo.funcName = "Num";
	FString &funcBody = ExtraInfo.funcBody;
	funcBody += EndLinePrintf(TEXT("\t%s *pTArray = FLuaUtil::TouserData<%s*>(InLuaState, 1, \"%s\");"), *m_TArrayInfo.PureType, *m_TArrayInfo.PureType, *m_TArrayInfo.PureType);
	funcBody += EndLinePrintf(TEXT("\tint32 ArrayNum = pTArray->Num();"));
	funcBody += EndLinePrintf(TEXT("\tFLuaUtil::Push(InLuaState, FLuaClassType<int32>(ArrayNum, \"int32\"));"));
	funcBody += EndLinePrintf(TEXT("\treturn 1;"));
	return ExtraInfo;
}

FExtraFuncMemberInfo FTArrayGenerator::ExtraAdd()
{
	FExtraFuncMemberInfo ExtraInfo;
	ExtraInfo.funcName = "Add";
	FString &funcBody = ExtraInfo.funcBody;
	funcBody += EndLinePrintf(TEXT("\t%s *pTArray = FLuaUtil::TouserData<%s*>(InLuaState, 1, \"%s\");"), *m_TArrayInfo.PureType, *m_TArrayInfo.PureType, *m_TArrayInfo.PureType);
	funcBody += EndLinePrintf(TEXT("\t%s ArrayItem = FLuaUtil::TouserData<%s>(InLuaState, 2, \"%s\");"), *m_ElementInfo.DeclareType, *m_ElementInfo.DeclareType, *m_ElementInfo.PureType);
	funcBody += EndLinePrintf(TEXT("\tpTArray->Add(%sArrayItem);"), *m_ElementInfo.UsedSelfVarPrefix);
	funcBody += EndLinePrintf(TEXT("\treturn 0;"));
	return ExtraInfo;
}

FExtraFuncMemberInfo FTArrayGenerator::ExtraGet()
{
	FExtraFuncMemberInfo ExtraInfo;
	ExtraInfo.funcName = "Get";
	FString &funcBody = ExtraInfo.funcBody;
	funcBody += EndLinePrintf(TEXT("\t%s *pTArray = FLuaUtil::TouserData<%s*>(InLuaState, 1, \"%s\");"), *m_TArrayInfo.PureType, *m_TArrayInfo.PureType, *m_TArrayInfo.PureType);
	funcBody += EndLinePrintf(TEXT("\tint32 ArrayIndex = FLuaUtil::TouserData<int32>(InLuaState, 2, \"int32\");"));
	funcBody += EndLinePrintf(TEXT("\t%s pItem = %s(*pTArray)[ArrayIndex];"), *m_ElementInfo.DeclareType, *m_ElementInfo.AssignValuePrefix);
	funcBody += EndLinePrintf(TEXT("\tFLuaUtil::Push(InLuaState, FLuaClassType<%s>(pItem, \"%s\"));"), *m_ElementInfo.DeclareType, *m_ElementInfo.PureType);
	funcBody += EndLinePrintf(TEXT("\treturn 1;"));
	return ExtraInfo;
}

FExtraFuncMemberInfo FTArrayGenerator::ExtraSet()
{
	if (g_LuaConfigManager->InvalidSetTypes.Contains(m_ElementInfo.PureType))
	{
		FExtraFuncMemberInfo ExtraInfo;
		ExtraInfo.funcName = g_LuaConfigManager->NoExportExtraFuncName;
		return ExtraInfo;
	}

	FExtraFuncMemberInfo ExtraInfo;
	ExtraInfo.funcName = "Set";
	FString &funcBody = ExtraInfo.funcBody;
	funcBody += EndLinePrintf(TEXT("\t%s *pTArray = FLuaUtil::TouserData<%s*>(InLuaState, 1, \"%s\");"), *m_TArrayInfo.PureType, *m_TArrayInfo.PureType, *m_TArrayInfo.PureType);
	funcBody += EndLinePrintf(TEXT("\tint32 ArrayIndex = FLuaUtil::TouserData<int32>(InLuaState, 2, \"int32\");"));
	funcBody += EndLinePrintf(TEXT("\t%s ArrayItem = FLuaUtil::TouserData<%s>(InLuaState, 3, \"%s\");"), *m_ElementInfo.DeclareType, *m_ElementInfo.DeclareType, *m_ElementInfo.PureType);
	funcBody += EndLinePrintf(TEXT("\t(*pTArray)[ArrayIndex] = %sArrayItem;"), *m_ElementInfo.UsedSelfVarPrefix);
	funcBody += EndLinePrintf(TEXT("\treturn 0;"));
	return ExtraInfo;
}

FExtraFuncMemberInfo FTArrayGenerator::ExtraEmpty()
{
	FExtraFuncMemberInfo ExtraInfo;
	ExtraInfo.funcName = "Empty";
	FString &funcBody = ExtraInfo.funcBody;
	funcBody += EndLinePrintf(TEXT("\t%s *pTArray = FLuaUtil::TouserData<%s*>(InLuaState, 1, \"%s\");"), *m_TArrayInfo.PureType, *m_TArrayInfo.PureType, *m_TArrayInfo.PureType);
	funcBody += EndLinePrintf(TEXT("\tpTArray->Empty();"));
	funcBody += EndLinePrintf(TEXT("\treturn 0;"));
	return ExtraInfo;
}

FExtraFuncMemberInfo FTArrayGenerator::ExtraCopy()
{
	FExtraFuncMemberInfo ExtraInfo;
	ExtraInfo.funcName = "Copy";
	FString &funcBody = ExtraInfo.funcBody;
	funcBody += EndLinePrintf(TEXT("\t%s *pSrc = FLuaUtil::TouserData<%s*>(InLuaState, 1, \"%s\");"), *m_TArrayInfo.PureType, *m_TArrayInfo.PureType, *m_TArrayInfo.PureType);
	funcBody += EndLinePrintf(TEXT("\t%s *pDest = FLuaUtil::TouserData<%s*>(InLuaState, 2, \"%s\");"), *m_TArrayInfo.PureType, *m_TArrayInfo.PureType, *m_TArrayInfo.PureType);
	funcBody += EndLinePrintf(TEXT("\t*pSrc = *pDest;"));
	funcBody += EndLinePrintf(TEXT("\treturn 0;"));
	return ExtraInfo;
}

FExtraFuncMemberInfo FTArrayGenerator::ExtraRemoveAt()
{
	FExtraFuncMemberInfo ExtraInfo;
	ExtraInfo.funcName = "RemoveAt";
	FString &funcBody = ExtraInfo.funcBody;
	funcBody += EndLinePrintf(TEXT("\t%s *pTArray = FLuaUtil::TouserData<%s*>(InLuaState, 1, \"%s\");"), *m_TArrayInfo.PureType, *m_TArrayInfo.PureType, *m_TArrayInfo.PureType);
	funcBody += EndLinePrintf(TEXT("\tint32 ArrayIndex = FLuaUtil::TouserData<int32>(InLuaState, 2, \"int32\");"));
	funcBody += EndLinePrintf(TEXT("\tpTArray->RemoveAt(ArrayIndex);"));
	funcBody += EndLinePrintf(TEXT("\treturn 0;"));
	return ExtraInfo;
}

void FTArrayGenerator::Init(UArrayProperty *pArrayProperty)
{
	m_bSupportElement = false;
	m_ClassName = NS_LuaGenerator::GetPlainType(NS_LuaGenerator::GetPropertyType(pArrayProperty));
	m_LuaFuncReg.Init(m_ClassName);
	m_TArrayInfo.InitByUProperty(pArrayProperty);
	m_ElementInfo.InitByUProperty(pArrayProperty->Inner);

	EVariableType  eElementType = ResolvePropertyType(pArrayProperty->Inner);
	switch (eElementType)
	{
	case EBaseType:
	{
		m_bSupportElement = true;
		break;
	}
	case EPoint:
	{
		m_bSupportElement = true;
		break;
	}
	case EObjectBase:
	{
		m_bSupportElement = false;
		break;
	}
	case EFName:
	{
		m_bSupportElement = false;
		break;
	}
	case EText:
	{
		m_bSupportElement = false;
		break;
	}
	case EFString:
	{
		m_bSupportElement = false;
		break;
	}
	case EClass:
	{
		m_bSupportElement = false;
		break;
	}
	case EVarTArray:
	{
		m_bSupportElement = false;
		break;
	}
	case EWeakObject:
	{
		m_bSupportElement = false;
		break;
	}
	case EStruct:
	{
		m_bSupportElement = true;
		break;
	}
	case EByte:
	{
		m_bSupportElement = false;
		break;
	}
	case EEnum:
	{
		m_bSupportElement = false;
		break;
	}
	case ETSubclassOf:
	{
		m_bSupportElement = false;
		break;
	}
	case EMulticastDelegate:
	{
		m_bSupportElement = false;
		break;
	}
	case EVarTMap:
	{
		m_bSupportElement = false;
		break;
	}
	case EVarTSet:
	{
		m_bSupportElement = false;
		break;
	}
	case EUnknow:
	{
		m_bSupportElement = false;
		break;
	}
	}	
}
