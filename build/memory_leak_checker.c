/*
 *Tencent is pleased to support the open source community by making xLua available.
 *Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.
 *Licensed under the MIT License (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
 *http://opensource.org/licenses/MIT
 *Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
*/

#define LUA_LIB

#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
#include <stdio.h>
#include <string.h>
#include "ltable.h"
#include "lstate.h"
#include "lobject.h"
#include "lapi.h"
#include "lgc.h"

#define gnodelast(h)	gnode(h, cast(size_t, sizenode(h)))

static int table_size (Table *h, int fast)
{
	if (fast)
	{
#if LUA_VERSION_NUM >= 504
		return (int)sizenode(h) + (int)h->alimit;
#else
		return (int)sizenode(h) + (int)h->sizearray;
#endif
	}
	else
	{
		Node *n, *limit = gnodelast(h);
		int i = (int)luaH_getn(h);
		for (n = gnode(h, 0); n < limit; n++)
		{ 
			if (!ttisnil(gval(n)))
			{
				i++;
			}
		}
		return i;
	}
}

typedef void (*TableSizeReport) (const void *p, int size);

// type: 1: value of table(key is string), 2: value of table(key is number), 3: key of table, 4: metatable of table, 5: upvalue of closure
typedef void (*ObjectRelationshipReport) (const void *parent, const void *child, int type, const char *key, double d, const char *key2);

LUA_API void xlua_report_table_size(lua_State *L, TableSizeReport cb, int fast)
{
	GCObject *p = G(L)->allgc;
	while (p != NULL)
	{
		if (p->tt == LUA_TTABLE)
		{
			Table *h = gco2t(p);
			cb(h, table_size(h, fast));
		}
		p = p->next;
	}
}


static void report_table(Table *h, ObjectRelationshipReport cb)
{
	Node *n, *limit = gnodelast(h);
    unsigned int i;
	
	if (h->metatable != NULL)
	{
		cb(h, h->metatable, 4, NULL, 0, NULL);
	}

#if LUA_VERSION_NUM >= 504
    for (i = 0; i < h->alimit; i++)
#else
	for (i = 0; i < h->sizearray; i++)
#endif
	{
		const TValue *item = &h->array[i];
		if (ttistable(item))
		{
		    cb(h, gcvalue(item), 2, NULL, i + 1, NULL);
		}
	}

    for (n = gnode(h, 0); n < limit; n++)
	{
        if (!ttisnil(gval(n)))
        {
#if LUA_VERSION_NUM >= 504
			const TValue* key = (const TValue *)&(n->u.key_val);
#else
            const TValue *key = gkey(n);
#endif
			if (ttistable(key))
			{
				cb(h, gcvalue(key), 3, NULL, 0, NULL);
			}
            const TValue *value = gval(n);
			if (ttistable(value))
			{
				if (ttisstring(key))
				{
					cb(h, gcvalue(value), 1, getstr(tsvalue(key)), 0, NULL);
				}
				else if(ttisnumber(key))
				{
					cb(h, gcvalue(value), 2, NULL, nvalue(key), NULL);
				}
				else
				{
					// ???
#if LUA_VERSION_NUM >= 504
					cb(h, gcvalue(value), 1, NULL, novariant(key->tt_), NULL);
#else
					cb(h, gcvalue(value), 1, NULL, ttnov(key), NULL);
#endif
				}
			}
		}
    }
}


LUA_API void xlua_report_object_relationship(lua_State *L, ObjectRelationshipReport cb)
{
	GCObject *p = G(L)->allgc;
	lua_Debug ar;
	int i;
	const char *name;
	
	while (p != NULL)
	{
		if (p->tt == LUA_TTABLE)
		{
			Table *h = gco2t(p);
			report_table(h, cb);
		}
#if LUA_VERSION_NUM >= 504
		else if (p->tt == LUA_VLCL)
#else
		else if (p->tt == LUA_TLCL)
#endif
		{
			LClosure *cl = gco2lcl(p);
			lua_lock(L);
#if LUA_VERSION_NUM >= 504 && LUA_VERSION_RELEASE_NUM >= 50406
			setclLvalue2s(L, L->top.p, cl);
#else
			setclLvalue(L, L->top, cl);
#endif
			api_incr_top(L);
			lua_unlock(L);
			
			lua_pushvalue(L, -1);
			
			lua_getinfo(L, ">S", &ar);
			
			for (i=1;;i++)
			{
				name = lua_getupvalue(L,-1,i);
				if (name == NULL)
					break;
				const void *pv = lua_topointer(L, -1);
				
				if (*name != '\0' && LUA_TTABLE == lua_type(L, -1))
				{
					cb(cl, pv, 5, ar.short_src, ar.linedefined, name);
				}
				lua_pop(L, 1);
			}
			
			lua_pop(L, 1);
		}
		p = p->next;
	}
}

LUA_API void *xlua_registry_pointer(lua_State *L)
{
	return gcvalue(&G(L)->l_registry);
}

LUA_API void *xlua_global_pointer(lua_State *L)
{
	Table *reg = hvalue(&G(L)->l_registry);
	const TValue *global;
    lua_lock(L);
	global = luaH_getint(reg, LUA_RIDX_GLOBALS);
	lua_unlock(L);
	return gcvalue(global);
}
