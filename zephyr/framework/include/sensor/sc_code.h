const int sc_code[1185] = {
	0x00007000, 0x0000013d, 0x000000dd, 0x000000df, 0x000000dd, 0x000000dd, 0x000000dd, 0x00000000, 
	0x00000000, 0x00000000, 0x00000000, 0x000000dd, 0x000000dd, 0x00000000, 0x000000dd, 0x00000169, 
	0x000000dd, 0x000000dd, 0x000000dd, 0x000000dd, 0x000000dd, 0x000000dd, 0x00000145, 0x00000157, 
	0x000000e1, 0x000000f3, 0x000000dd, 0x00000105, 0x000000dd, 0x00000000, 0x00000000, 0x00000000, 
	0xd00cf8df, 0xf818f000, 0x47004800, 0x00000761, 0x00007000, 0x0301ea40, 0xd003079b, 0xc908e009, 
	0xc0081f12, 0xd2fa2a04, 0xf811e003, 0xf8003b01, 0x1e523b01, 0x4770d2f9, 0x4d074c06, 0x68e0e006, 
	0x0301f040, 0x0007e894, 0x34104798, 0xd3f642ac, 0xffdaf7ff, 0x00001258, 0x00001278, 0xe7fee7fe, 
	0x2008b510, 0xf86cf000, 0x4010e8bd, 0xf0002000, 0xb510bcbb, 0xf0002009, 0xe8bdf863, 0x20014010, 
	0xbcb2f000, 0x200bb510, 0xf85af000, 0x20014909, 0x0114f8c1, 0x14c4f44f, 0x2300e004, 0x21144a06, 
	0xfe28f000, 0xf50088e0, 0x880110c4, 0x42918882, 0xbd10d1f3, 0x40020000, 0x000007b1, 0xf824f000, 
	0xff9ef7ff, 0x2006b510, 0xf83af000, 0x4010e8bd, 0xf0002000, 0xb510bd15, 0xf0002007, 0xe8bdf831, 
	0x20014010, 0xbd0cf000, 0xf000b510, 0x4805f873, 0x68826841, 0x60414411, 0x60812100, 0x1c496801, 
	0xbd106001, 0x00003000, 0x20004909, 0x480961c8, 0xf8c02113, 0x21f11100, 0x49086001, 0x60084806, 
	0x30804806, 0xf4416801, 0x60010170, 0x00004770, 0x40010000, 0x40001000, 0x00000000, 0xe000ed08, 
	0xdb092800, 0x021ff000, 0x40912101, 0x00800940, 0x20e0f100, 0x1280f8c0, 0x28004770, 0xf000db09, 
	0x2101021f, 0x09404091, 0xf1000080, 0xf8c020e0, 0x47701100, 0xc808e002, 0xc1081f12, 0xd1fa2a00, 
	0x47704770, 0xe0012000, 0x1f12c101, 0xd1fb2a00, 0x88824770, 0x89408801, 0xb809f000, 0xb1208980, 
	0xf0211cc9, 0x44080103, 0x46084770, 0x428a4770, 0x1a88d201, 0x1a80e001, 0x1e404408, 0x21034770, 
	0xf0f1fb90, 0x9000b508, 0x91001e41, 0x4608d301, 0xbd08e7fa, 0xf04fb510, 0x698320e0, 0x69826901, 
	0x490703cc, 0x4293d401, 0x688bd204, 0x739ff603, 0x6900608b, 0x1a806888, 0x709ff600, 0x0000bd10, 
	0x00003000, 0xfab0b120, 0xf1c0f080, 0x4770001f, 0x30fff04f, 0x00004770, 0x48030401, 0x68014408, 
	0x0140f021, 0x47706001, 0x40050000, 0x48040401, 0x69414408, 0x68016141, 0x0140f041, 0x47706001, 
	0x40050000, 0x6853b5f8, 0x78139300, 0xea4f4c2c, 0x07de4500, 0x0004eb05, 0x0301f04f, 0xfa036906, 
	0xd001f301, 0xe000431e, 0x6106439e, 0xf04f7813, 0x079e0701, 0x0304f101, 0xfa076906, 0xd501f303, 
	0xe000431e, 0x6106439e, 0xf04f7813, 0x075f0601, 0x0308f101, 0xf303fa06, 0xd5016906, 0xe000431e, 
	0x6106439e, 0xf04f7813, 0x071f0601, 0x030cf101, 0xf303fa06, 0xd5016906, 0xe000431e, 0x6106439e, 
	0xf0209800, 0xeb014300, 0xeb050041, 0x44200080, 0x68d16183, 0x0111f3c1, 0x7a1360d1, 0x0340f3c3, 
	0xea43005b, 0x8a130181, 0x5103ea41, 0x691161c1, 0xd0032900, 0xf04169c1, 0x61c10101, 0x0000bdf8, 
	0x40050000, 0x460bb510, 0x46114604, 0xf0004618, 0x4a02f811, 0x4104eb02, 0xbd106148, 0x40050000, 
	0x48030401, 0x69484401, 0x40086909, 0x00004770, 0x40050000, 0x20004602, 0x29012301, 0x2902d006, 
	0x2904d007, 0x2908d007, 0x320cd102, 0xf002fa03, 0x1d124770, 0x3208e7fa, 0x0000e7f8, 0xeb010400, 
	0xeb000141, 0x48050181, 0x69814408, 0x4100f021, 0x69816181, 0x4100f041, 0x47706181, 0x40050000, 
	0xeb010400, 0xeb000141, 0x48030181, 0x69814408, 0x4100f021, 0x47706181, 0x40050000, 0x460cb4f0, 
	0x78194615, 0x078e4a11, 0xf1a24910, 0xeb020290, 0xeb011300, 0xf10221c0, 0xeb020220, 0xeb0100c0, 
	0xf8302144, 0xd5042014, 0x2d010852, 0x2d02d004, 0xf853d109, 0xe0030024, 0x0024f853, 0x44104411, 
	0xf7ffbcf0, 0xbcf0be17, 0x00004770, 0x000032fc, 0xb4104a0c, 0x1cc46853, 0x40a12101, 0x6053430b, 
	0x68130492, 0x6013438b, 0x430b6813, 0x49066013, 0x4100eb01, 0xf042680a, 0x600a0220, 0x4903bc10, 
	0xb846f000, 0x40001000, 0x40050000, 0x000186a0, 0x460ab570, 0x25004606, 0xfeeef7ff, 0xeb004819, 
	0x68604406, 0x00506060, 0x682060a0, 0x000ff020, 0x60201d80, 0xf0004630, 0xb108f863, 0xe0101f2d, 
	0xf0004630, 0xb110f841, 0x0502f06f, 0x6860e009, 0xd5020780, 0x35fff04f, 0x6860e003, 0xd10007c0, 
	0x68201e85, 0x000ff020, 0x000af040, 0x46306020, 0xf862f000, 0xf06fb108, 0x46300504, 0xfec6f7ff, 
	0xbd704628, 0x40050000, 0xf0002200, 0x0000b875, 0xb5104a0a, 0xf1f1fbb2, 0x22004c09, 0x0380eb04, 
	0x0402611a, 0x44104807, 0xf4226802, 0x60026278, 0xea426802, 0x600111c1, 0x0000bd10, 0x001e8480, 
	0x40001000, 0x40050000, 0x4604b570, 0xf0002600, 0x490afdf3, 0xeb014605, 0xe0074404, 0xfdecf000, 
	0x28321b40, 0xf04fdd02, 0xe00536ff, 0x05c06860, 0xf44fd5f4, 0x60607080, 0xbd704630, 0x40050000, 
	0x4604b570, 0xf0002600, 0x490afdd7, 0xeb014605, 0xe0074404, 0xfdd0f000, 0x28321b40, 0xf04fdd02, 
	0xe00436ff, 0x06406860, 0x2040d5f4, 0x46306060, 0x0000bd70, 0x40050000, 0x4604b570, 0xf0002600, 
	0x490afdbb, 0xeb014605, 0xe0074404, 0xfdb4f000, 0x28321b40, 0xf04fdd02, 0xe00436ff, 0x06006860, 
	0x2080d5f4, 0x46306060, 0x0000bd70, 0x40050000, 0xf0002201, 0x0000b801, 0x5ff0e92d, 0x460d4690, 
	0x26004683, 0xfe38f7ff, 0xeb00484c, 0x6860440b, 0x89a86060, 0x39fff04f, 0x0008ea50, 0x0a01f06f, 
	0x8828d02b, 0x60a00040, 0xf0206820, 0x1d80000f, 0x46586020, 0xffa4f7ff, 0x1f30b110, 0x9ff0e8bd, 
	0xf7ff4658, 0x2800ff81, 0x6860d15d, 0xd4420780, 0x07c06860, 0x2700d047, 0x6868e00c, 0x60a05dc0, 
	0xf0206820, 0x1c80000f, 0x46586020, 0xff6cf7ff, 0x1c7fbb98, 0x42b889a8, 0xf1b8dcef, 0xd0120f00, 
	0xe00c2700, 0x5dc068a8, 0x682060a0, 0x000ff020, 0x60201c80, 0xf7ff4658, 0xb9f0ff57, 0x89e81c7f, 
	0xdcef42b8, 0x89a8e039, 0x200cb108, 0x2004e000, 0x22018829, 0x0141eb02, 0x682160a1, 0x010ff021, 
	0xf0414301, 0x60200002, 0xf7ff4658, 0xb920ff3d, 0x07806860, 0x464ed502, 0xe014e01f, 0x07c06860, 
	0x2700d001, 0x4656e016, 0x1e40e017, 0x682042b8, 0x000ff020, 0x1cc0d101, 0x1c80e000, 0x46586020, 
	0xff22f7ff, 0xf06fb110, 0xe7970002, 0x68a968a0, 0x1c7f55c8, 0x42b889e8, 0x6820dce7, 0x000ff020, 
	0x000af040, 0x46586020, 0xff46f7ff, 0xf06fb108, 0x46580604, 0xfdaaf7ff, 0xe77f4630, 0x40050000, 
	0xfd00f000, 0xfd5cf000, 0xf7ff2000, 0x2001fe81, 0xfe7ef7ff, 0xf0002000, 0x2001fc63, 0xfc60f000, 
	0xfa4ef000, 0xf7ff200b, 0x2008fd28, 0xfd25f7ff, 0xf7ff2009, 0x2006fd22, 0xfd1ff7ff, 0xf7ff2007, 
	0xb662fd1c, 0xf9b0f000, 0xf9c6f000, 0x0000e7fc, 0x41f0e92d, 0x460c8808, 0xf101460f, 0xf1010608, 
	0x49a6051c, 0xf2a04690, 0xf5015004, 0x280672c0, 0xe8dfd27e, 0x2103f000, 0xfd9a8a40, 0xb12088e0, 
	0xd0082801, 0xd1112802, 0x89a2e00b, 0x68a188a0, 0xfc90f000, 0x89a2e00a, 0x68a188a0, 0xfc52f000, 
	0x68a0e004, 0x88a07801, 0xfbf8f000, 0x88a12300, 0xf240461a, 0xe0165004, 0x462588e0, 0x2801b120, 
	0x2802d007, 0xe011d10a, 0x463188a8, 0xfef0f7ff, 0x88a8e003, 0xf7ff4631, 0x4603fe77, 0x88a9b29a, 
	0xf2402300, 0xf0005005, 0xe104f978, 0x88a88929, 0xfe2ef7ff, 0x68f0e7f1, 0xf5002218, 0x60f010c4, 
	0xeb0088a0, 0xeb010040, 0x88e11080, 0x0141eb01, 0x00c1eb00, 0xf7ff4631, 0x88a1fc0d, 0x68f04a78, 
	0x1141eb02, 0x4f7788e2, 0x0022f841, 0x88a14a74, 0xeb023240, 0x88e21101, 0xf8218a30, 0x68700012, 
	0xd4090380, 0x88e188a0, 0x3000eb07, 0x2041eb00, 0x1203e9d6, 0xfbeef7ff, 0x88e188a0, 0x3000eb07, 
	0x2041eb00, 0x88a060f0, 0xfae0f000, 0xe00088e1, 0x88a0e0b4, 0xf0004632, 0x6870fae5, 0xda5a2800, 
	0x88a088e1, 0xfb44f000, 0x88a0e0b5, 0x0240eb00, 0x1282eb01, 0xeb0188e1, 0xeb020341, 0x686204c3, 
	0xda662a00, 0xfb44f000, 0x68f0e0a5, 0x10c4f500, 0x88a060f0, 0xeb0088e1, 0xeb020040, 0xeb011040, 
	0xeb000141, 0x221800c1, 0xf7ff4631, 0x4a4cfbb3, 0x3a3088a1, 0x1101eb02, 0x68f088e2, 0xf8414f48, 
	0x4a470022, 0x3a1088a1, 0x01c1eb02, 0x8a3088e2, 0x0760f107, 0x0012f821, 0x07c07930, 0x88a0d109, 
	0xeb0788e1, 0xeb0020c0, 0xe9d62041, 0xf7ff1203, 0x88a0fb91, 0xeb0788e1, 0xeb0020c0, 0x60f02041, 
	0xf7ff88a0, 0x88e1fc93, 0x463288a0, 0xfc9af7ff, 0x28006870, 0x8828db2f, 0xf3c02100, 0xf0002085, 
	0x8828f97d, 0x2085f3c0, 0xf970f000, 0xf3c08828, 0xf3c02285, 0xf3c01144, 0xf0000043, 0x8828f943, 
	0x0101f000, 0x2085f3c0, 0xf968f000, 0xe0016829, 0xe022e016, 0xd03e07c8, 0x2085f3c1, 0xd83a2803, 
	0x23010c09, 0xf0002200, 0x8828fbdd, 0x2085f3c0, 0xfc0cf000, 0x88e1e02f, 0xf7ff88a0, 0xe02afcef, 
	0xeb0088a0, 0xeb020140, 0x88e11241, 0x0341eb01, 0x04c3eb02, 0x2a006862, 0x8aa0db0d, 0xf3c02100, 
	0xf0002085, 0x8aa0f93b, 0x2085f3c0, 0xd8122803, 0xfbdef000, 0xf7ffe00f, 0xe00cfce3, 0x24004d07, 
	0x0520f1b5, 0x2024f855, 0x4641b112, 0x47904638, 0x2c081c64, 0x4640dbf6, 0x81f0e8bd, 0x0000302c, 
	0x0000329c, 0x000042fc, 0x41f0e92d, 0x0500ea4f, 0xfc96f7ff, 0xfc06f7ff, 0xdb3e2800, 0xeb0017c1, 
	0xf0217191, 0x1a860203, 0x491d1088, 0x7010f831, 0xeb05491c, 0xeb010045, 0xeb061040, 0xeb000146, 
	0x463a04c1, 0x46284631, 0xfc6cf7ff, 0xf3c08aa0, 0x30101044, 0xf8eaf000, 0x28006860, 0x8aa0db0b, 
	0x2085f3c0, 0xf8e2f000, 0xf3c08aa0, 0x28032085, 0xf000d801, 0x7920fb5d, 0xd00507c0, 0x463a4623, 
	0x46284631, 0xfc9af7ff, 0x4632463b, 0xf44f4629, 0xf00060a1, 0x4628f9a9, 0xe8bde7ba, 0x000081f0, 
	0x00001250, 0x000031ac, 0x461a2300, 0xf2402101, 0xf0005001, 0x0000b812, 0x4c06b510, 0xf8c42000, 
	0x200a0108, 0x7484f504, 0xfb89f7ff, 0x60202001, 0x0000bd10, 0x40020000, 0xe92d4770, 0x469841f0, 
	0x460e4615, 0xb6724607, 0x14c4f44f, 0x8b202200, 0xf5002114, 0xf00010c4, 0xb170f922, 0x80868007, 
	0xf8a080c5, 0x21008008, 0x8b208141, 0xf5002114, 0xf00010c4, 0xf7fff953, 0xb662ffcf, 0x81f0e8bd, 
	0x41f0e92d, 0x0500ea4f, 0xf9dcf000, 0xfb7af7ff, 0xdb3e2800, 0xeb0017c1, 0xf0217151, 0x1a860207, 
	0x491d10c8, 0x7010f831, 0xeb05491c, 0xeb010045, 0xeb061080, 0xeb000146, 0x463a04c1, 0x46284631, 
	0xf9b2f000, 0xf3c08aa0, 0x30101044, 0xf85ef000, 0x28006860, 0x8aa0db0b, 0x2085f3c0, 0xf856f000, 
	0xf3c08aa0, 0x28032085, 0xf000d801, 0x6860fad1, 0xd5050380, 0x463a4623, 0x46284631, 0xf9d4f000, 
	0x4632463b, 0xf2404629, 0xf0005006, 0x4628f91d, 0xe8bde7ba, 0x000081f0, 0x0000124c, 0x0000302c, 
	0x20004907, 0x68886008, 0x20006088, 0x0280eb01, 0xf0236913, 0x61134300, 0x280b1c40, 0x4770ddf6, 
	0x40020000, 0x48100083, 0x69034418, 0x4300f023, 0x69036103, 0x69036103, 0x6370f423, 0x69036103, 
	0x2101ea43, 0x69016101, 0x011ff021, 0x69016101, 0x61014311, 0xf0216901, 0x61014100, 0xf0416901, 
	0x61014100, 0x00004770, 0x40020000, 0x40812101, 0x60814801, 0x00004770, 0x40020000, 0xb5104a0c, 
	0x68142301, 0xb1094083, 0xe000431c, 0x6014439c, 0x2b0b1f03, 0x2301d80a, 0x40831f00, 0x0118f8d2, 
	0x4318b109, 0x4398e000, 0x0118f8c2, 0x0000bd10, 0x40020000, 0x41f0e92d, 0x460d4617, 0x88824604, 
	0x89408841, 0xfaa3f7ff, 0x1a108962, 0xd0271e40, 0x88638921, 0xc00cf8b4, 0xf50618ce, 0xf1bc16c4, 
	0xd00b0f00, 0xb9386830, 0x80602000, 0x88a08020, 0xd0152800, 0x16c4f501, 0xe006ce20, 0xd3004285, 
	0x1ad04605, 0xd20042a8, 0xb1074605, 0x8860603d, 0x440889a1, 0x89614428, 0xfa84f000, 0x46308060, 
	0x81f0e8bd, 0x4604b570, 0xfa68f7ff, 0x46204605, 0xfa5ff7ff, 0x1a088961, 0x42851e40, 0xf04fd902, 
	0xbd7030ff, 0x44288820, 0xfa6cf000, 0x80608020, 0xbd702000, 0x4614b57f, 0x4606461d, 0x9901aa01, 
	0xffa8f7ff, 0xd0030001, 0x9a014628, 0x900147a0, 0x99014630, 0xffd6f7ff, 0xb0049801, 0xe92dbd70, 
	0x461741f0, 0xf7ff4605, 0x4604fa39, 0x882988ea, 0xf7ff8968, 0x4601fa3c, 0x88ea8968, 0x1a80892b, 
	0x89aa189e, 0x16c4f506, 0x42a1b1a2, 0x42a0d304, 0x1a08d20c, 0xd20242a0, 0xe8bd2000, 0x200081f0, 
	0x80e86030, 0x892e80a8, 0x16c4f506, 0x1a2089a8, 0xe007c601, 0xd200428c, 0x42884621, 0x4608d300, 
	0xd0ea2800, 0x6038b107, 0x89aa88e9, 0x44084411, 0xf0008969, 0x80e8fa17, 0xe7de4630, 0x4604b570, 
	0xf9fcf7ff, 0x46204605, 0xf9f3f7ff, 0xd20242a8, 0x30fff04f, 0x88a0bd70, 0x44288961, 0xfa02f000, 
	0x80e080a0, 0xbd702000, 0x41f0e92d, 0x46144698, 0x4606460d, 0xf44fb672, 0x220017c4, 0x210a8b38, 
	0x10c4f500, 0xff9bf7ff, 0x8006b160, 0x80c48085, 0x8008f8a0, 0x210a8b38, 0x10c4f500, 0xffcef7ff, 
	0xfe4af7ff, 0xe8bdb662, 0x000081f0, 0x69ca4904, 0x4000eb01, 0x680161c2, 0x3100f441, 0x47706001, 
	0x40030000, 0x6853b5f8, 0x78139300, 0xea4f4c1d, 0x07de4500, 0x0004eb05, 0x0301f04f, 0xfa036986, 
	0xd001f301, 0xe000431e, 0x6186439e, 0xf04f7813, 0x079f0601, 0x0308f101, 0xf303fa06, 0xd5016986, 
	0xe000431e, 0x6186439e, 0xf0209800, 0xeb054380, 0x442000c1, 0x68d16203, 0x0111f3c1, 0x7a1360d1, 
	0x0340f3c3, 0xea43005b, 0x8a130181, 0x5103ea41, 0x69116241, 0xd0032900, 0xf0416a41, 0x62410101, 
	0x0000bdf8, 0x40030000, 0x460bb510, 0x46114604, 0xf0004618, 0x4a02f811, 0x4104eb02, 0xbd1061c8, 
	0x40030000, 0x48030401, 0x69c84401, 0x40086989, 0x00004770, 0x40030000, 0x20004602, 0x29012301, 
	0x2902d002, 0x3208d102, 0xf002fa03, 0x00004770, 0xeb000400, 0x480501c1, 0x6a014408, 0x4180f021, 
	0x6a016201, 0x4180f041, 0x47706201, 0x40030000, 0xeb000400, 0x480301c1, 0x6a014408, 0x4140f021, 
	0x47706201, 0x40030000, 0x460cb4f0, 0x46157819, 0x4a11078e, 0xeb02490f, 0xeb011340, 0xf1023100, 
	0xeb020240, 0xeb011000, 0xf8302144, 0xd5042014, 0x2d010852, 0x2d02d004, 0xf853d109, 0xe0030024, 
	0x0024f853, 0x44104411, 0xf7ffbcf0, 0xbcf0b853, 0x00004770, 0x000042fc, 0x0000329c, 0xf000b109, 
	0xf000b811, 0x0000b801, 0x48050401, 0x68014408, 0x0108f041, 0x68016001, 0x3100f441, 0x47706001, 
	0x40030000, 0x48050401, 0x68014408, 0x3100f421, 0x68016001, 0x0108f021, 0x47706001, 0x40030000, 
	0xb4104a15, 0x1c446853, 0x40a12101, 0x6053430b, 0x68130492, 0x6013438b, 0x430b6813, 0x490f6013, 
	0x4100eb01, 0xf422680a, 0x600a3280, 0x600a680a, 0xf442680a, 0x600a5200, 0xf042680a, 0x600a0220, 
	0xf042680a, 0x600a0210, 0xf442680a, 0x600a3200, 0x4903bc10, 0xb82cf000, 0x40001000, 0x40030000, 
	0x003d0900, 0x4b11b570, 0xeb03685c, 0x60444000, 0x4600f44f, 0x4633e017, 0xdc00429a, 0x1ad24613, 
	0x68046103, 0x0401f044, 0x24006004, 0x6845e005, 0xd1fc07ed, 0x550d68c5, 0x429c1c64, 0x6803dbf7, 
	0x0303f023, 0x2a006003, 0xbd70d1e5, 0x40030000, 0x48070081, 0x68814408, 0x7140f421, 0x68816081, 
	0x68816081, 0x010ff021, 0x68816081, 0x47706081, 0x40001000, 0x480f0403, 0x4418b510, 0x60436843, 
	0xf0436803, 0x60030302, 0xe0052300, 0x07246844, 0x5cccd4fc, 0x1c5b6084, 0xd3f74293, 0x07496841, 
	0x6841d5fc, 0xd4fc06c9, 0xf0216801, 0x60010103, 0x0000bd10, 0x40030000, 0x68004801, 0x00004770, 
	0x00003000, 0x20004908, 0x60486008, 0xf04f6088, 0xf64021e0, 0x614a729f, 0x22e04b04, 0x2d14f883, 
	0x20076188, 0x47706108, 0x00003000, 0xe000e00f, 0x48030141, 0x68014408, 0x0101f041, 0x47706001, 
	0x40010000, 0x4c10b570, 0x1440eb04, 0xf0456825, 0x60250501, 0x25024e0d, 0x0080eb06, 0xf44f6185, 
	0x4341707a, 0x68206061, 0x0004f020, 0x68206020, 0x0083ea40, 0x68206020, 0x0002f020, 0x68206020, 
	0x0042ea40, 0xbd706020, 0x40010000, 0x40001000, 0x48050141, 0x68014408, 0x0101f041, 0x68016001, 
	0x0120f021, 0x47706001, 0x40010000, 0x48030141, 0x68014408, 0x0120f041, 0x47706001, 0x40010000, 
	0x68414807, 0x0120f041, 0x04806041, 0xf0216801, 0x60010120, 0xf0416801, 0x60010120, 0x00004770, 
	0x40001000, 0xd3004288, 0x47701a40, 0x00020001, 0x00020001, 0x00080004, 0x00001278, 0x00003000, 
	0x0000000c, 0x000001f4, 0x00001284, 0x0000300c, 0x000032f0, 0x00000204, 0x00000000, 0x00000000, 
	0x00000000
};

const int sc_code_length = 1185;