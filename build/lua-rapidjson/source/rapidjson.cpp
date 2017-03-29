#include <limits>
#include <cstdio>
#include <cmath>
#include <vector>
#include <algorithm>
#include <string>

#include "lua.hpp"

#include "i64lib.h"

// __SSE2__ and __SSE4_2__ are recognized by gcc, clang, and the Intel compiler.
// We use -march=native with gmake to enable -msse2 and -msse4.2, if supported.
#if defined(__SSE4_2__)
#  define RAPIDJSON_SSE42
#elif defined(__SSE2__)
#  define RAPIDJSON_SSE2
#endif

#include "rapidjson/document.h"
#include "rapidjson/encodedstream.h"
#include "rapidjson/encodings.h"
#include "rapidjson/error/en.h"
#include "rapidjson/error/error.h"
#include "rapidjson/filereadstream.h"
#include "rapidjson/filewritestream.h"
#include "rapidjson/rapidjson.h"
#include "rapidjson/reader.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"
#include "rapidjson/prettywriter.h"

using namespace rapidjson;

#ifndef LUA_RAPIDJSON_VERSION
#define LUA_RAPIDJSON_VERSION "scm"
#endif

static const char* JSON_TABLE_TYPE_FIELD = "__jsontype";
enum json_table_type {
	JSON_TABLE_TYPE_OBJECT = 0,
	JSON_TABLE_TYPE_ARRAY = 1,
	JSON_TABLE_TYPE_MAX
};

static const char* JSON_TABLE_TYPE_NAMES[JSON_TABLE_TYPE_MAX] = { "object", "array" };
static const char* JSON_TABLE_TYPE_METAS[JSON_TABLE_TYPE_MAX] = { "json.object", "json.array" };


static void setfuncs(lua_State* L, const luaL_Reg *funcs)
{
#if LUA_VERSION_NUM >= 502 // LUA 5.2 or above
	luaL_setfuncs(L, funcs, 0);
#else
	luaL_register(L, NULL, funcs);
#endif
}


#if LUA_VERSION_NUM < 502
#define lua_rawlen   lua_objlen
#endif



static FILE* openForRead(const char* filename)
{
	FILE* fp = NULL;
#if WIN32
	fopen_s(&fp, filename, "rb");
#else
	fp = fopen(filename, "r");
#endif

	return fp;
}

static FILE* openForWrite(const char* filename)
{
	FILE* fp = NULL;
#if WIN32
	fopen_s(&fp, filename, "wb");
#else
	fp = fopen(filename, "w");
#endif

	return fp;
}


static int null = LUA_NOREF;

/**
* Returns json.null.
*/
static int json_null(lua_State* L)
{
	lua_rawgeti(L, LUA_REGISTRYINDEX, null);
	return 1;
}

static void createSharedMeta(lua_State* L, json_table_type type)
{
	luaL_newmetatable(L, JSON_TABLE_TYPE_METAS[type]); // [meta]
	lua_pushstring(L, JSON_TABLE_TYPE_NAMES[type]); // [meta, name]
	lua_setfield(L, -2, JSON_TABLE_TYPE_FIELD); // [meta]
	lua_pop(L, 1); // []
}

static int makeTableType(lua_State* L, int idx, json_table_type type)
{
	bool isnoarg = lua_isnoneornil(L, idx);
	bool istable = lua_istable(L, idx);
	if (!isnoarg && !istable)
		return luaL_argerror(L, idx, "optional table excepted");

	if (isnoarg)
		lua_createtable(L, 0, 0); // [table]
	else // is table.
	{
		lua_pushvalue(L, idx); // [table]
		if (lua_getmetatable(L, -1))
		{
			// already have a metatable, just set the __jsontype field.
			// [table, meta]
			lua_pushstring(L, JSON_TABLE_TYPE_NAMES[type]); // [table, meta, name]
			lua_setfield(L, -2, JSON_TABLE_TYPE_FIELD); // [table, meta]
			lua_pop(L, 1); // [table]
			return 1;
		}
		// else fall through
	}

	// Now we have a table without meta table
	luaL_getmetatable(L, JSON_TABLE_TYPE_METAS[type]); // [table, meta]
	lua_setmetatable(L, -2); // [table]
	return 1;
}

