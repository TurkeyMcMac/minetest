#include "mapgen_dfimport.h"
#include "cavegen.h"
#include "emerge.h"
#include "map.h"
#include "mapnode.h"
#include "mg_biome.h"
#include "mg_decoration.h"
#include "mg_ore.h"
#include <cmath>

// TODO: IDK why this is necessary or what the root cause of discontinuity is.
#define BREAK_BIAS (-6)

#define WIDTH 272
#define HEIGHT 272

// TODO: figure out width-height order
s16 m_terrain[WIDTH][HEIGHT] =
#include "df_terrain.inc"
;

MapgenDFImport::MapgenDFImport(MapgenParams *params, EmergeParams *emerge):
	MapgenBasic(MAPGEN_DFIMPORT, params, emerge)
{
	// 2D noise
		//new Noise(&params->np_height_select,   seed, csize.X, csize.Z);
	NoiseParams np;
	noise_filler_depth = new Noise(&np, seed, 5, 5);
		//new Noise(&params->np_filler_depth,    seed, csize.X, csize.Z);
}

MapgenDFImport::Interpolator::Interpolator(f64 x, f64 y, f64 r, f64 sl, f64 sr)
{
	f64 xl = x - r;
	f64 yl = y - sl * r;
	m_a = (sr - sl) / 4 / r;
	m_b = sl - 2 * m_a * xl;
	m_c = yl - m_a * xl * xl - m_b * xl;
}

s16 MapgenDFImport::getElevation(v2s16 cp)
{
	s16 z = -cp.Y + HEIGHT / 2;
	s16 x = cp.X + WIDTH / 2;
	s16 el = z >= 0 && z < HEIGHT && x >= 0 && x < WIDTH ? m_terrain[z][x] : 0;
	return (el - 98) * 2;
}

MapgenDFImport::Interpolator MapgenDFImport::makeXInterpolator(v2s16 cp)
{
	f64 el = getElevation(cp);
	f64 el_m = getElevation(cp - v2s16(1, 0));
	f64 el_p = getElevation(cp + v2s16(1, 0));
	return Interpolator((cp.X + 0.5) * m_chunk_side, el, m_chunk_side / 2, (el - el_m) / m_chunk_side, (el_p - el) / m_chunk_side);
}

MapgenDFImport::Interpolator MapgenDFImport::makeZInterpolator(v2s16 cp)
{
	f64 el = getElevation(cp);
	f64 el_m = getElevation(cp - v2s16(0, 1));
	f64 el_p = getElevation(cp + v2s16(0, 1));
	return Interpolator((cp.Y + 0.5) * m_chunk_side, el, m_chunk_side / 2, (el - el_m) / m_chunk_side, (el_p - el) / m_chunk_side);
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

	//v3s16 cp3d = EmergeManager::getContainingChunk(blockpos_min, m_chunk_side / MAP_BLOCKSIZE);
	v2s16 cp(std::floor((f64) node_min.X / m_chunk_side), std::floor((f64) node_min.Z / m_chunk_side));

	v2s16 midp(node_min.X + m_chunk_side / 2, node_min.Z + m_chunk_side / 2);
	
	Interpolator int_x_sw = makeXInterpolator(cp);
	Interpolator int_x_se = makeXInterpolator(cp + v2s16(1, 0));
	Interpolator int_x_nw = makeXInterpolator(cp + v2s16(0, 1));
	Interpolator int_x_ne = makeXInterpolator(cp + v2s16(1, 1));
	Interpolator int_z_sw = makeZInterpolator(cp);
	Interpolator int_z_se = makeZInterpolator(cp + v2s16(1, 0));
	Interpolator int_z_nw = makeZInterpolator(cp + v2s16(0, 1));
	Interpolator int_z_ne = makeZInterpolator(cp + v2s16(1, 1));

	MapNode n_air(CONTENT_AIR);
	MapNode n_water(c_water_source);
	MapNode n_stone(c_stone);

	for (s16 z = node_min.Z; z <= node_max.Z; ++z)
	for (s16 x = node_min.X; x <= node_max.X; ++x) {
		f64 y_e = (z < midp.Y + BREAK_BIAS ? int_z_se : int_z_ne)(z);
		f64 y_n = (x < midp.X + BREAK_BIAS ? int_x_nw : int_x_ne)(x);
		f64 y_w = (z < midp.Y + BREAK_BIAS ? int_z_sw : int_z_nw)(z);
		f64 y_s = (x < midp.X + BREAK_BIAS ? int_x_sw : int_x_se)(x);

		f64 w_e = (m_chunk_side - (node_max.X - x)) * (m_chunk_side - (node_max.X - x));
		f64 w_n = (m_chunk_side - (node_max.Z - z)) * (m_chunk_side - (node_max.Z - z));
		f64 w_w = (m_chunk_side - (x - node_min.X)) * (m_chunk_side - (x - node_min.X));
		f64 w_s = (m_chunk_side - (z - node_min.Z)) * (m_chunk_side - (z - node_min.Z));

		f64 y_avg = (y_e * w_e + y_n * w_n + y_w * w_w + y_s * w_s) / (w_e + w_n + w_w + w_s);
		s16 y_stone = std::round(y_avg);
		s16 y = node_max.Y;
		for (; y > 1 && y > y_stone && y >= node_min.Y; --y) {
			u32 i = vm->m_area.index(x, y, z);
			if (vm->m_data[i].getContent() == CONTENT_IGNORE)
				vm->m_data[i] = n_air;
		}
		for (; y > y_stone && y >= node_min.Y; --y) {
			vm->m_data[vm->m_area.index(x, y, z)] = n_water;
		}
		for (; y >= node_min.Y; --y) {
			vm->m_data[vm->m_area.index(x, y, z)] = n_stone;
		}
	}

	// Generate base and mountain terrain
	s16 stone_surface_max_y = node_max.Y;

	// Create heightmap
	updateHeightmap(node_min, node_max);

	// Init biome generator, place biome-specific nodes, and build biomemap
	if (flags & MG_BIOMES) {
		biomegen->calcBiomeNoise(node_min);
		generateBiomes();
	}

	// Generate tunnels, caverns and large randomwalk caves
	if (flags & MG_CAVES) {
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
	if (1)
		calcLighting(node_min - v3s16(1, 1, 1) * MAP_BLOCKSIZE,
			node_max + v3s16(1, 0, 1) * MAP_BLOCKSIZE, full_node_min, full_node_max);

	this->generating = false;
}

int MapgenDFImport::getSpawnLevelAtPoint(v2s16 p)
{
	// TODO
	return 10;
}
