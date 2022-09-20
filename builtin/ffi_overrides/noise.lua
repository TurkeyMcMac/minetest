local ffi, ie = ...

local metatable = ie.debug.getregistry().PerlinNoiseMap
local methodtable = metatable.__metatable
local C = ffi.C
local table_new = _G.table.new
local rawset, type = _G.rawset, _G.type

ffi.cdef[[
float *mtffi_pnm_get_result(void *ud);

uint32_t mtffi_pnm_get_sx(void *ud);
uint32_t mtffi_pnm_get_sy(void *ud);
uint32_t mtffi_pnm_get_sz(void *ud);
]]

-- This is required for security:
local calc_2d_map = methodtable.calc_2d_map
local calc_3d_map = methodtable.calc_3d_map

function methodtable:get_2d_map(pos)
	calc_2d_map(self, pos)
	local result = C.mtffi_pnm_get_result(self)
	local sx = C.mtffi_pnm_get_sx(self)
	local sy = C.mtffi_pnm_get_sy(self)
	local i = 0
	local map = table_new(sy, 0)
	for y = 1, sy do
		local row = table_new(sx, 0)
		for x = 1, sx do
			row[x] = result[i]
			i = i + 1
		end
		map[y] = row
	end
	return map
end

function methodtable:get_2d_map_flat(pos, buffer)
	calc_2d_map(self, pos)
	local result = C.mtffi_pnm_get_result(self)
	local sx = C.mtffi_pnm_get_sx(self)
	local sy = C.mtffi_pnm_get_sy(self)
	local maplen = sx * sy
	if type(buffer) ~= "table" then
		buffer = table_new(maplen, 0)
	end
	for i = 1, maplen do
		rawset(buffer, i, result[i - 1])
	end
	return buffer
end

function methodtable:get_3d_map(pos)
	calc_3d_map(self, pos)
	local result = C.mtffi_pnm_get_result(self)
	local sx = C.mtffi_pnm_get_sx(self)
	local sy = C.mtffi_pnm_get_sy(self)
	local sz = C.mtffi_pnm_get_sz(self)
	if sz <= 1 then
		return
	end
	local i = 0
	local map = table_new(sz, 0)
	for z = 1, sz do
		local sheet = table_new(sy, 0)
		for y = 1, sy do
			local row = table_new(sx, 0)
			for x = 1, sx do
				row[x] = result[i]
				i = i + 1
			end
			sheet[y] = row
		end
		map[z] = sheet
	end
	return map
end

function methodtable:get_3d_map_flat(pos, buffer)
	calc_3d_map(self, pos)
	local result = C.mtffi_pnm_get_result(self)
	local sx = C.mtffi_pnm_get_sx(self)
	local sy = C.mtffi_pnm_get_sy(self)
	local sz = C.mtffi_pnm_get_sz(self)
	if sz <= 1 then
		return
	end
	local maplen = sx * sy * sz
	if type(buffer) ~= "table" then
		buffer = table_new(maplen, 0)
	end
	for i = 1, maplen do
		rawset(buffer, i, result[i - 1])
	end
	return buffer
end
