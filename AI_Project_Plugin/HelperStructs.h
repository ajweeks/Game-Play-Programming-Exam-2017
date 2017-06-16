#pragma once

#include <vector>
#include <algorithm>
#include <iterator>

#undef min
#undef max

namespace SteeringBehaviours
{
	class ISteeringBehaviour;
}

//BOX2D HELPERS
//*************
#define b2_pidiv2 1.570796326795f
#define b2_pidiv4 0.78539816339f

inline b2Vec2 operator*(const b2Vec2& a, const b2Vec2& b)
{
	return{ a.x*b.x, a.y*b.y };
}

inline b2Vec2 operator*(const b2Vec2& a, float b)
{
	return{ a.x*b, a.y*b };
}

inline b2Vec2 operator/(const b2Vec2& a, float b)
{
	return{ a.x / b, a.y / b };
}

inline b2Vec2 operator*(const b2Vec2& a, const b2Mat22& m)
{
	return b2Vec2(m.ex.x * a.x + m.ey.x * a.y,
		m.ex.y * a.x + m.ey.y * a.y);
}

//MATH HELPERS
//************
inline float ToRadians(const float angle)
{
	return angle * (b2_pi / 180.f);
}

inline float randomFloat(float max = 1.f)
{
	return max * (float(rand()) / RAND_MAX);
}

inline float randomBinomial(float max = 1.f)
{
	return randomFloat(max) - randomFloat(max);
}

inline b2Vec2 randomVector2(float max = 1.f)
{
	return{ randomBinomial(max),randomBinomial(max) };
}

inline b2Vec2 OrientationToVector(float orientation)
{
	orientation -= b2_pi;
	orientation /= 2.f;
	return b2Vec2(cos(orientation), sin(orientation));
}

inline float GetOrientationFromVelocity(b2Vec2 velocity)
{
	if (velocity.Length() == 0)
		return 0.f;

	return atan2f(velocity.x, -velocity.y);
}

inline b2Vec2 Clamp(const b2Vec2& a, float max)
{
	auto scale = max / a.Length();
	scale = scale < 1.f ? scale : 1.f;
	return a * scale;
}

inline float Distance(b2Vec2 a, b2Vec2 b)
{
	return sqrt((a.x - b.x) * (a.x - b.x) + (a.y - b.y) * (a.y - b.y));
}

inline b2Vec2 abs(const b2Vec2 a)
{
	return b2Vec2(abs(a.x), abs(a.y));
}

inline bool PointInTriangleBoundingBox(const b2Vec2& point, const b2Vec2& currentTip, const b2Vec2& previous, const b2Vec2& next)
{
	auto xMin = std::min(currentTip.x, std::min(previous.x, next.x)) - FLT_EPSILON;
	auto xMax = std::max(currentTip.x, std::max(previous.x, next.x)) + FLT_EPSILON;
	auto yMin = std::min(currentTip.y, std::min(previous.y, next.y)) - FLT_EPSILON;
	auto yMax = std::max(currentTip.y, std::max(previous.y, next.y)) + FLT_EPSILON;

	if (point.x < xMin || xMax < point.x || point.y < yMin || yMax < point.y)
		return false;
	return true;
}

inline float DistanceSquarePointToLine(const b2Vec2& p1, const b2Vec2& p2, const b2Vec2& point)
{
	//http://totologic.blogspot.be/2014/01/accurate-point-in-triangle-test.html
	auto p1p2_squareLength = (p2.x - p1.x)*(p2.x - p1.x) + (p2.y - p1.y)*(p2.y - p1.y);
	auto dp = ((point.x - p1.x)*(p2.x - p1.x) + (point.y - p1.y)*(p2.y - p1.y)) / p1p2_squareLength;
	if (dp < 0)
		return (point.x - p1.x) * (point.x - p1.x) + (point.y - p1.y) *  (point.y - p1.y);
	else if (dp <= 1)
	{
		auto pp1_squareLength = (p1.x - point.x)*(p1.x - point.x) + (p1.y - point.y) * (p1.y - point.y);
		return pp1_squareLength - dp * dp * p1p2_squareLength;
	}
	else
		return (point.x - p2.x)*(point.x - p2.x) + (point.y - p2.y) * (point.y - p2.y);
}

inline bool IsPointOnLine(const b2Vec2& origin, const b2Vec2& segmentEnd, const b2Vec2& point)
{
	auto line = segmentEnd - origin;
	line.Normalize();
	//Projection
	auto w = point - origin;
	auto proj = b2Dot(w, line);
	if (proj < 0) //Not on line
		return false;

	auto vsq = b2Dot(line, line);
	if (proj > vsq) //Not on line
		return false;

	return true;
}

