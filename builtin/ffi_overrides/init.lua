local insecure_environment = ...

if not _G.core.settings:get_bool("use_ffi", true) then
	return
end

if not _G.core.global_exists("jit") then
	-- Not LuaJIT, nothing to do.
	return
end

local has_ffi, ffi = _G.pcall(insecure_environment.require, "ffi")
if not has_ffi then
	_G.core.log("warning",
		"Since the FFI library is absent, " ..
		"optimized FFI implementions are unavailable.")
	return
end

local ffipath = _G.core.get_builtin_path() .. "ffi_overrides" .. DIR_DELIM

if _G.VoxelManip then
	assert(loadfile(ffipath .. "voxelmanip.lua"))(ffi, insecure_environment)
end
