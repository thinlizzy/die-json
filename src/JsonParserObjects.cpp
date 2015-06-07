#include "JsonParserObjects.h"

std::ostream & operator<<(std::ostream & os, die::json::Position const & pos)
{
	return os << "(line " << pos.line << ", column " << pos.column << ')';
}

std::ostream & operator<<(std::ostream & os, die::json::ValueType valueType)
{
	switch(valueType) {
		case die::json::ValueType::null:    return os << "null";
		case die::json::ValueType::string:  return os << "string";
		case die::json::ValueType::number:  return os << "number";
		case die::json::ValueType::boolean: return os << "boolean";
	}
	return os;
}
