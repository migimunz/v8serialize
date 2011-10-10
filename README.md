v8serialize (aka v8s)
=====================

v8serialize is a header-only library for easy conversion between 
C++ and JavaScript types. The primary goal of the library is to use it as
a method of serializing C++ data to json and back using v8.

Building and dependencies 
-------------------------

v8s is header only so there's nothing to build. Just download, include and use.
v8s depends on STL and some boost libraries (just `boost::shared_ptr` at 
the moment, boost::function might be added later). boost functionality is 
optional, define `V8SERIALIZE_NO_BOOST` to use v8s without it.

Supported compilers
-------------------

So far v8s has only been tested on g++ 4.5.2, but it should work without
problems with any sane compiler.

Converting types
----------------


Converting types from JavaScript to C++ is done using `from_js`, from C++ 
to JavaScript using `to_js`. v8s can convert the following types out of 
the box: 
 
* `int16_t`, `uint16_t`
* `int32_t`, `uint32_t`
* `int64_t`, `uint64_t`
* `double`
* `float`
* `std::string` (const char* conversion is not supported)
* `std::vector<T>` and `std::list<T>` if T is convertible
* `std::map<std::string, T>` if T is convertible
* `boost::shared_ptr<T>` if T is convertible *(only if* `V8SERIALIZE_NO_BOOST` *is not defined)*

The library exposes two functions for conversion : `v8s::from_js` and `v8s::to_js`. 
All v8serialize functions and classes are members of `namespace v8s`, but it will 
be omitted in the examples for readability. 

Examples
--------

    Handle<Value> js_value; //value to convert to C++
    uint32_t cpp_value; //value to convert to
    
    from_js(js_value, cpp_value);

This will either convert js_value to cpp_value or throw a `bad_conversion_exception`
if js_value can't be converted to a uint32_t. To convert from
C++ types to JavaScript, use `to_js`. Note that `to_js` returns a 
`v8::Handle<Value>`, whereas `from_js` requires you to pass a C++ value 
to be filled by reference.

    //An example of converting a std::string to a JavaScript string :
    std::string cpp_str = "Yay!";
    Handle<Value> js_str = to_js(cpp_str);

A more advanced example of out-of-the-box conversion would be converting
nested structures. An object with lists defined in json :

    {
    "even":  [2, 4, 6, 8, 10],
    "odd":   [1, 3, 5, 7, 9],
    }

Can be converted to C++ like this :

    Handle<Value> js_numbers = json_from_file("example.json");
    std::map<std::string, std::vector<int_32> > cpp_numbers;
    from_js(js_numbers, cpp_numbers);
    
    int_32 n = cpp_numbers["odd"][3];
    //n = 5

To convert user types to and from JavaScript, the user type `T` must expose 
static methods `void load(load_info &data, T &o)` and 
`void save(save_info &data, const T &o)`. `load` will be called when 
converting from JavaScript to C++, `save` when converting from C++ to 
JavaScript. Take a look at these two structs :

    struct screen
    {
		int width, height;
		bool fullscreen;
		int bpp;
		
		static void load(v8s::load_info &data, screen &o)
		static void save(v8s::save_info &data, const screen &o);
	};

	struct config
	{
		screen screen_settings;
		std::string window_name;
		
		static void load(v8s::load_info &data, config &o)
		static void save(v8s::save_info &data, const config &o);		
	};


Since they aren't convertible out of the box, we will have to provide
static `save` and `load` methods. 

	void screen::load(v8s::load_info &data, screen &o)
	{
		data.get("width", o.width, 800);
		data.get("height", o.height, 600);
		data.get("bpp", o.bpp, 32);
		data.get("fullscreen", o.fullscreen, false);
	}
	void screen::save(v8s::save_info &data, const screen &o)
	{
		data.set("width", o.width);
		data.set("height", o.height);
		data.set("bpp", o.bpp);
		data.set("fullscreen", o.fullscreen);
	}    
    
And for `config` :

	void load(v8s::load_info &data, config &o)
	{
		data.get("screen", o.screen_settings);
		data.get("windowName", o.window_name, std::string("Generic window"));
	}
	void save(v8s::save_info &data, const config &o)
	{
		data.set("screen", o.screen_settings);
		data.set("windowName", o.window_name);
	}

Converting them from js to C++ and back :

	boost::shared_ptr<config> config;
	Handle<Value> json = json_file_parse("config_in.json");
	from_js(json, config);

	Handle<Value> out = to_js(config);
	json_to_file(out, "config_out.json");    

The `load_info::get` method can be called with 3 arguments, the third one 
being the default value. In `config::load`, property "windowName" is being
converted and written into `o.window_name`. If the conversion fails, either
because "windowName" is not a string or is simply not defined, value of 
`o.window_name` will be set to the default value "Generic window".

When converting from a JavaScript type to `boost::shared_ptr<T>`, if 
there is a valid conversion from that type to T, a value of type T will be
created using `new` and converted as it normally would. 
