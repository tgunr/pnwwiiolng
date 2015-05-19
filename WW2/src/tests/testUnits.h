// TestUnit class.
// Copyright (C) PlayNet Inc, 2010.
// Provides a unit testing class framework.
// Inherit to your own test class.
// Originating Programmer: Oliver "kfsone" Smith <osmith@playnet.com>

#include <iostream>

extern bool enableTestDebug;

class TestUnit
{
	//// Base Ctor/Dtor
public:	TestUnit(const char* unitName);
public: virtual ~TestUnit() {}

	//// Your class must have a "Run()" member. This will be invoked by the test manager.
protected: virtual bool Run() = 0;

	//// Your class can choose to provide functions for Beginning and End of stage.
public: virtual void BeginStage() {}
public: virtual void EndStage() {}

	//// Helper/simulation functions.
	// The initial teulClock64 value we're planning to use.
public: static TeulTimeMS BaseClock() { return 600000ULL; }
	// Set the clock to a specific value.
public: static void SetClock_ms(const TeulTimeMS& time_ms) { teulClock64 = time_ms; teulClockTime = (UINT32)(teulClock64 / 1000); }
	// Set the teul clock back to its baseline value.
public: static void ResetClock() { SetClock_ms(BaseClock()); }
	// Advance the teul clock time by milliseconds or seconds.
public: static void AdvanceClock_ms(const TeulTimeMS& step) { SetClock_ms(teulClock64 + step); }
public:	static void AdvanceClock_s(const UINT32& step) { SetClock_ms(teulClock64 + (step * 1000)); }

	// Test helpers.
public: template<typename _Container> size_t NumEntriesInContainer(const _Container& container) const { return (container.empty() ? 0 : container.size()); }
public: template<typename _Container> bool IsContainerEmpty(const _Container& container) const { return NumEntriesInContainer(container) == 0; }
public: template<typename _Container, typename _Indexer> bool DoesContainerContain(const _Container& container, const _Indexer& index) const { return (container.find(index) != container.end()); }

	// Accessors.
public: const char* Name() const { return m_unit.c_str(); }
public: const char* Stage() const { return m_stage.c_str(); }
public: const char* Fails() const { return m_fails.c_str(); }
public: bool Completed() const { return m_success == true && m_fails.empty(); }
public: UINT32 UnitPasses() const { return m_unitPasses; }
public: UINT32 StagePasses() const { return m_stagePasses; }
public: UINT32 UnitErrors() const { return m_unitErrors; }
public: UINT32 StageErrors() const { return m_stageErrors; }

public: enum Equality { IsLt = -1, IsEqual, IsGt };
public: template<typename _Lhs, typename _Rhs> Equality Equals(const _Lhs& result, const _Rhs& expect) const
		{
			snprintf(m_lhsVal, 64, "%llu", (UINT64)result);
			snprintf(m_rhsVal, 64, "%llu", (UINT64)expect);
			if ( result < (const _Lhs)expect ) return IsLt;
			if ( result > (const _Lhs)expect ) return IsGt;
			return IsEqual;
		}
public: Equality Equals(const FULL_OBJ_ID& result, const FULL_OBJ_ID& expect) const
		{
			snprintf(m_lhsVal, 64, "{%d,%d}.%u", result.cell.ox, result.cell.oy, result.id);
			snprintf(m_rhsVal, 64, "{%d,%d}.%u", expect.cell.ox, expect.cell.oy, expect.id);
			if ( result.Key() < expect.Key() ) return IsLt;
			if ( result.Key() > expect.Key() ) return IsGt;
			return IsEqual;
		}
private: mutable char m_lhsVal[65], m_rhsVal[65];

protected: const char* OperandDesc(Equality value) const { switch ( value ) { case IsLt: return "less than"; case IsEqual: return "equal to"; case IsGt: default: return "greater than"; } }
protected: const char* OperandDesc(bool value) const { return ( value == true ? "true" : "false" ); }

