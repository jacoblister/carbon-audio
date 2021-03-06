// kX DSP Generated file

// 10kX microcode
// Patch name: 'timbresel16'

char *timbresel16_copyright="(c) Max Mikhailov and Eugene Gavrilov, 2002-2003";
char *timbresel16_engine="kX";
char *timbresel16_comment="v0.7, beta";
char *timbresel16_created="2002/02/15";
char *timbresel16_guid="C439111A-1E49-4c32-B31D-84A901142D33";

char *timbresel16_name="timbresel16";
int timbresel16_itramsize=0,timbresel16_xtramsize=0;

efx_register_info timbresel16_info[]={
	{ "in_0",0x4000,0x7,0xffff,0x0 },
	{ "in_1",0x4001,0x7,0xffff,0x0 },
	{ "in_2",0x4002,0x7,0xffff,0x0 },
	{ "in_3",0x4003,0x7,0xffff,0x0 },
	{ "in_4",0x4004,0x7,0xffff,0x0 },
	{ "in_5",0x4005,0x7,0xffff,0x0 },
	{ "in_6",0x4006,0x7,0xffff,0x0 },
	{ "in_7",0x4007,0x7,0xffff,0x0 },
	{ "in_8",0x4008,0x7,0xffff,0x0 },
	{ "in_9",0x4009,0x7,0xffff,0x0 },
	{ "in_10",0x400a,0x7,0xffff,0x0 },
	{ "in_11",0x400b,0x7,0xffff,0x0 },
	{ "in_12",0x400c,0x7,0xffff,0x0 },
	{ "in_13",0x400d,0x7,0xffff,0x0 },
	{ "in_14",0x400e,0x7,0xffff,0x0 },
	{ "in_15",0x400f,0x7,0xffff,0x0 },
	{ "insel_0",0x4010,0x7,0xffff,0x0 },
	{ "insel_1",0x4011,0x7,0xffff,0x0 },
	{ "out_0",0x8000,0x8,0xffff,0x0 },
	{ "out_1",0x8001,0x8,0xffff,0x0 },
	{ "out_2",0x8002,0x8,0xffff,0x0 },
	{ "out_3",0x8003,0x8,0xffff,0x0 },
	{ "out_4",0x8004,0x8,0xffff,0x0 },
	{ "out_5",0x8005,0x8,0xffff,0x0 },
	{ "out_6",0x8006,0x8,0xffff,0x0 },
	{ "out_7",0x8007,0x8,0xffff,0x0 },
	{ "out_8",0x8008,0x8,0xffff,0x0 },
	{ "out_9",0x8009,0x8,0xffff,0x0 },
	{ "out_10",0x800a,0x8,0xffff,0x0 },
	{ "out_11",0x800b,0x8,0xffff,0x0 },
	{ "out_12",0x800c,0x8,0xffff,0x0 },
	{ "out_13",0x800d,0x8,0xffff,0x0 },
	{ "out_14",0x800e,0x8,0xffff,0x0 },
	{ "out_15",0x800f,0x8,0xffff,0x0 },
	{ "ha1_0",0x8010,0x4,0xffff,0xcb71380d },
	{ "hb0_0",0x8011,0x4,0xffff,0x20000000 },
	{ "hb1_0",0x8012,0x4,0xffff,0xf2dc4e03 },
	{ "la1_0",0x8013,0x4,0xffff,0x827d1f6e },
	{ "lb1_0",0x8014,0x4,0xffff,0x827d1f6e },
	{ "ha1_1",0x8015,0x4,0xffff,0xcb71380d },
	{ "hb0_1",0x8016,0x4,0xffff,0x20000000 },
	{ "hb1_1",0x8017,0x4,0xffff,0xf2dc4e03 },
	{ "la1_1",0x8018,0x4,0xffff,0x827d1f6e },
	{ "lb1_1",0x8019,0x4,0xffff,0x827d1f6e },
	{ "ha1_2",0x801a,0x4,0xffff,0xcb71380d },
	{ "hb0_2",0x801b,0x4,0xffff,0x20000000 },
	{ "hb1_2",0x801c,0x4,0xffff,0xf2dc4e03 },
	{ "la1_2",0x801d,0x4,0xffff,0x827d1f6e },
	{ "lb1_2",0x801e,0x4,0xffff,0x827d1f6e },
	{ "ha1_3",0x801f,0x4,0xffff,0xcb71380d },
	{ "hb0_3",0x8020,0x4,0xffff,0x20000000 },
	{ "hb1_3",0x8021,0x4,0xffff,0xf2dc4e03 },
	{ "la1_3",0x8022,0x4,0xffff,0x827d1f6e },
	{ "lb1_3",0x8023,0x4,0xffff,0x827d1f6e },
	{ "ha1_4",0x8024,0x4,0xffff,0xcb71380d },
	{ "hb0_4",0x8025,0x4,0xffff,0x20000000 },
	{ "hb1_4",0x8026,0x4,0xffff,0xf2dc4e03 },
	{ "la1_4",0x8027,0x4,0xffff,0x827d1f6e },
	{ "lb1_4",0x8028,0x4,0xffff,0x827d1f6e },
	{ "ha1_5",0x8029,0x4,0xffff,0xcb71380d },
	{ "hb0_5",0x802a,0x4,0xffff,0x20000000 },
	{ "hb1_5",0x802b,0x4,0xffff,0xf2dc4e03 },
	{ "la1_5",0x802c,0x4,0xffff,0x827d1f6e },
	{ "lb1_5",0x802d,0x4,0xffff,0x827d1f6e },
	{ "ha1_6",0x802e,0x4,0xffff,0xcb71380d },
	{ "hb0_6",0x802f,0x4,0xffff,0x20000000 },
	{ "hb1_6",0x8030,0x4,0xffff,0xf2dc4e03 },
	{ "la1_6",0x8031,0x4,0xffff,0x827d1f6e },
	{ "lb1_6",0x8032,0x4,0xffff,0x827d1f6e },
	{ "ha1_7",0x8033,0x4,0xffff,0xcb71380d },
	{ "hb0_7",0x8034,0x4,0xffff,0x20000000 },
	{ "hb1_7",0x8035,0x4,0xffff,0xf2dc4e03 },
	{ "la1_7",0x8036,0x4,0xffff,0x827d1f6e },
	{ "lb1_7",0x8037,0x4,0xffff,0x827d1f6e },
	{ "ha1_8",0x8038,0x4,0xffff,0xcb71380d },
	{ "hb0_8",0x8039,0x4,0xffff,0x20000000 },
	{ "hb1_8",0x803a,0x4,0xffff,0xf2dc4e03 },
	{ "la1_8",0x803b,0x4,0xffff,0x827d1f6e },
	{ "lb1_8",0x803c,0x4,0xffff,0x827d1f6e },
	{ "ha1_9",0x803d,0x4,0xffff,0xcb71380d },
	{ "hb0_9",0x803e,0x4,0xffff,0x20000000 },
	{ "hb1_9",0x803f,0x4,0xffff,0xf2dc4e03 },
	{ "la1_9",0x8040,0x4,0xffff,0x827d1f6e },
	{ "lb1_9",0x8041,0x4,0xffff,0x827d1f6e },
	{ "ha1_10",0x8042,0x4,0xffff,0xcb71380d },
	{ "hb0_10",0x8043,0x4,0xffff,0x20000000 },
	{ "hb1_10",0x8044,0x4,0xffff,0xf2dc4e03 },
	{ "la1_10",0x8045,0x4,0xffff,0x827d1f6e },
	{ "lb1_10",0x8046,0x4,0xffff,0x827d1f6e },
	{ "ha1_11",0x8047,0x4,0xffff,0xcb71380d },
	{ "hb0_11",0x8048,0x4,0xffff,0x20000000 },
	{ "hb1_11",0x8049,0x4,0xffff,0xf2dc4e03 },
	{ "la1_11",0x804a,0x4,0xffff,0x827d1f6e },
	{ "lb1_11",0x804b,0x4,0xffff,0x827d1f6e },
	{ "ha1_12",0x804c,0x4,0xffff,0xcb71380d },
	{ "hb0_12",0x804d,0x4,0xffff,0x20000000 },
	{ "hb1_12",0x804e,0x4,0xffff,0xf2dc4e03 },
	{ "la1_12",0x804f,0x4,0xffff,0x827d1f6e },
	{ "lb1_12",0x8050,0x4,0xffff,0x827d1f6e },
	{ "ha1_13",0x8051,0x4,0xffff,0xcb71380d },
	{ "hb0_13",0x8052,0x4,0xffff,0x20000000 },
	{ "hb1_13",0x8053,0x4,0xffff,0xf2dc4e03 },
	{ "la1_13",0x8054,0x4,0xffff,0x827d1f6e },
	{ "lb1_13",0x8055,0x4,0xffff,0x827d1f6e },
	{ "ha1_14",0x8056,0x4,0xffff,0xcb71380d },
	{ "hb0_14",0x8057,0x4,0xffff,0x20000000 },
	{ "hb1_14",0x8058,0x4,0xffff,0xf2dc4e03 },
	{ "la1_14",0x8059,0x4,0xffff,0x827d1f6e },
	{ "lb1_14",0x805a,0x4,0xffff,0x827d1f6e },
	{ "ha1_15",0x805b,0x4,0xffff,0xcb71380d },
	{ "hb0_15",0x805c,0x4,0xffff,0x20000000 },
	{ "hb1_15",0x805d,0x4,0xffff,0xf2dc4e03 },
	{ "la1_15",0x805e,0x4,0xffff,0x827d1f6e },
	{ "lb1_15",0x805f,0x4,0xffff,0x827d1f6e },
	{ "inchan_0",0x8060,0x4,0xffff,0x0 },
	{ "inchan_1",0x8061,0x4,0xffff,0x0 },
	{ "lstate_0",0x8062,0x1,0xffff,0x0 },
	{ "hstate_0",0x8063,0x1,0xffff,0x0 },
	{ "lstate_1",0x8064,0x1,0xffff,0x0 },
	{ "hstate_1",0x8065,0x1,0xffff,0x0 },
	{ "lstate_2",0x8066,0x1,0xffff,0x0 },
	{ "hstate_2",0x8067,0x1,0xffff,0x0 },
	{ "lstate_3",0x8068,0x1,0xffff,0x0 },
	{ "hstate_3",0x8069,0x1,0xffff,0x0 },
	{ "lstate_4",0x806a,0x1,0xffff,0x0 },
	{ "hstate_4",0x806b,0x1,0xffff,0x0 },
	{ "lstate_5",0x806c,0x1,0xffff,0x0 },
	{ "hstate_5",0x806d,0x1,0xffff,0x0 },
	{ "lstate_6",0x806e,0x1,0xffff,0x0 },
	{ "hstate_6",0x806f,0x1,0xffff,0x0 },
	{ "lstate_7",0x8070,0x1,0xffff,0x0 },
	{ "hstate_7",0x8071,0x1,0xffff,0x0 },
	{ "lstate_8",0x8072,0x1,0xffff,0x0 },
	{ "hstate_8",0x8073,0x1,0xffff,0x0 },
	{ "lstate_9",0x8074,0x1,0xffff,0x0 },
	{ "hstate_9",0x8075,0x1,0xffff,0x0 },
	{ "lstate_10",0x8076,0x1,0xffff,0x0 },
	{ "hstate_10",0x8077,0x1,0xffff,0x0 },
	{ "lstate_11",0x8078,0x1,0xffff,0x0 },
	{ "hstate_11",0x8079,0x1,0xffff,0x0 },
	{ "lstate_12",0x807a,0x1,0xffff,0x0 },
	{ "hstate_12",0x807b,0x1,0xffff,0x0 },
	{ "lstate_13",0x807c,0x1,0xffff,0x0 },
	{ "hstate_13",0x807d,0x1,0xffff,0x0 },
	{ "lstate_14",0x807e,0x1,0xffff,0x0 },
	{ "hstate_14",0x807f,0x1,0xffff,0x0 },
	{ "lstate_15",0x8080,0x1,0xffff,0x0 },
	{ "hstate_15",0x8081,0x1,0xffff,0x0 },
	{ "_ACfffffffd",0x8082,0x1,0xffff,0xfffffffd },
	{ "_ACfffffffc",0x8083,0x1,0xffff,0xfffffffc },
	{ "_ACfffffffb",0x8084,0x1,0xffff,0xfffffffb },
	{ "_ACfffffffa",0x8085,0x1,0xffff,0xfffffffa },
	{ "_ACfffffff9",0x8086,0x1,0xffff,0xfffffff9 },
	{ "_ACfffffff8",0x8087,0x1,0xffff,0xfffffff8 },
	{ "_ACfffffff7",0x8088,0x1,0xffff,0xfffffff7 },
	{ "_ACfffffff6",0x8089,0x1,0xffff,0xfffffff6 },
	{ "_ACfffffff5",0x808a,0x1,0xffff,0xfffffff5 },
	{ "_ACfffffff4",0x808b,0x1,0xffff,0xfffffff4 },
	{ "_ACfffffff3",0x808c,0x1,0xffff,0xfffffff3 },
	{ "_ACfffffff2",0x808d,0x1,0xffff,0xfffffff2 },
	{ "_ACfffffff1",0x808e,0x1,0xffff,0xfffffff1 },
	{ "x",0x808f,0x3,0xffff,0x0 },
	{ "y",0x8090,0x3,0xffff,0x0 },
};

