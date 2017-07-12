#include "gtest/gtest.h"

#define NO_LOGGER_ASSERT

#include "Misc/temp_basicstring.h"
#include "Misc/temp_Throwback.h"

// TEST FUNCTIONS

int Throwback_SpinTest_2values ( int v1, int v2, int v3 )
{
	typedef Throwback_Arithmetic<int, int, 2> tb_int;

	tb_int tb;

	tb.Store ( v1, 0 );
	tb.Store ( v2, 0 );
	tb.Store ( v3, 0 );
	tb.Store ( v2, 0 );
	tb.Store ( v3, 0 );
	tb.Store ( v1, 0 );

	tb_int::inner_type values[ 2 ];
	size_t amount;

	tb.CopyHistory ( values, amount );

	return values[ 1 ].v; // return the oldest value in history
}

int Throwback_SpinTest_5values ( int v1, int v2, int v3, int v4, int v5, int v6, int v7, int v8, int v9 )
{
	typedef Throwback_Arithmetic<int, int, 5> tb_int;

	tb_int tb;

	tb.Store ( v1, 0 );
	tb.Store ( v2, 0 );
	tb.Store ( v3, 0 );
	tb.Store ( v4, 0 );
	tb.Store ( v5, 0 );
	tb.Store ( v6, 0 );
	tb.Store ( v7, 0 );
	tb.Store ( v8, 0 );
	tb.Store ( v9, 0 );

	tb_int::inner_type values[ 5 ];
	size_t amount;

	tb.CopyHistory ( values, amount );

	return values[ 1 ].v; // return the second value ( past the latest stored = v8 )
}

float Throwback_AvgTest ( int v1, int v2, int v3 )
{
	typedef Throwback_Arithmetic<int, int, 3> tb_int;

	tb_int tb;

	tb.Store ( v1, 0 );
	tb.Store ( v2, 0 );
	tb.Store ( v3, 0 );

	return tb.Average ();
}

int Throwback_MinTest ( int v1, int v2, int v3 )
{
	typedef Throwback_Arithmetic<int, int, 3> tb_int;

	tb_int tb;

	tb.Store ( v1, 0 );
	tb.Store ( v2, 0 );
	tb.Store ( v3, 0 );

	return tb.Min ();
}

int Throwback_MaxTest ( int v1, int v2, int v3 )
{
	typedef Throwback_Arithmetic<int, int, 3> tb_int;

	tb_int tb;

	tb.Store ( v1, 0 );
	tb.Store ( v2, 0 );
	tb.Store ( v3, 0 );

	return tb.Max ();
}

// TEST MACROS

TEST ( Throwback_Class, SpinTest )
{
	ASSERT_EQ ( 4, Throwback_SpinTest_2values ( 5, 8, 4 ) );
	ASSERT_EQ ( 8, Throwback_SpinTest_5values ( 1, 2, 3, 4, 5, 6, 7, 8, 9 ) );
}

TEST ( Throwback_Class, AvgTest )
{
	ASSERT_EQ ( 5.0f, Throwback_AvgTest ( 5, 5, 5 ) );
	ASSERT_EQ ( 5.0f, Throwback_AvgTest ( 0, 10, 5 ) );
}

TEST ( Throwback_Class, AvgTest_Neg )
{
	ASSERT_EQ ( -5.0f / 3.0f, Throwback_AvgTest ( 0, -10, 5 ) );
}

TEST ( Throwback_Class, MinTest )
{
	ASSERT_EQ ( 5, Throwback_MinTest ( 5, 5, 5 ) );
	ASSERT_EQ ( 0, Throwback_MinTest ( 0, 10, 5 ) );
}

TEST ( Throwback_Class, MinTest_Neg )
{
	ASSERT_EQ ( -10, Throwback_MinTest ( 0, -10, 5 ) );
}

TEST ( Throwback_Class, MaxTest )
{
	ASSERT_EQ ( 5, Throwback_MaxTest ( 5, 5, 5 ) );
	ASSERT_EQ ( 10, Throwback_MaxTest ( 0, 10, 5 ) );
}

TEST ( Throwback_Class, MaxTest_Neg )
{
	ASSERT_EQ ( 5, Throwback_MaxTest ( 0, -10, 5 ) );
	ASSERT_EQ ( 0, Throwback_MaxTest ( 0, -10, -5 ) );
}

