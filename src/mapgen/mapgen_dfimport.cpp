#include "mapgen_dfimport.h"
#include "cavegen.h"
#include "emerge.h"
#include "map.h"
#include "mapnode.h"
#include "mg_biome.h"
#include "mg_decoration.h"
#include "mg_ore.h"
#include <cmath>
#include <memory>

// TODO: IDK why this is necessary or what the root cause of discontinuity is.
#define BREAK_BIAS (-6)

namespace // private stuff
{

constexpr f64 square(f64 u) { return u * u; }

void readGrid(std::ifstream &in, v2s16 &dims, s16 *&data)
{
	dims.X = readU16(in);
	dims.Y = readU16(in);
	s32 n_elems = dims.X * dims.Y;
	data = new s16[n_elems];
	for (s32 i = 0; i < n_elems; ++i) {
		data[i] = readU16(in);
	}
}

class Grid
{
public:
	Grid(v2s16 dims, s16 *tiles, s16 default_):
		m_dims(dims), m_tiles(tiles), m_default(default_)
	{}

	~Grid() = default;

	s16 get(v2s16 pos) const
	{
		pos.X += m_dims.X / 2;
		pos.Y = -pos.Y + m_dims.Y / 2;
		if (pos.X < 0 || pos.X >= m_dims.X || pos.Y < 0 || pos.Y >= m_dims.Y)
			return m_default;
		return m_tiles[pos.Y * m_dims.Y + pos.X];
	}

	s16 getWidth() const { return m_dims.X; }
	s16 getHeight() const { return m_dims.Y; }

private:
	v2s16 m_dims;
	s16 *m_tiles;
	s16 m_default;
};

class Interpolator
{
public:
	Interpolator() = default;

	Interpolator(f64 x, f64 y, f64 r, f64 sl, f64 sr)
	{
		f64 xl = x - r;
		f64 yl = y - sl * r;
		m_a = (sr - sl) / 4 / r;
		m_b = sl - 2 * m_a * xl;
		m_c = yl - m_a * xl * xl - m_b * xl;
	}

	~Interpolator() = default;

	f64 operator()(f64 u) const { return m_a * u * u + m_b * u + m_c; }

private:
	f64 m_a, m_b, m_c;
};

Interpolator makeXInterpolator(v2s16 cp, s16 chunk_side, const Grid &grid)
{
	f64 el = grid.get(cp);
	f64 el_m = grid.get(cp - v2s16(1, 0));
	f64 el_p = grid.get(cp + v2s16(1, 0));
	return Interpolator((cp.X + 0.5) * chunk_side, el, chunk_side / 2, (el - el_m) / chunk_side, (el_p - el) / chunk_side);
}

Interpolator makeZInterpolator(v2s16 cp, s16 chunk_side, const Grid &grid)
{
	f64 el = grid.get(cp);
	f64 el_m = grid.get(cp - v2s16(0, 1));
	f64 el_p = grid.get(cp + v2s16(0, 1));
	return Interpolator((cp.Y + 0.5) * chunk_side, el, chunk_side / 2, (el - el_m) / chunk_side, (el_p - el) / chunk_side);
}

class ChunkInterpolator
{
public:
	ChunkInterpolator(v3s16 minp, v3s16 maxp, s16 chunk_side, const Grid &grid);

	~ChunkInterpolator() = default;

