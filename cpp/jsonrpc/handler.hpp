#include <map>
#include <string>
#include <typeindex>
#include "../value/value.hpp"

std::string json_type( const std::type_info &t );

class RpcFunction {
public:
    virtual core::Value service_def() const = 0;

    virtual core::Value call( const core::cvector &params ) const = 0;
};