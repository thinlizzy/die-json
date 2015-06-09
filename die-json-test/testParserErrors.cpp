#include <tut.h>

#include "../die-json.h"
#include <sstream>

namespace {
	struct setup {
		die::json::Parser parser;

		void didNotFinish(std::string toParse, int line, int column)
		{
			std::istringstream iss(toParse);
			auto parsed = parser.parse(iss);
			tut::ensure_not("parser should never reach final state", parsed);
			auto pos = parser.lastPosition();
			tut::ensure_equals(pos.line, line);
			tut::ensure_equals(pos.column, column);
		}
	};
}

namespace tut {
	using std::istringstream;

	typedef test_group<setup> tg;
	tg parser_test_group("JsonParserErrors");

	typedef tg::object testobject;

	template<>
	template<>
	void testobject::test<1>()
	{
		set_test_name("unexpected garbage at start");
		didNotFinish(" auu {}",1,2);
	}

	template<>
	template<>
	void testobject::test<2>()
	{
		set_test_name("closing too many objects");
		istringstream iss("{}}");
		bool error = false;
		try {
			parser.parse(iss);
		} catch(die::json::UnexpectedObjectClosing & e) {
			error = true;
			ensure_equals(e.offendingChar(), '}');
		}
		ensure("parser should have thrown exception", error);
		auto pos = parser.lastPosition();
		ensure_equals(pos.line, 1);
		ensure_equals(pos.column, 3);
	}

	template<>
	template<>
	void testobject::test<3>()
	{
		set_test_name("closing too many arrays");
		istringstream iss("{ \"a\":[1,2]] }");
		bool error = false;
		try {
			parser.parse(iss);
		} catch(die::json::UnexpectedArrayClosing & e) {
			error = true;
			ensure_equals(e.offendingChar(), ']');
		}
		ensure("parser should have thrown exception", error);
		auto pos = parser.lastPosition();
		ensure_equals(pos.line, 1);
		ensure_equals(pos.column, 12);
	}

	template<>
	template<>
	void testobject::test<4>()
	{
		set_test_name("object never closed");
		didNotFinish(" { ",1,4);
	}

	template<>
	template<>
	void testobject::test<5>()
	{
		set_test_name("comma trail over");
		didNotFinish("{ \"a\":1, }",1,10);
	}

	template<>
	template<>
	void testobject::test<6>()
	{
		set_test_name("missing :");
		didNotFinish("{ \"a\" 1 }",1,7);
	}

	template<>
	template<>
	void testobject::test<7>()
	{
		set_test_name("expected char");
		istringstream iss("{ \"a\", 1 }");
		bool error = false;
		try {
			parser.parse(iss);
		} catch(die::json::ExpectedChar & e) {
			error = true;
			ensure_equals(e.offendingChar(), ',');
		}
		ensure("parser should have thrown exception", error);
		auto pos = parser.lastPosition();
		ensure_equals(pos.line, 1);
		ensure_equals(pos.column, 6);
	}

	template<>
	template<>
	void testobject::test<8>()
	{
		set_test_name("unexpected :");
		istringstream iss("{ \"a\" : [\"hey\" : \"hey\"] }");
		bool error = false;
		try {
			parser.parse(iss);
		} catch(die::json::UnexpectedChar & e) {
			error = true;
			ensure_equals(e.offendingChar(), ':');
		}
		ensure("parser should have thrown exception", error);
		auto pos = parser.lastPosition();
		ensure_equals(pos.line, 1);
		ensure_equals(pos.column, 16);
	}
}