	f64 operator()(v2s16 pos) const;

private:
	Interpolator m_int_x_ne, m_int_x_nw, m_int_x_sw, m_int_x_se;
	Interpolator m_int_z_ne, m_int_z_nw, m_int_z_sw, m_int_z_se;
	s16 m_chunk_side;
	s16 m_min_x, m_min_z, m_max_x, m_max_z, m_mid_x, m_mid_z;
};

ChunkInterpolator::ChunkInterpolator(v3s16 minp, v3s16 maxp, s16 chunk_side, const Grid &grid)
{
	m_chunk_side = chunk_side;
	m_min_x = minp.X;
	m_min_z = minp.Z;
	m_max_x = maxp.X;
	m_max_z = maxp.Z;
	m_mid_x = m_min_x + m_chunk_side / 2 + BREAK_BIAS;
	m_mid_z = m_min_z + m_chunk_side / 2 + BREAK_BIAS;
	v2s16 cp(std::floor((f64) m_min_x / m_chunk_side),
			std::floor((f64) m_min_z / m_chunk_side));
	m_int_x_sw = makeXInterpolator(cp, m_chunk_side, grid);
	m_int_x_se = makeXInterpolator(cp + v2s16(1, 0), m_chunk_side, grid);
	m_int_x_nw = makeXInterpolator(cp + v2s16(0, 1), m_chunk_side, grid);
	m_int_x_ne = makeXInterpolator(cp + v2s16(1, 1), m_chunk_side, grid);
	m_int_z_sw = makeZInterpolator(cp, m_chunk_side, grid);
	m_int_z_se = makeZInterpolator(cp + v2s16(1, 0), m_chunk_side, grid);
	m_int_z_nw = makeZInterpolator(cp + v2s16(0, 1), m_chunk_side, grid);
	m_int_z_ne = makeZInterpolator(cp + v2s16(1, 1), m_chunk_side, grid);
}

f64 ChunkInterpolator::operator()(v2s16 pos) const
{
	s16 x = pos.X, z = pos.Y;
	
	f64 y_e, y_n, y_w, y_s;
	if (x < m_mid_x) {
		y_n = m_int_x_nw(x);
		y_s = m_int_x_sw(x);
	} else {
		y_n = m_int_x_ne(x);
		y_s = m_int_x_se(x);
	}
	if (z < m_mid_z) {
		y_e = m_int_z_se(z);
		y_w = m_int_z_sw(z);
	} else {
		y_e = m_int_z_ne(z);
		y_w = m_int_z_nw(z);
	}

	f64 w_e = square(m_chunk_side - (m_max_x - x));
	f64 w_n = square(m_chunk_side - (m_max_z - z));
	f64 w_w = square(m_chunk_side - (x - m_min_x));
	f64 w_s = square(m_chunk_side - (z - m_min_z));

	f64 y_avg = (y_e * w_e + y_n * w_n + y_w * w_w + y_s * w_s) / (w_e + w_n + w_w + w_s);
	return y_avg;
}

} // end private namespace

MapgenDFImport::MapgenDFImport(MapgenParams *params, EmergeParams *emerge):
	MapgenBasic(MAPGEN_DFIMPORT, params, emerge),
	m_chunk_side(params->chunksize * MAP_BLOCKSIZE)
{
	// 2D noise
		//new Noise(&params->np_height_select,   seed, csize.X, csize.Z);
	NoiseParams np;
	noise_filler_depth = new Noise(&np, seed, 5, 5);
		//new Noise(&params->np_filler_depth,    seed, csize.X, csize.Z);

	np.spread = v3f(6, 6, 6);
	np.octaves = 3;
	np.scale = 5;
	m_heat_noise = new Noise(&np, seed + 2, m_chunk_side, m_chunk_side);

	std::ifstream terrain_in("el.dat", std::ios::binary);
	readGrid(terrain_in, m_terrain_dims, m_terrain);
	for (s32 i = 0; i < (s32) m_terrain_dims.X * m_terrain_dims.Y; ++i) {
		m_terrain[i] = (m_terrain[i] - 98) * 2;
	}

	std::ifstream heat_in("tmp.dat", std::ios::binary);
	readGrid(heat_in, m_heat_dims, m_heat);
	for (s32 i = 0; i < (s32) m_heat_dims.X * m_heat_dims.Y; ++i) {
		m_heat[i] = std::round((m_heat[i] - 91) * 1.5);
	}

	std::ifstream humidity_in("rain.dat", std::ios::binary);
	readGrid(humidity_in, m_humidity_dims, m_humidity);
	for (s32 i = 0; i < (s32) m_humidity_dims.X * m_humidity_dims.Y; ++i) {
		m_humidity[i] = m_humidity[i] * 1;
	}
}

MapgenDFImport::~MapgenDFImport()
{
	delete[] m_humidity;
	delete[] m_heat;
	delete[] m_terrain;
	delete m_heat_noise;
}