inline bool PointInTriangle(const b2Vec2& point, const b2Vec2& currentTip, const b2Vec2& previous, const b2Vec2& next, bool onLineAllowed = false)
{
	//Do bounding box test first
	if (!PointInTriangleBoundingBox(point, currentTip, previous, next))
		return false;

	//Reference: http://www.blackpawn.com/texts/pointinpoly/default.html
	//Compute direction vectors
	auto v0 = previous - currentTip;
	auto v1 = next - currentTip;
	auto v2 = point - currentTip;

	//Compute dot products
	auto dot00 = b2Dot(v0, v0);
	auto dot01 = b2Dot(v0, v1);
	auto dot02 = b2Dot(v0, v2);
	auto dot11 = b2Dot(v1, v1);
	auto dot12 = b2Dot(v1, v2);

	// Compute barycentric coordinates
	auto invDenom = 1 / (dot00 * dot11 - dot01 * dot01);
	auto u = (dot11 * dot02 - dot01 * dot12) * invDenom;
	auto v = (dot00 * dot12 - dot01 * dot02) * invDenom;

	// Check if point is in triangle
	if (u < 0 || v < 0 || u > 1 || v > 1 || (u + v) > 1)
	{
		if (onLineAllowed)
		{
			//Check special case where these barycentric coordinates are not enough for on line detection!
			auto epsilonSquared = (FLT_EPSILON * FLT_EPSILON) * 2;
			if (DistanceSquarePointToLine(currentTip, next, point) <= FLT_EPSILON)
				return true;
			if (DistanceSquarePointToLine(next, previous, point) <= FLT_EPSILON)
				return true;
			if (DistanceSquarePointToLine(previous, currentTip, point) <= FLT_EPSILON)
				return true;
		}

		return false;
	}
	return true;
}


struct SteeringParams
{
	b2Vec2 Position;
	float Orientation;

	b2Vec2 LinearVelocity;
	float AngularVelocity;
	//b2Vec2 NextNavMeshPosition;

	SteeringParams(b2Vec2 position = b2Vec2_zero, float orientation = 0.0f, b2Vec2 linearVel = b2Vec2_zero, 
		float angularVel = 0.0f) :
		Position(position),
		Orientation(orientation),
		LinearVelocity(linearVel),
		AngularVelocity(angularVel)
		//NextNavMeshPosition(nextNavMeshPosition)
	{}

#pragma region Functions
	void Clear()
	{
		Position.SetZero();
		LinearVelocity.SetZero();

		Orientation = 0.f;
		AngularVelocity = 0.f;
	}

	b2Vec2 GetDirection() const  //Zero Orientation > {0,-1}
	{
		return b2Vec2(cos(Orientation - b2_pidiv2), sin(Orientation - b2_pidiv2));
	}

	float GetOrientationFromVelocity() const
	{
		if (LinearVelocity.Length() == 0)
			return 0.f;

		return atan2f(LinearVelocity.x, -LinearVelocity.y);
	}
#pragma endregion

#pragma region Operator Overloads
	SteeringParams(const SteeringParams& other)
	{
		Position = other.Position;
		Orientation = other.Orientation;
		LinearVelocity = other.LinearVelocity;
		AngularVelocity = other.AngularVelocity;
		//NextNavMeshPosition = other.NextNavMeshPosition;
	}

	SteeringParams& operator=(const SteeringParams& other)
	{
		Position = other.Position;
		Orientation = other.Orientation;
		LinearVelocity = other.LinearVelocity;
		AngularVelocity = other.AngularVelocity;
		//NextNavMeshPosition = other.NextNavMeshPosition;

		return *this;
	}

	bool operator==(const SteeringParams& other) const
	{
		return
			(Position == other.Position &&
				Orientation == other.Orientation &&
				LinearVelocity == other.LinearVelocity &&
				AngularVelocity == other.AngularVelocity);
				//NextNavMeshPosition == other.NextNavMeshPosition);
	}

	bool operator!=(const SteeringParams& other) const
	{
		return !(*this == other);
	}
#pragma endregion

};

struct SteeringOutput
{
	b2Vec2 LinearVelocity = { 0.0f, 0.0f };
	float AngularVelocity = 0.0f;

	SteeringOutput(b2Vec2 linVel = { 0.0f, 0.0f }, float angVel = 0.0f)
	{
		LinearVelocity = linVel;
		AngularVelocity = angVel;
	}

	SteeringOutput& operator=(const SteeringOutput& other)
	{
		LinearVelocity += other.LinearVelocity;
		AngularVelocity += other.AngularVelocity;

		return *this;
	}

	SteeringOutput& operator*=(const SteeringOutput& other)
	{
		LinearVelocity = LinearVelocity * other.LinearVelocity;
		AngularVelocity = AngularVelocity * other.AngularVelocity;

		return *this;
	}

	SteeringOutput& operator*=(float f)
	{
		LinearVelocity = f * LinearVelocity;
		AngularVelocity = f * AngularVelocity;

		return *this;
	}

	SteeringOutput& operator/=(float f)
	{
		LinearVelocity = LinearVelocity / f;
		AngularVelocity = AngularVelocity / f;

		return *this;
	}

	//Includes the Angular component, Just to check if this output contains something
	bool IsEmpty(float epsilon = 0.0001f)
	{
		//LengthSquared VS Length (Distance checks > use LengthSquared, faster than Length)
		return (LinearVelocity.LengthSquared() + (AngularVelocity*AngularVelocity)) < (epsilon*epsilon);
	}
};




