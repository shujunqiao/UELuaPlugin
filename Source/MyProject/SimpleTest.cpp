#include "SimpleTest.h"
DEFINE_LOG_CATEGORY(LogMyProject);

UMyTestClass1::UMyTestClass1(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

UMyTestClass::UMyTestClass(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

FMyTest* FMyTest::CreateSelf()
{
	return new FMyTest();
}

void FMyTest::TestConstPoint(const FMyTest1 *InTest) const
{
	UE_LOG(LogMyProject, Log, TEXT("TestConstPoint: number:%d"), InTest->GetMember());
}

void FMyTest::TestPointRef(FMyTest1 *&InTest) const
{
	UE_LOG(LogMyProject, Log, TEXT("TestPointRef: number:%d"), InTest->GetMember());
}

void FMyTest::TestConstRef(const FMyTest1 &InTest) const
{
	UE_LOG(LogMyProject, Log, TEXT("TestConstRef: number:%d"), InTest.GetMember());
}

void FMyTest::TestRef(FMyTest1 &InTest) const
{
	UE_LOG(LogMyProject, Log, TEXT("TestRef: number:%d"), InTest.GetMember());
}


void FMyTest::StaticPrint()
{
	UE_LOG(LogMyProject, Log, TEXT("StaticPrint"));
}

void FMyTest::StaticPrint1()
{
	UE_LOG(LogMyProject, Log, TEXT("StaticPrint111"));
}

void FMyTest::GetPlayerController()
{

}

FMyTest1* FMyTest1::CreateSelf()
{
	return new FMyTest1();
}

void FMyTest1::SetMember(int32 m)
{
	m_member = m;
}

void FMyTest1::Print() const
{
	UE_LOG(LogMyProject, Log, TEXT("FMyTest1:m_member is:%d"), m_member);
}

FMyTestBase* FMyTestBase::CreateSelf()
{
	return new FMyTestBase;
}

void FMyTestBase::SetMember(float m)
{
	m_member = m;
}

void FMyTestBase::Print() const
{
	UE_LOG(LogMyProject, Log, TEXT("FMyTestBase:m_member is:%f"), m_member);
}

void FMyTestBase::TestBaseFunc() const
{
	UE_LOG(LogMyProject, Log, TEXT("TestBaseFunc"));
}
