#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb_image.h"
#include "stb_image_write.h"
#include "stb_image_resize2.h"
#include "gfx_convert.h"

#define REMOVE_MEGADRIVE_THRUCOLOR

typedef struct
{
	byte r, g, b;
} palentry_t;

// SRB 32X palette
palentry_t palette[256] = {
	{ 0, 0, 0 },
{ 246, 246, 246 },
{ 237, 237, 237 },
{ 228, 228, 228 },
{ 218, 218, 218 },
{ 209, 209, 209 },
{ 200, 200, 200 },
{ 191, 191, 191 },
{ 183, 183, 183 },
{ 175, 175, 175 },
{ 167, 167, 167 },
{ 159, 159, 159 },
{ 151, 151, 151 },
{ 143, 143, 143 },
{ 135, 135, 135 },
{ 127, 127, 127 },
{ 119, 119, 119 },
{ 111, 111, 111 },
{ 103, 103, 103 },
{ 95, 95, 95 },
{ 87, 87, 87 },
{ 79, 79, 79 },
{ 71, 71, 71 },
{ 63, 63, 63 },
{ 55, 55, 55 },
{ 47, 47, 47 },
{ 39, 39, 39 },
{ 31, 31, 31 },
{ 23, 23, 23 },
{ 15, 15, 15 },
{ 7, 7, 7 },
{ 0, 0, 0 },
{ 255, 127, 127 },
{ 255, 95, 95 },
{ 255, 63, 63 },
{ 255, 0, 0 },
{ 239, 0, 0 },
{ 223, 0, 0 },
{ 207, 0, 0 },
{ 191, 0, 0 },
{ 175, 0, 0 },
{ 159, 0, 0 },
{ 143, 0, 0 },
{ 127, 0, 0 },
{ 111, 0, 0 },
{ 95, 0, 0 },
{ 71, 0, 0 },
{ 55, 0, 0 },
{ 255, 219, 192 },
{ 255, 203, 151 },
{ 255, 185, 117 },
{ 255, 168, 85 },
{ 255, 151, 54 },
{ 255, 134, 25 },
{ 255, 117, 0 },
{ 236, 105, 0 },
{ 221, 94, 0 },
{ 208, 88, 0 },
{ 196, 79, 0 },
{ 181, 68, 0 },
{ 159, 56, 0 },
{ 138, 41, 0 },
{ 129, 32, 0 },
{ 121, 24, 0 },
{ 235, 219, 87 },
{ 215, 187, 67 },
{ 195, 155, 47 },
{ 175, 123, 31 },
{ 155, 91, 19 },
{ 135, 67, 7 },
{ 117, 41, 0 },
{ 85, 0, 0 },
{ 255, 255, 79 },
{ 255, 255, 0 },
{ 227, 217, 15 },
{ 201, 187, 14 },
{ 170, 155, 11 },
{ 136, 120, 9 },
{ 112, 96, 7 },
{ 90, 73, 5 },
{ 255, 255, 207 },
{ 255, 255, 175 },
{ 255, 255, 143 },
{ 255, 255, 115 },
{ 235, 222, 129 },
{ 208, 194, 128 },
{ 183, 169, 119 },
{ 150, 131, 93 },
{ 222, 255, 168 },
{ 199, 228, 148 },
{ 173, 200, 128 },
{ 149, 173, 107 },
{ 124, 146, 88 },
{ 100, 119, 68 },
{ 74, 90, 48 },
{ 50, 63, 29 },
{ 119, 255, 79 },
{ 112, 240, 75 },
{ 105, 224, 70 },
{ 97, 208, 65 },
{ 90, 192, 60 },
{ 82, 176, 55 },
{ 75, 160, 50 },
{ 67, 144, 45 },
{ 60, 128, 40 },
{ 53, 112, 35 },
{ 45, 96, 30 },
{ 38, 80, 25 },
{ 30, 64, 20 },
{ 23, 48, 15 },
{ 15, 32, 10 },
{ 7, 15, 4 },
{ 0, 255, 0 },
{ 0, 223, 0 },
{ 0, 191, 0 },
{ 0, 159, 0 },
{ 0, 127, 0 },
{ 0, 95, 0 },
{ 0, 63, 0 },
{ 0, 45, 0 },
{ 183, 251, 231 },
{ 102, 247, 203 },
{ 21, 242, 176 },
{ 11, 210, 151 },
{ 3, 177, 128 },
{ 2, 147, 107 },
{ 2, 115, 84 },
{ 1, 86, 63 },
{ 206, 250, 255 },
{ 166, 241, 255 },
{ 117, 231, 255 },
{ 87, 213, 255 },
{ 79, 199, 255 },
{ 71, 185, 255 },
{ 55, 165, 255 },
{ 32, 138, 225 },
{ 24, 111, 182 },
{ 21, 83, 134 },
{ 14, 53, 86 },
{ 7, 30, 48 },
{ 116, 209, 201 },
{ 66, 179, 179 },
{ 23, 136, 136 },
{ 0, 95, 95 },
{ 231, 231, 255 },
{ 198, 198, 255 },
{ 173, 173, 255 },
{ 140, 140, 255 },
{ 115, 115, 255 },
{ 82, 82, 255 },
{ 49, 49, 255 },
{ 24, 24, 255 },
{ 0, 0, 255 },
{ 0, 0, 223 },
{ 0, 0, 196 },
{ 0, 0, 172 },
{ 0, 0, 149 },
{ 0, 0, 128 },
{ 0, 0, 102 },
{ 0, 0, 82 },
{ 216, 183, 255 },
{ 199, 153, 255 },
{ 173, 106, 255 },
{ 152, 68, 255 },
{ 127, 22, 255 },
{ 107, 0, 238 },
{ 91, 0, 201 },
{ 72, 0, 159 },
{ 51, 0, 113 },
{ 36, 0, 81 },
{ 151, 151, 213 },
{ 119, 119, 187 },
{ 84, 84, 167 },
{ 65, 65, 131 },
{ 46, 46, 92 },
{ 33, 34, 78 },
{ 255, 202, 255 },
{ 255, 170, 255 },
{ 255, 138, 255 },
{ 255, 106, 255 },
{ 255, 74, 255 },
{ 255, 0, 255 },
{ 221, 0, 221 },
{ 191, 0, 191 },
{ 162, 0, 162 },
{ 121, 0, 121 },
{ 85, 0, 85 },
{ 53, 0, 53 },
{ 197, 232, 0 },
{ 167, 202, 4 },
{ 140, 168, 11 },
{ 108, 124, 18 },
{ 207, 127, 207 },
{ 183, 111, 183 },
{ 159, 95, 159 },
{ 135, 79, 135 },
{ 111, 63, 111 },
{ 87, 47, 87 },
{ 64, 32, 64 },
{ 43, 21, 43 },
{ 255, 196, 224 },
{ 255, 153, 192 },
{ 245, 112, 165 },
{ 221, 87, 140 },
{ 199, 61, 116 },
{ 177, 52, 102 },
{ 157, 47, 91 },
{ 133, 39, 77 },
{ 255, 230, 219 },
{ 255, 191, 191 },
{ 255, 159, 159 },
{ 225, 133, 133 },
{ 204, 113, 113 },
{ 194, 99, 99 },
{ 181, 83, 83 },
{ 167, 63, 63 },
{ 255, 207, 179 },
{ 255, 193, 158 },
{ 255, 183, 139 },
{ 247, 171, 123 },
{ 239, 163, 115 },
{ 227, 151, 103 },
{ 215, 139, 91 },
{ 207, 131, 83 },
{ 191, 123, 75 },
{ 179, 115, 71 },
{ 171, 111, 67 },
{ 163, 107, 63 },
{ 155, 99, 59 },
{ 143, 95, 55 },
{ 135, 87, 51 },
{ 127, 83, 47 },
{ 119, 79, 43 },
{ 107, 71, 39 },
{ 95, 67, 35 },
{ 83, 63, 31 },
{ 75, 55, 27 },
{ 63, 47, 23 },
{ 51, 43, 19 },
{ 43, 35, 15 },
{ 191, 167, 143 },
{ 175, 152, 128 },
{ 159, 137, 113 },
{ 146, 125, 101 },
{ 134, 114, 90 },
{ 126, 106, 82 },
{ 117, 98, 74 },
{ 109, 90, 66 },
{ 101, 83, 59 },
{ 93, 75, 51 },
{ 87, 69, 45 },
{ 75, 60, 35 },
{ 255, 231, 246 },
{ 0, 0, 63 },
{ 0, 0, 32 },
{ 255, 255, 255 },
};

