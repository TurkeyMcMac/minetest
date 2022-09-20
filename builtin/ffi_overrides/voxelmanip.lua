local ffi, ie = ...

local metatable = ie.debug.getregistry().VoxelManip
local get_real_metatable = ie.debug.getmetatable
local C = ffi.C
local rawequal, rawget, rawset, type, tonumber, error, pcall =
	_G.rawequal, _G.rawget, _G.rawset, _G.type, _G.tonumber, _G.error, _G.pcall
local table_new = _G.table.new

ffi.cdef[[
struct MapNode {
	uint16_t param0;
	uint8_t param1, param2;
};

bool mtffi_vm_lock_area(void *ud);

void mtffi_vm_unlock_area(void *ud);

struct MapNode *mtffi_vm_get_data(void *ud);

int32_t mtffi_vm_get_volume(void *ud);
]]

local function check(self)
	if not rawequal(get_real_metatable(self), metatable) then
		error("VoxelManip method called on invalid object")
	end
end

local function lock_area(self)
	if not C.mtffi_vm_lock_area(self) then
		error("Cannot lock VoxelManip area")
	end
end

local function unlock_area(self)
	C.mtffi_vm_unlock_area(self)
end

local methodtable = metatable.__metatable

local function bulk_getter(field)
	local function get_protected(self, buf)
		local data = C.mtffi_vm_get_data(self)
		local volume = C.mtffi_vm_get_volume(self)
		if type(buf) ~= "table" then
			buf = table_new(volume, 0)
		end
		for i = 1, volume do
			rawset(buf, i, data[i - 1][field])
		end
		return buf
	end

	return function(self, buf)
		check(self)
		lock_area(self)
		local ok, result = pcall(get_protected, self, buf)
		unlock_area(self)
		if not ok then
			error(result, 0)
		end
		return result
	end
end

local function bulk_setter(field)
	local function set_protected(self, buf)
		local data = C.mtffi_vm_get_data(self)
		local volume = C.mtffi_vm_get_volume(self)
		for i = 1, volume do
			data[i - 1][field] = tonumber(rawget(buf, i)) or 0
		end
	end

	return function(self, buf)
		check(self)
		lock_area(self)
		local ok, result = pcall(set_protected, self, buf)
		unlock_area(self)
		if not ok then
			error(result, 0)
		end
	end
end

methodtable.get_data = bulk_getter("param0")
methodtable.set_data = bulk_setter("param0")

methodtable.get_light_data = bulk_getter("param1")
methodtable.set_light_data = bulk_setter("param1")

methodtable.get_param2_data = bulk_getter("param2")
methodtable.set_param2_data = bulk_setter("param2")
