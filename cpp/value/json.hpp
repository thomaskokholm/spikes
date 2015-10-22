#ifndef __JSON_HPP__
#define __JSON_HPP__ 1

#include "variant.hpp"
#include <istream>
#include <ostream>

namespace nbpp {
	Variant json_parse( istream &is );
	ostream &json_serialize( ostream &os, const Variant &val );
}

#endif
