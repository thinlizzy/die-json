#include <tut.h>

#include "../die-json.h"
#include <sstream>
#include <iostream>
#include <vector>
#include <memory>

namespace {
	struct Value {
		die::json::Parser::ObjectName name;
		die::json::ValueType valueType;
		die::json::Parser::ValueStr value;
		bool operator==(Value const & v) const {
			return name == v.name && valueType == v.valueType && value == v.value;
		};
		bool operator!=(Value const & v) const { return ! operator==(v); }
	};
}

namespace {
	struct setup {
		die::json::Parser parser;

		void parseSingleValue(std::string const & strToParse, die::json::ValueType valueType, die::json::Parser::ValueStr const & valueStr)
		{
			bool calledV = false;
			parser.value([&](auto name, auto type, auto value) {
				tut::ensure_equals(name, "name");
				tut::ensure_equals(type, valueType);
				tut::ensure_equals(value, valueStr);
				calledV = true;
			});

			std::istringstream iss(strToParse);
			auto parsed = parser.parse(iss);
			tut::ensure("parser did not reach final state", parsed);
			tut::ensure("value has not been called", calledV);
		}

		void setParserEvents(std::string & path, std::vector<Value> & values, std::vector<std::string> & arrays)
		{
			parser
				.startObject([&path](auto name) {
					path += name + "/";
				})
				.endObject([&path](auto name) {
					auto rem = name + "/";
					auto p = path.rfind(rem);
					auto ok = p != std::string::npos && path.size() - p == rem.size();
					tut::ensure(path + "does not end with " + rem, ok);
					path.resize(path.size() - rem.size());
				})
				.value([&values,&path](auto name, auto type, auto value) {
					values.push_back({path+name,type,value});
				})
				.startArray([&arrays](auto name) {
					arrays.push_back(name + '[');
				})
				.endArray([&arrays](auto name) {
					arrays.push_back(name + ']');
				})
			;
		}
	};

	struct SingleCall {
		// shared_ptr is a needed kludge here because I am copying into std::function
		std::shared_ptr<bool> calledPtr = std::make_shared<bool>(false);
		std::string expectedName;
		bool called() const { return *calledPtr; }
		void operator()(std::string const & name) {
			tut::ensure_equals("regarding object name", expectedName, name);
			*calledPtr = true;
		}
	};
}

std::ostream & operator<<(std::ostream & os, Value value)
{
   	return os << value.name << ',' << value.valueType << ',' << value.value;
}

namespace tut {
	using std::istringstream;

	typedef test_group<setup> tg;
	tg parser_test_group("JsonParserOK");

	typedef tg::object testobject;

	template<>
	template<>
	void testobject::test<1>()
	{
		set_test_name("parse empty");

		bool called = false;
		parser.startObject([&called](auto name) {
			ensure("root object name should be empty", name.empty());
			called = true;
		});

		istringstream iss("{}");
		auto parsed = parser.parse(iss);
		ensure("parser did not reach final state", parsed);
		ensure("startObject has not been called", called);
	}

	template<>
	template<>
	void testobject::test<2>()
	{
		set_test_name("parse single string");

		SingleCall startObj,endObj;
		parser
			.startObject(startObj)
			.endObject(endObj);
		bool calledV = false;
		parser.value([&calledV](auto name, auto type, auto value) {
			ensure_equals(name, "name");
			ensure_equals(type, die::json::ValueType::string);
			ensure_equals(value, "value");
			calledV = true;
		});

		istringstream iss("{ \"name\" : \"value\" }");
		auto parsed = parser.parse(iss);
		ensure("parser did not reach final state", parsed);
		ensure("startObject has not been called", startObj.called());
		ensure("endObject has not been called", endObj.called());
		ensure("value has not been called", calledV);
	}