union CheapVariant
{
	CheapVariant() {}

	//INT
	int iVal;
	CheapVariant(int val) { iVal = val; }
	operator int() const { return iVal; }

	//UINT
	UINT uiVal;
	CheapVariant(UINT val) { uiVal = val; }
	operator UINT() const { return uiVal; }

	//FLOAT
	float fVal;
	CheapVariant(float val) { fVal = val; }
	operator float() const { return fVal; }

	//BOOL
	bool bVal;
	CheapVariant(bool val) { bVal = val; }
	operator bool() const { return bVal; }
};

struct PluginOutput
{
	b2Vec2 LinearVelocity = { 0.0f, 0.0f };
	float AngularVelocity = 0.0f;
	bool AutoOrientate = true;
	bool RunMode = false;
};

enum eEntityType
{
	ENEMY,
	ITEM
	// ...
	, _LASTTYPE = ENEMY //Stores the biggest value of enum type (int)
};

enum eItemType
{
	PISTOL, // Shoot Enemies (Ammo Depletion)
	HEALTH, // Increments Health
	FOOD,	// Increments Energy
	GARBAGE // Just Garbage
			// ...
	, _LASTITEM = GARBAGE //Stores the biggest value of enum type (int)
};

struct EntityInfo
{
	eEntityType Type;
	int EntityHash;

	b2Vec2 Position;
};

struct EnemyInfo
{
	int EnemyHash;
	int Health;
};

struct Enemy
{
	EnemyInfo info;
	b2Vec2 Position;
	b2Vec2 LastPosition;
	b2Vec2 Velocity;

	bool InFieldOfView;
	b2Vec2 PredictedPosition; // Used when not in FOV
};
bool operator==(const Enemy& lhs, const Enemy& rhs);

struct ItemInfo
{
	eItemType Type;
	int ItemHash;
};

struct Item
{
	ItemInfo info;
	bool valid; // False for empty items (instead of nullptr)
};
bool operator==(const Item& lhs, const Item& rhs);

struct Pistol
{
	ItemInfo itemInfo;

	b2Vec2 Position;
	int Ammo;
	float DPS;
	float Range;
	bool fresh; // True when we picked this item up this frame, false when stale (hash is then incorrect)
};
bool operator==(const Pistol& lhs, const Pistol& rhs);

struct HealthPack
{
	ItemInfo itemInfo;

	b2Vec2 Position;
	int HealingAmount;
	bool fresh; // True when we picked this item up this frame, false when stale (hash is then incorrect)
};
bool operator==(const HealthPack& lhs, const HealthPack& rhs);

struct Food
{
	ItemInfo itemInfo;

	b2Vec2 Position;
	int EnergyAmount;
	bool fresh; // True when we picked this item up this frame, false when stale (hash is then incorrect)
};
bool operator==(const Food& lhs, const Food& rhs);

struct HouseInfo
{
	b2Vec2 Center;
	b2Vec2 Size;
};
bool operator==(const HouseInfo& lhs, const HouseInfo& rhs);

struct WorldInfo
{
	b2Vec2 Center;
	b2Vec2 Dimensions;
};

struct AgentInfo
{
	float Stamina;
	float Health;
	float Energy;
	bool RunMode;
	float GrabRange;
	bool IsInHouse;
	bool Bitten;
	bool Death;

	float FOV_Angle;
	float FOV_Range;

	b2Vec2 LinearVelocity;
	float AngularVelocity;
	float CurrentLinearSpeed;
	b2Vec2 Position;
	float Orientation;
	float MaxLinearSpeed;
	float MaxAngularSpeed;
	float AgentSize;
};

struct GameDebugParams //Debuggin Purposes only (Ignored during release build)
{
	GameDebugParams() {}
	GameDebugParams(int enemyAmount, bool autoGrab, bool ignoreEnergy, bool godMode, bool overrideDifficulty = false, float difficulty = 0.5f)
	{
		EnemySpawnAmount = enemyAmount;
		AutoGrabClosestItem = autoGrab;
		IgnoreEnergy = ignoreEnergy;
		GodMode = godMode;
		Difficulty = difficulty;
		OverrideDifficulty = overrideDifficulty;
	}

	int EnemySpawnAmount = 20; //Amount of enemies to spawn
	bool AutoGrabClosestItem = false; //ITEM_GRAB auto selects the closest item
	bool IgnoreEnergy = false; //No energy depletion
	bool GodMode = false; //Enemies can't kill you
	float Difficulty = 0.5f; //Scales from 0 > ...
	bool OverrideDifficulty = false;
};

template<class T>
bool Contains(const std::vector<T>& vec, const T& t)
{
	for (size_t i = 0; i < vec.size(); i++)
	{
		if (vec[i] == t) return true;
	}

	return false;
}


template<typename T>
typename std::vector<T>::iterator IndexOf(std::vector<T>& vec, T& t)
{
	for (std::vector<T>::iterator iter = vec.begin(); iter != vec.end(); ++iter)
	{
		if (*iter == t) return iter;
	}

	return vec.end();
}
