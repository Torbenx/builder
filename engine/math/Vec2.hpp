#pragma once

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
        Vec2 floor();
        Vec2 ceil();
        Vec2 fract();
        float length();
        Vec2 normalize();
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
        static float dot(Vec2 left, Vec2 right);
};

Vec2 operator+(Vec2 left,Vec2 right);
Vec2 operator+(Vec2 left,float right);

Vec2 operator*(Vec2 left,Vec2 right);
Vec2 operator*(Vec2 left,float right);
Vec2 operator*(float left,Vec2 right);

Vec2 operator/(Vec2 left,Vec2 right);
Vec2 operator/(Vec2 left,float right);

Vec2 operator-(Vec2 left,Vec2 right);
Vec2 operator-(Vec2 left,float right);

Vec2 operator-(Vec2 v);
Vec2 operator+(Vec2 v);

bool operator==(Vec2 left,Vec2 right);
bool operator!=(Vec2 left,Vec2 right);