efx_code timbresel16_code[]={
	{ 0x6,0x8000,0x4000,0x2040,0x2040 },
	{ 0x6,0x2040,0x8060,0x2040,0x2040 },
	{ 0xf,0x2040,0x2057,0x2048,0x2041 },
	{ 0x6,0x8000,0x8000,0x4010,0x2040 },
	{ 0x6,0x2040,0x8061,0x2040,0x2040 },
	{ 0xf,0x2040,0x2057,0x2048,0x2041 },
	{ 0x6,0x8000,0x8000,0x4011,0x2040 },
	{ 0x0,0x8090,0x8062,0x8000,0x204f },
	{ 0x0,0x8062,0x2040,0x8000,0x8014 },
	{ 0x1,0x8062,0x2056,0x8090,0x8013 },
	{ 0x0,0x808f,0x8063,0x8090,0x8011 },
	{ 0x0,0x8063,0x2046,0x8090,0x8012 },
	{ 0x1,0x8063,0x2056,0x808f,0x8010 },
	{ 0x4,0x8000,0x2040,0x808f,0x2044 },
	{ 0x6,0x8001,0x4001,0x2040,0x2040 },
	{ 0x6,0x2040,0x8060,0x2050,0x2040 },
	{ 0xf,0x2040,0x2057,0x2048,0x2041 },
	{ 0x6,0x8001,0x8001,0x4010,0x2040 },
	{ 0x6,0x2040,0x8061,0x2050,0x2040 },
	{ 0xf,0x2040,0x2057,0x2048,0x2041 },
	{ 0x6,0x8001,0x8001,0x4011,0x2040 },
	{ 0x0,0x8090,0x8064,0x8001,0x204f },
	{ 0x0,0x8064,0x2040,0x8001,0x8019 },
	{ 0x1,0x8064,0x2056,0x8090,0x8018 },
	{ 0x0,0x808f,0x8065,0x8090,0x8016 },
	{ 0x0,0x8065,0x2046,0x8090,0x8017 },
	{ 0x1,0x8065,0x2056,0x808f,0x8015 },
	{ 0x4,0x8001,0x2040,0x808f,0x2044 },
	{ 0x6,0x8002,0x4002,0x2040,0x2040 },
	{ 0x6,0x2040,0x8060,0x2051,0x2040 },
	{ 0xf,0x2040,0x2057,0x2048,0x2041 },
	{ 0x6,0x8002,0x8002,0x4010,0x2040 },
	{ 0x6,0x2040,0x8061,0x2051,0x2040 },
	{ 0xf,0x2040,0x2057,0x2048,0x2041 },
	{ 0x6,0x8002,0x8002,0x4011,0x2040 },
	{ 0x0,0x8090,0x8066,0x8002,0x204f },
	{ 0x0,0x8066,0x2040,0x8002,0x801e },
	{ 0x1,0x8066,0x2056,0x8090,0x801d },
	{ 0x0,0x808f,0x8067,0x8090,0x801b },
	{ 0x0,0x8067,0x2046,0x8090,0x801c },
	{ 0x1,0x8067,0x2056,0x808f,0x801a },
	{ 0x4,0x8002,0x2040,0x808f,0x2044 },
	{ 0x6,0x8003,0x4003,0x2040,0x2040 },
	{ 0x6,0x2040,0x8060,0x8082,0x2040 },
	{ 0xf,0x2040,0x2057,0x2048,0x2041 },
	{ 0x6,0x8003,0x8003,0x4010,0x2040 },
	{ 0x6,0x2040,0x8061,0x8082,0x2040 },
	{ 0xf,0x2040,0x2057,0x2048,0x2041 },
	{ 0x6,0x8003,0x8003,0x4011,0x2040 },
	{ 0x0,0x8090,0x8068,0x8003,0x204f },
	{ 0x0,0x8068,0x2040,0x8003,0x8023 },
	{ 0x1,0x8068,0x2056,0x8090,0x8022 },
	{ 0x0,0x808f,0x8069,0x8090,0x8020 },
	{ 0x0,0x8069,0x2046,0x8090,0x8021 },
	{ 0x1,0x8069,0x2056,0x808f,0x801f },
	{ 0x4,0x8003,0x2040,0x808f,0x2044 },
	{ 0x6,0x8004,0x4004,0x2040,0x2040 },
	{ 0x6,0x2040,0x8060,0x8083,0x2040 },
	{ 0xf,0x2040,0x2057,0x2048,0x2041 },
	{ 0x6,0x8004,0x8004,0x4010,0x2040 },
	{ 0x6,0x2040,0x8061,0x8083,0x2040 },
	{ 0xf,0x2040,0x2057,0x2048,0x2041 },
	{ 0x6,0x8004,0x8004,0x4011,0x2040 },
	{ 0x0,0x8090,0x806a,0x8004,0x204f },
	{ 0x0,0x806a,0x2040,0x8004,0x8028 },
	{ 0x1,0x806a,0x2056,0x8090,0x8027 },
	{ 0x0,0x808f,0x806b,0x8090,0x8025 },
	{ 0x0,0x806b,0x2046,0x8090,0x8026 },
	{ 0x1,0x806b,0x2056,0x808f,0x8024 },
	{ 0x4,0x8004,0x2040,0x808f,0x2044 },
	{ 0x6,0x8005,0x4005,0x2040,0x2040 },
	{ 0x6,0x2040,0x8060,0x8084,0x2040 },
	{ 0xf,0x2040,0x2057,0x2048,0x2041 },
	{ 0x6,0x8005,0x8005,0x4010,0x2040 },
	{ 0x6,0x2040,0x8061,0x8084,0x2040 },
	{ 0xf,0x2040,0x2057,0x2048,0x2041 },
	{ 0x6,0x8005,0x8005,0x4011,0x2040 },
	{ 0x0,0x8090,0x806c,0x8005,0x204f },
	{ 0x0,0x806c,0x2040,0x8005,0x802d },
	{ 0x1,0x806c,0x2056,0x8090,0x802c },
	{ 0x0,0x808f,0x806d,0x8090,0x802a },
	{ 0x0,0x806d,0x2046,0x8090,0x802b },
	{ 0x1,0x806d,0x2056,0x808f,0x8029 },
	{ 0x4,0x8005,0x2040,0x808f,0x2044 },
	{ 0x6,0x8006,0x4006,0x2040,0x2040 },
	{ 0x6,0x2040,0x8060,0x8085,0x2040 },
	{ 0xf,0x2040,0x2057,0x2048,0x2041 },
	{ 0x6,0x8006,0x8006,0x4010,0x2040 },
	{ 0x6,0x2040,0x8061,0x8085,0x2040 },
	{ 0xf,0x2040,0x2057,0x2048,0x2041 },
	{ 0x6,0x8006,0x8006,0x4011,0x2040 },
	{ 0x0,0x8090,0x806e,0x8006,0x204f },
	{ 0x0,0x806e,0x2040,0x8006,0x8032 },
	{ 0x1,0x806e,0x2056,0x8090,0x8031 },
	{ 0x0,0x808f,0x806f,0x8090,0x802f },
	{ 0x0,0x806f,0x2046,0x8090,0x8030 },
	{ 0x1,0x806f,0x2056,0x808f,0x802e },
	{ 0x4,0x8006,0x2040,0x808f,0x2044 },
	{ 0x6,0x8007,0x4007,0x2040,0x2040 },
	{ 0x6,0x2040,0x8060,0x8086,0x2040 },
	{ 0xf,0x2040,0x2057,0x2048,0x2041 },
	{ 0x6,0x8007,0x8007,0x4010,0x2040 },
	{ 0x6,0x2040,0x8061,0x8086,0x2040 },
	{ 0xf,0x2040,0x2057,0x2048,0x2041 },
	{ 0x6,0x8007,0x8007,0x4011,0x2040 },
	{ 0x0,0x8090,0x8070,0x8007,0x204f },
	{ 0x0,0x8070,0x2040,0x8007,0x8037 },
	{ 0x1,0x8070,0x2056,0x8090,0x8036 },
	{ 0x0,0x808f,0x8071,0x8090,0x8034 },
	{ 0x0,0x8071,0x2046,0x8090,0x8035 },
	{ 0x1,0x8071,0x2056,0x808f,0x8033 },
	{ 0x4,0x8007,0x2040,0x808f,0x2044 },
	{ 0x6,0x8008,0x4008,0x2040,0x2040 },
	{ 0x6,0x2040,0x8060,0x8087,0x2040 },
	{ 0xf,0x2040,0x2057,0x2048,0x2041 },
	{ 0x6,0x8008,0x8008,0x4010,0x2040 },
	{ 0x6,0x2040,0x8061,0x8087,0x2040 },
	{ 0xf,0x2040,0x2057,0x2048,0x2041 },
	{ 0x6,0x8008,0x8008,0x4011,0x2040 },
	{ 0x0,0x8090,0x8072,0x8008,0x204f },
	{ 0x0,0x8072,0x2040,0x8008,0x803c },
	{ 0x1,0x8072,0x2056,0x8090,0x803b },
	{ 0x0,0x808f,0x8073,0x8090,0x8039 },
	{ 0x0,0x8073,0x2046,0x8090,0x803a },
	{ 0x1,0x8073,0x2056,0x808f,0x8038 },
	{ 0x4,0x8008,0x2040,0x808f,0x2044 },
	{ 0x6,0x8009,0x4009,0x2040,0x2040 },
	{ 0x6,0x2040,0x8060,0x8088,0x2040 },
	{ 0xf,0x2040,0x2057,0x2048,0x2041 },
	{ 0x6,0x8009,0x8009,0x4010,0x2040 },
	{ 0x6,0x2040,0x8061,0x8088,0x2040 },
	{ 0xf,0x2040,0x2057,0x2048,0x2041 },
	{ 0x6,0x8009,0x8009,0x4011,0x2040 },
	{ 0x0,0x8090,0x8074,0x8009,0x204f },
	{ 0x0,0x8074,0x2040,0x8009,0x8041 },
	{ 0x1,0x8074,0x2056,0x8090,0x8040 },
	{ 0x0,0x808f,0x8075,0x8090,0x803e },
	{ 0x0,0x8075,0x2046,0x8090,0x803f },
	{ 0x1,0x8075,0x2056,0x808f,0x803d },
	{ 0x4,0x8009,0x2040,0x808f,0x2044 },
	{ 0x6,0x800a,0x400a,0x2040,0x2040 },
	{ 0x6,0x2040,0x8060,0x8089,0x2040 },
	{ 0xf,0x2040,0x2057,0x2048,0x2041 },
	{ 0x6,0x800a,0x800a,0x4010,0x2040 },
	{ 0x6,0x2040,0x8061,0x8089,0x2040 },
	{ 0xf,0x2040,0x2057,0x2048,0x2041 },
	{ 0x6,0x800a,0x800a,0x4011,0x2040 },
	{ 0x0,0x8090,0x8076,0x800a,0x204f },
	{ 0x0,0x8076,0x2040,0x800a,0x8046 },
	{ 0x1,0x8076,0x2056,0x8090,0x8045 },
	{ 0x0,0x808f,0x8077,0x8090,0x8043 },
	{ 0x0,0x8077,0x2046,0x8090,0x8044 },
	{ 0x1,0x8077,0x2056,0x808f,0x8042 },
	{ 0x4,0x800a,0x2040,0x808f,0x2044 },
	{ 0x6,0x800b,0x400b,0x2040,0x2040 },
	{ 0x6,0x2040,0x8060,0x808a,0x2040 },
	{ 0xf,0x2040,0x2057,0x2048,0x2041 },
	{ 0x6,0x800b,0x800b,0x4010,0x2040 },
	{ 0x6,0x2040,0x8061,0x808a,0x2040 },
	{ 0xf,0x2040,0x2057,0x2048,0x2041 },
	{ 0x6,0x800b,0x800b,0x4011,0x2040 },
	{ 0x0,0x8090,0x8078,0x800b,0x204f },
	{ 0x0,0x8078,0x2040,0x800b,0x804b },
	{ 0x1,0x8078,0x2056,0x8090,0x804a },
	{ 0x0,0x808f,0x8079,0x8090,0x8048 },
	{ 0x0,0x8079,0x2046,0x8090,0x8049 },
	{ 0x1,0x8079,0x2056,0x808f,0x8047 },
	{ 0x4,0x800b,0x2040,0x808f,0x2044 },
	{ 0x6,0x800c,0x400c,0x2040,0x2040 },
	{ 0x6,0x2040,0x8060,0x808b,0x2040 },
	{ 0xf,0x2040,0x2057,0x2048,0x2041 },
	{ 0x6,0x800c,0x800c,0x4010,0x2040 },
	{ 0x6,0x2040,0x8061,0x808b,0x2040 },
	{ 0xf,0x2040,0x2057,0x2048,0x2041 },
	{ 0x6,0x800c,0x800c,0x4011,0x2040 },
	{ 0x0,0x8090,0x807a,0x800c,0x204f },
	{ 0x0,0x807a,0x2040,0x800c,0x8050 },
	{ 0x1,0x807a,0x2056,0x8090,0x804f },
	{ 0x0,0x808f,0x807b,0x8090,0x804d },
	{ 0x0,0x807b,0x2046,0x8090,0x804e },
	{ 0x1,0x807b,0x2056,0x808f,0x804c },
	{ 0x4,0x800c,0x2040,0x808f,0x2044 },
	{ 0x6,0x800d,0x400d,0x2040,0x2040 },
	{ 0x6,0x2040,0x8060,0x808c,0x2040 },
	{ 0xf,0x2040,0x2057,0x2048,0x2041 },
	{ 0x6,0x800d,0x800d,0x4010,0x2040 },
	{ 0x6,0x2040,0x8061,0x808c,0x2040 },
	{ 0xf,0x2040,0x2057,0x2048,0x2041 },
	{ 0x6,0x800d,0x800d,0x4011,0x2040 },
	{ 0x0,0x8090,0x807c,0x800d,0x204f },
	{ 0x0,0x807c,0x2040,0x800d,0x8055 },
	{ 0x1,0x807c,0x2056,0x8090,0x8054 },
	{ 0x0,0x808f,0x807d,0x8090,0x8052 },
	{ 0x0,0x807d,0x2046,0x8090,0x8053 },
	{ 0x1,0x807d,0x2056,0x808f,0x8051 },
	{ 0x4,0x800d,0x2040,0x808f,0x2044 },
	{ 0x6,0x800e,0x400e,0x2040,0x2040 },
	{ 0x6,0x2040,0x8060,0x808d,0x2040 },
	{ 0xf,0x2040,0x2057,0x2048,0x2041 },
	{ 0x6,0x800e,0x800e,0x4010,0x2040 },
	{ 0x6,0x2040,0x8061,0x808d,0x2040 },
	{ 0xf,0x2040,0x2057,0x2048,0x2041 },
	{ 0x6,0x800e,0x800e,0x4011,0x2040 },
	{ 0x0,0x8090,0x807e,0x800e,0x204f },
	{ 0x0,0x807e,0x2040,0x800e,0x805a },
	{ 0x1,0x807e,0x2056,0x8090,0x8059 },
	{ 0x0,0x808f,0x807f,0x8090,0x8057 },
	{ 0x0,0x807f,0x2046,0x8090,0x8058 },
	{ 0x1,0x807f,0x2056,0x808f,0x8056 },
	{ 0x4,0x800e,0x2040,0x808f,0x2044 },
	{ 0x6,0x800f,0x400f,0x2040,0x2040 },
	{ 0x6,0x2040,0x8060,0x808e,0x2040 },
	{ 0xf,0x2040,0x2057,0x2048,0x2041 },
	{ 0x6,0x800f,0x800f,0x4010,0x2040 },
	{ 0x6,0x2040,0x8061,0x808e,0x2040 },
	{ 0xf,0x2040,0x2057,0x2048,0x2041 },
	{ 0x6,0x800f,0x800f,0x4011,0x2040 },
	{ 0x0,0x8090,0x8080,0x800f,0x204f },
	{ 0x0,0x8080,0x2040,0x800f,0x805f },
	{ 0x1,0x8080,0x2056,0x8090,0x805e },
	{ 0x0,0x808f,0x8081,0x8090,0x805c },
	{ 0x0,0x8081,0x2046,0x8090,0x805d },
	{ 0x1,0x8081,0x2056,0x808f,0x805b },
	{ 0x4,0x800f,0x2040,0x808f,0x2044 },
};

