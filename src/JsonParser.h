#ifndef JSONPARSER_H_DIE_JSON_2015_06_03
#define JSONPARSER_H_DIE_JSON_2015_06_03

#include <functional>
#include <string>
#include <stack>
#include <istream>
#include "automata/automata.h"
#include "JsonParserObjects.h"

namespace die {
namespace json {

template<typename CharType>
struct CharTraits {
	template<typename U>
	static std::basic_string<CharType> parseUnicode(std::basic_string<U> const & strCode) {
		// default is just to passthrough the encoded char for the client to encode
		return "\\u" + strCode;
	}
};

class Parser {
public:
	using CharType = char; // TODO template it. type needs to be convertible from char
	using ObjectName = std::basic_string<CharType>;
	using ValueStr = std::basic_string<CharType>;

	using start_event = std::function<void(ObjectName const &)>;
	using end_event = std::function<void(ObjectName const &)>;
	using value_event = std::function<void(ObjectName const &, ValueType, ValueStr const &)>;
private:
	using Automata = automata::FiniteAutomata<automata::Range<CharType>,automata::MealyTransition<automata::Range<CharType>>>;
	enum class Status { start, object, array, };
	enum class NumberPart { beforeDot, afterDot, afterE, };
	struct Context {
		Status status;
		ValueStr objectName;
	};
	using Contexts = std::stack<Context>;

	Contexts contexts;
	Context context;

	bool isObjectName;
	ObjectName objectName;
	ValueStr valueStr;
	ValueStr unicodeStr;
	ValueType keywordType;
	NumberPart numberPart;

	Automata parserAut;

	Position pos;

	start_event startObjectFn;
	end_event endObjectFn;
	start_event startArrayFn;
	end_event endArrayFn;
	value_event valueFn;
public:
	Parser();
	Parser & startObject(start_event fn);
	Parser & endObject(end_event fn);
	Parser & startArray(start_event fn);
	Parser & endArray(end_event fn);
	Parser & value(value_event fn);

	bool parse(std::basic_istream<CharType> & is);

	Position lastPosition() const { return pos; }
private:
	// aux dumb functions to avoid strange captures on local lambdas
	using CharFunc = std::function<void(char)>;
	void changeContext(Status status);
	void popContext(char ch);
	void emitValue(ValueType valueType);
	void emitAndClear(ValueType valueType);
	void emitAndEnd(ValueType valueType, CharFunc endFn, char ch);
	void startValue();
	void addValue(char ch);
};

} /* namespace json */
} /* namespace die */

#endif /* JSONPARSER_H_ */