static int json_object(lua_State* L)
{
	return makeTableType(L, 1, JSON_TABLE_TYPE_OBJECT);
}

static int json_array(lua_State* L)
{
	return makeTableType(L, 1, JSON_TABLE_TYPE_ARRAY);
}


struct Ctx {
	Ctx() : fn_(&topFn){}
	Ctx(const Ctx& rhs) : table_(rhs.table_), index(rhs.index), fn_(rhs.fn_)
	{
	}
	const Ctx& operator=(const Ctx& rhs){
		if (this != &rhs) {
			table_ = rhs.table_;
			index = rhs.index;
			fn_ = rhs.fn_;
		}
		return *this;
	}
	static Ctx Object(int table) {
		return Ctx(table, &objectFn);
	}
	static Ctx Array(int table)
	{
		return Ctx(table, &arrayFn);
	}
	void submit(lua_State* L)
	{
		fn_(L, this);
	}
private:
	Ctx(int table, void(*f)(lua_State* L, Ctx* ctx)) : table_(table), index(1), fn_(f) {}

	int table_;
	int index;
	void(*fn_)(lua_State* L, Ctx* ctx);

	static void objectFn(lua_State* L, Ctx* ctx)
	{
		lua_rawset(L, ctx->table_);
	}

	static void arrayFn(lua_State* L, Ctx* ctx)
	{
		lua_rawseti(L, ctx->table_, ctx->index++);
	}
	static void topFn(lua_State* L, Ctx* ctx)
	{
	}
};

struct ToLuaHandler {
	ToLuaHandler(lua_State* aL) : L(aL) { stack_.reserve(32); }

	bool Null() {
		json_null(L);
		current_.submit(L);
		return true;
	}
	bool Bool(bool b) {
		lua_pushboolean(L, b);
		current_.submit(L);
		return true;
	}
	bool Int(int i) {
		lua_pushinteger(L, i);
		current_.submit(L);
		return true;
	}
	bool Uint(unsigned u) {
		if (u <= static_cast<unsigned>(std::numeric_limits<lua_Integer>::max()))
			lua_pushinteger(L, static_cast<lua_Integer>(u));
		else
			lua_pushnumber(L, static_cast<lua_Number>(u));
		current_.submit(L);
		return true;
	}
	bool Int64(int64_t i) {
		lua_pushint64(L, i);
		current_.submit(L);
		return true;
	}
	bool Uint64(uint64_t u) {
		lua_pushuint64(L, u);
		current_.submit(L);
		return true;
	}
	bool Double(double d) {
		lua_pushnumber(L, static_cast<lua_Number>(d));
		current_.submit(L);
		return true;
	}
	bool String(const char* str, SizeType length, bool copy) {
		lua_pushlstring(L, str, length);
		current_.submit(L);
		return true;
	}
	bool StartObject() {
		lua_createtable(L, 0, 0); // [..., object]

		// mark as object.
		luaL_getmetatable(L, "json.object");  //[..., object, json.object]
		lua_setmetatable(L, -2); // [..., object]

		stack_.push_back(current_);
		current_ = Ctx::Object(lua_gettop(L));
		return true;
	}
	bool Key(const char* str, SizeType length, bool copy) {
		lua_pushlstring(L, str, length);
		return true;
	}
	bool EndObject(SizeType memberCount) {
		current_ = stack_.back();
		stack_.pop_back();
		current_.submit(L);
		return true;
	}
	bool StartArray() {
		lua_createtable(L, 0, 0);

		// mark as array.
		luaL_getmetatable(L, "json.array");  //[..., array, json.array]
		lua_setmetatable(L, -2); // [..., array]

		stack_.push_back(current_);
		current_ = Ctx::Array(lua_gettop(L));
		return true;
	}
	bool EndArray(SizeType elementCount) {
		current_ = stack_.back();
		stack_.pop_back();
		current_.submit(L);
		return true;
	}
private:
	lua_State* L;
	std::vector < Ctx > stack_;
	Ctx current_;
};

