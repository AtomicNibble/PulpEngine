#include "lua.h"
#include "lauxlib.h"

#if 1



#include <stdlib.h>
#include <math.h>

#define lgcveclib_c
#define LUA_LIB

#include "lua.h"

#include "lauxlib.h"
#include "lualib.h"

#define checkvec(L, n) (vec_t*)luaL_checkudata( L, n, "vec.vec" )

typedef struct
{
	float x, y, z, w;
} vec_t;

static void new_vec(lua_State* L, float x, float y, float z, float w)
{
	vec_t* v = (vec_t*)lua_newuserdata(L, sizeof(vec_t));
	v->x = x;
	v->y = y;
	v->z = z;
	v->w = w;
	luaL_getmetatable(L, "vec.vec");
	lua_setmetatable(L, -2);
}

static int gcvec_new(lua_State *L) {
	float x = (float)lua_tonumber(L, 1);
	float y = (float)lua_tonumber(L, 2);
	float z = (float)lua_tonumber(L, 3);
	float w = (float)lua_tonumber(L, 4);
	new_vec(L, x, y, z, w);
	return 1;
}

static int gcvec_dot(lua_State *L) {
	const vec_t* v1 = checkvec(L, 1);
	const vec_t* v2 = checkvec(L, 2);
	lua_pushnumber(L, v1->x*v2->x + v1->y*v2->y + v1->z*v2->z + v1->w*v2->w);
	return 1;
}

static int gcvec_cross(lua_State *L) {
	const vec_t* v1 = checkvec(L, 1);
	const vec_t* v2 = checkvec(L, 2);
	new_vec(L, v1->y * v2->z - v1->z * v2->y, v1->z * v2->x - v1->x * v2->z, v1->x * v2->y - v1->y * v2->x, 0.0f);
	return 1;
}

static int gcvec_length(lua_State *L) {
	const vec_t* v = checkvec(L, 1);
	lua_pushnumber(L, sqrtf(v->x*v->x + v->y*v->y + v->z*v->z + v->w*v->w));
	return 1;
}

static int gcvec_normalize(lua_State *L) {
	const vec_t* v = checkvec(L, 1);
	float s = 1.0f / sqrtf(v->x*v->x + v->y*v->y + v->z*v->z + v->w*v->w);
	new_vec(L, v->x*s, v->y*s, v->z*s, v->w*s);
	return 1;
}

static int gcvec_set_item(lua_State* L)
{
	vec_t* v = checkvec(L, 1);
	int i = luaL_checkint(L, 2);
	float val = (float)luaL_checknumber(L, 3);
	luaL_argcheck(L, i >= 1 && i <= LUA_VEC_SIZE, 2, "index out of range");
	(&v->x)[i - 1] = val;
	return 0;
}

static int gcvec_get_item(lua_State* L)
{
	vec_t* v = checkvec(L, 1);
	int i = luaL_checkint(L, 2);
	luaL_argcheck(L, i >= 1 && i <= LUA_VEC_SIZE, 2, "index out of range");
	lua_pushnumber(L, (&v->x)[i - 1]);
	return 1;
}

static int gcvec_add(lua_State* L)
{
	vec_t* v1 = checkvec(L, 1);
	vec_t* v2 = checkvec(L, 2);
	new_vec(L, v1->x + v2->x, v1->y + v2->y, v1->z + v2->z, v1->w + v2->w);
	return 1;
}

static int gcvec_sub(lua_State* L)
{
	vec_t* v1 = checkvec(L, 1);
	vec_t* v2 = checkvec(L, 2);
	new_vec(L, v1->x - v2->x, v1->y - v2->y, v1->z - v2->z, v1->w - v2->w);
	return 1;
}

static int gcvec_mul(lua_State* L)
{
	vec_t* v1 = checkvec(L, 1);
	if (lua_isuserdata(L, 2))
	{
		// vector * vector
		vec_t* v2 = checkvec(L, 2);
		new_vec(L, v1->x * v2->x, v1->y * v2->y, v1->z * v2->z, v1->w * v2->w);
	}
	else
	{
		// vector * scalar
		float s = (float)luaL_checknumber(L, 2);
		new_vec(L, v1->x * s, v1->y * s, v1->z * s, v1->w * s);
	}
	return 1;
}

static int gcvec_div(lua_State* L)
{
	vec_t* v1 = checkvec(L, 1);
	float s = (float)luaL_checknumber(L, 2);
	luaL_argcheck(L, s != 0.0f, 2, "division by zero");
	new_vec(L, v1->x / s, v1->y / s, v1->z / s, v1->w / s);
	return 1;
}

