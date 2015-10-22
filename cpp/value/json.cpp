/**
    JSON parser and serialized, made for using nb++ Variant as dynamic
    type container.
    
    JSON: RFC-4627
*/

#include "variant.hpp"

#include <assert.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <istream>
#include <sstream>
#include <iostream>

using namespace std;
using namespace nbpp;

// convert a unicode char to a utf8 char, as this is what we use internal
static string utf8_decode( unsigned c )
{
    string res;
    if (c <= 0x7F) { /* 0XXX XXXX one byte */
        res.push_back((char) c);
    } else if (c <= 0x7FF) { /* 110X XXXX  two bytes */
        res.push_back((char)( 0xC0 | (c >> 6) ));
        res.push_back((char)( 0x80 | (c & 0x3F)));
    } else if (c <= 0xFFFF) { /* 1110 XXXX  three bytes */
        res.push_back((char)(0xE0 | (c >> 12)));
        res.push_back((char)(0x80 | ((c >> 6) & 0x3F)));
        res.push_back((char)(0x80 | (c & 0x3F)));
     } else if (c <= 0x1FFFFF) { /* 1111 0XXX  four bytes */
        res.push_back((char)(0xF0 | (c >> 18)));
        res.push_back((char)(0x80 | ((c >> 12) & 0x3F)));
        res.push_back((char)(0x80 | ((c >> 6) & 0x3F)));
        res.push_back((char)(0x80 | (c & 0x3F)));
     }
     
     return res; 
}

class Tokenizer {
public:
    enum ttype {
        typeEof,
        typeError,
        typeDelim,
        typeSymbol,
        typeString,
        typeNumber
    };
    
    Tokenizer(istream &is) : _is( is ) {
        next();
    }

    ttype type() const {return _type;}
    
    ttype next();

    string val() const {return _val;}

    string str_pos() const;
private:
    istream &_is;
    string _val;
    enum ttype _type;
    int _line, _col;
};

Tokenizer::ttype Tokenizer::next()
{
    _val.clear();
        
    int ch;
    
    while(((ch = _is.get()) != -1) && isspace( ch )) {
        if( ch == '\n' )
            _line++;
    }

    if( ! _is )
        return _type = typeEof;
    
    if( strchr( "[]{},:", ch )) {
        _type = typeDelim;
        _val.push_back( (char)ch );
    } else if( ch == '"' ) {
        _type = typeString;
        
        while ((ch = _is.get()) != '"' && _is ) {
            if ( ch == '\\') {
                ch = _is.get();
                switch (ch) {
                case '"':
                case '\\':
                case '/':
                case 'b':
                case 'f':
                case 'n':
                case 'r':
                case 't':
                    ch = _is.get();
                    break;
                case 'u': {
                    string num;

                    while ( isxdigit(ch = _is.get()) && num.length() < 4 && _is )
                        num += (char)ch;
                    
                    if (num.length() < 4)
                        throw invalid_argument( "hex unicode must contain 4 digits. not just " + num );
                    else
                        _val += utf8_decode( atoi( num.c_str()));
                    }
                    break;

                default:
                    throw invalid_argument("unknown escape character after backslash: " + ch);
                }
            } else
                _val += (char)ch;
        }
    } else if( isdigit( ch ) || strchr( ".-", ch )) {
        _val.push_back( ch );
        
        while((ch = _is.get()) != -1 && _is ) {
            if( isdigit( ch ) || ch == '.' )
                _val.push_back( ch );
            else {
                _is.unget();
                break;
            }
        }
        _type = typeNumber;
    } else if( isalpha( ch )) { // null, undefined, true, false
        _val.push_back( ch );
        
        while((ch = _is.get()) != -1 && _is) {
            if( isalpha( ch ))
                _val.push_back( ch );
            else {
                _is.unget();
                break;
            }
        }
        _type = typeSymbol;
    } else
        _type = typeError;
    
    return _type;
}

string Tokenizer::str_pos() const
{
    stringstream os;

    os << "line : " << _line << ", col " << _col;

    return os.str();
}

static Variant json_parse_value( Tokenizer &tok );

static Variant json_parse_list( Tokenizer &tok )
{
    cvector res;
    
    if( tok.type() == Tokenizer::typeDelim && tok.val() == "[" ) {
        while( Tokenizer::typeEof != tok.next()) {
            if( tok.type() == Tokenizer::typeDelim && tok.val() == "]" )
                break;

            res.push_back( json_parse_value( tok ));

            if( Tokenizer::typeEof == tok.next() )
                throw invalid_argument( "bad list termination as " + tok.str_pos());

            if( tok.type() != Tokenizer::typeDelim && tok.val() != "," )
                break;

            if( tok.type() == Tokenizer::typeDelim && tok.val() == "]" )
                break;
        }
    }

    return Variant( res );
}