template<typename Stream>
inline int decode(lua_State* L, Stream* s)
{
	int top = lua_gettop(L);
	ToLuaHandler handler(L);
	Reader reader;
	ParseResult r = reader.Parse(*s, handler);

	if (!r) {
		lua_settop(L, top);
		lua_pushnil(L);
		lua_pushfstring(L, "%s (%d)", GetParseError_En(r.Code()), r.Offset());
		return 2;
	}

	return 1;
}

static int json_decode(lua_State* L)
{
	size_t len = 0;
	const char* contents = luaL_checklstring(L, 1, &len);
	StringStream s(contents);
	return decode(L, &s);
}



static int json_load(lua_State* L)
{
	const char* filename = luaL_checklstring(L, 1, NULL);
	FILE* fp = openForRead(filename);
	if (fp == NULL)
		luaL_error(L, "error while open file: %s", filename);

	static const size_t BufferSize = 16 * 1024;
	std::vector<char> readBuffer(BufferSize);
	FileReadStream fs(fp, &readBuffer.front(), BufferSize);
	AutoUTFInputStream<unsigned, FileReadStream> eis(fs);

	int n = decode(L, &eis);

	fclose(fp);
	return n;
}

struct Key
{
	Key(const char* k, SizeType l) : key(k), size(l) {}
	bool operator<(const Key& rhs) const {
		return strcmp(key, rhs.key) < 0;
	}
	const char* key;
	SizeType size;
};




class Encoder {
	bool pretty;
	bool sort_keys;
	int max_depth;
	static const int MAX_DEPTH_DEFAULT = 128;
public:
	Encoder(lua_State*L, int opt) : pretty(false), sort_keys(false), max_depth(MAX_DEPTH_DEFAULT)
	{
		if (lua_isnoneornil(L, opt))
			return;
		luaL_checktype(L, opt, LUA_TTABLE);

		pretty = optBooleanField(L, opt, "pretty", false);
		sort_keys = optBooleanField(L, opt, "sort_keys", false);
		max_depth = optIntegerField(L, opt, "max_depth", MAX_DEPTH_DEFAULT);
	}

private:
	bool optBooleanField(lua_State* L, int idx, const char* name, bool def)
	{
		bool v = def;
		lua_getfield(L, idx, name);  // [field]
		if (!lua_isnoneornil(L, -1))
			v = lua_toboolean(L, -1) != 0;;
		lua_pop(L, 1);
		return v;
	}
	int optIntegerField(lua_State* L, int idx, const char* name, int def)
	{
		int v = def;
		lua_getfield(L, idx, name);  // [field]
		if (lua_isnumber(L, -1))
			v = lua_tointeger(L, -1);
		lua_pop(L, 1);
		return v;
	}
	static bool isJsonNull(lua_State* L, int idx)
	{
		lua_pushvalue(L, idx); // [value]

		json_null(L); // [value, json.null]

		bool is = lua_rawequal(L, -1, -2) != 0;

		lua_pop(L, 2); // []

		return is;
	}

	static bool isInteger(lua_State* L, int idx, int64_t* out)
	{
#if LUA_VERSION_NUM >= 503
		if (lua_isinteger(L, idx)) // but it maybe not detect all integers.
		{
			*out = lua_tointeger(L, idx);
			return true;
		}
#endif
		double intpart;
		if (modf(lua_tonumber(L, idx), &intpart) == 0.0)
		{
			if (std::numeric_limits<lua_Integer>::min() <= intpart
			&& intpart <= std::numeric_limits<lua_Integer>::max())
			{
				*out = (int64_t)intpart;
				return true;
			}
		}
		return false;
	}


