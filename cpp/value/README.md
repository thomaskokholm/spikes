# Dynamic value system

Define a dynamic value system that uses modern C++, as much of the rtti as
possible and as C++ natural to use as I can imaging.

On of the main reasons for using dynamic values in C++ are integrations to
a more dynamic world, like the web where Javascript have made JSON the
defacto way of exchanging data.

## Example

Short demo of the syntax used for conversion, between Value and C++ types.

```cpp
Value s("string value");
Value pi( 3.1415 );

if( pi.is_type<float>()) 
    float c_pi = pi.get<float>();

```

We also have complex values like cmap and cvector, and these can again be in a Value.

```cpp
Value m( cmap{
    {"str_val", Value( "some string value")},
    {"n_val", Value( 42 )}
});

if( m.is_type<cmap>()) {
    Value s = m.get<cmap>()[ "str_val" ];

    string c_s = s.get<string>();
}
```
