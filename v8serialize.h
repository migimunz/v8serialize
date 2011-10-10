/*
 * v8serialize.h
 *
 *  Created on: Oct 8, 2011
 *      Author: migimunz
 */

#ifndef V8SERIALIZE_H_
#define V8SERIALIZE_H_
#include <v8.h>
#include <string>
#include <vector>
#include <list>
#include <map>
#include <exception>

#ifndef V8SERIALIZE_NO_BOOST
///Planning to include v8::Function to boost::function conversion.
#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>
#endif /*V8SERIALIZE_NO_BOOST*/


typedef v8::Handle<v8::Object> ObjectHandle;
typedef v8::Handle<v8::Value> ValueHandle;

/**
 * \class bad_conversion_exception
 * Bad conversion exception
 *
 * Thrown when a value can't be converted to js or from js
 */
class bad_conversion_exception : public std::exception
{
	const char *message;
public:
	bad_conversion_exception()
	:message("Conversion failed")
	{
	}

	bad_conversion_exception(const char *message)
	:message(message)
	{
	}

	virtual const char* what() const throw()
	{
		return message;
	}
};

class load_info;
class save_info;


/**
 * \class convert
 * Class template used to convert values from and to v8 values
 *
 * Every every specialization of this template class defines a conversion from and to a type.
 * Specialized templates are defined within this file and are used to convert primitives and
 * some STL types, as well as shared pointers.
 *
 * User types are converted in the unspecialized template by calling T::load and T::save to
 * convert js->C++ and C++->js respectively.
 */
template<typename T>
struct convert
{
	/**
	 * Converts a v8::Value to T
	 * @param data the v8::Value to convert
	 * @param result
	 */
	void from_json(const ValueHandle &data, T &result)
	{
		load_info info(data);
		T::load(info, result);
	}
	/**
	 * Converts T to a v8::Value
	 * @param value The value to convert
	 * @return v8::Value
	 */
	ValueHandle to_json(const T &value)
	{
		using namespace v8;
		ObjectHandle ret = Object::New();
		save_info data(ret);
		T::save(data, value);
		return ret;
	}
};


/**
 * \class load_info
 * Helper object passed to T::load when performing user-defined conversions
 * The class is a thin wrapper for a value handle, and should olny be used by
 * calling load_info::get.
 *
 * A user doesn't need to create this object, they'll always be passed by reference to T::load
 */
struct load_info
{

	ValueHandle value;
	/**
	 * load_info constructor
	 * @param value a value handle
	 */
	explicit load_info(const ValueHandle &value)
		:value(value) {}

	/**
	 * Converts a property of an object named \c name to T, assigns the result of conversion to \c result,
	 * throws a \c bad_conversion_exception on failure.
	 * @param name name of the property
	 * @param result result of the conversion (assigned to the reference)
	 */
	template<typename T>
	void get(const std::string &name, T &result)
	{
		using namespace v8;
		HandleScope scope;
		TryCatch try_catch;

		ObjectHandle value_obj = ObjectHandle::Cast(value);
		ValueHandle child = value_obj->Get(String::New(name.c_str()));
		if(try_catch.HasCaught() || child->IsUndefined())
			throw bad_conversion_exception();
		convert<T>().from_json(child, result);
	}

	/**
	 * Converts a property of an object named \c name to T, assigns the result of conversion to \c result,
	 * returns a default value \c def on failure.
	 * @param name name of the property
	 * @param result result result of the conversion (assigned to the reference)
	 * @param def default value to return if the conversion fails
	 */
	template<typename T>
	void get(const std::string &name, T &result, const T &def)
	{
		using namespace v8;
		try
		{
			get(name, result);
		}
		catch(bad_conversion_exception &e)
		{
			result = def;
		}
	}

};

/**
 * \class save_info
 * Helper object passed to T::save when performing user-defined conversions
 * The class is a thin wrapper for an object handle, and should olny be used by
 * calling load_info::set.
 *
 * A user doesn't need to create this object, they'll always be passed by reference to T::save
 */
struct save_info
{
	ObjectHandle object;

	/**
	 * save_info constructor
	 * @param object object handle
	 */
	save_info(const ObjectHandle &object)
	:object(object)
	{
	}

	/**
	 * Converts T to a v8::Value and assigns it to property named \c of it's internal v8::Object.
	 * If the conversion fails, /c bad_conversion_exception is thrown
	 * @param name name of the property
	 * @param svalue value to convert
	 */
	template<typename T>
	void set(const std::string &name, const T &svalue)
	{
		using namespace v8;
		HandleScope scope;
		TryCatch try_catch;
		if(object.IsEmpty()) //if handle is not bound to an object
			throw bad_conversion_exception();

		object->Set(String::New(name.c_str()), convert<T>().to_json(svalue));
		if(try_catch.HasCaught())
			throw bad_conversion_exception();
	}
};

