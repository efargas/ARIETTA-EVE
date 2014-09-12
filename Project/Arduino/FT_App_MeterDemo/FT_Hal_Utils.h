#ifndef _FT_HAL_UTILS_H_
#define _FT_HAL_UTILS_H_


#define RGB(r, g, b)  ((((vc_int32_t)(r)) << 16) | (((vc_int32_t)(g)) << 8) | (b))
#define SQ(v) ((v) * (v))
#define MIN(x,y)  ((x) > (y) ? (y) : (x))
#define MAX(x,y)  ((x) > (y) ? (x) : (y))
#define PLAYCOLOR        0x00A0A080
#define NOTE(n, sharp)   (((n) - 'C') + ((sharp) * 128))
#define F16(s)           ((vc_int32_t)((s) * 65536))
#define INVALID_TOUCH_XY   0x8000
#define ABS(x)  ((x) > (0) ? (x) : (-x))


#endif /* _FT_HAL_UTILS_H_ */