	static bool hasJsonType(lua_State* L, int idx, bool& isarray)
	{
		bool has = false;
		if (lua_getmetatable(L, idx)){
			// [metatable]
			lua_getfield(L, -1, JSON_TABLE_TYPE_FIELD); // [metatable, metatable.__jsontype]
			if (lua_isstring(L, -1))
			{
				size_t len;
				const char* s = lua_tolstring(L, -1, &len);
				isarray = (s != NULL && strncmp(s, "array", 6) == 0);
				has = true;
			}
			lua_pop(L, 2); // []
		}

		return has;
	}

	static bool isArray(lua_State* L, int idx)
	{
		bool isarray = false;
		if (hasJsonType(L, idx, isarray)) // any table with a meta field __jsontype set to 'array' are arrays
			return isarray;

		return (lua_rawlen(L, idx) > 0); // any table has length > 0 are treat as array.
	}

	template<typename Writer>
	void encodeValue(lua_State* L, Writer* writer, int idx, int depth = 0)
	{
		size_t len;
		const char* s;
		int64_t integer;
		int t = lua_type(L, idx);
		switch (t) {
		case LUA_TBOOLEAN:
			writer->Bool(lua_toboolean(L, idx) != 0);
			return;
		case LUA_TNUMBER:
			if (isInteger(L, idx, &integer))
				writer->Int64(integer);
			else
            {
                if (!writer->Double(lua_tonumber(L, idx)))
                    luaL_error(L, "error while encode double value.");
            }
			return;
		case LUA_TSTRING:
			s = lua_tolstring(L, idx, &len);
			writer->String(s, static_cast<SizeType>(len));
			return;
		case LUA_TTABLE:
			return encodeTable(L, writer, idx, depth + 1);
		case LUA_TNIL:
			writer->Null();
			return;
		case LUA_TFUNCTION:
			if (isJsonNull(L, idx))
			{
				writer->Null();
				return;
			}
		case LUA_TUSERDATA:
		    if (lua_isint64(L, idx))
			{
				writer->Int64(lua_toint64(L, idx));
				return;
			}
			if (lua_isuint64(L, idx))
			{
				writer->Uint64(lua_touint64(L, idx));
				return;
			}
			// otherwise fall thought
		case LUA_TLIGHTUSERDATA: // fall thought
		case LUA_TTHREAD: // fall thought
		case LUA_TNONE: // fall thought
		default:
			luaL_error(L, "value type : %s", lua_typename(L, t));
			return;
		}
	}

	template<typename Writer>
	void encodeTable(lua_State* L, Writer* writer, int idx, int depth)
	{
		if (depth > max_depth)
			luaL_error(L, "nested too depth");

		if (!lua_checkstack(L, 4)) // requires at least 4 slots in stack: table, key, value, key
			luaL_error(L, "stack overflow");

		lua_pushvalue(L, idx); // [table]
		if (isArray(L, -1))
		{
			encodeArray(L, writer, depth);
			lua_pop(L, 1); // []
			return;
		}

		// is object.
		if (!sort_keys)
		{
			encodeObject(L, writer, depth);
			lua_pop(L, 1); // []
			return;
		}

		lua_pushnil(L); // [table, nil]
		std::vector<Key> keys;

		while (lua_next(L, -2))
		{
			// [table, key, value]

			if (lua_type(L, -2) == LUA_TSTRING)
			{
				size_t len = 0;
				const char *key = lua_tolstring(L, -2, &len);
				keys.push_back(Key(key, static_cast<SizeType>(len)));
			}

			// pop value, leaving original key
			lua_pop(L, 1);
			// [table, key]
		}
		// [table]
		encodeObject(L, writer, depth, keys);
		lua_pop(L, 1);
	}

