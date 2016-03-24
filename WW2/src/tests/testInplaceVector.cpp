#include <syscfg.h> // Path is project specific.
// Test the Chat::LocalizedMessage functionality.

#include "tests/testUnits.h"
#include "include/inplace_vector.h"

// Test unit class.
class InplaceVectorTest : public TestUnit
{
public:	InplaceVectorTest() : TestUnit("inplace_vector") {}

public: virtual ~InplaceVectorTest() {}

public:	virtual bool Run() override;
};

bool
InplaceVectorTest::Run()
{
	START_TEST_STAGE( "Basic Tests" )

		START_SUB_STAGE( "Constructor" )

			WWII::inplace_vector<int, 8> iv;
			CRITICAL(
				ExpectEqual( iv.size(), 0 ) && ExpectEqual( iv.capacity(), 8 ) && ExpectEqual( iv.max_size(), 8 )
			);

			CRITICAL(
				ExpectTrue( iv.begin() == iv.end() )
				&& ExpectTrue( iv.rbegin() == iv.rend() )
				&& ExpectTrue( iv.empty() )
			);

			// Sanity check that data is not a null pointer.
			CRITICAL(
				ExpectGt( iv.data(), 0ULL )
			);
		END_SUB_STAGE( "Constructor" )

		START_SUB_STAGE( "push_back" )

			WWII::inplace_vector<int, 5> iv;
			CRITICAL( ExpectEqual( iv.capacity(), 5 ) && ExpectEqual( iv.size(), 0 ) );

			iv.push_back(99);

			CRITICAL(
				ExpectEqual( iv.size(), 1 )
				&& ExpectEqual( iv.capacity(), 5 )
				&& ExpectFalse( iv.empty() )
				&& ExpectTrue( iv.begin() != iv.end() )
				&& ExpectTrue( iv.rbegin() != iv.rend() )
				&& ExpectEqual( *(iv.begin()), 99 )
				&& ExpectEqual( iv.front(), 99 )
				&& ExpectEqual( iv.back(), 99 )
				&& ExpectEqual( *(iv.data()), 99 )
				&& ExpectEqual( iv.at(0), 99 )
				&& ExpectEqual( iv[0], 99 )
			);

		END_SUB_STAGE( "push_back" )

		START_SUB_STAGE( "pop_back" )

			WWII::inplace_vector<int, 15> iv;
			CRITICAL( ExpectEqual( iv.capacity(), 15 ) && ExpectEqual( iv.size(), 0 ) );

			iv.push_back(123);

			iv.pop_back();

			CRITICAL(
				ExpectEqual( iv.size(), 0 )
				&& ExpectEqual( iv.capacity(), 15 )
				&& ExpectTrue( iv.empty() )
				&& ExpectTrue( iv.begin() == iv.end() )
			);

		END_SUB_STAGE( "pop_back" )

		START_SUB_STAGE( "fill to capacity" )
			WWII::inplace_vector<const char*, 3> iv;
			CRITICAL( ExpectEqual( iv.capacity(), 3 ) && ExpectEqual( iv.size(), 0 ) );

			CRITICAL(
				ExpectTrue( iv.push_back("first") )
				&& ExpectTrue( iv.push_back("second") )
				&& ExpectTrue( iv.push_back("third") )
			);

			// Make sure things measure up.

			CRITICAL(
				ExpectEqual( iv.capacity(), 3 )
				&& ExpectEqual( iv.size(), 3 )
				&& ExpectFalse( iv.empty() )
			);
			CRITICAL(
				ExpectTrue( strcmp(iv[0], "first") == 0 )
				&& ExpectTrue( strcmp(iv[1], "second") == 0 )
				&& ExpectTrue( strcmp(iv[2], "third") == 0 )
			);
			CRITICAL( ExpectTrue( strcmp(iv.front(), "first") == 0 ) );
			CRITICAL( ExpectTrue( strcmp(iv.back(), "third") == 0 ) );

			CRITICAL( ExpectTrue( strcmp(*iv.rbegin(), "third") == 0 ) );

			CRITICAL( ExpectTrue( strcmp(*(iv.rbegin()), "third") == 0 ) );
			CRITICAL( ExpectTrue( strcmp(*(iv.rend() - 1), "first") == 0 ) );
			CRITICAL(
				ExpectTrue( strcmp(iv.at(0), "first") == 0 )
				&& ExpectTrue( strcmp(iv.at(1), "second") == 0 )
				&& ExpectTrue( strcmp(iv.at(2), "third") == 0 )
			);

			ExpectFalse( iv.push_back("exceeds capacity") );

		END_SUB_STAGE( "fill to capacity" )

		START_SUB_STAGE( "swap_to_back_and_pop" )

			WWII::inplace_vector<std::string, 4> iv;
			CRITICAL( ExpectEqual( iv.capacity(), 4 ) && ExpectEqual( iv.size(), 0 ) );

			CRITICAL(
				ExpectTrue( iv.push_back("first") )
				&& ExpectTrue( iv.push_back("second") )
				&& ExpectTrue( iv.push_back("third") )
				&& ExpectTrue( iv.push_back("fourth") )
				&& ExpectFalse( iv.push_back("exceeds capacity") )
			);

			CRITICAL(
				ExpectEqual( iv.size(), 4 )
				&& ExpectFalse( iv.empty() )
				&& ExpectEqual( iv.capacity(), 4 )
				&& ExpectEqual( iv.max_size(), 4 )
				&& ExpectTrue( iv.begin() != iv.end() )
				&& ExpectTrue( iv.front() == "first" )
				&& ExpectTrue( iv.back() == "fourth" )
			);

			START_SUB_STAGE( "tail removal" )
				iv.swap_to_back_and_pop(iv.end() - 1);
				CRITICAL(
					ExpectEqual( iv.size(), 3 )
					&& ExpectTrue( iv.front() == "first" )
					&& ExpectTrue( iv.back() == "third" )
				);
			END_SUB_STAGE( "tail removal" )

			START_SUB_STAGE( "mid-removal" )
				iv.swap_to_back_and_pop(iv.begin() + 1);
				CRITICAL(
					ExpectEqual( iv.size(), 2 )
					&& ExpectTrue( iv.front() == "first" )
					&& ExpectTrue( iv[1] == "third" )
				);
			END_SUB_STAGE( "mid-removal" )

			START_SUB_STAGE( "front-removal non-emptying" )
				iv.swap_to_back_and_pop(iv.begin());
				CRITICAL(
					ExpectEqual( iv.size(), 1 )
					&& ExpectTrue( iv.front() == "third" )
					&& ExpectTrue( *(iv.end() - 1) == iv.front() )
				);
			END_SUB_STAGE( "front-removal non-emptying" )

			iv[1] = "data moved";

			START_SUB_STAGE( "front-removal emptying" )
				iv.swap_to_back_and_pop(iv.begin());
				CRITICAL(
					ExpectEqual( iv.size(), 0 )
					&& ExpectTrue( iv[0] == "third" )
				);
			END_SUB_STAGE( "front-removal emptying")

		END_SUB_STAGE( "swap_to_back_and_pop" )

		START_SUB_STAGE( "clear" )
			WWII::inplace_vector<UINT64, 4> iv;

			static const UINT64 TestPattern1 = 0xB50FABE74D2A1CFFULL;
			static const UINT64 TestPattern2 = ~TestPattern1;
			static const UINT64 TestPattern3 = 0ULL;
			static const UINT64 TestPattern4 = ~TestPattern3;

			CRITICAL(
				ExpectTrue( iv.push_back(TestPattern1) )
				&& ExpectTrue( iv.push_back(TestPattern2) )
				&& ExpectTrue( iv.push_back(TestPattern3) )
				&& ExpectTrue( iv.push_back(TestPattern4) )
			);

			CRITICAL(
				ExpectEqual( iv.capacity(), iv.size() )
				&& ExpectEqual( iv.max_size(), 4 )
				&& ExpectEqual( iv[0], TestPattern1 )
				&& ExpectEqual( iv[1], TestPattern2 )
				&& ExpectEqual( iv[2], TestPattern3 )
				&& ExpectEqual( iv[3], TestPattern4 )
			);

			// Should move pattern4 to position 3 ([2]).
			iv.swap_to_back_and_pop(iv.begin() + 2);
			ExpectEqual( iv[2], TestPattern4 );

			iv.clear();
			ExpectTrue( iv.empty() );
			ExpectEqual( iv.size(), 0 );
			ExpectEqual( iv.capacity(), 4 );
		END_SUB_STAGE( "clear" )

	END_TEST_STAGE( "Basic Tests" )

	return true;
}

static InplaceVectorTest test;

