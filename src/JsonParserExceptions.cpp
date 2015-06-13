#include "JsonParserExceptions.h"
#include <sstream>

namespace die {
namespace json {

using namespace std::string_literals;

std::string posToStr(die::json::Position const & pos)
{
	std::ostringstream oss;
	oss << pos;
	return oss.str();
}

UnexpectedChar::UnexpectedChar(std::string const & message, char ch):
	Exception(message),
	ch(ch)
{}

UnexpectedChar::UnexpectedChar(Position pos, char ch):
	Exception(posToStr(pos) + " unexpected char "s + ch),
	ch(ch)
{}

ExpectedChar::ExpectedChar(Position pos, char expected, char ch):
	UnexpectedChar(posToStr(pos) + " expected char "s + expected + " but it was "s + ch, ch)
{}

EmptyObjectName::EmptyObjectName(Position pos):
	UnexpectedChar(posToStr(pos) + " empty object name"s, '"')
{

}

} /* namespace json */
} /* namespace die */
