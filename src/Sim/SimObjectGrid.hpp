#ifndef PFFG_OBJECT_GRID_HDR
#define PFFG_OBJECT_GRID_HDR

#include <list>
#include <vector>

#include "../Math/vec3fwd.hpp"
#include "../Math/vec3.hpp"

template<typename T> class SimObjectGrid {
public:
	struct GridCell {
	public:
		typename std::list<T>::iterator AddObject(T object) {
			objects.push_back(object); return --(objects.end());
		}
		void DelObject(const typename std::list<T>::iterator& it) { objects.erase(it); }
		const std::list<T>& GetObjects() const { return objects; }
		unsigned int GetSize() const { return objects.size(); }
		bool IsEmpty() const { return objects.empty(); }
	private:
		std::list<T> objects;
	};

	static SimObjectGrid<T>* GetInstance(const vec3i& size, const vec3f& gmins, const vec3f& gmaxs) {
		static SimObjectGrid<T>* grid = NULL;
		static unsigned int depth = 0;

		if (grid == NULL) {
			assert(depth == 0);

			depth += 1;
			grid = new SimObjectGrid<T>(size, gmins, gmaxs);
			depth -= 1;
		}

		return grid;
	}

	static void FreeInstance(SimObjectGrid<T>* grid) {
		delete grid;
	}



	SimObjectGrid<T>(const vec3i& size, const vec3f& gmins, const vec3f& gmaxs): gsize(size), mins(gmins), maxs(gmaxs) {
		cells.resize(gsize.x * gsize.y * gsize.z, GridCell());

		assert(gsize.x > 0); csize.x = (maxs.x - mins.x) / gsize.x;
		assert(gsize.y > 0); csize.y = (maxs.y - mins.y) / gsize.y;
		assert(gsize.z > 0); csize.z = (maxs.z - mins.z) / gsize.z;
	}
	~SimObjectGrid() {
		cells.clear();
	}

	void GetObjects(const vec3f& pos, const vec3f& radii, std::list<T>& objects) {
		const vec3i& cellIdx = GetCellIdx(pos, true);
		const vec3f& cellSize = GetCellSize();
		const vec3i numCells((radii.x / cellSize.x), (radii.y / cellSize.y), (radii.z / cellSize.z));

		for (int x = cellIdx.x - numCells.x; x <= cellIdx.x + numCells.x; x++) {
			for (int y = cellIdx.y - numCells.y; y <= cellIdx.y + numCells.y; y++) {
				for (int z = cellIdx.z - numCells.z; z <= cellIdx.z + numCells.z; z++) {
					const vec3i idx(x, y, z);

					if (!IdxInBounds(idx)) {
						continue;
					}

					const GridCell& cell = GetCell(idx);
					const std::list<T>& cellObjs = cell.GetObjects();

					for (typename std::list<T>::const_iterator it = cellObjs.begin(); it != cellObjs.end(); ++it) {
						objects.push_back(*it);
					}
				}
			}
		}
	}

	typename std::list<T>::iterator AddObject(T object) {
		// for an object position outside the bounding-box,
		// the cell indices must be computed WITH clamping
		const vec3f& pos = object->GetPos();
		const vec3i& idx = GetCellIdx(pos, !PosInBounds(pos));

		GridCell& cell = GetCell(idx);
		return (cell.AddObject(object));
	}

	void DelObject(T object, const typename std::list<T>::iterator& objectIt) {
		const vec3f& pos = object->GetPos();
		const vec3i& idx = GetCellIdx(pos, !PosInBounds(pos));

		GridCell& cell = GetCell(idx);
		cell.DelObject(objectIt);
	}



	bool IdxInBounds(const vec3i& idx) const {
		return
			(idx.x >= 0 && idx.x < gsize.x) &&
			(idx.y >= 0 && idx.y < gsize.y) &&
			(idx.z >= 0 && idx.z < gsize.z);
	}
	bool PosInBounds(const vec3f& pos) const {
		return (IdxInBounds(GetCellIdx(pos, false)));
	}

	// return the index of the cell that
	// contains spatial position <pos>
	//
	// note: lack of numerical accuracy
	// can cause the indices to be OOB,
	// so optionally clamp them
	vec3i GetCellIdx(const vec3f& pos, bool clamp = false) const {
		vec3i idx;
			idx.x = int(((pos.x - mins.x) / (maxs.x - mins.x)) * gsize.x);
			idx.y = int(((pos.y - mins.y) / (maxs.y - mins.y)) * gsize.y);
			idx.z = int(((pos.z - mins.z) / (maxs.z - mins.z)) * gsize.z);

		if (clamp) {
			idx.x = std::max(0, std::min(gsize.x - 1, idx.x));
			idx.y = std::max(0, std::min(gsize.y - 1, idx.y));
			idx.z = std::max(0, std::min(gsize.z - 1, idx.z));
		}

		return idx;
	}


	// return the cell containing spatial position <pos>
	// (auto-clamps the indices corresponding to <pos>)
	GridCell& GetCell(const vec3f& pos) {
		return (GetCell(GetCellIdx(pos, true)));
	}

	// return the cell at grid index <idx>; uses
	// indexing scheme z * (W * H) + y * (W) + x
	// note: assumes index-vector is in-bounds
	GridCell& GetCell(const vec3i& idx) {
		return (cells[idx.z * (gsize.y * gsize.z) + idx.y * (gsize.y) + idx.x]);
	}


	const vec3i& GetGridSize() const { return gsize; }
	const vec3f& GetCellSize() const { return csize; }

private:
	std::vector<GridCell> cells;

	// number of cells along each dimension
	vec3i gsize;
	// spatial size per dimension of each cell
	vec3f csize;

	// spatial extends of the grid
	vec3f mins;
	vec3f maxs;
};

#endif
