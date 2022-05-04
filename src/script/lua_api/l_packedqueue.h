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

#pragma once

#include "common/c_packer.h"
#include "lua_api/l_base.h"
#include "util/container.h"
#include <memory>

class LuaPackedQueue : public ModApiBase
{
private:
	typedef MutexedQueue<std::unique_ptr<PackedValue> > Queue;
	typedef std::shared_ptr<Queue> SharedQueue;

	SharedQueue m_queue;

	LuaPackedQueue(SharedQueue &&queue): m_queue(queue) {}

	static void create(lua_State *L, SharedQueue &&queue);

	static const char className[];
	static const luaL_Reg methods[];

	static int gc_object(lua_State *L);

	static int l_empty(lua_State *L);
	static int l_push_back(lua_State *L);
	static int l_pop_front(lua_State *L);
	static int l_try_pop_front(lua_State *L);

public:
	~LuaPackedQueue() = default;

	// LuaPackedQueue()
	// Creates a LuaPackedQueue and leaves it on top of stack
	static int create_object(lua_State *L);

	static LuaPackedQueue *checkobject(lua_State *L, int narg);

	static void *packIn(lua_State *L, int idx);
	static void packOut(lua_State *L, void *ptr);

	static void Register(lua_State *L);
};