	template<>
	template<>
	void testobject::test<3>()
	{
		set_test_name("string with escape and unicode");
		parseSingleValue(R"json({ "name" : "\"value\" \ua1B0 \\ \/ \b\f\n\r\t" })json",
			die::json::ValueType::string,
			std::string(R"json("value" \ua1B0 \ / )json") + '\b' + '\f' + '\n' + '\r' + '\t');
	}

	template<>
	template<>
	void testobject::test<4>()
	{
		set_test_name("parse keywords");
		parseSingleValue("{ \"name\" : null }",die::json::ValueType::null,"null");
		parseSingleValue("{ \"name\" : true }",die::json::ValueType::boolean,"true");
		parseSingleValue("{ \n"
				"\"name\" : false \n"
				"}",die::json::ValueType::boolean,"false");
	}

	template<>
	template<>
	void testobject::test<5>()
	{
		set_test_name("numbers");
		parseSingleValue("{ \"name\" : 0 }",die::json::ValueType::number,"0");
		parseSingleValue("{ \"name\" : 1029 }",die::json::ValueType::number,"1029");
		parseSingleValue("{ \"name\" : -129 }",die::json::ValueType::number,"-129");
		parseSingleValue("{ \"name\" : 0.3 }",die::json::ValueType::number,"0.3");
		parseSingleValue("{ \"name\" : -1.324 }",die::json::ValueType::number,"-1.324");
		parseSingleValue("{ \"name\" : -0.55 }",die::json::ValueType::number,"-0.55");
		parseSingleValue("{ \"name\" : 0.55e10 }",die::json::ValueType::number,"0.55e10");
		parseSingleValue("{ \"name\" : 55E+88 }",die::json::ValueType::number,"55E+88");
		parseSingleValue("{ \"name\" : -4.5E-2878 }",die::json::ValueType::number,"-4.5E-2878");
		parseSingleValue("{ \"name\" : 77.66e+20 }",die::json::ValueType::number,"77.66e+20");
	}

	template<>
	template<>
	void testobject::test<6>()
	{
		set_test_name("more values");

		std::vector<Value> values;
		parser.value([&values](auto name, auto type, auto value) {
			values.push_back({name,type,value});
		});

		istringstream iss("{ \n"
				"\"nameStr\" : \"value\"   ,\n"
				"\"nameNum\" : 10,\n"
				"\"zero\" :	 0,\n"
				"   \"superTrue\":true,\n"
				"\"myNull\" : null,\n"
				"\"nameBool\" : false\n"
				" }");
		auto parsed = parser.parse(iss);
		ensure("parser did not reach final state", parsed);
		ensure_equals(values.size(), 6);
		ensure_equals(values[0], Value{"nameStr", die::json::ValueType::string, "value"});
		ensure_equals(values[1], Value{"nameNum", die::json::ValueType::number, "10"});
		ensure_equals(values[2], Value{"zero", die::json::ValueType::number, "0"});
		ensure_equals(values[3], Value{"superTrue", die::json::ValueType::boolean, "true"});
		ensure_equals(values[4], Value{"myNull", die::json::ValueType::null, "null"});
		ensure_equals(values[5], Value{"nameBool", die::json::ValueType::boolean, "false"});
	}

	template<>
	template<>
	void testobject::test<7>()
	{
		set_test_name("nested objects");

		std::string path;
		std::vector<Value> values;
		std::vector<std::string> arrays;
		setParserEvents(path,values,arrays);

		istringstream iss(R"json(
{
	"nameStr" : "value",
	"object1" : {
		"zero":0,
		"nameStr" : "value",
		"placeholder" : null
	},
	"object2" : {},
	"anotherStr":"valueStr2",
	"object3" : {
		"numbah" : 0.5E-23,
		"nested" : { "meh" : null }
	},
	"lastValue":-1
}
		)json");

		auto parsed = parser.parse(iss);
		ensure("parser did not reach final state", parsed);
		ensure_equals(values.size(), 8);
		ensure_equals(values[0], Value{"/nameStr", die::json::ValueType::string, "value"});
		ensure_equals(values[1], Value{"/object1/zero", die::json::ValueType::number, "0"});
		ensure_equals(values[2], Value{"/object1/nameStr", die::json::ValueType::string, "value"});
		ensure_equals(values[3], Value{"/object1/placeholder", die::json::ValueType::null, "null"});
		ensure_equals(values[4], Value{"/anotherStr", die::json::ValueType::string, "valueStr2"});
		ensure_equals(values[5], Value{"/object3/numbah", die::json::ValueType::number, "0.5E-23"});
		ensure_equals(values[6], Value{"/object3/nested/meh", die::json::ValueType::null, "null"});
		ensure_equals(values[7], Value{"/lastValue", die::json::ValueType::number, "-1"});
	}

	template<>
	template<>
	void testobject::test<8>()
	{
		set_test_name("empty array");

		SingleCall startObj,endObj,startArr,endArr;
		startArr.expectedName = "array";
		endArr.expectedName = "array";
		parser
			.startObject(startObj)
			.endObject(endObj)
			.startArray(startArr)
			.endArray(endArr);

		bool calledV = false;
		parser.value([&calledV](auto name, auto type, auto value) {
			calledV = true;
		});

		istringstream iss("{ \"array\" : [] }");
		auto parsed = parser.parse(iss);
		ensure("parser did not reach final state", parsed);
		ensure("startObject has not been called", startObj.called());
		ensure("startArray has not been called", startArr.called());
		ensure("endArray has not been called", endArr.called());
		ensure("endObject has not been called", endObj.called());
		ensure_not("value should never be called", calledV);
	}

	template<>
	template<>
	void testobject::test<9>()
	{
		set_test_name("arrays of values");

		std::vector<Value> values;
		SingleCall startObj,endObj,startArr,endArr;
		startArr.expectedName = "array";
		endArr.expectedName = "array";
		parser
			.startObject(startObj)
			.endObject(endObj)
			.startArray(startArr)
			.endArray(endArr)
			.value([&values](auto name, auto type, auto value) {
				values.push_back({name,type,value});
			})
		;

		istringstream iss("{ \"array\" : [ 1,2, null, \"oi\",false,0 ] }");
		auto parsed = parser.parse(iss);
		ensure("parser did not reach final state", parsed);
		ensure("startObject has not been called", startObj.called());
		ensure("startArray has not been called", startArr.called());
		ensure("endArray has not been called", endArr.called());
		ensure("endObject has not been called", endObj.called());
		ensure_equals(values.size(), 6);
		ensure_equals(values[0], Value{"array", die::json::ValueType::number, "1"});
		ensure_equals(values[1], Value{"array", die::json::ValueType::number, "2"});
		ensure_equals(values[2], Value{"array", die::json::ValueType::null, "null"});
		ensure_equals(values[3], Value{"array", die::json::ValueType::string, "oi"});
		ensure_equals(values[4], Value{"array", die::json::ValueType::boolean, "false"});
		ensure_equals(values[5], Value{"array", die::json::ValueType::number, "0"});
	}

	template<>
	template<>
	void testobject::test<10>()
	{
		set_test_name("arrays of objects and values");

		std::string path;
		std::vector<Value> values;
		std::vector<std::string> arrays;
		setParserEvents(path,values,arrays);

		istringstream iss(R"json(
{
	"nameStr" : "value",
	"myObjects" : [
		{
			"zero":0,
			"nameStr" : "value",
			"placeholder" : null
		},
		{},
		"valueStr2",
		{
			"numbah" : 0.5E-23,
			"nested" : { "meh" : null }
		},
		-1
	],
	"otherArray" : [2,1]
}
		)json");

		auto parsed = parser.parse(iss);
		ensure("parser did not reach final state", parsed);
		ensure_equals(values.size(), 10);
		ensure_equals(values[0], Value{"/nameStr", die::json::ValueType::string, "value"});
		ensure_equals(values[1], Value{"/myObjects/zero", die::json::ValueType::number, "0"});
		ensure_equals(values[2], Value{"/myObjects/nameStr", die::json::ValueType::string, "value"});
		ensure_equals(values[3], Value{"/myObjects/placeholder", die::json::ValueType::null, "null"});
		ensure_equals(values[4], Value{"/myObjects", die::json::ValueType::string, "valueStr2"});
		ensure_equals(values[5], Value{"/myObjects/numbah", die::json::ValueType::number, "0.5E-23"});
		ensure_equals(values[6], Value{"/myObjects/nested/meh", die::json::ValueType::null, "null"});
		ensure_equals(values[7], Value{"/myObjects", die::json::ValueType::number, "-1"});
		ensure_equals(values[8], Value{"/otherArray", die::json::ValueType::number, "2"});
		ensure_equals(values[9], Value{"/otherArray", die::json::ValueType::number, "1"});
		ensure_equals(arrays.size(), 4);
		ensure_equals(arrays[0], "myObjects[");
		ensure_equals(arrays[1], "myObjects]");
		ensure_equals(arrays[2], "otherArray[");
		ensure_equals(arrays[3], "otherArray]");
	}

	template<>
	template<>
	void testobject::test<11>()
	{
		set_test_name("arrays of arrays");

		std::string path;
		std::vector<Value> values;
		std::vector<std::string> arrays;
		setParserEvents(path,values,arrays);

		istringstream iss(R"json(
{
	"myObjects" : [
		1,
		{
			"zero":0,
			"nameStr" : "value",
			"placeholder" : null
		},
		"valueStr3",
		[1,2,3],
		{
			"numbah" : 0.5E-23,
			"nested" : { "meh" : null }
		},
		[4,5,{ "six": [7,8,9], "ten" : null }, 11 ]
	]
}
		)json");

		auto parsed = parser.parse(iss);
		ensure("parser did not reach final state", parsed);
		ensure_equals(values.size(), 17);
		ensure_equals(values[0], Value{"/myObjects", die::json::ValueType::number, "1"});
		ensure_equals(values[1], Value{"/myObjects/zero", die::json::ValueType::number, "0"});
		ensure_equals(values[2], Value{"/myObjects/nameStr", die::json::ValueType::string, "value"});
		ensure_equals(values[3], Value{"/myObjects/placeholder", die::json::ValueType::null, "null"});
		ensure_equals(values[4], Value{"/myObjects", die::json::ValueType::string, "valueStr3"});
		ensure_equals(values[5], Value{"/myObjects", die::json::ValueType::number, "1"});
		ensure_equals(values[6], Value{"/myObjects", die::json::ValueType::number, "2"});
		ensure_equals(values[7], Value{"/myObjects", die::json::ValueType::number, "3"});
		ensure_equals(values[8], Value{"/myObjects/numbah", die::json::ValueType::number, "0.5E-23"});
		ensure_equals(values[9], Value{"/myObjects/nested/meh", die::json::ValueType::null, "null"});
		ensure_equals(values[10], Value{"/myObjects", die::json::ValueType::number, "4"});
		ensure_equals(values[11], Value{"/myObjects", die::json::ValueType::number, "5"});
		ensure_equals(values[12], Value{"/myObjects/six", die::json::ValueType::number, "7"});
		ensure_equals(values[13], Value{"/myObjects/six", die::json::ValueType::number, "8"});
		ensure_equals(values[14], Value{"/myObjects/six", die::json::ValueType::number, "9"});
		ensure_equals(values[15], Value{"/myObjects/ten", die::json::ValueType::null, "null"});
		ensure_equals(values[16], Value{"/myObjects", die::json::ValueType::number, "11"});
		ensure_equals(arrays.size(), 8);
		ensure_equals(arrays[0], "myObjects[");
		ensure_equals(arrays[1], "myObjects[");
		ensure_equals(arrays[2], "myObjects]");
		ensure_equals(arrays[3], "myObjects[");
		ensure_equals(arrays[4], "six[");
		ensure_equals(arrays[5], "six]");
		ensure_equals(arrays[6], "myObjects]");
		ensure_equals(arrays[7], "myObjects]");
	}
}