	template<typename Writer>
	void encodeObject(lua_State* L, Writer* writer, int depth)
	{
		writer->StartObject();

		// [table]
		lua_pushnil(L); // [table, nil]
		while (lua_next(L, -2))
		{
			// [table, key, value]
			if (lua_type(L, -2) == LUA_TSTRING)
			{
				size_t len = 0;
				const char *key = lua_tolstring(L, -2, &len);
				writer->Key(key, static_cast<SizeType>(len));
				encodeValue(L, writer, -1, depth);
			}

			// pop value, leaving original key
			lua_pop(L, 1);
			// [table, key]
		}
		// [table]
		writer->EndObject();
	}

	template<typename Writer>
	void encodeObject(lua_State* L, Writer* writer, int depth, std::vector<Key> &keys)
	{
		// [table]
		writer->StartObject();

		std::sort(keys.begin(), keys.end());

		std::vector<Key>::const_iterator i = keys.begin();
		std::vector<Key>::const_iterator e = keys.end();
		for (; i != e; ++i)
		{
			writer->Key(i->key, static_cast<SizeType>(i->size));
			lua_pushlstring(L, i->key, i->size); // [table, key]
			lua_gettable(L, -2); // [table, value]
			encodeValue(L, writer, -1, depth);
			lua_pop(L, 1); // [table]
		}
		// [table]
		writer->EndObject();
	}

	template<typename Writer>
	void encodeArray(lua_State* L, Writer* writer, int depth)
	{
		// [table]
		writer->StartArray();
		int MAX = static_cast<int>(lua_rawlen(L, -1)); // lua_rawlen always returns value >= 0
		for (int n = 1; n <= MAX; ++n)
		{
			lua_rawgeti(L, -1, n); // [table, element]
			encodeValue(L, writer, -1, depth);
			lua_pop(L, 1); // [table]
		}
		writer->EndArray();
		// [table]
	}

public:
	template<typename Stream>
	void encode(lua_State* L, Stream* s, int idx)
	{
		if (pretty)
		{
			PrettyWriter<Stream> writer(*s);
			encodeValue(L, &writer, idx);
		}
		else
		{
			Writer<Stream> writer(*s);
			encodeValue(L, &writer, idx);
		}
	}
};


static int json_encode(lua_State* L)
{
	try{
		Encoder encode(L, 2);
		StringBuffer s;
		encode.encode(L, &s, 1);
		lua_pushlstring(L, s.GetString(), s.GetSize());
		return 1;
	}
	catch (...) {
		luaL_error(L, "error while encoding");
	}
	return 0;
}


static int json_dump(lua_State* L)
{
	Encoder encoder(L, 3);

	const char* filename = luaL_checkstring(L, 2);
	FILE* fp = openForWrite(filename);
	if (fp == NULL)
		luaL_error(L, "error while open file: %s", filename);

	static const size_t sz = 4 * 1024;
	std::vector<char> buffer(sz);
	FileWriteStream fs(fp, &buffer.front(), sz);
	encoder.encode(L, &fs, 1);
	fclose(fp);
	return 0;
}


static const luaL_Reg methods[] = {
	// string <--> json
	{ "decode", json_decode },
	{ "encode", json_encode },

	// file <--> json
	{ "load", json_load },
	{ "dump", json_dump },

	// special tags place holder
	{ "null", json_null },
	{ "object", json_object },
	{ "array", json_array },
	{ NULL, NULL }
};


extern "C" {

LUALIB_API int luaopen_rapidjson(lua_State* L)
{
	lua_newtable(L); // [rapidjson]

	setfuncs(L, methods); // [rapidjson]

	lua_pushliteral(L, "rapidjson"); // [rapidjson, name]
	lua_setfield(L, -2, "_NAME"); // [rapidjson]

	lua_pushliteral(L, LUA_RAPIDJSON_VERSION); // [rapidjson, version]
	lua_setfield(L, -2, "_VERSION"); // [rapidjson]

	lua_getfield(L, -1, "null"); // [rapidjson, json.null]
	null = luaL_ref(L, LUA_REGISTRYINDEX); // [rapidjson]

	createSharedMeta(L, JSON_TABLE_TYPE_OBJECT);
	createSharedMeta(L, JSON_TABLE_TYPE_ARRAY);

	return 1;
}

}
