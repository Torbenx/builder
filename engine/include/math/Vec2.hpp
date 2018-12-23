#ifndef HEADER_MATH_VEC2_H
#define HEADER_MATH_VEC2_H

class Vec2{
	public:
		float x,y;
		Vec2();
		Vec2(const Vec2& xy);
		Vec2(float xy);
		Vec2(float x,float y);
		void set(float x,float y);
		void set(float xy);
		void set(Vec2& xy);
		void floor();
		void ceil();
		void fract();
		float length();
		void normalize();
		Vec2 operator+=(Vec2 right);
		Vec2 operator+=(float right);
		Vec2 operator-=(Vec2 right);
		Vec2 operator-=(float right);
		Vec2 operator*=(Vec2 right);
		Vec2 operator*=(float right);
		Vec2 operator/=(Vec2 right);
		Vec2 operator/=(float right);
		Vec2 operator=(Vec2 right);
		void debug();
};

float dot(Vec2 left,Vec2 right);

Vec2 operator+(Vec2 left,Vec2 right);
Vec2 operator+(Vec2 left,float right);

Vec2 operator*(Vec2 left,Vec2 right);
Vec2 operator*(Vec2 left,float right);
Vec2 operator*(float left,Vec2 right);

Vec2 operator/(Vec2 left,Vec2 right);
Vec2 operator/(Vec2 left,float right);

Vec2 operator-(Vec2 left,Vec2 right);
Vec2 operator-(Vec2 left,float right);

bool operator==(Vec2 left,Vec2 right);
bool operator!=(Vec2 left,Vec2 right);

#endif