static int gcvec_negate(lua_State* L)
{
	vec_t* v = checkvec(L, 1);
	new_vec(L, -v->x, -v->y, -v->z, -v->w);
	return 1;
}

static int gcvec_tostring(lua_State* L)
{
	vec_t* v = checkvec(L, 1);
	lua_pushfstring(L, "vec(%f, %f, %f, %f)", v->x, v->y, v->z, v->w);
	return 1;
}

static const luaL_Reg gcveclib_f[] = {
	{ "new", gcvec_new },
	{ "dot", gcvec_dot },
	{ "cross", gcvec_cross },
	{ "length", gcvec_length },
	{ "normalize", gcvec_normalize },
	{ NULL, NULL }
};

static const luaL_Reg gcveclib_m[] = {
	{ "__newindex", gcvec_set_item },
	{ "__index", gcvec_get_item },
	{ "__add", gcvec_add },
	{ "__sub", gcvec_sub },
	{ "__mul", gcvec_mul },
	{ "__div", gcvec_div },
	{ "__unm", gcvec_negate },
	{ "__tostring", gcvec_tostring },
	{ NULL, NULL }
};


/*
** Open veclib
*/
LUALIB_API int luaopen_vec(lua_State *L)
{
	// init mt
	luaL_newmetatable(L, "vec.vec");
	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "__index");     // metatable.__index = metatable
	luaL_register(L, NULL, gcveclib_m);

	luaL_register(L, LUA_VECLIBNAME, gcveclib_f);

	// numeric constants
	new_vec(L, 0, 0, 0, 0);
	lua_setfield(L, -2, "zero");
	new_vec(L, 1, 1, 1, 1);
	lua_setfield(L, -2, "one");
	return 1;
}

#else

int g_vectortag = 0;
int g_vectorMetatable = 0;
const void* g_metatablePointer = 0;


int vl_isvector(lua_State* L, int index);


float *newvector(lua_State* L)
{
	float *v = (float *)lua_newuserdata(L, sizeof(float)* 3);
	int nparams = lua_gettop(L);
	if (nparams>0)
	{
		v[0] = lua_tonumber(L, 1);
		v[1] = lua_tonumber(L, 2);
		v[2] = lua_tonumber(L, 3);
	}
	else{
		v[0] = v[1] = v[2] = 0.0f;
	}
	lua_getref(L, g_vectorMetatable);
	lua_setmetatable(L, -2);
	return v;
}

lua_Number luaL_check_number(lua_State* L, int index)
{
	return lua_tonumber(L, index);
}

const char* luaL_check_string(lua_State* L, int index)
{
	return lua_tostring(L, index);
}

int vector_set(lua_State* L)
{
	float *v = (float *)lua_touserdata(L, 1);
	if (v)
	{
		const char* idx = luaL_check_string(L, 2);
		if (idx)
		{
			switch (idx[0])
			{
				case 'x':case 'r':
				v[0] = luaL_check_number(L, 3);
				return 0;
				case 'y':case 'g':
				v[1] = luaL_check_number(L, 3);
				return 0;
				case 'z':case 'b':
				v[2] = luaL_check_number(L, 3);
				return 0;
				default:
				break;
			}
		}
	}
	return 0;
}

int vector_get(lua_State* L)
{
	float *v = (float *)lua_touserdata(L, 1);
	if (v)
	{
		const char* idx = luaL_check_string(L, 2);
		if (idx)
		{
			switch (idx[0])
			{
				case 'x':case 'r':
				lua_pushnumber(L, v[0]);
				return 1;
				case 'y':case 'g':
				lua_pushnumber(L, v[1]);
				return 1;
				case 'z':case 'b':
				lua_pushnumber(L, v[2]);
				return 1;
				default:
				return 0;
				break;
			}
		}
	}
	return 0;
}

int vector_mul(lua_State* L)
{
	float *v = (float *)lua_touserdata(L, 1);
	if (v)
	{
		if (vl_isvector(L, 2))
		{
			float *v2 = (float *)lua_touserdata(L, 2);
			float res = v[0] * v2[0] + v[1] * v2[1] + v[2] * v2[2];
			lua_pushnumber(L, res);
			return 1;
		}
		else if (lua_isnumber(L, 2))
		{
			float f = lua_tonumber(L, 2);
			float *newv = newvector(L);
			newv[0] = v[0] * f;
			newv[1] = v[1] * f;
			newv[2] = v[2] * f;
			return 1;

		}
		else 
			luaL_error(L, "mutiplying a vector with an invalid type");
	}
	return 0;
}

