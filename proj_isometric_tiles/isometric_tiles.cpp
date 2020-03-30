#define OLC_PGE_APPLICATION
#include "olcPixelGameEngine.h"

class Isometric_Tiles : public olc::PixelGameEngine {

private:
	// Number of tiles in world
	olc::vi2d vWorldSize = { 15, 11 };

	// Size of single tile graphic
	olc::vi2d vTileSize = { 40, 20 };

	// Where to place tile (0,0) on screen (in tile size steps)
	olc::vi2d vOrigin = { 5, 1 };

	// Sprite that holds all imagery
	olc::Sprite *sprIsom = nullptr;

	// Pointer to create 2D world array
	int *pWorld = nullptr;

public:
	Isometric_Tiles() {
		sAppName = "Isometric Tiles";
	}

	bool OnUserCreate() override {

		sprIsom = new olc::Sprite("tiles.png");
		pWorld = new int[vWorldSize.x * vWorldSize.y]{ 0 };

		return true;
	}

	bool OnUserUpdate(float fElapsedTime) override {
		
		Clear(olc::WHITE);

		olc::vi2d vMouse = { GetMouseX(), GetMouseY() };
		olc::vi2d vCell = { vMouse.x / vTileSize.x, vMouse.y / vTileSize.y };

		olc::vi2d vOffset = { vMouse.x % vTileSize.x, vMouse.y % vTileSize.y };
		olc::Pixel col = sprIsom->GetPixel(3 * vTileSize.x + vOffset.x, vOffset.y);

		olc::vi2d vSelected = {
			(vCell.y - vOrigin.y) + (vCell.x - vOrigin.x),
			(vCell.y - vOrigin.y) - (vCell.x - vOrigin.x)
		};

		auto ToScreen = [&](int x, int y) {
			return olc::vi2d {
				(vOrigin.x * vTileSize.x) + (x - y) * (vTileSize.x / 2),
				(vOrigin.y * vTileSize.y) + (x + y) * (vTileSize.y / 2)
			};
		};

		if (col == olc::RED) vSelected += { -1, 0 };
		if (col == olc::BLUE) vSelected += { 0, -1 };
		if (col == olc::GREEN) vSelected += { 0, 1 };
		if (col == olc::YELLOW) vSelected += { 1, 0 };

		if (GetMouse(0).bPressed) {
			if (vSelected.x >=0 and vSelected.x < vWorldSize.x and vSelected.y >= 0 and vSelected.y < vWorldSize.y)
				++pWorld[vSelected.y * vWorldSize.x + vSelected.x] %= 6;
		}	

		// Skip pixels with transparency
		SetPixelMode(olc::Pixel::MASK);
		
		for (int y = 0; y < vWorldSize.y; y++) {
			for (int x = 0; x < vWorldSize.x; x++) {
				olc::vi2d vWorld = ToScreen(x, y);

				switch (pWorld[y * vWorldSize.x + x]) {
					case 0:
						// Invisble Tile
						DrawPartialSprite(vWorld.x, vWorld.y, sprIsom, 1 * vTileSize.x, 0, vTileSize.x, vTileSize.y);
						break;

					case 1:
						// Visible Tile
						DrawPartialSprite(vWorld.x, vWorld.y, sprIsom, 2 * vTileSize.x, 0, vTileSize.x, vTileSize.y);
						break;
					
					case 2:
						// Tree
						DrawPartialSprite(vWorld.x, vWorld.y - vTileSize.y, sprIsom, 0 * vTileSize.x, 1 * vTileSize.y, vTileSize.x, vTileSize.y * 2);
						break;
					
					case 3:
						// Spooky Tree
						DrawPartialSprite(vWorld.x, vWorld.y - vTileSize.y, sprIsom, 1 * vTileSize.x, 1 * vTileSize.y, vTileSize.x, vTileSize.y * 2);
						break;
					
					case 4:
						// Beach
						DrawPartialSprite(vWorld.x, vWorld.y - vTileSize.y, sprIsom, 2 * vTileSize.x, 1 * vTileSize.y, vTileSize.x, vTileSize.y * 2);
						break;
					
					case 5:
						// Water
						DrawPartialSprite(vWorld.x, vWorld.y - vTileSize.y, sprIsom, 3 * vTileSize.x, 1 * vTileSize.y, vTileSize.x, vTileSize.y * 2);
						break;
					
					default:
						break;
				}

			}
		}

		SetPixelMode(olc::Pixel::ALPHA);

		olc::vi2d vSelectedWorld = ToScreen(vSelected.x, vSelected.y);
		DrawPartialSprite(vSelectedWorld.x, vSelectedWorld.y, sprIsom, 0 * vTileSize.x, 0, vTileSize.x, vTileSize.y);


		SetPixelMode(olc::Pixel::NORMAL);

		// Draw Hovered Cell Boundary
		// DrawRect(vCell.x * vTileSize.x, vCell.y * vTileSize.y, vTileSize.x, vTileSize.y, olc::RED);
				
		// Draw Debug Info
		DrawString(4, 4, "Mouse		: " + std::to_string(vMouse.x) + ", " + std::to_string(vMouse.y), olc::BLACK);
		DrawString(4, 14, "Cell		: " + std::to_string(vCell.x) + ", " + std::to_string(vCell.y), olc::BLACK);
		DrawString(4, 24, "Selected	: " + std::to_string(vSelected.x) + ", "  + std::to_string(vSelected.y), olc::BLACK);

		return true;
	}
};

int main() {
	Isometric_Tiles app;
	if (app.Construct(520, 300, 2, 2))
		app.Start();

	return 0;
}
