#ifndef JSONPARSEREXCEPTIONS_H_DIE_JSON_2015_06_04
#define JSONPARSEREXCEPTIONS_H_DIE_JSON_2015_06_04

#include <stdexcept>
#include <string>
#include "JsonParserObjects.h"

namespace die {
namespace json {

class Exception: public std::invalid_argument {
	char ch;
public:
	Exception(std::string const & message, char ch):
		std::invalid_argument(message),
		ch(ch)
	{}
	char offendingChar() const { return ch; }
};

class ExpectedChar: public Exception {
public:
	ExpectedChar(Position pos, char expected, char ch);
};

class UnexpectedChar: public Exception {
public:
	UnexpectedChar(Position pos, char ch);
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