// SRB2 2.0 palette
/*palentry_t palette[256] = {
	{247, 247, 247},
	{239, 239, 239},
	{231, 231, 231},
	{223, 223, 223},
	{215, 215, 215},
	{207, 207, 207},
	{199, 199, 199},
	{191, 191, 191},
	{183, 183, 183},
	{175, 175, 175},
	{167, 167, 167},
	{159, 159, 159},
	{151, 151, 151},
	{143, 143, 143},
	{135, 135, 135},
	{127, 127, 127},
	{119, 119, 119},
	{111, 111, 111},
	{103, 103, 103},
	{95, 95, 95},
	{87, 87, 87},
	{79, 79, 79},
	{71, 71, 71},
	{63, 63, 63},
	{55, 55, 55},
	{47, 47, 47},
	{39, 39, 39},
	{31, 31, 31},
	{23, 23, 23},
	{15, 15, 15},
	{7, 7, 7},
	{0, 0, 0},
	{191, 167, 143},
	{183, 160, 136},
	{175, 152, 128},
	{167, 144, 120},
	{159, 137, 113},
	{150, 129, 105},
	{142, 121, 97},
	{134, 114, 90},
	{126, 106, 82},
	{117, 98, 74},
	{109, 90, 66},
	{101, 83, 59},
	{93, 75, 51},
	{84, 67, 43},
	{76, 60, 36},
	{67, 51, 27},
	{191, 123, 75},
	{179, 115, 71},
	{171, 111, 67},
	{163, 107, 63},
	{155, 99, 59},
	{143, 95, 55},
	{135, 87, 51},
	{127, 83, 47},
	{119, 79, 43},
	{107, 71, 39},
	{95, 67, 35},
	{83, 63, 31},
	{75, 55, 27},
	{63, 47, 23},
	{51, 43, 19},
	{43, 35, 15},
	{255, 235, 223},
	{255, 227, 211},
	{255, 219, 199},
	{255, 211, 187},
	{255, 207, 179},
	{255, 199, 167},
	{255, 191, 155},
	{255, 187, 147},
	{255, 179, 131},
	{247, 171, 123},
	{239, 163, 115},
	{231, 155, 107},
	{223, 147, 99},
	{215, 139, 91},
	{207, 131, 83},
	{203, 127, 79},
	{255, 238, 220},
	{255, 220, 185},
	{255, 203, 151},
	{255, 185, 117},
	{255, 168, 85},
	{255, 151, 54},
	{255, 134, 25},
	{255, 117, 0},
	{243, 109, 0},
	{229, 101, 0},
	{216, 93, 0},
	{203, 85, 0},
	{190, 77, 0},
	{177, 69, 0},
	{164, 61, 0},
	{151, 54, 0},
	{255, 255, 239},
	{255, 255, 207},
	{255, 255, 175},
	{255, 255, 143},
	{255, 255, 111},
	{255, 255, 79},
	{255, 255, 47},
	{255, 255, 15},
	{255, 255, 0},
	{207, 207, 0},
	{175, 175, 0},
	{143, 143, 0},
	{111, 111, 0},
	{79, 79, 0},
	{47, 47, 0},
	{15, 15, 0},
	{255, 255, 115},
	{235, 219, 87},
	{215, 187, 67},
	{195, 155, 47},
	{175, 123, 31},
	{155, 91, 19},
	{135, 67, 7},
	{115, 43, 0},
	{255, 255, 255},
	{255, 223, 223},
	{255, 191, 191},
	{255, 159, 159},
	{255, 127, 127},
	{255, 95, 95},
	{255, 63, 63},
	{255, 31, 31},
	{255, 0, 0},
	{239, 0, 0},
	{223, 0, 0},
	{207, 0, 0},
	{191, 0, 0},
	{175, 0, 0},
	{159, 0, 0},
	{143, 0, 0},
	{127, 0, 0},
	{111, 0, 0},
	{95, 0, 0},
	{79, 0, 0},
	{63, 0, 0},
	{47, 0, 0},
	{31, 0, 0},
	{15, 0, 0},
	{255, 183, 183},
	{243, 163, 163},
	{231, 143, 143},
	{219, 123, 123},
	{203, 107, 107},
	{191, 91, 91},
	{179, 79, 79},
	{167, 63, 63},
	{142, 46, 0},
	{134, 39, 0},
	{126, 32, 0},
	{117, 25, 0},
	{109, 18, 0},
	{101, 11, 0},
	{93, 5, 0},
	{85, 0, 0},
	{119, 255, 79},
	{112, 240, 75},
	{105, 224, 70},
	{97, 208, 65},
	{90, 192, 60},
	{82, 176, 55},
	{75, 160, 50},
	{67, 144, 45},
	{60, 128, 40},
	{53, 112, 35},
	{45, 96, 30},
	{38, 80, 25},
	{30, 64, 20},
	{23, 48, 15},
	{15, 32, 10},
	{7, 15, 4},
	{222, 255, 168},
	{199, 228, 148},
	{173, 200, 128},
	{149, 173, 107},
	{124, 146, 88},
	{100, 119, 68},
	{74, 90, 48},
	{50, 63, 29},
	{0, 255, 0},
	{0, 223, 0},
	{0, 191, 0},
	{0, 159, 0},
	{0, 127, 0},
	{0, 95, 0},
	{0, 63, 0},
	{0, 31, 0},
	{255, 111, 255},
	{255, 0, 255},
	{223, 0, 223},
	{191, 0, 191},
	{159, 0, 159},
	{127, 0, 127},
	{95, 0, 95},
	{63, 0, 63},
	{233, 233, 243},
	{196, 196, 225},
	{157, 157, 206},
	{119, 119, 187},
	{84, 84, 167},
	{65, 65, 131},
	{46, 46, 92},
	{27, 27, 52},
	{213, 241, 255},
	{191, 235, 255},
	{170, 227, 255},
	{149, 221, 255},
	{128, 214, 255},
	{106, 207, 255},
	{85, 200, 255},
	{63, 191, 255},
	{55, 157, 223},
	{47, 143, 191},
	{39, 119, 159},
	{31, 95, 127},
	{0, 191, 191},
	{0, 127, 127},
	{0, 95, 95},
	{0, 63, 63},
	{239, 239, 255},
	{207, 207, 255},
	{175, 175, 255},
	{143, 143, 255},
	{111, 111, 255},
	{79, 79, 255},
	{47, 47, 255},
	{15, 15, 255},
	{0, 0, 255},
	{0, 0, 223},
	{0, 0, 207},
	{0, 0, 191},
	{0, 0, 175},
	{0, 0, 159},
	{0, 0, 143},
	{0, 0, 127},
	{0, 0, 111},
	{0, 0, 95},
	{0, 0, 79},
	{0, 0, 63},
	{0, 0, 47},
	{0, 0, 31},
	{0, 0, 15},
	{0, 255, 255},
	{207, 127, 207},
	{183, 111, 183},
	{159, 95, 159},
	{135, 79, 135},
	{111, 63, 111},
	{87, 47, 87},
	{63, 31, 63},
	{39, 15, 39},
};*/

