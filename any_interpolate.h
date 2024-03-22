#ifndef ANY_INTERPOLATE_INCLUDE
#define ANY_INTERPOLATE_INCLUDE

#ifdef __GNUC__
#define ANY_INTERPOLATE_ATTRIBUTE(...) __attribute__((__VA_ARGS__))
#else
#define ANY_INTERPOLATE_ATTRIBUTE(...)
#endif

ANY_INTERPOLATE_ATTRIBUTE(const)
double any_sign(double x);

ANY_INTERPOLATE_ATTRIBUTE(const)
double any_lerp(double x, double a, double b);

ANY_INTERPOLATE_ATTRIBUTE(const)
double any_step(double x, double edge);

ANY_INTERPOLATE_ATTRIBUTE(const)
double any_normalize(double x, double edge0, double edge1);

ANY_INTERPOLATE_ATTRIBUTE(const)
double any_clamp(double x, double min, double max);

ANY_INTERPOLATE_ATTRIBUTE(const)
double any_steps(double x, unsigned int n);

ANY_INTERPOLATE_ATTRIBUTE(const)
double any_smoothstep(double x);

ANY_INTERPOLATE_ATTRIBUTE(const)
double any_inverse_smoothstep(double x);

ANY_INTERPOLATE_ATTRIBUTE(const)
double any_smootherstep(double x);

ANY_INTERPOLATE_ATTRIBUTE(const)
double any_smootheststep(double x, double s);

ANY_INTERPOLATE_ATTRIBUTE(const)
double any_inverse_smootheststep(double x, double s);

ANY_INTERPOLATE_ATTRIBUTE(const)
double any_sstep(double x);

ANY_INTERPOLATE_ATTRIBUTE(const)
double any_ssteppow(double x, double k);

ANY_INTERPOLATE_ATTRIBUTE(const)
double any_ssteppow2(double x);

ANY_INTERPOLATE_ATTRIBUTE(const)
double any_inverse_ssteppow2(double x);

ANY_INTERPOLATE_ATTRIBUTE(const)
double any_gain(double x, double k);

ANY_INTERPOLATE_ATTRIBUTE(const)
double any_inverse_gain(double x, double k);

ANY_INTERPOLATE_ATTRIBUTE(const)
double any_gain2(double x);

ANY_INTERPOLATE_ATTRIBUTE(const)
double any_inverse_gain2(double x);

ANY_INTERPOLATE_ATTRIBUTE(const)
double any_hyperbolic_tan(double x, double k);

ANY_INTERPOLATE_ATTRIBUTE(const)
double any_quadratic_bezier(double x, double a, double b, double c);

ANY_INTERPOLATE_ATTRIBUTE(const)
double any_cubic_bezier(double x, double a, double b, double c, double d);

#endif

#ifdef ANY_INTERPOLATE_IMPLEMENT

#include <math.h>

//
// 			 | -1   if x < 0
// sign(x) = | 0    if x == 0
//           | 1    if x > 0
//
double any_sign(double x)
{
	return copysign(1.0, x);
}

//
// lerp(x) = a (1 - x) + b x
//
double any_lerp(double x, double a, double b)
{
	return a * (1.0 - x) + (b * x);
}

//
// step(x) = | 0    if x < edge
//           | 1    if x >= edge
//
double any_step(double x, double edge)
{
	return x < edge ? 0.0 : 1.0;
}

//
//                e1 - x
// normalize(x) = -------   => [0, 1]
//                e1 - e0
//
double any_normalize(double x, double edge0, double edge1)
{
	return (edge1 - x) / (edge1 - edge0);
}

//            | 0    if x < min
// clamp(x) = | x    if min <= x <= max
//            | 1    if x > max
//
double any_clamp(double x, double min, double max)
{
    const double t = x < min ? min : x;
    return t > max ? max : t;
}

double any_steps(double x, unsigned int n)
{
	return floor(x * n) * (1.0 / n);
}

//
// smoothstep(x) = 3 x^2 - 2 x^3
//
double any_smoothstep(double x)
{
   return x * x * (3.0 - 2.0 * x);
}

double any_inverse_smoothstep(double x)
{
	return 0.5 - sin(asin(1.0 - 2.0 * x) / 3.0);
}

//
// smootherstep(x) = 6 x^5 - 15 x^4 + 10 x^3
//
double any_smootherstep(double x)
{
	return x * x * x * (x * (6.0 * x - 15.0) + 10.0);
}

double any_smootheststep(double x, double s)
{
	// 2.0 / ln(2.0)
	const double ss = 2.88539008178;
    return 1.0 / (1.0 + exp2(tan(x * M_PI - 0.5 * M_PI) * -(s * ss)));
}

double any_inverse_smootheststep(double x, double s)
{
	// 2.0 / ln(2.0)
	const double ss = 2.88539008178;
    return atan(log2(1.0 / (1.0 - x) - 1.0) / s) * (1.0 / M_PI) + 0.5;
}

double any_sstep(double x)
{
	const double ix = 1.0 - x;
	x *= x;
	return x / (x + ix * ix);
}

double any_ssteppow(double x, double k)
{
	double ix = 1.0 - x;
    x = pow(x, k);
    ix = pow(ix, k);
    return x / (x + ix);
}

double any_ssteppow2(double x)
{
    return x * x / ((2.0 * x * (x - 1.0)) + 1.0);
}

double any_inverse_ssteppow2(double x);
{
    return (x - sqrt(x * (1.0 - x))) / (2.0 * x - 1.0);
}

double any_gain(double x, double k)
{
	const double sign = any_sign(x - 0.5);
    const double o = (1.0 + sign) / 2.0;
    return o - 0.5 * sign * pow(2.0 * (o - sign * x), k);
}

double any_inverse_gain(double x, double k)
{
	const double sign = any_sign(x - 0.5);
    const double o = (1.0 + sign) / 2.0;
    return o - 0.5 * sign * pow(2.0 * (o - sign * x), 1.0 / k);
}

double any_gain2(double x)
{
	return (x < 0.5) ? 2.0 * x * x : 2.0 * x * (2.0 - x) - 1.0;
}

double any_inverse_gain2(double x)
{
	return (x < 0.5) ? sqrt(2.0 * x) / 2.0 : 1.0 - sqrt(2.0 - 2.0 * x) / 2.0;
}

double any_hyperbolic_tan(double x, double k)
{
	return 0.5 + 0.5 * tanh((x - 0.5) * k);
}

//
// quadratic_bezier(x) = a (1 - x)^2 + 2b (1 - x) x + c x^2
//
double any_quadratic_bezier(double x, double a, double b, double c)
{
	const double t = 1 - x;
	return a * t * t + 2 * b * t * x + c * x * x;
}

//
// cubic_bezier(x) = a (1 - x)^3 + 3b (1 - x)^2 x + 3c (1 - x) x^2 + d x^3
//
double any_cubic_bezier(double x, double a, double b, double c, double d)
{
	const double t = 1 - x;
	return a * (t * t * t) + 3 * b * (t * t * x) + 3 * c * (t * x * x) + d * (x * x * x);
}

// TODO: Add spline and hermite

#endif

// Sources
// - https://en.wikipedia.org/wiki/Smoothstep
// - https://www.shadertoy.com/view/ltSfWV
// - https://www.shadertoy.com/view/ltjcWW
// - https://www.shadertoy.com/view/MdBfR1
// - https://www.shadertoy.com/view/ltByWW
// - http://demofox.org/bezcubic1drational.html

// MIT License
//
// Copyright (c) 2024 Federico Angelilli
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//
