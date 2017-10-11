#include "GeneratorDefine.h"
#include "CoreUObject.h"
#include "ScriptGeneratorManager.h"

#define Def_TEXT(Str) #Str

FScriptGeneratorManager *g_ScriptGeneratorManager = nullptr;


DEFINE_LOG_CATEGORY(LogLuaGenerator);

namespace NS_LuaGenerator
{
	FString ProjectPath;
	FString GameModuleName;
	FString ClassScriptHeaderSuffix(".script.h");

	const FString LuaConfigFileRelativePath("Config/LuaConfig.ini");

	const TCHAR* SupportModuleSection = TEXT("SupportModule");
	const TCHAR* SupportModuleKey = TEXT("SupportModuleKey");

	const TCHAR* NotSupportClassSection = TEXT("NotSupportClass");
	const TCHAR* NotSupportClassKey = TEXT("NotSupportClassKey");

	const TCHAR* BaseTypeSection = TEXT("BaseType");
	const TCHAR* BaseTypeKey = TEXT("BaseTypeKey");
	TArray<FString> BaseTypes;

	const TCHAR* ConfigClassFilesSection = TEXT("ConfigClassFiles");
	const TCHAR* ConfigClassFileKey = TEXT("ConfigClassFileName");
	const FString ClassConfigFileRelativeFolder("Config");
	TArray<FString> ClassConfigFileNames;


	bool StringForwardContainSub(const FString &&SrcStr, const FString &&SubStr, int32 SrcIndex)
	{
		int32 SubIndex = 0;

		if (SubStr.IsEmpty())
		{
			return false;
		}

		while (true)
		{
			if (SubIndex == SubStr.Len())
			{
				return true;
			}

			if (SrcIndex>=SrcStr.Len())
			{
				return false;
			}

			if (SrcStr[SrcIndex]!=SubStr[SubIndex])
			{
				return false;
			}

			++SrcIndex;
			++SubIndex;
		}
	}

	bool StringBackContainSub(const FString &&SrcStr, const FString &&SubStr, int32 SrcTailIndex)
	{
		int32 SubTailIndex = SubStr.Len()-1;

		if (SubStr.IsEmpty())
		{
			return false;
		}

		while (true)
		{
			if (SubTailIndex == -1)
			{
				return true;
			}

			if (SrcTailIndex ==-1)
			{
				return false;
			}

			if (SrcStr[SrcTailIndex] != SubStr[SubTailIndex])
			{
				return false;
			}

			--SrcTailIndex;
			--SubTailIndex;
		}
	}

