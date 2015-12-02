#include "service.hpp"
#include "../value/value.hpp"

using namespace std;
using namespace core;

static map<type_index, string> json_types {
    {typeid( int ), "number"},
    {typeid( float ), "number"},
    {typeid( double ), "number"},
    {typeid( string ), "string"},
    {typeid( bool ), "boolean"},
    {typeid( void ), "undefined"},
    {typeid( cvector ), "array"},
    {typeid( cmap ), "object"}
};

string Service::json_type( const type_info &t )
{
    return json_types[ t ];
}