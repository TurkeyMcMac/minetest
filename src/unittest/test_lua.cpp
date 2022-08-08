/*
Minetest
Copyright (C) 2013 celeron55, Perttu Ahola <celeron55@gmail.com>
Copyright (C) 2021 TurkeyMcMac, Jude Melton-Houghton <jwmhjwmh@gmail.com>

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

#include "test.h"
#include "config.h"

extern "C" {
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}

class TestLua : public TestBase
{
public:
	TestLua() { TestManager::registerTestModule(this); }
	const char *getName() { return "TestLua"; }

	void runTests(IGameDef *gamedef);

	void testLuaDestructors();
#if USE_LUAJIT
	void testLuaJITFFILinking();
#endif
};

static TestLua g_test_instance;

void TestLua::runTests(IGameDef *gamedef)
{
	TEST(testLuaDestructors);
#if USE_LUAJIT
	TEST(testLuaJITFFILinking);
#endif
}

////////////////////////////////////////////////////////////////////////////////

namespace
{

	class DestructorDetector {
		bool *did_destruct;
	public:
		DestructorDetector(bool *did_destruct) : did_destruct(did_destruct)
		{
			*did_destruct = false;
		}
		~DestructorDetector()
		{
			*did_destruct = true;
		}
	};

}

void TestLua::testLuaDestructors()
{
	bool did_destruct = false;

	lua_State *L = luaL_newstate();
	lua_cpcall(L, [](lua_State *L) -> int {
		DestructorDetector d(reinterpret_cast<bool*>(lua_touserdata(L, 1)));
		luaL_error(L, "error");
		return 0;
	}, &did_destruct);
	lua_close(L);

	UASSERT(did_destruct);
}

#if USE_LUAJIT
extern "C" void mtffi_test_symbol() {}

void TestLua::testLuaJITFFILinking()
{
	lua_State *L = luaL_newstate();
	assert(L);
	try {
		luaL_openlibs(L);
		int result = luaL_dostring(L,
			"local has_ffi, ffi = pcall(require, 'ffi')\n"
			"if has_ffi then\n"
			"    ffi.cdef('void mtffi_test_symbol();')\n"
			"    assert(ffi.C.mtffi_test_symbol, 'Test symbol not found')\n"
			"end\n"
		);
		UTEST(result == 0, "Lua error: %s", luaL_checkstring(L, -1));
	} catch (...) {
		lua_close(L);
		throw;
	}
	lua_close(L);
}
#endif // USE_LUAJIT