	// Modifiers.
public: void Stage(std::string text) { m_stage = text; m_stageErrors = 0; m_subStage.clear(); m_testFocus = "";}
public: void TestFocus(std::string label) { m_testFocus = label; }
public: void TestFocus(const char* fmt, ...); //__attribute__ ((format (printf,2,3)));	// 2,3 because ... arg#1 is this... UGH.
public: void TestFocus(const wchar_t* label);
public: void StartSubStage(std::string text) { m_subStage.push_back(text); }
public: void EndSubStage() { if ( !m_subStage.empty() )  m_subStage.pop_back(); }
public: void Error(const char* error, bool critical=false);
private: void _failedTest(const char* file, unsigned int lineNo, const char* operation, const char* obtained, const char* expected, bool withOperands);
public:	template<typename _Type> bool Expect(_Type obtained, _Type expected, const char* file, unsigned int lineNo, const char* operation, bool withOperands=false)
	{
		if ( obtained == expected )
		{
			++m_stagePasses;
			++m_unitPasses;
			return true;
		}

		_failedTest(file, lineNo, operation, OperandDesc(obtained), OperandDesc(expected), withOperands);

		return false;
	}
public:
	// Members.
private: std::string m_unit, m_stage;
private: std::string m_fails;
private: bool m_success;
protected: UINT32 m_stagePasses, m_unitPasses;
protected: UINT32 m_stageErrors, m_unitErrors;
private: std::vector<std::string> m_subStage;
private: std::string m_testFocus;
private: std::vector<std::string> m_testFocii;
public: void SaveTestFocus() { m_testFocii.push_back(m_testFocus); }
public: void RestoreTestFocus() { m_testFocus = m_testFocii.back(); m_testFocii.pop_back(); }

public: static void RunTests();
};

// Defines an error that terminates this pass of the test.
#define CRITICAL( op ) { if ( (op) == false ) return false; }

struct StageRAII {
	TestUnit* m_test;

	StageRAII(TestUnit* test_)
		: m_test(test_)
	{
		m_test->SaveTestFocus();
		m_test->BeginStage();
	}
	~StageRAII() {
		m_test->EndStage();
		m_test->RestoreTestFocus();
	}
};

struct SubStageRAII {
	TestUnit* m_test;
	const char* m_name;

	SubStageRAII(TestUnit* test_, const char* name_)
		: m_test(test_)
		, m_name(name_)
	{
		m_test->SaveTestFocus();
		m_test->StartSubStage(m_name);
	}

	~SubStageRAII()
	{
		m_test->EndSubStage();
		m_test->RestoreTestFocus();
	}
};

// Macros for a test scope block
#define START_TEST_STAGE( _stage_ ) \
	{ \
		if ( enableTestDebug ) \
			std::cout << "#Starting Stage " << _stage_ << std::endl; \
		m_stagePasses = 0; \
		StageRAII __stageRAII(this); \
		try {	\
			Stage(_stage_);

#define BEGIN_TEST_STAGE( _stage_ ) START_TEST_STAGE( _stage_ )

#define END_TEST_STAGE( _stage_ ) \
			if ( enableTestDebug ) \
				std::cout << "#Ending Stage " << _stage_ << std::endl; \
		} \
		catch ( std::exception& e ) { std::cout << "FAIL: " << Name() << " " << e.what() << std::endl << Fails() << std::endl; return false; } \
	}

#define START_SUB_STAGE( _substage_ )	\
	{ \
		SubStageRAII __substageRAII(this, _substage_); \
		try {
#define BEGIN_SUB_STAGE( _substage_ ) START_SUB_STAGE(_substage_)

#define	END_SUB_STAGE( _substage_ ) \
		} catch ( std::exception& e ) { std::cout << "FAIL: " << Name() << " " << e.what() << std::endl << Fails() << std::endl; return false; } \
	}

// Test macros
#define Not(op) !(op)
#define ExpectTrue( _expression_ ) Expect((_expression_), true, __FILE__, __LINE__, #_expression_, false)
#define ExpectFalse( _expression_ ) Expect((_expression_), false, __FILE__, __LINE__, #_expression_, false)
#define ExpectLt( _expression_, _rhs_ ) Expect(Equals((_expression_), _rhs_), IsLt, __FILE__, __LINE__, #_expression_, true)
#define ExpectEqual( _expression_, _rhs_ ) Expect(Equals((_expression_), _rhs_), IsEqual, __FILE__, __LINE__, #_expression_, true)
#define ExpectGt( _expression_, _rhs_ ) Expect(Equals((_expression_), _rhs_), IsGt, __FILE__, __LINE__, #_expression_, true)