/*
* Doom palette
palentry_t palette[256] = {
	{0, 0, 0},
	{31, 23, 11},
	{23, 15, 7},
	{75, 75, 75},
	{255, 255, 255},
	{27, 27, 27},
	{19, 19, 19},
	{11, 11, 11},
	{7, 7, 7},
	{47, 55, 31},
	{35, 43, 15},
	{23, 31, 7},
	{15, 23, 0},
	{79, 59, 43},
	{71, 51, 35},
	{63, 43, 27},
	{255, 183, 183},
	{247, 171, 171},
	{243, 163, 163},
	{235, 151, 151},
	{231, 143, 143},
	{223, 135, 135},
	{219, 123, 123},
	{211, 115, 115},
	{203, 107, 107},
	{199, 99, 99},
	{191, 91, 91},
	{187, 87, 87},
	{179, 79, 79},
	{175, 71, 71},
	{167, 63, 63},
	{163, 59, 59},
	{155, 51, 51},
	{151, 47, 47},
	{143, 43, 43},
	{139, 35, 35},
	{131, 31, 31},
	{127, 27, 27},
	{119, 23, 23},
	{115, 19, 19},
	{107, 15, 15},
	{103, 11, 11},
	{95, 7, 7},
	{91, 7, 7},
	{83, 7, 7},
	{79, 0, 0},
	{71, 0, 0},
	{67, 0, 0},
	{255, 235, 223},
	{255, 227, 211},
	{255, 219, 199},
	{255, 211, 187},
	{255, 207, 179},
	{255, 199, 167},
	{255, 191, 155},
	{255, 187, 147},
	{255, 179, 131},
	{247, 171, 123},
	{239, 163, 115},
	{231, 155, 107},
	{223, 147, 99},
	{215, 139, 91},
	{207, 131, 83},
	{203, 127, 79},
	{191, 123, 75},
	{179, 115, 71},
	{171, 111, 67},
	{163, 107, 63},
	{155, 99, 59},
	{143, 95, 55},
	{135, 87, 51},
	{127, 83, 47},
	{119, 79, 43},
	{107, 71, 39},
	{95, 67, 35},
	{83, 63, 31},
	{75, 55, 27},
	{63, 47, 23},
	{51, 43, 19},
	{43, 35, 15},
	{239, 239, 239},
	{231, 231, 231},
	{223, 223, 223},
	{219, 219, 219},
	{211, 211, 211},
	{203, 203, 203},
	{199, 199, 199},
	{191, 191, 191},
	{183, 183, 183},
	{179, 179, 179},
	{171, 171, 171},
	{167, 167, 167},
	{159, 159, 159},
	{151, 151, 151},
	{147, 147, 147},
	{139, 139, 139},
	{131, 131, 131},
	{127, 127, 127},
	{119, 119, 119},
	{111, 111, 111},
	{107, 107, 107},
	{99, 99, 99},
	{91, 91, 91},
	{87, 87, 87},
	{79, 79, 79},
	{71, 71, 71},
	{67, 67, 67},
	{59, 59, 59},
	{55, 55, 55},
	{47, 47, 47},
	{39, 39, 39},
	{35, 35, 35},
	{119, 255, 111},
	{111, 239, 103},
	{103, 223, 95},
	{95, 207, 87},
	{91, 191, 79},
	{83, 175, 71},
	{75, 159, 63},
	{67, 147, 55},
	{63, 131, 47},
	{55, 115, 43},
	{47, 99, 35},
	{39, 83, 27},
	{31, 67, 23},
	{23, 51, 15},
	{19, 35, 11},
	{11, 23, 7},
	{191, 167, 143},
	{183, 159, 135},
	{175, 151, 127},
	{167, 143, 119},
	{159, 135, 111},
	{155, 127, 107},
	{147, 123, 99},
	{139, 115, 91},
	{131, 107, 87},
	{123, 99, 79},
	{119, 95, 75},
	{111, 87, 67},
	{103, 83, 63},
	{95, 75, 55},
	{87, 67, 51},
	{83, 63, 47},
	{159, 131, 99},
	{143, 119, 83},
	{131, 107, 75},
	{119, 95, 63},
	{103, 83, 51},
	{91, 71, 43},
	{79, 59, 35},
	{67, 51, 27},
	{123, 127, 99},
	{111, 115, 87},
	{103, 107, 79},
	{91, 99, 71},
	{83, 87, 59},
	{71, 79, 51},
	{63, 71, 43},
	{55, 63, 39},
	{255, 255, 115},
	{235, 219, 87},
	{215, 187, 67},
	{195, 155, 47},
	{175, 123, 31},
	{155, 91, 19},
	{135, 67, 7},
	{115, 43, 0},
	{255, 255, 255},
	{255, 219, 219},
	{255, 187, 187},
	{255, 155, 155},
	{255, 123, 123},
	{255, 95, 95},
	{255, 63, 63},
	{255, 31, 31},
	{255, 0, 0},
	{239, 0, 0},
	{227, 0, 0},
	{215, 0, 0},
	{203, 0, 0},
	{191, 0, 0},
	{179, 0, 0},
	{167, 0, 0},
	{155, 0, 0},
	{139, 0, 0},
	{127, 0, 0},
	{115, 0, 0},
	{103, 0, 0},
	{91, 0, 0},
	{79, 0, 0},
	{67, 0, 0},
	{231, 231, 255},
	{199, 199, 255},
	{171, 171, 255},
	{143, 143, 255},
	{115, 115, 255},
	{83, 83, 255},
	{55, 55, 255},
	{27, 27, 255},
	{0, 0, 255},
	{0, 0, 227},
	{0, 0, 203},
	{0, 0, 179},
	{0, 0, 155},
	{0, 0, 131},
	{0, 0, 107},
	{0, 0, 83},
	{255, 255, 255},
	{255, 235, 219},
	{255, 215, 187},
	{255, 199, 155},
	{255, 179, 123},
	{255, 163, 91},
	{255, 143, 59},
	{255, 127, 27},
	{243, 115, 23},
	{235, 111, 15},
	{223, 103, 15},
	{215, 95, 11},
	{203, 87, 7},
	{195, 79, 0},
	{183, 71, 0},
	{175, 67, 0},
	{255, 255, 255},
	{255, 255, 215},
	{255, 255, 179},
	{255, 255, 143},
	{255, 255, 107},
	{255, 255, 71},
	{255, 255, 35},
	{255, 255, 0},
	{167, 63, 0},
	{159, 55, 0},
	{147, 47, 0},
	{135, 35, 0},
	{79, 59, 39},
	{67, 47, 27},
	{55, 35, 19},
	{47, 27, 11},
	{0, 0, 83},
	{0, 0, 71},
	{0, 0, 59},
	{0, 0, 47},
	{0, 0, 35},
	{0, 0, 23},
	{0, 0, 11},
	{0, 255, 255},
	{255, 159, 67},
	{255, 231, 75},
	{255, 123, 255},
	{255, 0, 255},
	{207, 0, 207},
	{159, 0, 155},
	{111, 0, 107},
	{167, 107, 107},
};*/

