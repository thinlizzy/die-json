#include "JsonParser.h"
#include "JsonParserExceptions.h"

namespace {
	static std::string const S = " \t\n\r";
}

namespace die {
namespace json {

Parser::Parser():
	startObjectFn([](auto){}),
	endObjectFn([](auto){}),
	startArrayFn(startObjectFn),
	endArrayFn(endObjectFn),
	valueFn([](auto,auto,auto){})
{
	// local aux dumb functions

	auto startObject = [this](char) {
		startObjectFn(context.objectName);
		changeContext(Status::object);
	};
	auto endObject = [this](char ch) {
		if( context.status != Status::object ) throw UnexpectedObjectClosing(pos);
		popContext(ch);
		endObjectFn(context.objectName);
	};
	auto startArray = [this](char) {
		startArrayFn(context.objectName);
		changeContext(Status::array);
		valueStr.clear();
	};
	auto endArray = [this](char ch) {
		if( context.status != Status::array ) throw UnexpectedArrayClosing(pos);
		popContext(ch);
		endArrayFn(context.objectName);
	};

	auto addValueCh = [this](char ch) {
		addValue(ch);
	};
	auto startNumber = [this](char ch) {
		addValue(ch);
		numberPart = NumberPart::beforeDot;
	};

	auto emitNumberAndClear = [this](char) {
		emitAndClear(ValueType::number);
	};
	auto emitNumberAndEndObject = [this,endObject](char ch) {
		emitAndEnd(ValueType::number,endObject,ch);
	};
	auto emitNumberAndEndArray = [this,endArray](char ch) {
		emitAndEnd(ValueType::number,endArray,ch);
	};
	auto addAndEmitKeyword = [this](char ch) {
		addValue(ch);
		emitValue(keywordType);
	};

	// define the automaton

	automata::RangeSetter<CharType,automata::MealyTransition> rs(parserAut);

	// start
	rs.setTrans("start",S,"start");
	parserAut.setTrans("start",'{',"startObject").output = startObject;

	// startObject
	rs.setTrans("startObject",S,"startObject");
	parserAut.setTrans("startObject",'}',"endAggregate").output = endObject; // empty object
	parserAut.setTrans("startObject",'"',"stringValue").output = [this](auto) {
		isObjectName = true;
		valueStr.clear();
	};

	// stringValue
	parserAut.getNode("stringValue").defaultTransition = [this](char ch) {
		addValue(ch);
		return &parserAut.getNode("stringValue");
	};
	parserAut.setTrans("stringValue",'\\',"stringEscape");
	parserAut.setTrans("stringValue",'"',"endValue").output = [this](char ch) {
		if( isObjectName ) {
			context.objectName = valueStr;
		} else {
			emitValue(ValueType::string);
		}
	};

	// stringEscape and unicodeChar
	rs.setTrans("stringEscape","/\\\"","stringValue",addValueCh);
	parserAut.setTrans("stringEscape",'b',"stringValue").output = [this](char) {
		addValue('\b');
	};
	parserAut.setTrans("stringEscape",'f',"stringValue").output = [this](char) {
		addValue('\f');
	};
	parserAut.setTrans("stringEscape",'n',"stringValue").output = [this](char) {
		addValue('\n');
	};
	parserAut.setTrans("stringEscape",'r',"stringValue").output = [this](char) {
		addValue('\r');
	};
	parserAut.setTrans("stringEscape",'t',"stringValue").output = [this](char) {
		addValue('\t');
	};
	parserAut.setTrans("stringEscape",'u',"unicodeChar").output = [this](auto) {
		unicodeStr.clear();
	};
	parserAut.getNode("unicodeChar").defaultTransition = [this](char ch) -> Automata::NodeRef {
		if( (ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') ) {
			unicodeStr.push_back(ch);
			if( unicodeStr.size() == 4 ) {
				valueStr += CharTraits<CharType>::parseUnicode(unicodeStr);
				return &parserAut.getNode("stringValue");
			} else {
				return &parserAut.getNode("unicodeChar");
			}
		} else {
			return nullptr;
		}
	};

	// startValue
	rs.setTrans("startValue",S,"startValue");
	parserAut.setTrans("startValue",'{',"startObject").output = startObject;
	parserAut.setTrans("startValue",'[',"startValue").output = startArray;
	parserAut.setTrans("startValue",']',"endAggregate").output = endArray; // empty array
	parserAut.setTrans("startValue",'"',"stringValue");
	parserAut.setTrans("startValue",'-',"negativeNumber").output = addValueCh;
	parserAut.setTrans("startValue",'0',"startFractionalNumber").output = addValueCh;
	rs.setTrans("startValue","1-9","numberValue",startNumber);
	auto newKeyword = [this](ValueType keywordType_) {
		return [this,keywordType_](char ch) {
			keywordType = keywordType_;
			addValue(ch);
		};
	};
	parserAut.setTrans("startValue",'n',"nullValue-ull").output = newKeyword(ValueType::null);
	parserAut.setTrans("startValue",'t',"trueValue-rue").output =
	parserAut.setTrans("startValue",'f',"falseValue-alse").output = newKeyword(ValueType::boolean);

	// endValue
	rs.setTrans("endValue",S,"endValue");
	parserAut.setTrans("endValue",':',"startValue").output = [this](auto ch) {
		if( ! isObjectName ) throw UnexpectedChar(pos,':');
		isObjectName = false;
		valueStr.clear();
	};
	parserAut.setTrans("endValue",',',"startValue").output = [this](char ch) {
		if( isObjectName ) throw ExpectedChar(pos,':',ch);
		startValue();
	};
	parserAut.setTrans("endValue",'}',"endAggregate").output = endObject;
	parserAut.setTrans("endValue",']',"endAggregate").output = endArray;

	// negativeNumber
	parserAut.setTrans("negativeNumber",'0',"startFractionalNumber").output = addValueCh;
	rs.setTrans("negativeNumber","1-9","numberValue",startNumber);

	// startFractionalNumber (or just zero)
	rs.setTrans("startFractionalNumber",S,"endValue",[this](char) {
		emitValue(ValueType::number);
	});
	parserAut.setTrans("startFractionalNumber",',',"startValue").output = emitNumberAndClear;
	auto startDot = [this](char ch) {
		addValue(ch);
		numberPart = NumberPart::afterDot;
	};
	parserAut.setTrans("startFractionalNumber",'.',"numberValue").output = startDot;
	parserAut.setTrans("startFractionalNumber",'}',"endAggregate").output = emitNumberAndEndObject;
	parserAut.setTrans("startFractionalNumber",']',"endAggregate").output = emitNumberAndEndArray;

	// numberValue
	rs.setTrans("numberValue",S,"endValue",[this](char) {
		emitValue(ValueType::number);
	});
	parserAut.setTrans("numberValue",',',"startValue").output = emitNumberAndClear;
	parserAut.setTrans("numberValue",'}',"endAggregate").output = emitNumberAndEndObject;
	parserAut.setTrans("numberValue",']',"endAggregate").output = emitNumberAndEndArray;
	rs.setTrans("numberValue","0-9","numberValue",addValueCh);
	parserAut.setTrans("numberValue",'.',"numberValue").output = [this,startDot](char ch) {
		if( numberPart >= NumberPart::afterDot ) throw UnexpectedChar(pos,'.');
		startDot(ch);
	};
	parserAut.setTrans("numberValue",'e',"numberE").output =
	parserAut.setTrans("numberValue",'E',"numberE").output = [this](char ch) {
		if( numberPart >= NumberPart::afterE ) throw UnexpectedChar(pos,ch);
		addValue(ch);
		numberPart = NumberPart::afterE;
	};

	// numberE
	parserAut.setTrans("numberE",'+',"exponentialNumber").output =
	parserAut.setTrans("numberE",'-',"exponentialNumber").output = addValueCh;
	rs.setTrans("numberE","0-9","numberValue",addValueCh);
	// exponentialNumber
	rs.setTrans("exponentialNumber","0-9","numberValue",addValueCh);

	// null
	parserAut.setTrans("nullValue-ull",'u',"nullValue-ll").output =
	parserAut.setTrans("nullValue-ll",'l',"nullValue-l").output = addValueCh;
	parserAut.setTrans("nullValue-l",'l',"endValue").output = addAndEmitKeyword;
	// true
	parserAut.setTrans("trueValue-rue",'r',"trueValue-ue").output =
	parserAut.setTrans("trueValue-ue",'u',"trueValue-e").output = addValueCh;
	parserAut.setTrans("trueValue-e",'e',"endValue").output = addAndEmitKeyword;
	// false
	parserAut.setTrans("falseValue-alse",'a',"falseValue-lse").output =
	parserAut.setTrans("falseValue-lse",'l',"falseValue-se").output =
	parserAut.setTrans("falseValue-se",'s',"falseValue-e").output = addValueCh;
	parserAut.setTrans("falseValue-e",'e',"endValue").output = addAndEmitKeyword;

	// endAggregate
	rs.setTrans("endAggregate",S,"endAggregate");
	parserAut.setTrans("endAggregate",'}',"endAggregate").output = endObject;
	parserAut.setTrans("endAggregate",']',"endAggregate").output = endArray;
	parserAut.setTrans("endAggregate",',',"startValue").output = [this](char) {
		if( context.status == Status::start ) throw UnexpectedChar(pos,',');
		startValue();
	};

	parserAut.setStart("start");
	parserAut.getNode("start").final = true;
	parserAut.getNode("endAggregate").final = true;
}

Parser & Parser::startObject(start_event fn) { startObjectFn = fn; return *this; }
Parser & Parser::endObject(end_event fn) { endObjectFn = fn; return *this; }
Parser & Parser::startArray(start_event fn) { startArrayFn = fn; return *this; }
Parser & Parser::endArray(end_event fn) { endArrayFn = fn; return *this; }
Parser & Parser::value(value_event fn) { valueFn = fn; return *this; }

bool Parser::parse(std::basic_istream<CharType> & is)
{
	Contexts().swap(contexts);  // really? no .clear()? no rvalue swap() either?
	context.status = Status::start;
	pos.column = 1;
	pos.line = 1;

	auto consumer = parserAut.getConsumer();
	auto ch = is.get();
	while( ch != std::basic_istream<CharType>::traits_type::eof() && consumer.consume(ch) ) {
		if( ch == '\n' ) {
			++pos.line;
			pos.column = 1;
		} else {
			++pos.column;
		}
		ch = is.get();
	}
	return consumer.final();
}

// aux dumb functions

void Parser::changeContext(Status newStatus)
{
	contexts.push(context);
	context.status = newStatus;
	if( newStatus != Status::array ) {
		context.objectName.clear();
	}
}

void Parser::popContext(char ch)
{
	if( contexts.empty() ) throw UnexpectedChar(pos,ch);
	context = contexts.top();
	contexts.pop();
}

void Parser::emitValue(ValueType valueType)
{
	valueFn(context.objectName,valueType,valueStr);
}

void Parser::emitAndClear(ValueType valueType)
{
	emitValue(valueType);
	startValue();
}

void Parser::emitAndEnd(ValueType valueType, CharFunc endFn, char ch)
{
	emitValue(valueType);
	endFn(ch);
}

void Parser::startValue()
{
	valueStr.clear();
	if( context.status == Status::object ) {
		isObjectName = true;
	}
}

void Parser::addValue(char ch)
{
	valueStr.push_back(ch);
}

} /* namespace json */
} /* namespace die */
