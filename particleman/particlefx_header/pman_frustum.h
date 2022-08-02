/*
* reGS: reverse-engineered GoldSrc engine
* File: pman_frustum.h
*/

class CFrustum
{
private:
	float g_flFrustum[6][4]; // in IDA it was in struct

public:
	void CalculateFrustum();
	void NormalizeFrustumPlane(float** frustum, int side);
	bool PlaneInsideFrustum(float x, float y, float z, float size);
	bool PointInsideFrustum(float x, float y, float z);
	bool SphereInsideFrustum(float x, float y, float z, float radius);
};

enum FrustumSide // : __int32
{
	RIGHT = 0, // 0x0
	LEFT = 1, // 0x1
	BOTTOM = 2, // 0x2
	TOP = 3, // 0x3
	BACK = 4, // 0x4
	FRONT = 5, // 0x5
};

enum PlaneData // : __int32
{
	A = 0, // 0x0
	B = 1, // 0x1
	C = 2, // 0x2
	D = 3, // 0x3
};