byte GetIndexFromRGB(byte r, byte g, byte b)
{
	for (int i = 0; i < 256; i++)
	{
		const palentry_t *palEntry = &palette[i];

		if (palEntry->r == r && palEntry->g == g && palEntry->b == b)
			return (byte)i;
	}

	// If we got here, no entry matched. Try to find the closest match.

	int32_t closestIndex = 0;
	int32_t closestDist = -1;
	for (int32_t i = 0; i < 256; i++)
	{
		const palentry_t *palEntry = &palette[i];

		int32_t dist_r = abs(palEntry->r - r);
		int32_t dist_g = abs(palEntry->g - g);
		int32_t dist_b = abs(palEntry->b - b);

		int32_t dist = (dist_r * dist_r) + (dist_g * dist_g) + (dist_b * dist_b);

		if (closestDist < 0 || dist < closestDist)
		{
			closestIndex = i;
			closestDist = dist;
		}
	}

#ifdef REMOVE_MEGADRIVE_THRUCOLOR
	if (closestIndex == 0xfc)
		closestIndex = 0xd0;
#endif

	return (byte)closestIndex;
}

byte *RGBToIndexed(const byte *rgbData, int32_t length)
{
	byte *indexedImage = (byte *)malloc(length / 3);
	int32_t z = 0;
	for (int32_t i = 0; i < length; i += 3)
	{
		byte r = rgbData[i];
		byte g = rgbData[i + 1];
		byte b = rgbData[i + 2];

		byte palIndex = GetIndexFromRGB(r, g, b);

		indexedImage[z++] = palIndex;
	}

	return indexedImage;
}

byte *RGBToIndexed(const byte *rgbData, int32_t width, int32_t height)
{
	return RGBToIndexed(rgbData, width * height * 3);
}

byte *IndexedToRGB(const byte *indData, int32_t width, int32_t height)
{
	byte *rgbImage = (byte *)malloc(width * height * 3);
	byte *cursor = rgbImage;
	int32_t z = 0;
	for (int32_t i = 0; i < width * height; i++)
	{
		palentry_t *palEntry = &palette[indData[i]];

		*cursor++ = palEntry->r;
		*cursor++ = palEntry->g;
		*cursor++ = palEntry->b;
	}

	return rgbImage;
}

