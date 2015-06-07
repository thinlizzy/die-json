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

ExpectedChar::ExpectedChar(Position pos, char expected, char ch):
	Exception(posToStr(pos) + " expected char "s + expected + " but it was "s + ch, ch)
{}

UnexpectedChar::UnexpectedChar(Position pos, char ch):
	Exception(posToStr(pos) + " unexpected char "s + ch, ch)
{}

} /* namespace json */
} /* namespace die */
