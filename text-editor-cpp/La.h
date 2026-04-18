#ifndef __LA_H__
#define __LA_H__

struct Vec2f
{
    float x;
    float y;
};

Vec2f vec2f(float x, float y);
Vec2f vec2fs(float x);

Vec2f vec2f_add(Vec2f a, Vec2f b);
Vec2f vec2f_sub(Vec2f a, Vec2f b);
Vec2f vec2f_mul(Vec2f a, Vec2f b);
Vec2f vec2f_div(Vec2f a, Vec2f b);

#endif // __LA_H__ 