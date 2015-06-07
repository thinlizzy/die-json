#ifndef JSONPARSEROBJECTS_H_DIE_JSON_2015_06_06
#define JSONPARSEROBJECTS_H_DIE_JSON_2015_06_06

#include <ostream>

namespace die {
namespace json {

struct Position {
	int column,line;
};

enum class ValueType { null, string, number, boolean, };

} /* namespace json */
} /* namespace die */

std::ostream & operator<<(std::ostream & os, die::json::Position const & pos);
std::ostream & operator<<(std::ostream & os, die::json::ValueType valueType);

#endif /* JSONPARSEROBJECTS_H_ */