byte GetPixel(const byte *rawData, int32_t width, int32_t x, int32_t y)
{
	size_t pixelLocation = (y * width) + x;

#ifdef REMOVE_MEGADRIVE_THRUCOLOR
	if (rawData[pixelLocation] == 0xfc)
		return 0xd0;
#endif

	return rawData[pixelLocation];
}

void SetPixel(byte *rawData, int32_t width, int32_t x, int32_t y, byte pixel)
{
	size_t pixelLocation = (y * width) + x;

	rawData[pixelLocation] = pixel;
}

byte *FlatMipmaps(const byte *data, int dataLen, int numlevels, int *outputLen)
{
	int flatSize = (int)sqrt(dataLen);

	byte *rgbImage = IndexedToRGB(data, flatSize, flatSize);
	int fullSize = flatSize;

	byte *totalBuffer = (byte *)malloc(1024 * 1024); // Will never be this big
	int dataSize = 0;

	memcpy(totalBuffer, rgbImage, flatSize * flatSize * 3);
	dataSize += flatSize * flatSize * 3;
	flatSize >>= 1;

	for (int i = 0; i < numlevels-1; i++)
	{
		byte *resized = stbir_resize_uint8_linear(rgbImage, fullSize, fullSize, 0, NULL, flatSize, flatSize, 0, STBIR_RGB);

		memcpy(&totalBuffer[dataSize], resized, flatSize * flatSize * 3);
		dataSize += flatSize * flatSize * 3;

		free(resized);

		flatSize >>= 1;

		if (flatSize < 1)
			flatSize = 1;
	}

	// Convert back to indexed graphic
	byte *indexedFinal = RGBToIndexed(totalBuffer, dataSize);
	free(totalBuffer);

	free(rgbImage);

	// All done! Send it back.
	*outputLen = dataSize / 3;
	return indexedFinal;
}

byte *PatchMipmaps(const byte *data, int width, int height, int numlevels, int *outputLen)
{
	byte *rgbImage = IndexedToRGB(data, width, height);
	int mipWidth = width;
	int mipHeight = height;

	byte *totalBuffer = (byte *)malloc(1024 * 1024); // Will never be this big
	int dataSize = 0;

	memcpy(totalBuffer, rgbImage, mipWidth * mipHeight * 3);
	dataSize += mipWidth * mipHeight * 3;
	mipWidth >>= 1;
	mipHeight >>= 1;

	for (int i = 0; i < numlevels - 1; i++)
	{
		byte *resized = stbir_resize_uint8_linear(rgbImage, width, height, 0, NULL, mipWidth, mipHeight, 0, STBIR_RGB);

		memcpy(&totalBuffer[dataSize], resized, mipWidth * mipHeight * 3);
		dataSize += mipWidth * mipHeight * 3;

		free(resized);

		mipWidth >>= 1;
		if (mipWidth < 1)
			mipWidth = 1;

		mipHeight >>= 1;
		if (mipHeight < 1)
			mipHeight = 1;
	}

	// Convert back to indexed graphic
	byte *indexedFinal = RGBToIndexed(totalBuffer, dataSize);
	free(totalBuffer);

	free(rgbImage);

	// All done! Send it back.
	*outputLen = dataSize / 3;
	return indexedFinal;
}

byte *FlatToPNG(const byte *flatData, int32_t width, int32_t height, int32_t *outputLen)
{
	return stbi_write_png_to_mem(flatData, 0, width, height, 1, outputLen);
}

byte *RawToPNG(const byte *rawData, int32_t width, int32_t height, int32_t *outputLen)
{
	return stbi_write_png_to_mem(rawData, 0, width, height, 1, outputLen);
}

byte *PNGToFlat(byte *pngData, int32_t pngLength, int32_t *width, int32_t *height)
{
	int32_t channels;

	byte *rawImage = stbi_load_from_memory(pngData, pngLength, width, height, &channels, 3);
	byte *indexedImage = RGBToIndexed(rawImage, *width, *height);

	free(rawImage);

	return indexedImage;
}

// PC patch to a raw image
byte *PatchToRaw(const byte *patchData, size_t dataLen, int32_t *outputLen, byte transparentIndex)
{
	const patchHeader_t *header = (patchHeader_t *)patchData;

	byte *rawImage = (byte *)malloc(header->width * header->height * 1);
	memset(rawImage, transparentIndex, header->width * header->height * 1); // Transparent value

	for (int32_t i = 0; i < header->width; i++)
	{
		uint32_t colOffset = header->columnofs[i];
		const post_t *post = (post_t *)(patchData + colOffset);

		int32_t yPos = 0;
		while (post->topdelta != 255)
		{
			yPos = post->topdelta;
			const byte *pixel = post->data;

			for (int32_t j = 0; j < post->length; j++)
			{
				size_t pixelLocation = (yPos * header->width) + i;

#ifdef REMOVE_MEGADRIVE_THRUCOLOR
				if (*pixel == 0xfc)
					rawImage[pixelLocation] = 0xd0;
				else
#endif
					rawImage[pixelLocation] = *pixel;

				pixel++;
				yPos++;
			}

			pixel++; // dummy value
			post = (const post_t *)pixel;
		}
	}

	*outputLen = header->width * header->height;
	return rawImage;
}

//
// VerticalFlip
//
// Vertically flips an image
// destImage must already be allocated to the proper size!
//
// bpp - BYTES per pixel
//
void VerticalFlip(const byte *srcImage, byte *destImage, const int16_t width, const int16_t height, const byte bpp)
{
	destImage += width * height * bpp;
	destImage -= width * bpp;

	int32_t i;
	for (i = 0; i < height; i++)
	{
		memcpy(destImage, srcImage, width * bpp);
		srcImage += width * bpp;
		destImage -= width * bpp;
	}
}

bool ContainsPixel(const byte *rawImage, uint16_t width, uint16_t height, byte index)
{
	for (int32_t i = 0; i < width * height; i++)
	{
		if (rawImage[i] == index)
			return true;
	}

	return false;
}

byte *RawToJagTexture(const byte *rawImage, uint16_t width, uint16_t height)
{
	// Now we have to flip it.. ugh
	byte *flippedImage = (byte *)malloc(width * height);
	VerticalFlip(rawImage, flippedImage, width, height, 1);

	byte *rotatedImage = (byte *)malloc(width * height);

	// Re-draw the raw image as row-major
	for (int32_t y = 0, destinationColumn = height - 1; y < height; y++, --destinationColumn)
	{
		int32_t offset = y * width;

		for (int32_t x = 0; x < width; x++)
			rotatedImage[(x * height) + destinationColumn] = flippedImage[offset + x];
	}

	free(flippedImage);

	return rotatedImage;
}

