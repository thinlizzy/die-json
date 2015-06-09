#ifndef JSONPARSEREXCEPTIONS_H_DIE_JSON_2015_06_04
#define JSONPARSEREXCEPTIONS_H_DIE_JSON_2015_06_04

#include <stdexcept>
#include <string>
#include "JsonParserObjects.h"

namespace die {
namespace json {

class Exception: public std::invalid_argument {
public:
	using std::invalid_argument::invalid_argument;
};

class UnexpectedChar: public Exception {
	char ch;
public:
	UnexpectedChar(std::string const & message, char ch);
	UnexpectedChar(Position pos, char ch);
	char offendingChar() const { return ch; }
};

class ExpectedChar: public UnexpectedChar {
public:
	ExpectedChar(Position pos, char expected, char ch);
};

class UnexpectedObjectClosing: public UnexpectedChar {
public:
	UnexpectedObjectClosing(Position pos): UnexpectedChar(pos,'}') {}
};

class UnexpectedArrayClosing: public UnexpectedChar {
public:
	UnexpectedArrayClosing(Position pos): UnexpectedChar(pos,']') {}
};

} /* namespace json */
} /* namespace die */

#endif /* JSONPARSEREXCEPTIONS_H_ */
