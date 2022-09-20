local np = {
	offset = 0,
	scale = 1,
	spread = {x = 500, y = 500, z = 500},
	seed = 5713472,
	octaves = 2,
	persistence = 0.63,
	lacunarity = 2.0,
}

local size = vector.new(51, 52, 53)

local noise = PerlinNoiseMap(np, size)

local pos = vector.new(1, 2, 3)

local noise_single = PerlinNoise(np)

local function approx_equal(a, b)
	return math.abs(a - b) < 0.01
end

local function assert_equal_2d(map, map_flat, x, y)
	local n = noise_single:get_2d({x = pos.x + x, y = pos.y + y})
	local n_map = map[y + 1][x + 1]
	local n_map_flat = map_flat[y * size.x + x + 1]
	assert(approx_equal(n, n_map))
	assert(approx_equal(n, n_map_flat))
end

local function assert_equal_3d(map, map_flat, x, y, z)
	local n = noise_single:get_3d(pos:offset(x, y, z))
	local n_map = map[z + 1][y + 1][x + 1]
	local n_map_flat = map_flat[z * size.y * size.x + y * size.x + x + 1]
	assert(approx_equal(n, n_map))
	assert(approx_equal(n, n_map_flat))
end

local function test_2d_noise_map()
	local map = noise:get_2d_map(pos)
	local map_flat = noise:get_2d_map_flat(pos, "not a table")
	assert(#map == size.y)
	assert(#map[1] == size.x)
	assert(#map_flat == size.x * size.y)
	assert_equal_2d(map, map_flat, 0, 0)
	assert_equal_2d(map, map_flat, 12, 34)
	assert_equal_2d(map, map_flat, 50, 51)

	local map_flat_2 = {}
	assert(map_flat_2 == noise:get_2d_map_flat(pos, map_flat_2))
	assert(approx_equal(#map_flat_2, #map_flat))
	assert(approx_equal(map_flat_2[1], map_flat[1]))
	assert(approx_equal(map_flat_2[123], map_flat[123]))
	assert(approx_equal(map_flat_2[#map_flat_2], map_flat[#map_flat]))
end
unittests.register("test_2d_noise_map", test_2d_noise_map)

local function test_3d_noise_map()
	local map = noise:get_3d_map(pos)
	local map_flat = noise:get_3d_map_flat(pos, "not a table")
	assert(#map == size.z)
	assert(#map[1] == size.y)
	assert(#map[1][1] == size.x)
	assert(#map_flat == size.x * size.y * size.z)
	assert_equal_3d(map, map_flat, 0, 0, 0)
	assert_equal_3d(map, map_flat, 12, 34, 32)
	assert_equal_3d(map, map_flat, 50, 51, 52)

	local map_flat_2 = {}
	assert(map_flat_2 == noise:get_3d_map_flat(pos, map_flat_2))
	assert(approx_equal(#map_flat_2, #map_flat))
	assert(approx_equal(map_flat_2[1], map_flat[1]))
	assert(approx_equal(map_flat_2[123], map_flat[123]))
	assert(approx_equal(map_flat_2[#map_flat_2], map_flat[#map_flat]))
end
unittests.register("test_3d_noise_map", test_3d_noise_map)