byte *PatchToJagTexture(const byte *patchData, size_t dataLen, int32_t *outputLen)
{
	const patchHeader_t *header = (patchHeader_t *)patchData;

	// I'm lazy... convert to a regular raw image first.
	byte *rawImage = PatchToRaw(patchData, dataLen, outputLen, 0);

	byte *jagTexture = RawToJagTexture(rawImage, header->width, header->height);

	free(rawImage);

	*outputLen = header->width * header->height;
	return jagTexture;
}

// Careful! Returns NULL if nothing was cropped!
byte *CropPCPatch(const byte *patchData, size_t dataLen, int32_t *outputLen, byte transparentIndex)
{
	const patchHeader_t *header = (patchHeader_t *)patchData;
	int32_t cropTop = 0; // heh...
	int32_t cropLeft = 0;
	int32_t cropRight = 0;
	int32_t cropBottom = 0;

	// First, convert it to a raw image
	byte *rawImage = PatchToRaw(patchData, dataLen, outputLen, transparentIndex);

	// Check if we should crop anything from the top
	for (int32_t y = 0; y < header->height; y++)
	{
		bool keep = false;
		for (int32_t x = 0; x < header->width; x++)
		{
			byte pixel = GetPixel(rawImage, header->width, x, y);
			if (pixel != transparentIndex)
			{
				keep = true;
				break;
			}
		}

		if (keep)
			break;

		cropTop++;
	}

	// Crop anything from the bottom?
	for (int32_t y = header->height - 1; y >= 0; y--)
	{
		bool keep = false;
		for (int32_t x = 0; x < header->width; x++)
		{
			byte pixel = GetPixel(rawImage, header->width, x, y);
			if (pixel != transparentIndex)
			{
				keep = true;
				break;
			}
		}

		if (keep)
			break;

		cropBottom++;
	}
	
	// Crop anything from the left?
	for (int32_t x = 0; x < header->width; x++)
	{
		bool keep = false;
		for (int32_t y = 0; y < header->height; y++)
		{
			byte pixel = GetPixel(rawImage, header->width, x, y);
			if (pixel != transparentIndex)
			{
				keep = true;
				break;
			}
		}

		if (keep)
			break;
		
		cropLeft++;
	}

	// Crop anything from the right?
	for (int32_t x = header->width - 1; x >= 0; x--)
	{
		bool keep = false;
		for (int32_t y = 0; y < header->height; y++)
		{
			byte pixel = GetPixel(rawImage, header->width, x, y);
			if (pixel != transparentIndex)
			{
				keep = true;
				break;
			}
		}

		if (keep)
			break;

		cropRight++;
	}

	if (cropLeft == 0 && cropTop == 0 && cropRight == 0 && cropBottom == 0)
		return NULL;

	printf("Crop %d, %d, %d, %d ", cropLeft, cropTop, cropRight, cropBottom);

	int16_t newWidth = header->width - cropLeft;
	newWidth -= cropRight;
	int16_t newHeight = header->height - cropTop;
	newHeight -= cropBottom;
	byte *newImage = (byte*)malloc(newWidth * newHeight);
	memset(newImage, transparentIndex, newWidth * newHeight);

	int newX = 0;
	int newY = 0;
	for (int32_t x = cropLeft; x < header->width - cropRight; x++)
	{
		newY = 0;
		for (int32_t y = cropTop; y < header->height - cropBottom; y++)
		{
			byte srcPixel = GetPixel(rawImage, header->width, x, y);

			SetPixel(newImage, newWidth, newX, newY, srcPixel);
			newY++;
		}
		newX++;
	}

	free(rawImage);

	// Convert back to PC patch, adjust offsets
	byte *newPatch = RawToPatch(newImage, newWidth, newHeight, outputLen, transparentIndex);
	patchHeader_t *newHeader = (patchHeader_t *)newPatch;
	newHeader->leftoffset = header->leftoffset;
	newHeader->topoffset = header->topoffset;
	newHeader->leftoffset -= cropLeft;
	newHeader->topoffset -= cropTop;

	return newPatch;
}

// Works for patches, sprites, all of the transparency-format Doom graphics
// Returns an allocated representation of the 8-bit PNG data (albeit without palette information).
// Up to you to manage the memory lifetime of it!
byte *PatchToPNG(const byte *patchData, size_t dataLen, int32_t *outputLen, byte transparentIndex)
{
	const patchHeader_t *header = (patchHeader_t *)patchData;
	byte *rawImage = PatchToRaw(patchData, dataLen, outputLen, transparentIndex);


	byte *pngData = stbi_write_png_to_mem(rawImage, 0, header->width, header->height, 1, outputLen);
	free(rawImage);

	return pngData;
}

typedef struct
{
	uint16_t address; // We keep this un-swapped for easier debugging
	int32_t length;
	byte data[512]; // Because memory is cheap now
} jagPostCache_t;