int vector_add(lua_State* L)
{
	float *v = (float *)lua_touserdata(L, 1);
	if (v)
	{
		if (vl_isvector(L, 2))
		{
			float *v2 = (float *)lua_touserdata(L, 2);
			float *newv = newvector(L);
			newv[0] = v[0] + v2[0];
			newv[1] = v[1] + v2[1];
			newv[2] = v[2] + v2[2];
			return 1;
		}
		else 
			luaL_error(L, "adding a vector with an invalid type");
	}
	return 0;
}

int vector_div(lua_State* L)
{
	float *v = (float *)lua_touserdata(L, 1);
	if (v)
	{
		if (lua_isnumber(L, 2))
		{
			float f = lua_tonumber(L, 2);
			float *newv = newvector(L);
			newv[0] = v[0] / f;
			newv[1] = v[1] / f;
			newv[2] = v[2] / f;
			return 1;

		}
		else 
			luaL_error(L, "dividing a vector with an invalid type");
	}
	return 0;
}

int vector_sub(lua_State* L)
{
	float *v = (float *)lua_touserdata(L, 1);
	if (v)
	{
		if (vl_isvector(L, 2))
		{
			float *v2 = (float *)lua_touserdata(L, 2);
			float *newv = newvector(L);
			newv[0] = v[0] - v2[0];
			newv[1] = v[1] - v2[1];
			newv[2] = v[2] - v2[2];
			return 1;
		}
		else if (lua_isnumber(L, 2))
		{
			float f = lua_tonumber(L, 2);
			float *newv = newvector(L);
			newv[0] = v[0] - f;
			newv[1] = v[1] - f;
			newv[2] = v[2] - f;
			return 1;

		}
		else 
			luaL_error(L, "subtracting a vector with an invalid type");
	}
	return 0;
}

int vector_unm(lua_State* L)
{
	float *v = (float *)lua_touserdata(L, 1);
	if (v)
	{
		float *newv = newvector(L);
		newv[0] = -v[0];
		newv[1] = -v[1];
		newv[2] = -v[2];
		return 1;
	}
	return 0;
}

int vector_pow(lua_State* L)
{
	float *v = (float *)lua_touserdata(L, 1);
	if (v)
	{
		if (vl_isvector(L, 2))
		{
			float *v2 = (float *)lua_touserdata(L, 2);
			float *newv = newvector(L);
			newv[0] = v[1] * v2[2] - v[2] * v2[1];
			newv[1] = v[2] * v2[0] - v[0] * v2[2];
			newv[2] = v[0] * v2[1] - v[1] * v2[0];
			return 1;
		}
		else 
			luaL_error(L, "cross product between vector and an invalid type");
	}
	return 0;
}

int vl_newvector(lua_State* L)
{
	newvector(L);
	return 1;
}

int vl_isvector(lua_State* L, int index)
{
	const void* ptr;
	if (lua_type(L, index) != LUA_TUSERDATA)
		return 0;
	lua_getmetatable(L, index);
	ptr = lua_topointer(L, -1);
	lua_pop(L, 1);
	return (ptr == g_metatablePointer);
}

void vl_SetEventFunction(lua_State* L, const char *sEvent, lua_CFunction fn, int nTable)
{
	lua_pushstring(L, sEvent);
	lua_pushcclosure(L, fn, 0);
	lua_rawset(L, nTable);
}

LUALIB_API int x_initvectorlib(lua_State* L)
{
	int nTable;
	// Create a new vector metatable.
	lua_newtable(L);
	nTable = lua_gettop(L);

	g_metatablePointer = lua_topointer(L, nTable);

	vl_SetEventFunction(L, "__newindex", vector_set, nTable);
	vl_SetEventFunction(L, "__index", vector_get, nTable);
	vl_SetEventFunction(L, "__mul", vector_mul, nTable);
	vl_SetEventFunction(L, "__div", vector_div, nTable);
	vl_SetEventFunction(L, "__add", vector_add, nTable);
	vl_SetEventFunction(L, "__sub", vector_sub, nTable);
	vl_SetEventFunction(L, "__pow", vector_pow, nTable);
	vl_SetEventFunction(L, "__unm", vector_unm, nTable);

	g_vectorMetatable = lua_ref(L, nTable); // pop table
	return 1;
}

#endif
