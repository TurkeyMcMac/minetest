#pragma once

#include "mapgen.h"
#include "irrlichttypes.h"

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

	~MapgenDFImport() = default;

	virtual MapgenType getType() const { return MAPGEN_DFIMPORT; }

	void makeChunk(BlockMakeData *data);

	int getSpawnLevelAtPoint(v2s16 p);

private:
	s32 m_chunk_side = 80;

	class Interpolator
	{
	public:
		Interpolator(f64 x, f64 y, f64 r, f64 sl, f64 sr);

		~Interpolator() = default;

		f64 operator()(f64 u) { return m_a * u * u + m_b * u + m_c; }

	private:
		f64 m_a, m_b, m_c;
	};

	s16 getElevation(v2s16 cp);

	Interpolator makeXInterpolator(v2s16 cp);

	Interpolator makeZInterpolator(v2s16 cp);
};
