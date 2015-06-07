#include <tut.h>

#include "../src/JsonParser.h"
#include <sstream>
#include <iostream>
#include <vector>
#include <memory>

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
    };

    struct Value {
    	die::json::Parser::ObjectName name;
    	die::json::ValueType valueType;
    	die::json::Parser::ValueStr value;
    	bool operator==(Value const & v) const {
    		return name == v.name && valueType == v.valueType && value == v.value;
    	};
    	bool operator!=(Value const & v) const { return ! operator==(v); }
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
    tg parser_test_group("JsonParser");

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
        parseSingleValue("{ \"name\" : \"\\\"value\\\" \\ua1B0\" }",
        		die::json::ValueType::string,"\"value\" \\ua1B0");
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
        		"\"zero\" :     0,\n"
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

    // nested objects

    // arrays of objects

    // incomplete parsing

    // exceptions

    // implement \ properly on strings. test

    /*
        std::cout << parser.getLastPosition() << std::endl;
        int calledS = 0;
        parser.startObject([&calledS](auto name) {
        	ensure("root object <"+name+"> should be empty or name", name.empty() || name == "name");
        	++calledS;
        });
        int calledE = 0;
        parser.endObject([&calledE](auto name) {
        	ensure("root object <"+name+"> should be empty or name", name.empty() || name == "name");
        	++calledE;
        });

     */
}