//
// Allocate sufficient space in 'jagHeader' and 'jagData' before calling.
//
void PCSpriteToJag(const byte *lumpData, int32_t lumpSize, byte *jagHeader, int32_t *jagHeaderLen, byte *jagData, int32_t *jagDataLen)
{
	// Casting to a structure makes it easier to read
	patchHeader_t *header = (patchHeader_t *)lumpData;
	jagPatchHeader_t *jagPatchHeader = (jagPatchHeader_t *)jagHeader;
	jagPatchHeader->width = swap_endian16(header->width);
	jagPatchHeader->height = swap_endian16(header->height);
	jagPatchHeader->leftoffset = swap_endian16(header->leftoffset);
	jagPatchHeader->topoffset = swap_endian16(header->topoffset);

	// Column pointers; Convert them from uint32_t to uint16_t
	for (int32_t column = 0; column < header->width; column++)
		jagPatchHeader->columnofs[column] = swap_endian16((uint16_t)header->columnofs[column]);

	byte *dataPtr = jagData;
	uint16_t headerSize = 8 + (header->width * 2);
	byte *headerPtr = jagHeader + headerSize;

	// Keep a cache of posts already drawn. If we come across one that matches one of the previous, re-use it.
	// This can make a file size significantly smaller if there are lots of repeating posts.
	jagPostCache_t jagPostCache[1024];
	int32_t numJagPostCache = 0;
	int32_t numReusedPosts = 0;

	// 'Draw' the PC Doom graphic into the Jaguar one
	for (int32_t i = 0; i < header->width; i++)
	{
		uint16_t colOffset = (uint16_t)header->columnofs[i];
		const post_t *post = (post_t *)(lumpData + colOffset);

		jagPatchHeader->columnofs[i] = swap_endian16((uint16_t)(headerPtr - jagHeader));

		while (post->topdelta != 255)
		{
			byte len = post->length;
			jagPost_t *jagPost = (jagPost_t *)headerPtr;
			jagPost->topdelta = post->topdelta;
			jagPost->length = len;
			jagPost->dataofs = swap_endian16(dataPtr - jagData);

			const byte *pixel = post->data;

			// First, draw into a temporary buffer. See if we already have this data previously
			byte tempBuffer[2048];
			for (int32_t j = 0; j < post->length; j++)
				tempBuffer[j] = *pixel++;

			bool foundDuplicate = false;
			for (int32_t j = 0; j < numJagPostCache; j++)
			{
				if (jagPostCache[j].length != jagPost->length)
					continue;

				if (!memcmp(jagPostCache[j].data, tempBuffer, jagPost->length))
				{
					foundDuplicate = true;
					numReusedPosts++;
					jagPost->dataofs = swap_endian16(jagPostCache[j].address);

					// Advance the PC graphic
					pixel = post->data;
					for (int32_t k = 0; k < post->length; k++)
						pixel++;
					pixel++; // dummy value in PC gfx
					post = (const post_t *)pixel;
					break;
				}
			}

			if (!foundDuplicate)
			{
				pixel = post->data;
				for (int32_t j = 0; j < post->length; j++)
				{
#ifdef REMOVE_MEGADRIVE_THRUCOLOR
					if (*pixel == 0xfc)
						*dataPtr++ = 0xd0;
					else
#endif
					*dataPtr++ = *pixel;
					jagPostCache[numJagPostCache].data[j] = *pixel;
					pixel++;
				}

				jagPostCache[numJagPostCache].length = jagPost->length;
				jagPostCache[numJagPostCache].address = swap_endian16(jagPost->dataofs);
				numJagPostCache++;

				pixel++; // dummy value in PC gfx
				post = (const post_t *)pixel;
			}
			headerPtr += sizeof(jagPost_t);
		}

		*headerPtr++ = 0xff;
		*headerPtr++ = 0xff;
	}

	*jagHeaderLen = headerPtr - jagHeader;
	*jagDataLen = dataPtr - jagData;

	if (numReusedPosts > 0)
		printf("Reused %d posts!\n", numReusedPosts);
}

void PCSpriteToJagNarrow(const byte *lumpData, int32_t lumpSize, byte *jagHeader, int32_t *jagHeaderLen, byte *jagData, int32_t *jagDataLen)
{
	// Casting to a structure makes it easier to read
	patchHeader_t *header = (patchHeader_t *)lumpData;
	jagPatchHeader_t *jagPatchHeader = (jagPatchHeader_t *)jagHeader;
	uint16_t headerWidth = header->width / 2;
	jagPatchHeader->width = swap_endian16(headerWidth);
	jagPatchHeader->height = swap_endian16(header->height);
	jagPatchHeader->leftoffset = swap_endian16(header->leftoffset / 2);
	jagPatchHeader->topoffset = swap_endian16(header->topoffset);

	// Column pointers; Convert them from uint32_t to uint16_t
	int32_t column = 0;
	for (int32_t i = 0; column < headerWidth; i += 2)
		jagPatchHeader->columnofs[column++] = swap_endian16((uint16_t)header->columnofs[i]);

	byte *dataPtr = jagData;
	uint16_t headerSize = 8 + (headerWidth * 2);
	byte *headerPtr = jagHeader + headerSize;

	// Keep a cache of posts already drawn. If we come across one that matches one of the previous, re-use it.
	// This can make a file size significantly smaller if there are lots of repeating posts.
	jagPostCache_t jagPostCache[256];
	int32_t numJagPostCache = 0;

	// 'Draw' the PC Doom graphic into the Jaguar one
	int32_t jagColumn = 0;
	for (int32_t i = 0; jagColumn < headerWidth; i += 2, jagColumn++)
	{
		uint16_t colOffset = (uint16_t)header->columnofs[i];
		const post_t *post = (post_t *)(lumpData + colOffset);

		jagPatchHeader->columnofs[jagColumn] = swap_endian16((uint16_t)(headerPtr - jagHeader));

		while (post->topdelta != 255)
		{
			byte len = post->length;
			jagPost_t *jagPost = (jagPost_t *)headerPtr;
			jagPost->topdelta = post->topdelta;
			jagPost->length = len;
			jagPost->dataofs = swap_endian16(dataPtr - jagData);

			const byte *pixel = post->data;

			// First, draw into a temporary buffer. See if we already have this data previously
			byte tempBuffer[2048];
			for (int32_t j = 0; j < post->length; j++)
				tempBuffer[j] = *pixel++;

			bool foundDuplicate = false;
			for (int32_t j = 0; j < numJagPostCache; j++)
			{
				if (jagPostCache[j].length != jagPost->length)
					continue;

				if (!memcmp(jagPostCache[j].data, tempBuffer, jagPost->length))
				{
					foundDuplicate = true;
					jagPost->dataofs = swap_endian16(jagPostCache[j].address);

					// Advance the PC graphic
					pixel = post->data;
					for (int32_t k = 0; k < post->length; k++)
						pixel++;
					pixel++; // dummy value in PC gfx
					post = (const post_t *)pixel;
					break;
				}
			}

			if (!foundDuplicate)
			{
				pixel = post->data;
				for (int32_t j = 0; j < post->length; j++)
				{
#ifdef REMOVE_MEGADRIVE_THRUCOLOR
					if (*pixel == 0xfc)
						*dataPtr++ = 0xd0;
					else
#endif
					*dataPtr++ = *pixel;
					jagPostCache[numJagPostCache].data[j] = *pixel;
					pixel++;
				}

				jagPostCache[numJagPostCache].length = jagPost->length;
				jagPostCache[numJagPostCache].address = swap_endian16(jagPost->dataofs);
				numJagPostCache++;

				pixel++; // dummy value in PC gfx
				post = (const post_t *)pixel;
			}
			headerPtr += sizeof(jagPost_t);
		}

		*headerPtr++ = 0xff;
		*headerPtr++ = 0xff;
	}

	*jagHeaderLen = headerPtr - jagHeader;
	*jagDataLen = dataPtr - jagData;
}

