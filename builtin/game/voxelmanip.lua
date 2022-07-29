-- This must load after builtin/game/item_s.lua, since since that file
-- overrides core.get_content_id and core.get_name_from_content_id.

if not core.global_exists("jit") then
	-- Not LuaJIT, nothing to do.
	return
end

local ie = ...
local metatable = ie.debug.getregistry().VoxelManip
local has_ffi, ffi = pcall(ie.require, "ffi")
local get_real_metatable = ie.debug.getmetatable
ie = nil

if not has_ffi then
	-- FFI is required for the optimizations.
	return
end

ffi.cdef[[
int32_t mtffi_vm_get_node(void *vm, double x, double y, double z);

void mtffi_vm_set_node(void *vm, double x, double y, double z,
		uint16_t content, uint8_t param1, uint8_t param2);
]]

local C = ffi.C
local rawequal = rawequal
local tonumber = tonumber
local band, rshift = bit.band, bit.rshift
local get_content_id = core.get_content_id
local get_name_from_content_id = core.get_name_from_content_id

local function check(self)
	if not rawequal(get_real_metatable(self), metatable) then
		error("VoxelManip method called on invalid object")
	end
end

local methodtable = metatable.__metatable

function methodtable:get_node_at(pos)
	check(self)
	local data = tonumber(C.mtffi_vm_get_node(self, pos.x, pos.y, pos.z))
	local content = band(data, 0xffff)
	local param1 = band(rshift(data, 16), 0xff)
	local param2 = rshift(data, 24)
	return {name = get_name_from_content_id(content), param1 = param1, param2 = param2}
end

function methodtable:set_node_at(pos, node)
	check(self)
	C.mtffi_vm_set_node(self, pos.x, pos.y, pos.z,
			get_content_id(node.name), node.param1 or 0, node.param2 or 0)
end