/**
 * Serializes a v8::Value to a C++ value using defined conversion methods.
 * @param value value to convert
 * @param result result of the conversion is assigned to this
 */
template<typename T>
void from_json(const ValueHandle &value, T &result)
{
	using namespace v8;
	HandleScope scope;
	TryCatch try_catch;

	convert<T>().from_json(value, result);
	if(try_catch.HasCaught())
		throw bad_conversion_exception();
}

/**
 * Serializes a C++ type to v8::Value using defined conversion methods.
 * @param value C++ value to serialize to v8::Value
 * @return resulting v8::Value
 */
template<typename T>
ValueHandle to_json(const T &value)
{
	using namespace v8;
	HandleScope scope;
	TryCatch try_catch;

	ValueHandle ret = convert<T>().to_json(value);
	if(try_catch.HasCaught())
		throw bad_conversion_exception();
	else
		return scope.Close(ret);
}

//Primitive conversions

/**
 * A specialization of \c convert class template used to convert int16_t to and from v8::Value
 */
template<>
struct convert< int16_t >
{
	void from_json(const ValueHandle &data, int16_t &result)
	{
		if(data->IsNumber())
			result = data->Int32Value();
		else
			throw bad_conversion_exception();
	}
	ValueHandle to_json(const int16_t &value)
	{
		return v8::Integer::New(value);
	}
};
/**
 * A specialization of \c convert class template used to convert uint16_t to and from v8::Value
 */
template<>
struct convert< uint16_t >
{
	void from_json(const ValueHandle &data, uint16_t &result)
	{
		if(data->IsNumber())
			result = data->Uint32Value();
		else
			throw bad_conversion_exception();
	}
	ValueHandle to_json(const uint16_t &value)
	{
		return v8::Integer::NewFromUnsigned(value);
	}
};
/**
 * A specialization of \c convert class template used to convert int32_t to and from v8::Value
 */
template<>
struct convert< int32_t >
{
	void from_json(const ValueHandle &data, int32_t &result)
	{
		if(data->IsNumber())
			result = data->Int32Value();
		else
			throw bad_conversion_exception();
	}
	ValueHandle to_json(const int32_t &value)
	{
		return v8::Integer::New(value);
	}
};
/**
 * A specialization of \c convert class template used to convert uint32_t to and from v8::Value
 */
template<>
struct convert< uint32_t >
{
	void from_json(const ValueHandle &data, uint32_t &result)
	{
		if(data->IsNumber())
			result = data->Uint32Value();
		else
			throw bad_conversion_exception();
	}
	ValueHandle to_json(const uint32_t &value)
	{
		return v8::Integer::NewFromUnsigned(value);
	}
};
/**
 * A specialization of \c convert class template used to convert int64_t to and from v8::Value
 */
template<>
struct convert< int64_t >
{
	void from_json(const ValueHandle &data, int64_t &result)
	{
		if(data->IsNumber())
			result = data->IntegerValue();
		else
			throw bad_conversion_exception();
	}
	ValueHandle to_json(const int64_t &value)
	{
		return v8::Number::New(static_cast<double>(value));
	}
};
/**
 * A specialization of \c convert class template used to convert uint64_t to and from v8::Value
 */
template<>
struct convert< uint64_t >
{
	void from_json(const ValueHandle &data, uint64_t &result)
	{
		if(data->IsNumber())
			result = data->IntegerValue();
		else
			throw bad_conversion_exception();
	}
	ValueHandle to_json(const uint64_t &value)
	{
		return v8::Number::New(static_cast<double>(value));
	}
};
/**
 * A specialization of \c convert class template used to convert double to and from v8::Value
 */
template<>
struct convert< double >
{
	void from_json(const ValueHandle &data, double &result)
	{
		if(data->IsNumber())
			result = data->NumberValue();
		else
			throw bad_conversion_exception();
	}
	ValueHandle to_json(const double &value)
	{
		return v8::Number::New(value);
	}
};
/**
 * A specialization of \c convert class template used to convert float to and from v8::Value
 */
template<>
struct convert< float >
{
	void from_json(const ValueHandle &data, float &result)
	{
		if(data->IsNumber())
			result = data->NumberValue();
		else
			throw bad_conversion_exception();
	}
	ValueHandle to_json(const float &value)
	{
		return v8::Number::New(value);
	}
};
/**
 * A specialization of \c convert class template used to convert bool to and from v8::Value
 */
