#ifndef PFFG_OBJECT_GRID_HDR
#define PFFG_OBJECT_GRID_HDR

#include <vector>

#include "../Math/vec3fwd.hpp"
#include "../Math/vec3.hpp"
#ifdef PFFG_DEBUG
#include "../System/Debugger.hpp"
#endif

template<typename T> class SimObjectGrid {
public:
	struct GridCell {
	public:
		typename std::list<T>::iterator AddObject(T object) {
			objects.push_back(object); return --(objects.end());
		}
		void DelObject(const typename std::list<T>::iterator& it) { objects.erase(it); }
		const std::list<T>& GetObjects() const { return objects; }

		void SetListIt(const typename std::list<GridCell*>::iterator& it) { listIt = it; }
		const typename std::list<GridCell*>::iterator& GetListIt() const { return listIt; }

		unsigned int GetSize() const { return objects.size(); }
		bool IsEmpty() const { return objects.empty(); }
	private:
		typename std::list<T> objects;
		typename std::list<GridCell*>::iterator listIt;
	};

	static SimObjectGrid<T>* GetInstance(const vec3i& size, const vec3f& gmins, const vec3f& gmaxs) {
		static SimObjectGrid<T>* grid = NULL;
		static unsigned int depth = 0;

		if (grid == NULL) {
			ASSERT(depth == 0);

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

		ASSERT(gsize.x > 0); csize.x = (maxs.x - mins.x) / gsize.x;
		ASSERT(gsize.y > 0); csize.y = (maxs.y - mins.y) / gsize.y;
		ASSERT(gsize.z > 0); csize.z = (maxs.z - mins.z) / gsize.z;
	}
	~SimObjectGrid() {
		cells.clear();
		nonEmptyCells.clear();
	}

	const std::list<GridCell*>& GetNonEmptyCells() const { return nonEmptyCells; }

	// get all objects in the CUBE of cells within <radii> of <pos>
	void GetObjects(const vec3f& pos, const vec3f& radii, std::list<T>& objects) {
		const vec3i& cellIdx = GetCellIdx(pos, true);
		const vec3f& cellSize = GetCellSize();
		const vec3i numCells((radii.x / cellSize.x) + 1, (radii.y / cellSize.y) + 1, (radii.z / cellSize.z) + 1);

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



	void AddObject(T object, std::map< unsigned int, typename std::list<T>::iterator >& objCells) {
		typedef typename std::list<T>::iterator ListIt;
		typedef typename std::map<unsigned int, ListIt>::iterator MapListIt;

		const vec3f& objPos = object->GetPos();
		const float objRad = object->GetRadius();

		const vec3f minPos(objPos.x - objRad, objPos.y - objRad, objPos.z - objRad);
		const vec3f maxPos(objPos.x + objRad, objPos.y + objRad, objPos.z + objRad);

		const vec3i minIdx = GetCellIdx(minPos, !PosInBounds(minPos));
		const vec3i maxIdx = GetCellIdx(maxPos, !PosInBounds(maxPos));

		// add object to all overlapped cells
		for (int x = minIdx.x; x <= maxIdx.x; x++) {
			for (int y = minIdx.y; y <= maxIdx.y; y++) {
				for (int z = minIdx.z; z <= maxIdx.z; z++) {
					GridCell& cell = GetCell(vec3i(x, y, z));

					if (cell.IsEmpty()) {
						nonEmptyCells.push_back(&cell);
						cell.SetListIt(--(nonEmptyCells.end()));
					}

					objCells[ z * (gsize.y * gsize.z) + y * (gsize.y) + x ] = cell.AddObject(object);
				}
			}
		}

		/*
		// for an object position outside the bounding-box,
		// the cell indices must be computed WITH clamping
		const vec3f& pos = object->GetPos();
		const vec3i& idx = GetCellIdx(pos, !PosInBounds(pos));

		const unsigned int idx1D = idx.z * (gsize.y * gsize.z) + idx.y * (gsize.y) + idx.x;
		const MapListIt objCellsIt = objCells.find(idx1D);
			ASSERT(objCellsIt == objCells.end());

		GridCell& cell = GetCell(idx);

		if (cell.IsEmpty()) {
			nonEmptyCells.push_back(&cell);
			cell.SetListIt(--(nonEmptyCells.end()));
		}

		objCells[idx1D] = cell.AddObject(object);
		*/
	}

	void DelObject(T object, std::map<unsigned int, typename std::list<T>::iterator>& objCells) {
		typedef typename std::list<T>::iterator ListIt;
		typedef typename std::map<unsigned int, ListIt>::iterator MapListIt;

		// note: redundant
		object = object;

		// delete object from all overlapped cells
		for (MapListIt it = objCells.begin(); it != objCells.end(); ++it) {
			GridCell& cell = cells[it->first];
			cell.DelObject(it->second);

			if (cell.IsEmpty()) {
				nonEmptyCells.erase(cell.GetListIt());
			}

			objCells.erase(it->first);
		}

		/*
		const vec3f& pos = object->GetPos();
		const vec3i& idx = GetCellIdx(pos, !PosInBounds(pos));

		const unsigned int idx1D = idx.z * (gsize.y * gsize.z) + idx.y * (gsize.y) + idx.x;
		const MapListIt objCellsIt = objCells.find(idx1D);
			ASSERT(objCellsIt != objCells.end());
		const ListIt& objectIt = objCellsIt->second;

		GridCell& cell = GetCell(idx);
		cell.DelObject(objectIt);

		if (cell.IsEmpty()) {
			nonEmptyCells.erase(cell.GetListIt());
		}

		objCells.erase(idx1D);
		*/
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
	// list of all currently non-empty cells
	std::list<GridCell*> nonEmptyCells;

	// number of cells along each dimension
	vec3i gsize;
	// spatial size per dimension of each cell
	vec3f csize;

	// spatial extends of the grid
	vec3f mins;
	vec3f maxs;
};

#endif
