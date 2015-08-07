// kX DSP Generated file

// 10kX microcode
// Patch name: 'TimbreX'

char *TimbreX_copyright="(c) Max Mikhailov and Eugene Gavrilov, 2002-2004";
char *TimbreX_engine="kX";
char *TimbreX_comment="v0.7, beta";
char *TimbreX_created="2002/02/15";
char *TimbreX_guid="B7D0E20B-60B4-4715-B233-08171476E0C1";

char *TimbreX_name="TimbreX";
int TimbreX_itramsize=0,TimbreX_xtramsize=0;

efx_register_info TimbreX_info[]={
	{ "in",0x4000,0x7,0xffff,0x0 },
	{ "out",0x8000,0x8,0xffff,0x0 },
	{ "ha1",0x8001,0x4,0xffff,0xcb71380d },
	{ "hb0",0x8002,0x4,0xffff,0x203addf1 },
	{ "hb1",0x8003,0x4,0xffff,0xf2a17012 },
	{ "la1",0x8004,0x4,0xffff,0x827d1f6e },
	{ "lb1",0x8005,0x4,0xffff,0x82846d62 },
	{ "lstate",0x8006,0x1,0xffff,0x0 },
	{ "hstate",0x8007,0x1,0xffff,0x0 },
	{ "x",0x8008,0x3,0xffff,0x0 },
	{ "y",0x8009,0x3,0xffff,0x0 },
};

efx_code TimbreX_code[]={
	{ 0x0,0x8009,0x8006,0x4000,0x204f },
	{ 0x0,0x8006,0x2040,0x4000,0x8005 },
	{ 0x1,0x8006,0x2056,0x8009,0x8004 },
	{ 0x0,0x8008,0x8007,0x8009,0x8002 },
	{ 0x0,0x8007,0x2046,0x8009,0x8003 },
	{ 0x1,0x8007,0x2056,0x8008,0x8001 },
	{ 0x4,0x8000,0x2040,0x8008,0x2044 },
};

