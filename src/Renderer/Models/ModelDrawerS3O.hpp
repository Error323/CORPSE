#ifndef PFFG_MODELDRAWERS3O_HDR
#define PFFG_MODELDRAWERS3O_HDR

#include "./ModelDrawerBase.hpp"

class CModelDrawerS3O: public CModelDrawerBase {
	public:
		void Init(PieceBase*); // GenListHierarchy
		void Draw(const PieceBase*) const; // DrawLists
		void Free(const PieceBase*) const;

	private:
		void DrawPieceArrays(PieceBase*);
		void GenListsRec(PieceBase*);
};

extern CModelDrawerS3O* drawerS3O;

#endif