	FString GetPropertyType(UProperty *Property, uint32 PortFlags/*=0*/)
	{
		static FString EnumDecl(TEXT("enum "));
		static FString StructDecl(TEXT("struct "));
		static FString ClassDecl(TEXT("class "));
		static FString TEnumAsByteDecl(TEXT("TEnumAsByte<enum "));
		static FString TSubclassOfDecl(TEXT("TSubclassOf<class "));

		FString PropertyType = Property->GetCPPType(NULL, PortFlags);

		g_ScriptGeneratorManager->m_LogContent += FString::Printf(TEXT("PropertyType:%s,"), *PropertyType);

		if (Property->IsA(UArrayProperty::StaticClass()))
		{ // TArray
			auto PropertyArr = Cast<UArrayProperty>(Property);
			FString inerTypeCpp = GetPropertyType(PropertyArr->Inner, CPPF_ArgumentOrReturnValue);
			if (inerTypeCpp == "EObjectTypeQuery")
				inerTypeCpp = "TEnumAsByte<EObjectTypeQuery> ";
			PropertyType = FString::Printf(TEXT("TArray<%s>"), *inerTypeCpp);
		}
		else if (Property->IsA(UMapProperty::StaticClass()))
		{ // TMap
			UMapProperty *pMapProperty = Cast<UMapProperty>(Property);
			FString KeyProp = GetPropertyType(pMapProperty->KeyProp, CPPF_ArgumentOrReturnValue);
			FString ValueProp = GetPropertyType(pMapProperty->ValueProp, CPPF_ArgumentOrReturnValue);
			PropertyType = FString::Printf(TEXT("TMap<%s,%s>"), *KeyProp, *ValueProp);
		}
		else if (Property->IsA(USetProperty::StaticClass()))
		{ // TSet
			USetProperty *pSetProperty = Cast<USetProperty>(Property);
			FString ElementProp = GetPropertyType(pSetProperty->ElementProp, CPPF_ArgumentOrReturnValue);
			PropertyType = FString::Printf(TEXT("TSet<%s>"), *ElementProp);
		}
		// Strip any forward declaration keywords
		if (PropertyType.StartsWith(EnumDecl) || PropertyType.StartsWith(StructDecl) || PropertyType.StartsWith(ClassDecl))
		{
			int32 FirstSpaceIndex = PropertyType.Find(TEXT(" "));
			PropertyType = PropertyType.Mid(FirstSpaceIndex + 1);
		}
		else if (PropertyType.StartsWith(TEnumAsByteDecl))
		{ // TEnumAsByte
			int32 FirstSpaceIndex = PropertyType.Find(TEXT(" "));
			PropertyType = TEXT("TEnumAsByte<") + PropertyType.Mid(FirstSpaceIndex + 1);
		}
		else if (PropertyType.StartsWith(TSubclassOfDecl))
		{ // TSubclassOf
			int32 FirstSpaceIndex = PropertyType.Find(TEXT(" "));
			PropertyType = TEXT("TSubclassOf<") + PropertyType.Mid(FirstSpaceIndex + 1);
		}

		int32 LastCharIndex = PropertyType.Len()-1;
		while (LastCharIndex>=0 && PropertyType[LastCharIndex]==' ')
		{
			--LastCharIndex;
		}

		if (LastCharIndex>=0)
		{
			PropertyType = PropertyType.Left(LastCharIndex + 1);
		}

		g_ScriptGeneratorManager->m_LogContent += FString::Printf(TEXT(",%s\r\n"), *PropertyType);
		return PropertyType;
	}

