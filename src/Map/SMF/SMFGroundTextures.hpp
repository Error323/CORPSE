#ifndef PFFG_SMFGROUNDTEXTURES_HDR
#define PFFG_SMFGROUNDTEXTURES_HDR

class CFileHandler;
class CSMFReadMap;
struct Camera;

class CSMFGroundTextures {
	public:
		CSMFGroundTextures(CSMFReadMap*);
		~CSMFGroundTextures(void);
		void SetTexture(int x, int y);
		void DrawUpdate(const Camera*);
		void LoadSquare(int x, int y, int level);

	protected:
		CSMFReadMap* smfMap;

		const int bigSquareSize;
		const int numBigTexX;
		const int numBigTexY;

		int* textureOffsets;

		struct GroundSquare {
			/*GLu*/ unsigned int textureID;
			int texLevel;
			unsigned int lastUsed;
		};

		GroundSquare* squares;

		int*  tileMap;
		int   tileSize;
		char* tiles;
		int   tileMapXSize;
		int   tileMapYSize;

		// use Pixel Buffer Objects for async. uploading (DMA)
		/*GLu*/ unsigned int pboIDs[10];
		bool usePBO;
		int  currentPBO;

		float anisotropy;

		inline bool TexSquareInView(const Camera*, int, int);
};

#endif
