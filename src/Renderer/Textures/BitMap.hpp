#ifndef BITMAP_HDR
#define BITMAP_HDR

#include <string>
// #include "./NVDDS.hpp"

namespace nv_dds {
	class CDDSImage;
}

class CBitMap {
	public:
		CBitMap(unsigned char* data, int xsize, int ysize);
		CBitMap();
		CBitMap(const CBitMap& old);
		CBitMap& operator = (const CBitMap& bm);
		virtual ~CBitMap();

		void Alloc(int w, int h);
		bool Load(const std::string& filename, unsigned char defaultAlpha = 255);
		bool LoadGrayscale(const std::string& filename);
		bool Save(const std::string& filename, bool opaque = true);

		unsigned int CreateTexture(bool mipmaps = false);
		unsigned int CreateDDSTexture();

		void CreateAlpha(unsigned char r, unsigned char g, unsigned char b);
		void SetTransparent(unsigned char r, unsigned char g, unsigned char b);

		// void Renormalize(vec3 newCol);

		CBitMap GetRegion(int startx, int starty, int width, int height);
		CBitMap CreateMipMapLevel(void);

		unsigned char* mem;
		int xsize;
		int ysize;

		enum BitmapType {
			BitmapTypeStandardRGBA,
			BitmapTypeStandardAlpha,
			BitmapTypeDDS
		};

		int type;
		nv_dds::CDDSImage* ddsimage;

		CBitMap CreateRescaled(int newx, int newy);
		void ReverseYAxis();
		void InvertColors();
		void GrayScale();
		void Tint(const float tint[3]);
};

#endif
