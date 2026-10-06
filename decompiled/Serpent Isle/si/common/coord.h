#ifndef COORD_H
#define COORD_H

/* The world is 3072 cells square and wraps at its edges. */
#define WORLD_SIZE  3072

#ifdef __cplusplus
extern "C" {
#endif
int far GetGreatestDelta2D(int *x1, int *y1, int *x2, int *y2);
int far GetGreatestDelta3D(int *x1, int *y1, int z1, int *x2, int *y2, int z2);
#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
struct Coord;
struct CellCoord;
extern "C" int far CompareWorldCoords(Coord *, Coord *);

/* A world x or y, wrapped into 0..3071. */
struct Coord {
	int value;
	Coord() {}
	Coord(int n) { value = n; value = (value + WORLD_SIZE) % WORLD_SIZE; }
	Coord &operator=(const Coord &other) { value = other.value; return *this; }
	Coord &operator=(int n) { value = n; value = (value + WORLD_SIZE) % WORLD_SIZE; return *this; }
	operator int() { return value; }
	void operator+=(int n) { value += n; value = (value + WORLD_SIZE) % WORLD_SIZE; }
	void operator-=(int n) { value -= n; value = (value + WORLD_SIZE) % WORLD_SIZE; }
	void operator++() { value++; value = (value + WORLD_SIZE) % WORLD_SIZE; }
	Coord operator+(int n) { Coord c(value + n); return c; }
	Coord operator-(int n) { Coord c(value - n); return c; }
	/* how far this lies past other, the short way round */
	int operator-(Coord other) { return CompareWorldCoords(this, &other); }
	char operator<(CellCoord other);
	char operator<=(CellCoord other);
	char operator>(CellCoord other);
	char operator>=(CellCoord other);
	char operator==(const Coord &other) { return value == other.value; }
	char operator!=(const Coord &other) { return value != other.value; }
};

/* A coord passed by value. */
struct Loc {
	int value;
	Loc() {}
	Loc(int n) { value = n; }
	Loc(Coord &c) { value = c.value; }
	operator int() { return value; }
	Coord operator+(int n) { return value + n; }
	Coord operator-(int n) { return value - n; }
};

/* A coord argument: an int wraps in place, a coord or loc is copied. */
struct CellCoord : Coord {
	CellCoord(int n) { value = n; value = (value + WORLD_SIZE) % WORLD_SIZE; }
	CellCoord(const Coord &c) { value = c.value; }
	CellCoord(const Loc &l) { value = l.value; }
};

/* Order along the wrapping map; the right side is copied, so a sum is wrapped, then copied. */
inline char Coord::operator<(CellCoord other) { return CompareWorldCoords(this, &other) < 0; }
inline char Coord::operator<=(CellCoord other) { return CompareWorldCoords(this, &other) <= 0; }
inline char Coord::operator>(CellCoord other) { return CompareWorldCoords(this, &other) > 0; }
inline char Coord::operator>=(CellCoord other) { return CompareWorldCoords(this, &other) >= 0; }

/* A world coordinate wrapped into 0..3071. */
inline int WrapCoord(int v) { v = (v + WORLD_SIZE) % WORLD_SIZE; return v; }

/* The sign and size of a step. */
inline int GetSign(int d) { if (d == 0) return 0; if (d < 0) return -1; return 1; }
inline int GetMagnitude(int d) { return d < 0 ? -d : d; }

/* How far a lies past b, on the wrapping map. */
inline int GetDelta(Coord &a, Coord b) { return CompareWorldCoords(&a, &b); }

/* Flat and height-aware distance between two map points. */
inline int GetDistance(Coord x1, Coord y1, Coord x2, Coord y2)
{
	return GetGreatestDelta2D(&x1.value, &y1.value, &x2.value, &y2.value);
}
inline int GetDistance(Coord x1, Coord y1, int z1, Coord x2, Coord y2, int z2)
{
	return GetGreatestDelta3D(&x1.value, &y1.value, z1, &x2.value, &y2.value, z2);
}
#endif

#endif
