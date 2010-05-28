#ifndef PFFG_MODELDRAWERBASE_HDR
#define PFFG_MODELDRAWERBASE_HDR

struct PieceBase;
class CModelDrawerBase {
public:
	virtual ~CModelDrawerBase() {}
	virtual void Init(PieceBase*) = 0;
	virtual void Draw(const PieceBase*) const = 0;
	virtual void Free(const PieceBase*) const = 0;
};

#endif
