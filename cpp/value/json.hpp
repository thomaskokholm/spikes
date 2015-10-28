#ifndef __JSON_HPP__
#define __JSON_HPP__ 1

#include "value.hpp"
#include <istream>
#include <ostream>

namespace Core {
    using namespace std;
    
	Value json_parse( istream &is );
	ostream &json_serialize( ostream &os, const Value &val );
}

#endif
