/*
Minetest
Copyright (C) 2022 Minetest developers

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation; either version 2.1 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include "lua_api/l_packedqueue.h"
#include "exceptions.h"
#include "lua_api/l_internal.h"
#include <algorithm>
#include <climits>

void LuaPackedQueue::create(lua_State *L, SharedQueue &&queue)
{
	LuaPackedQueue *o = (LuaPackedQueue *)lua_newuserdata(L, sizeof(*o));
	new (o) LuaPackedQueue(std::move(queue));
	luaL_getmetatable(L, className);
	lua_setmetatable(L, -2);
}

int LuaPackedQueue::create_object(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;

	create(L, std::make_shared<Queue>());
	return 1;
}

int LuaPackedQueue::l_empty(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;

	LuaPackedQueue *o = checkobject(L, 1);
	lua_pushboolean(L, o->m_queue->empty());
	return 1;
}

int LuaPackedQueue::l_push_back(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;

	LuaPackedQueue *o = checkobject(L, 1);
	std::unique_ptr<PackedValue> pv(script_pack(L, 2));

	// Scan the packed value to catch packed queues, preventing circular references.
	if (pv->contains_userdata) {
		for (const PackedInstr &i : pv->i) {
			if (i.type == LUA_TUSERDATA && i.sdata == className)
				throw LuaError("Attempt to put PackedQueue into a PackedQueue");
		}
	}

	o->m_queue->push_back(std::move(pv));
	return 0;
}

int LuaPackedQueue::l_pop_front(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;

	LuaPackedQueue *o = checkobject(L, 1);
	std::unique_ptr<PackedValue> pv(o->m_queue->pop_frontNoEx());
	script_unpack(L, pv.get());
	return 1;
}

int LuaPackedQueue::l_try_pop_front(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;

	LuaPackedQueue *o = checkobject(L, 1);
	lua_Number timeout = luaL_optnumber(L, 2, 0);
	s64 timeout_ms = std::min(std::max((s64)(timeout * 1000), (s64)0), (s64)UINT32_MAX);
	std::unique_ptr<PackedValue> pv(o->m_queue->pop_frontNoEx(timeout_ms));
	if (pv) {
		lua_pushboolean(L, true);
		script_unpack(L, pv.get());
	} else {
		lua_pushboolean(L, false);
		lua_pushnil(L);
	}
	return 2;
}

void *LuaPackedQueue::packIn(lua_State *L, int idx)
{
	LuaPackedQueue *o = checkobject(L, idx);
	return new SharedQueue(o->m_queue);
}

void LuaPackedQueue::packOut(lua_State *L, void *ptr)
{
	SharedQueue *queue = (SharedQueue *)ptr;
	if (L)
		create(L, std::move(*queue));
	delete queue;
}

int LuaPackedQueue::gc_object(lua_State *L)
{
	LuaPackedQueue *o = (LuaPackedQueue *)lua_touserdata(L, 1);
	o->~LuaPackedQueue();
	return 0;
}

LuaPackedQueue *LuaPackedQueue::checkobject(lua_State *L, int narg)
{
	return (LuaPackedQueue *)luaL_checkudata(L, narg, className);
}

void LuaPackedQueue::Register(lua_State *L)
{
	lua_newtable(L);
	int methodtable = lua_gettop(L);
	luaL_newmetatable(L, className);
	int metatable = lua_gettop(L);

	// hide metatable from Lua getmetatable()
	lua_pushliteral(L, "__metatable");
	lua_pushvalue(L, methodtable);
	lua_settable(L, metatable);

	lua_pushliteral(L, "__index");
	lua_pushvalue(L, methodtable);
	lua_settable(L, metatable);

	lua_pushliteral(L, "__gc");
	lua_pushcfunction(L, gc_object);
	lua_settable(L, metatable);

	lua_pop(L, 1);  // drop metatable

	luaL_register(L, nullptr, methods);  // fill methodtable
	lua_pop(L, 1);  // drop methodtable

	// Can be created from Lua (PackedQueue())
	lua_register(L, className, create_object);

	script_register_packer(L, className, packIn, packOut);
}

const char LuaPackedQueue::className[] = "PackedQueue";

const luaL_Reg LuaPackedQueue::methods[] = {
	luamethod(LuaPackedQueue, empty),
	luamethod(LuaPackedQueue, push_back),
	luamethod(LuaPackedQueue, pop_front),
	{0,0}
};