	EVariableType ResolvePropertyType(UProperty *pProperty)
	{
		EVariableType eVariableType = EVariableType::EUnknow;
		FString PropertyType;
		bool bRecognize = false;
		FString OriginalType = GetPropertyType(pProperty);
		PropertyType += FString::Printf(TEXT("UPropertyRealType:%s"), *OriginalType);
		
		if (pProperty->IsA(UIntProperty::StaticClass()) ||
			pProperty->IsA(UUInt16Property::StaticClass()) ||
			pProperty->IsA(UUInt32Property::StaticClass()) ||
			pProperty->IsA(UInt64Property::StaticClass()) ||
			pProperty->IsA(UFloatProperty::StaticClass()) ||
			pProperty->IsA(UBoolProperty::StaticClass()) ||
			pProperty->IsA(UFloatProperty::StaticClass()))
		{
			PropertyType += FString::Printf(TEXT(",BaseType"));
			bRecognize = true;
			eVariableType = EVariableType::EBaseType;
		}

		if (pProperty->IsA(UObjectPropertyBase::StaticClass()) && !pProperty->IsA(UClassProperty::StaticClass()))
		{
			if (OriginalType.Contains("*"))
			{
				eVariableType = EVariableType::EPoint;
				bRecognize = true;
				PropertyType += FString::Printf(TEXT(",EPoint"));
			}
			else
			{
				eVariableType = EVariableType::EObjectBase;
				bRecognize = true;
				PropertyType += FString::Printf(TEXT(",UObjectPropertyBase"));
			}
		}

		if (pProperty->IsA(UNameProperty::StaticClass()))
		{
			eVariableType = EVariableType::EFName;
			bRecognize = true;
			PropertyType += FString::Printf(TEXT(",UNameProperty"));
		}

		if (pProperty->IsA(UTextProperty::StaticClass()))
		{
			eVariableType = EVariableType::EText;
			bRecognize = true;
			PropertyType += FString::Printf(TEXT(",UTextProperty"));
		}

		if (pProperty->IsA(UStrProperty::StaticClass()))
		{
			eVariableType = EVariableType::EFString;
			bRecognize = true;
			PropertyType += FString::Printf(TEXT(",UStrProperty"));
		}

		if (pProperty->IsA(UClassProperty::StaticClass()))
		{
			if (OriginalType.Contains("*"))
			{
				eVariableType = EVariableType::EPoint;
				bRecognize = true;
				PropertyType += FString::Printf(TEXT(",EPoint"));
			}
			else if (OriginalType.StartsWith("TSubclassOf<"))
			{
				eVariableType = EVariableType::ETSubclassOf;
				bRecognize = true;
				PropertyType += FString::Printf(TEXT(",TSubclassOf"));
			}
			else
			{
				eVariableType = EVariableType::EClass;
				bRecognize = true;
				PropertyType += FString::Printf(TEXT(",UClassProperty"));
			}
		}

		if (pProperty->IsA(UArrayProperty::StaticClass()))
		{
			eVariableType = EVariableType::ETArray;
			bRecognize = true;
			PropertyType += FString::Printf(TEXT(",UArrayProperty"));
		}

		if (pProperty->IsA(UWeakObjectProperty::StaticClass()))
		{
			eVariableType = EVariableType::EWeakObject;
			bRecognize = true;
			PropertyType += FString::Printf(TEXT(",UWeakObjectProperty"));
		}

		if (pProperty->IsA(UStructProperty::StaticClass()))
		{
			eVariableType = EVariableType::EStruct;
			bRecognize = true;
			PropertyType += FString::Printf(TEXT(",UStructProperty"));
		}

		if (pProperty->IsA(UByteProperty::StaticClass()))
		{
			eVariableType = EVariableType::EByte;
			bRecognize = true;
			PropertyType += FString::Printf(TEXT(",UByteProperty"));
		}

		if (pProperty->IsA(UEnumProperty::StaticClass()))
		{
			eVariableType = EVariableType::EEnum;
			bRecognize = true;
			PropertyType += FString::Printf(TEXT(",UEnumProperty"));
		}

		if (pProperty->IsA(UMulticastDelegateProperty::StaticClass()))
		{
			eVariableType = EVariableType::EMulticastDelegate;
			bRecognize = true;
			PropertyType += FString::Printf(TEXT(",UMulticastDelegateProperty"));
		}

		if (pProperty->IsA(UMapProperty::StaticClass()))
		{
			eVariableType = EVariableType::ETMap;
			bRecognize = true;
			PropertyType += FString::Printf(TEXT(",UMapProperty"));
		}

		if (pProperty->IsA(USetProperty::StaticClass()))
		{
			eVariableType = EVariableType::ETSet;
			bRecognize = true;
			PropertyType += FString::Printf(TEXT(",USetProperty"));
		}


		if (!bRecognize)
		{
			eVariableType = EVariableType::EUnknow;
			PropertyType += FString::Printf(TEXT(",unknowType"));
		}

		FString FinalType;
		switch (eVariableType)
		{
		case EBaseType:
		{
			FinalType = "EBaseType";
			break;
		}
		case EPoint:
		{
			FinalType = "EPoint";
			break;
		}
		case EObjectBase:
		{
			FinalType = "EObjectBase";
			break;
		}
		case EFName:
		{
			FinalType = "EName";
			break;
		}
		case EText:
		{
			FinalType = "EText";
			break;
		}
		case EFString:
		{
			FinalType = "EFString";
			break;
		}
		case EClass:
		{
			FinalType = "EClass";
			break;
		}
		case ETArray:
		{
			FinalType = "ETArray";
			break;
		}
		case EWeakObject:
		{
			FinalType = "EWeakObject";
			break;
		}
		case EStruct:
		{
			FinalType = "EStruct";
			break;
		}
		case EByte:
		{
			FinalType = "EByte";
			break;
		}
		case EEnum:
		{
			FinalType = "EEnum";
			break;
		}
		case ETSubclassOf:
		{
			FinalType = "ETSubclassOf";
			break;
		}
		case EMulticastDelegate:
		{
			FinalType = "EMulticastDelegate";
			break;
		}
		case ETMap:
		{
			FinalType = "ETMap";
			break;
		}
		case ETSet:
		{
			FinalType = "ETSet";
			break;
		}
		case EUnknow:
		{
			FinalType = "EUnknow";
			break;
		}
		default:
		{
			FinalType = "EUnknow";
		}
		}

		PropertyType += FString::Printf(TEXT("FinalType:%s\r\n"), *FinalType);
		g_ScriptGeneratorManager->m_PropertyType.Add(PropertyType);

		return eVariableType;
	}

}