byte *JagSpriteToPNG(byte *jagHeader, byte *jagData, size_t headerLen, size_t dataLen, int32_t *outputLen)
{
	jagPatchHeader_t *header = (jagPatchHeader_t *)jagHeader;

	uint16_t width = swap_endian16(header->width);
	uint16_t height = swap_endian16(header->height);
	uint16_t leftOffset = swap_endian16(header->leftoffset);
	uint16_t topOffset = swap_endian16(header->topoffset);

	byte *rawImage = (byte *)malloc(width * height * 1);
	memset(rawImage, 247, width * height * 1); // Transparent value

	for (int32_t i = 0; i < width; i++)
	{
		uint16_t colOffset = swap_endian16(header->columnofs[i]);
		const jagPost_t *post = (jagPost_t *)(jagHeader + colOffset);

		int32_t yPos = 0;
		while (post->topdelta != 255)
		{
			yPos = post->topdelta;
			byte len = post->length;
			uint16_t dataOffset = swap_endian16(post->dataofs);

			const byte *pixel = &jagData[dataOffset];

			for (int32_t j = 0; j < post->length; j++)
			{
				size_t pixelLocation = (yPos * width) + i;

				rawImage[pixelLocation] = *pixel;
				pixel++;
				yPos++;
			}

			post++;
		}
	}

	return stbi_write_png_to_mem(rawImage, 0, width, height, 1, outputLen);
}

byte *RawToPatch(byte *rawImage, int32_t width, int32_t height, int32_t *outputLen, byte transparentIndex)
{
	// Modern memory is cheap, so let's just allocate 1mb as workspace.
	byte *postData = (byte *)malloc(1 * 1024 * 1024);
	size_t postDataSize = 0;

	patchHeader_t header;
	header.leftoffset = 0x23;
	header.topoffset = 0x3b;
	header.width = (uint16_t)width;
	header.height = (uint16_t)height;

	uint32_t columnOfs[4096]; // Again, because memory is cheap...
	size_t numColumnOfs = 0;
	uint32_t nextAvailableColOf = 0; // Not going to know this start point until we finish (dependent on the # of posts we end up with)

	post_t post;
	post.unused = 0;

	for (int32_t x = 0; x < width; x++)
	{
		int32_t y = 0;
		bool lookingForColStart = true;

		bool wroteNothing = true;

		int32_t colLength = 0;
		while (y < height)
		{
			// Get pixel value
			byte pixel = GetPixel(rawImage, width, x, y);

			if (pixel == transparentIndex && !lookingForColStart)
			{
				lookingForColStart = true;

				// Flush post data
				postData[postDataSize++] = post.topdelta;
				postData[postDataSize++] = post.length;
				postData[postDataSize++] = post.unused;
				for (int i = 0; i < post.length; i++)
					postData[postDataSize++] = post.data[i];

				postData[postDataSize++] = post.data[post.length - 1]; // dummy byte
				colLength += post.length + 4;
			}
			else if (pixel != transparentIndex && lookingForColStart)
			{
				wroteNothing = false;

				post.topdelta = y;
				post.length = 0;
				post.unused = pixel;
				lookingForColStart = false;
				post.data[post.length++] = pixel;
			}
			else if (pixel != transparentIndex && !lookingForColStart)
			{
				// Add an additional pixel to this post
				post.data[post.length++] = pixel;
			}

			y++;
		}

		if (wroteNothing)
		{
			postData[postDataSize++] = 255; // End marker

			columnOfs[numColumnOfs++] = nextAvailableColOf;
			nextAvailableColOf += 1;
		}
		else if (!lookingForColStart) // Instead of hitting a transparent pixel to end this post, we hit the literal bottom of the image.
		{
			// Flush post data
			postData[postDataSize++] = post.topdelta;
			postData[postDataSize++] = post.length;
			postData[postDataSize++] = post.unused;
			for (int32_t i = 0; i < post.length; i++)
				postData[postDataSize++] = post.data[i];

			postData[postDataSize++] = post.data[post.length - 1]; // dummy byte

			postData[postDataSize++] = 255; // End marker

			colLength += post.length + 5;

			columnOfs[numColumnOfs++] = nextAvailableColOf;
			nextAvailableColOf += colLength;
		}
		else
		{
			postData[postDataSize++] = 255; // End marker

			colLength++;

			columnOfs[numColumnOfs++] = nextAvailableColOf;
			nextAvailableColOf += colLength;
		}
	}

	// Now put it all together
	size_t lumpSize = 8 + (numColumnOfs * 4) + (postDataSize);
	byte *patchImage = (byte *)malloc(lumpSize);

	byte *cursor = patchImage;
	// Write the header
	memcpy(cursor, &header, 8); // Only first 8 bytes of header
	cursor += 8;

	// translate the columnofs positions now that we know how big columnofs is going to be
	for (size_t i = 0; i < numColumnOfs; i++)
		columnOfs[i] += (uint32_t)(8 + (numColumnOfs * 4));

	// Write the columnofs information
	memcpy(cursor, columnOfs, sizeof(uint32_t) * numColumnOfs);
	cursor += sizeof(uint32_t) * numColumnOfs;

	// Finally, write the column post data
	memcpy(cursor, postData, postDataSize);

	// Cleanup
	free(postData);

	*outputLen = lumpSize;

	return patchImage;
}

byte *PNGToPatch(byte *pngData, size_t dataLen, int32_t *outputLen, byte transparentIndex)
{
	int32_t width, height;
	byte *indexedImage = PNGToFlat(pngData, dataLen, &width, &height);

	byte *png = RawToPatch(indexedImage, width, height, outputLen, transparentIndex);

	free(indexedImage);

	return png;
}
/*
void *PNGToJagSprite(byte *pngData, size_t pngLen, byte *sprHeader, int32_t *headerLen, byte *sprData, int32_t *dataLen)
{
	return nullptr;
}
*/