void MapgenDFImport::makeChunk(BlockMakeData *data)
{
	// Pre-conditions
	assert(data->vmanip);
	assert(data->nodedef);

	this->generating = true;
	this->vm   = data->vmanip;
	this->ndef = data->nodedef;

	v3s16 blockpos_min = data->blockpos_min;
	v3s16 blockpos_max = data->blockpos_max;
	node_min = blockpos_min * MAP_BLOCKSIZE;
	node_max = (blockpos_max + v3s16(1, 1, 1)) * MAP_BLOCKSIZE - v3s16(1, 1, 1);
	full_node_min = (blockpos_min - 1) * MAP_BLOCKSIZE;
	full_node_max = (blockpos_max + 2) * MAP_BLOCKSIZE - v3s16(1, 1, 1);

	blockseed = getBlockSeed2(full_node_min, seed);

	{
		Grid grid(m_terrain_dims, m_terrain, -98 * 2);
		ChunkInterpolator interp(node_min, node_max, m_chunk_side, grid);
		u32 i = 0;
		for (s16 z = node_min.Z; z <= node_max.Z; ++z)
		for (s16 x = node_min.X; x <= node_max.X; ++x, ++i) {
			heightmap[i] = std::round(interp(v2s16(x, z)));
		}
	}

	{
		MapNode n_air(CONTENT_AIR);
		MapNode n_water(c_water_source);
		MapNode n_stone(c_stone);

		for (s16 z = node_min.Z; z <= node_max.Z; ++z)
		for (s16 y = node_min.Y; y <= node_max.Y; ++y) {
			u32 i2d = (z - node_min.Z) * m_chunk_side;
			u32 i3d = vm->m_area.index(node_min.X, y, z);
			for (s16 x = node_min.X; x <= node_max.X; ++x, ++i2d, ++i3d) {
				i3d = vm->m_area.index(x, y, z);
				if (vm->m_data[i3d].getContent() == CONTENT_IGNORE) {
					s16 height = heightmap[i2d];
					vm->m_data[i3d] = y <= height ? n_stone : y <= 1 ? n_water : n_air;
				}
			}
		}
	}

	// Generate base and mountain terrain
	s16 stone_surface_max_y = node_max.Y;

	// Init biome generator, place biome-specific nodes, and build biomemap
	if (flags & MG_BIOMES) {
		BiomeGenOriginal *bg = (BiomeGenOriginal *) biomegen;
		{
			Grid grid(m_heat_dims, m_heat, 0);
			ChunkInterpolator interp(node_min, node_max, m_chunk_side, grid);
			float *noise = m_heat_noise->perlinMap2D(node_min.X, node_min.Z);
			u32 i = 0;
			for (s16 z = node_min.Z; z <= node_max.Z; ++z)
			for (s16 x = node_min.X; x <= node_max.X; ++x, ++i) {
				bg->heatmap[i] = interp(v2s16(x, z)) + noise[i];
			}
		}
		{
			Grid grid(m_humidity_dims, m_humidity, 0);
			ChunkInterpolator interp(node_min, node_max, m_chunk_side, grid);
			u32 i = 0;
			for (s16 z = node_min.Z; z <= node_max.Z; ++z)
			for (s16 x = node_min.X; x <= node_max.X; ++x, ++i) {
				bg->humidmap[i] = interp(v2s16(x, z));
			}
		}
		//bg->calcBiomeNoise(node_min);
		generateBiomes();
	}

	// Generate tunnels, caverns and large randomwalk caves
	if (flags & 0) {
		// Generate tunnels first as caverns confuse them
		generateCavesNoiseIntersection(stone_surface_max_y);

		// Generate caverns
		bool near_cavern = generateCavernsNoise(stone_surface_max_y);

		// Generate large randomwalk caves
		if (near_cavern)
			// Disable large randomwalk caves in this mapchunk by setting
			// 'large cave depth' to world base. Avoids excessive liquid in
			// large caverns and floating blobs of overgenerated liquid.
			generateCavesRandomWalk(stone_surface_max_y,
				-MAX_MAP_GENERATION_LIMIT);
		else
			generateCavesRandomWalk(stone_surface_max_y, large_cave_depth);
	}


	// Generate the registered ores
	if (flags & MG_ORES)
		m_emerge->oremgr->placeAllOres(this, blockseed, node_min, node_max);

	// Generate dungeons
	if (flags & MG_DUNGEONS)
		generateDungeons(stone_surface_max_y);

	// Generate the registered decorations
	if (flags & MG_DECORATIONS)
		m_emerge->decomgr->placeAllDecos(this, blockseed, node_min, node_max);

	// Sprinkle some dust on top after everything else was generated
	if (flags & MG_BIOMES)
		dustTopNodes();

	// Calculate lighting
	if (flags & MG_LIGHT)
		calcLighting(node_min - v3s16(1, 1, 1) * MAP_BLOCKSIZE,
			node_max + v3s16(1, 0, 1) * MAP_BLOCKSIZE, full_node_min, full_node_max);

	this->generating = false;
}

int MapgenDFImport::getSpawnLevelAtPoint(v2s16 p)
{
	// TODO
	return 10;
}