static Variant json_parse_object( Tokenizer &tok )
{
    cmap res;
    
    if( tok.type() == Tokenizer::typeDelim && tok.val() == "{" ) {       
        while( Tokenizer::typeEof != tok.next() ) {
            if( tok.type() == Tokenizer::typeDelim && tok.val() == "}" )
                break;

            if( tok.type() == Tokenizer::typeString ) {
                string key = tok.val();
                
                if( Tokenizer::typeDelim == tok.next() ) {
                    if( tok.val() == ":" ) {
                        tok.next();
                        
                        res[ key ] = json_parse_value( tok );
                        
                        if( Tokenizer::typeDelim != tok.next() && tok.val() != "," )
                            break;

                        if( tok.type() == Tokenizer::typeDelim && tok.val() == "}" )
                            break;
                    } else
                        throw invalid_argument( "syntax error in key value separator in object at " + tok.str_pos());
                } else
                    throw invalid_argument( "syntax error in object at " + tok.str_pos());
                
                if( tok.type() == Tokenizer::typeEof )
                    throw invalid_argument( "premature object termination" );
            } else
                throw invalid_argument( "must have a string as key in object at " + tok.str_pos());
        }
    }

    return Variant( res );
}

static Variant json_parse_value( Tokenizer &tok )
{
    Variant ret;
    
    switch( tok.type() ) {
    case Tokenizer::typeDelim:
        if( tok.val() == "[" )
            ret = json_parse_list( tok );
        else if( tok.val() == "{" )
            ret = json_parse_object( tok );
        else
            throw invalid_argument( "syntax error, unknown value construct at " + tok.str_pos() );
        break;
            
    case Tokenizer::typeSymbol:
        if( "null" == tok.val() || "undefined" == tok.val())
			;
        else if( tok.val() == "false" )
            ret = Variant( false );
        else if( tok.val() == "true" )
            ret = Variant( true );
        else
            throw invalid_argument( "unknown symbol at " + tok.str_pos() );
        break;

    case Tokenizer::typeNumber: {
        char *end = NULL;
        const char *p = tok.val().c_str();
        double v = strtod( p, &end );

        ret = Variant( v );
        break;
    }
    case Tokenizer::typeString:
        ret = Variant( tok.val());
        break;
            
    case Tokenizer::typeEof:
    default:
        break;
    }

    return ret;
}

// Serialize json into stream

static ostream &indent( ostream &os, int level )
{
	while( level-- ) 
		os << " ";
		
	return os;
}

/* Forward declaration for recursion */

static void json_escape_char(ostream &os, unsigned char const c) 
{
    char slashChar;
        /* Character that goes after the backslash, including 'u' for \uHHHH */
    
    switch (c) {
		case '"' : slashChar = '"';  break; /* U+0022 */
		case '\\': slashChar = '\\'; break; /* U+005C */
		case '\b': slashChar = 'b';  break; /* U+0008 */
		case '\f': slashChar = 'f';  break; /* U+000C */
		case '\n': slashChar = 'n';  break; /* U+000A */
		case '\r': slashChar = 'r';  break; /* U+000D */
		case '\t': slashChar = 't';  break; /* U+0009 */
		default:
			slashChar = 'u';
    };

    os << '\\' << slashChar;
    
    if (slashChar == 'u') {
		char hex[ 5 ];
		
        snprintf( hex, 4, "%04x", c);
        os << hex;
    }
}

// take care of string escaping
static void serialize_json_string(ostream &os, const string &str ) 
{
    string::const_iterator cur = str.begin();
    
    while (cur != str.end() ) {
        unsigned char const c = *cur++;

        if (c < 0x1F || c == '"' || c == '\\')
			json_escape_char(os, c);
		else
			os << c;
    }
}
static void serialize_value( ostream &os, const Variant &var, int level = 0 );

static void serialize_array( ostream &os, const cvector &v, int level = 0 ) 
{
	os << "[";
	
    for( auto i = v.begin(); i != v.end(); i++ ) {
		if( i != v.begin())
			os << ",";
			
        serialize_value( os, *i, level + 1 );
	}
	
    os << "]";
} 

static void serialize_object( ostream &os, const cmap &v, int level = 0) 
{
	os << "{" << endl;
        
    for( auto i = v.begin(); i != v.end(); i++ ) {
		if( i != v.begin()) 
			os << "," << endl;
			
		indent( os, level + 1 ) << "\"";
		serialize_json_string( os, i->first );
		os << "\":";
		serialize_value( os, i->second, level + 1 );
	}
	os << endl;
	indent( os, level ) << "}";
}

static void serialize_value( ostream &os, const Variant &var, int level )
{
	if( var.is_null())
		os << "null";
	else if( var.is_type<string>()) {
		os << "\"";
		serialize_json_string(os, var.get<string>());
		os << "\"";
	} else if( var.is_type<int>() || var.is_type<unsigned>() || var.is_type<long>() || var.is_type<long long>())
		os << var;
	else if( var.is_type<float>() || var.is_type<double>())
		os << var;
	else if( var.is_type<bool>())
		os << (var.get<bool>() == true ? "true" : "false");
	else if( var.is_type<cmap>())
		serialize_object( os, var.get<cmap>(), level);
	else if( var.is_type<cvector>())
		serialize_array( os, var.get<cvector>(), level);
	else {
		stringstream ss;
		
		ss << var;
		os << "\"";
		serialize_json_string( os, ss.str());
		os << "\"";
	}
}

namespace nbpp {
	ostream &json_serialize( ostream &os, const Variant &var ) 
	{
		serialize_value( os, var );
		return os;
	}

	Variant json_parse( istream &is )
	{
		Tokenizer tok( is );

		return json_parse_value( tok );
	}
}
