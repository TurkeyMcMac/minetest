#pragma once

#include "mapgen.h"

struct MapgenDFImportParams : public MapgenParams
{
	MapgenDFImportParams() = default;

	~MapgenDFImportParams() = default;

	void readParams(const Settings *settings) {}

	void writeParams(Settings *settings) const {}
};

class MapgenDFImport : public MapgenBasic
{
public:
	MapgenDFImport(MapgenParams *params, EmergeParams *emerge);

	~MapgenDFImport();

	virtual MapgenType getType() const { return MAPGEN_DFIMPORT; }

	void makeChunk(BlockMakeData *data);

	int getSpawnLevelAtPoint(v2s16 p);

private:
	s16 m_chunk_side;
	s16 *m_terrain;
	v2s16 m_terrain_dims;
	s16 *m_heat;
	v2s16 m_heat_dims;
	s16 *m_humidity;
	v2s16 m_humidity_dims;
	Noise *m_heat_noise;
};