template<>
struct convert< bool >
{
	void from_json(const ValueHandle &data, bool &result)
	{
		if(data->IsBoolean())
			result = data->BooleanValue();
		else
			throw bad_conversion_exception();
	}
	ValueHandle to_json(const bool &value)
	{
		return v8::Boolean::New(value);
	}
};
/**
 * A specialization of \c convert class template used to convert std::string to and from v8::Value
 */
template<>
struct convert< std::string >
{
	void from_json(const ValueHandle &data, std::string &result)
	{
		using namespace v8;
		if(data->IsString())
		{
			String::Utf8Value utf8_value(Handle<String>::Cast(data));
			result = *utf8_value;
		}
		else
			throw bad_conversion_exception();
	}
	ValueHandle to_json(const std::string &value)
	{
		return v8::String::New(value.c_str());
	}
};
/**
 * A specialization of \c convert class template used to convert std::map<std::string, T> to and from v8::Value.
 * This specializations converts a std::map to and from \c v8::Object.
 */
template<typename T>
struct convert< std::map<std::string, T> >
{
	void from_json(const ValueHandle &data, std::map<std::string, T> &result)
	{
		using namespace v8;
		HandleScope scope;
		TryCatch try_catch;

		Handle<Object> object = Handle<Object>::Cast(data);
		if(object.IsEmpty() || try_catch.HasCaught())
			throw bad_conversion_exception();

		Handle<Array> names = object->GetOwnPropertyNames();

		uint32_t length = names->Length();
		for(uint32_t i = 0; i < length; ++i)
		{
			Handle<String> js_name = Handle<String>::Cast(names->Get(i));
			String::Utf8Value name(js_name);
			std::string first(*name);

			ValueHandle value = object->Get(js_name);
			T second;
			convert<T>().from_json(value, second);
			result[first] = second;
		}
	}
	ValueHandle to_json(const std::map<std::string, T> &value)
	{
		using namespace v8;
		HandleScope scope;
		TryCatch try_catch;

		ObjectHandle object = Object::New();
		typename std::map<std::string, T>::const_iterator iter;
		for(iter = value.begin(); iter != value.end(); iter++)
		{
			ValueHandle ret = convert<T>().to_json(iter->second);
			object->Set(String::New(iter->first.c_str()), ret);
		}
		return scope.Close(object);
	}
};
/**
 * \c convert_list
 * A helper class template used to convert any collection that exposes a forward iterator with
 * \c begin() and \c end() and methods \c size() and \c push_back(). Currently used to convert
 * \c std::vector and \c std::list to and from \c v8::Value
 */
template<typename C, typename T>
struct convert_list
{
	void from_json(const ValueHandle &data, C &result)
	{
		using namespace v8;
		HandleScope scope;
		TryCatch try_catch;

		ValueHandle value = data;
		Handle<Array> array = Handle<Array>::Cast(value);
		if(array.IsEmpty() || try_catch.HasCaught())
			throw bad_conversion_exception();

		uint32_t length = array->Length();
		for(uint32_t i = 0; i < length; ++i)
		{
			T element;
			convert<T>().from_json(array->Get(i), element);
			result.push_back(element);
		}
	}
	ValueHandle to_json(const C &value)
	{
		using namespace v8;
		HandleScope scope;
		TryCatch try_catch;

		Handle<Array> array = Array::New(value.size());
		typename C::const_iterator iter;
		uint32_t i = 0;
		for(iter = value.begin(); iter != value.end(); iter++)
		{
			array->Set(i, convert<T>().to_json(*iter));
			i++;
		}
		return scope.Close(array);
	}
};

/**
 * A specialization of \c convert class template used to convert std::list<T> to and from v8::Value
 */
template<typename T>
struct convert< std::list<T> > : public convert_list< std::list<T>, T >
{

};

/**
 * A specialization of \c convert class template used to convert std::vector<T> to and from v8::Value
 */
template<typename T>
struct convert< std::vector<T> > : public convert_list< std::vector<T>, T >
{

};

#ifndef V8SERIALIZE_NO_BOOST

/**
 * A specialization of \c convert class template used to convert a \c boost::shared_ptr<T> to and from a \c v8::Value .
 * When deserializing (converting from js to C++ types), an object of type T will be allocated using \c new , and assigned to
 * \c result.
 */
template<typename T>
struct convert< boost::shared_ptr<T> >
{
	void from_json(const ValueHandle &data, boost::shared_ptr<T> &result)
	{
		using boost::shared_ptr;
		result = shared_ptr<T>(new T());
		convert<T>().from_json(data, *result);
	}
	ValueHandle to_json(const boost::shared_ptr<T> &value)
	{
		return convert<T>().to_json(*value);
	}
};

#endif /*V8SERIALIZE_NO_BOOST*/

#endif /* V8SERIALIZE_H_ */