TEST ( Sring_Class, Constructors )
{
	basic_string g;
	ASSERT_STREQ ( "", g.c_str () );

	char weak_string[] = { 'a', '\0', 'z' };
	basic_string t ( weak_string, 0, 3 );
	ASSERT_STREQ ( "a", t.c_str () );
	ASSERT_EQ(1, t.size());

	basic_string e ( t );
	ASSERT_STREQ ( "a", e.c_str () );

	basic_string s ( t.c_str () );
	ASSERT_STREQ ( "a", s.c_str () );

	basic_string q ( std::move ( t ) );
	ASSERT_STREQ ( "a", q.c_str () );
	ASSERT_STREQ ( "", t.c_str () );
	ASSERT_TRUE ( t.isempty () );

	basic_string z(weak_string, weak_string);
	ASSERT_STREQ("aa", z.c_str());
}

TEST ( Sring_Class, Assignements )
{
	basic_string g;
	char weak_string[] = { 'a', 'k', '\0', 'z' };

	g = weak_string;
	ASSERT_STREQ ( "ak", g.c_str () );

	basic_string t;
	t = g;
	ASSERT_STREQ ( "ak", t.c_str () );

	t = "u";
	ASSERT_STREQ ( "u", t.c_str () );

	g = std::move ( t );
	ASSERT_STREQ ( "u", g.c_str () );
	ASSERT_STREQ ( "", t.c_str () );
	ASSERT_TRUE ( t.isempty () );
}

TEST ( Sring_Class, Booleans )
{
	basic_string g ( "eGg" );
	basic_string q ( g );

	ASSERT_TRUE ( g == q );
	ASSERT_FALSE ( g == "egg" );
	ASSERT_FALSE ( g.isempty () );
	//ASSERT_TRUE ( basic_string::IsValidMultibyteString(q )); // This fucking function takes 23 ms !!!!!
}

TEST ( Sring_Class, Modifiers )
{
	basic_string g ( "qefiwshgwsbg" );

	g.replace ( basic_string ( "wsbg" ), basic_string ( "aaaaaaa" ) );
	ASSERT_EQ ( 15, g.length () );
	ASSERT_STREQ ( "qefiwshgaaaaaaa", g.c_str () );

	g.replace ( basic_string ( "efiwsh" ), basic_string ( "b" ) );
	ASSERT_STREQ ( "qbgaaaaaaa", g.c_str () );

	g.replace ( "qbg", 'b' );
	ASSERT_STREQ ( "bbbaaaaaaa", g.c_str () );

	g.remove ( 0 );
	ASSERT_STREQ ( "bbaaaaaaa", g.c_str () );

	g.append ( "qq" );
	ASSERT_STREQ ( "bbaaaaaaaqq", g.c_str () );

	g.upper ();
	ASSERT_STREQ ( "BBAAAAAAAQQ", g.c_str () );

	g.lower ();
	ASSERT_STREQ ( "bbaaaaaaaqq", g.c_str () );

	g.insert_at_start(basic_string("zsc"));
	ASSERT_STREQ("zscbbaaaaaaaqq", g.c_str());

	g.insert_at_start("zsc");
	ASSERT_STREQ("zsczscbbaaaaaaaqq", g.c_str());

	g.insert_at_start('z');
	ASSERT_STREQ("zzsczscbbaaaaaaaqq", g.c_str());

	g.insert_at_start('\0');
	ASSERT_STREQ("", g.c_str());
	ASSERT_EQ(0, g.size());
}

TEST ( Sring_Class, Size_Consistency )
{
	basic_string g ( "aaaaa:aaaa" );
	ASSERT_EQ ( 10, g.length () );

	g.replace ( ':', '\0' );
	ASSERT_EQ ( 5, g.length () );

	g.append ( "\0\0" );
	ASSERT_EQ ( 5, g.length () );

	g.append ( '\0' );
	ASSERT_EQ ( 5, g.length () );

	g.replace ( "a", "" );
	ASSERT_EQ ( 0, g.length () );
}

// main

int main ( int argc, char **argv )
{
	HeapMemoryManager::InitPool ();
	::testing::InitGoogleTest ( &argc, argv );
	int test_results ( RUN_ALL_TESTS () );
	HeapMemoryManager::FreePool ();
	return test_results;
}
