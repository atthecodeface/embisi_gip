/* 
 * Copyright (c) Tony Bybell 1999.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "rc.h"

#ifdef _MSC_VER
#define strcasecmp _stricmp
#endif

/*
 * the strings *must* be in this order for the
 * case insensitive string bsearch 
 */

#define Enum_WaveColors \
D4("alice blue", 240, 248, 255),\
D4("AliceBlue", 240, 248, 255),\
D4("antique white", 250, 235, 215),\
D4("AntiqueWhite", 250, 235, 215),\
D4("AntiqueWhite1", 255, 239, 219),\
D4("AntiqueWhite2", 238, 223, 204),\
D4("AntiqueWhite3", 205, 192, 176),\
D4("AntiqueWhite4", 139, 131, 120),\
D4("aquamarine", 127, 255, 212),\
D4("aquamarine1", 127, 255, 212),\
D4("aquamarine2", 118, 238, 198),\
D4("aquamarine3", 102, 205, 170),\
D4("aquamarine4", 69, 139, 116),\
D4("azure", 240, 255, 255),\
D4("azure1", 240, 255, 255),\
D4("azure2", 224, 238, 238),\
D4("azure3", 193, 205, 205),\
D4("azure4", 131, 139, 139),\
D4("beige", 245, 245, 220),\
D4("bisque", 255, 228, 196),\
D4("bisque1", 255, 228, 196),\
D4("bisque2", 238, 213, 183),\
D4("bisque3", 205, 183, 158),\
D4("bisque4", 139, 125, 107),\
D4("black", 0, 0, 0),\
D4("blanched almond", 255, 235, 205),\
D4("BlanchedAlmond", 255, 235, 205),\
D4("blue", 0, 0, 255),\
D4("blue violet", 138, 43, 226),\
D4("blue1", 0, 0, 255),\
D4("blue2", 0, 0, 238),\
D4("blue3", 0, 0, 205),\
D4("blue4", 0, 0, 139),\
D4("BlueViolet", 138, 43, 226),\
D4("brown", 165, 42, 42),\
D4("brown1", 255, 64, 64),\
D4("brown2", 238, 59, 59),\
D4("brown3", 205, 51, 51),\
D4("brown4", 139, 35, 35),\
D4("burlywood", 222, 184, 135),\
D4("burlywood1", 255, 211, 155),\
D4("burlywood2", 238, 197, 145),\
D4("burlywood3", 205, 170, 125),\
D4("burlywood4", 139, 115, 85),\
D4("cadet blue", 95, 158, 160),\
D4("CadetBlue", 95, 158, 160),\
D4("CadetBlue1", 152, 245, 255),\
D4("CadetBlue2", 142, 229, 238),\
D4("CadetBlue3", 122, 197, 205),\
D4("CadetBlue4", 83, 134, 139),\
D4("chartreuse", 127, 255, 0),\
D4("chartreuse1", 127, 255, 0),\
D4("chartreuse2", 118, 238, 0),\
D4("chartreuse3", 102, 205, 0),\
D4("chartreuse4", 69, 139, 0),\
D4("chocolate", 210, 105, 30),\
D4("chocolate1", 255, 127, 36),\
D4("chocolate2", 238, 118, 33),\
D4("chocolate3", 205, 102, 29),\
D4("chocolate4", 139, 69, 19),\
D4("coral", 255, 127, 80),\
D4("coral1", 255, 114, 86),\
D4("coral2", 238, 106, 80),\
D4("coral3", 205, 91, 69),\
D4("coral4", 139, 62, 47),\
D4("cornflower blue", 100, 149, 237),\
D4("CornflowerBlue", 100, 149, 237),\
D4("cornsilk", 255, 248, 220),\
D4("cornsilk1", 255, 248, 220),\
D4("cornsilk2", 238, 232, 205),\
D4("cornsilk3", 205, 200, 177),\
D4("cornsilk4", 139, 136, 120),\
D4("cyan", 0, 255, 255),\
D4("cyan1", 0, 255, 255),\
D4("cyan2", 0, 238, 238),\
D4("cyan3", 0, 205, 205),\
D4("cyan4", 0, 139, 139),\
D4("dark blue", 0, 0, 139),\
D4("dark cyan", 0, 139, 139),\
D4("dark goldenrod", 184, 134, 11),\
D4("dark gray", 169, 169, 169),\
D4("dark green", 0, 100, 0),\
D4("dark grey", 169, 169, 169),\
D4("dark khaki", 189, 183, 107),\
D4("dark magenta", 139, 0, 139),\
D4("dark olive green", 85, 107, 47),\
D4("dark orange", 255, 140, 0),\
D4("dark orchid", 153, 50, 204),\
D4("dark red", 139, 0, 0),\
D4("dark salmon", 233, 150, 122),\
D4("dark sea green", 143, 188, 143),\
D4("dark slate blue", 72, 61, 139),\
D4("dark slate gray", 47, 79, 79),\
D4("dark slate grey", 47, 79, 79),\
D4("dark turquoise", 0, 206, 209),\
D4("dark violet", 148, 0, 211),\
D4("DarkBlue", 0, 0, 139),\
D4("DarkCyan", 0, 139, 139),\
D4("DarkGoldenrod", 184, 134, 11),\
D4("DarkGoldenrod1", 255, 185, 15),\
D4("DarkGoldenrod2", 238, 173, 14),\
D4("DarkGoldenrod3", 205, 149, 12),\
D4("DarkGoldenrod4", 139, 101, 8),\
D4("DarkGray", 169, 169, 169),\
D4("DarkGreen", 0, 100, 0),\
D4("DarkGrey", 169, 169, 169),\
D4("DarkKhaki", 189, 183, 107),\
D4("DarkMagenta", 139, 0, 139),\
D4("DarkOliveGreen", 85, 107, 47),\
D4("DarkOliveGreen1", 202, 255, 112),\
D4("DarkOliveGreen2", 188, 238, 104),\
D4("DarkOliveGreen3", 162, 205, 90),\
D4("DarkOliveGreen4", 110, 139, 61),\
D4("DarkOrange", 255, 140, 0),\
D4("DarkOrange1", 255, 127, 0),\
D4("DarkOrange2", 238, 118, 0),\
D4("DarkOrange3", 205, 102, 0),\
D4("DarkOrange4", 139, 69, 0),\
D4("DarkOrchid", 153, 50, 204),\
D4("DarkOrchid1", 191, 62, 255),\
D4("DarkOrchid2", 178, 58, 238),\
D4("DarkOrchid3", 154, 50, 205),\
D4("DarkOrchid4", 104, 34, 139),\
D4("DarkRed", 139, 0, 0),\
D4("DarkSalmon", 233, 150, 122),\
D4("DarkSeaGreen", 143, 188, 143),\
D4("DarkSeaGreen1", 193, 255, 193),\
D4("DarkSeaGreen2", 180, 238, 180),\
D4("DarkSeaGreen3", 155, 205, 155),\
D4("DarkSeaGreen4", 105, 139, 105),\
D4("DarkSlateBlue", 72, 61, 139),\
D4("DarkSlateGray", 47, 79, 79),\
D4("DarkSlateGray1", 151, 255, 255),\
D4("DarkSlateGray2", 141, 238, 238),\
D4("DarkSlateGray3", 121, 205, 205),\
D4("DarkSlateGray4", 82, 139, 139),\
D4("DarkSlateGrey", 47, 79, 79),\
D4("DarkTurquoise", 0, 206, 209),\
D4("DarkViolet", 148, 0, 211),\
D4("deep pink", 255, 20, 147),\
D4("deep sky blue", 0, 191, 255),\
D4("DeepPink", 255, 20, 147),\
D4("DeepPink1", 255, 20, 147),\
D4("DeepPink2", 238, 18, 137),\
D4("DeepPink3", 205, 16, 118),\
D4("DeepPink4", 139, 10, 80),\
D4("DeepSkyBlue", 0, 191, 255),\
D4("DeepSkyBlue1", 0, 191, 255),\
D4("DeepSkyBlue2", 0, 178, 238),\
D4("DeepSkyBlue3", 0, 154, 205),\
D4("DeepSkyBlue4", 0, 104, 139),\
D4("dim gray", 105, 105, 105),\
D4("dim grey", 105, 105, 105),\
D4("DimGray", 105, 105, 105),\
D4("DimGrey", 105, 105, 105),\
D4("dodger blue", 30, 144, 255),\
D4("DodgerBlue", 30, 144, 255),\
D4("DodgerBlue1", 30, 144, 255),\
D4("DodgerBlue2", 28, 134, 238),\
D4("DodgerBlue3", 24, 116, 205),\
D4("DodgerBlue4", 16, 78, 139),\
D4("firebrick", 178, 34, 34),\
D4("firebrick1", 255, 48, 48),\
D4("firebrick2", 238, 44, 44),\
D4("firebrick3", 205, 38, 38),\
D4("firebrick4", 139, 26, 26),\
D4("floral white", 255, 250, 240),\
D4("FloralWhite", 255, 250, 240),\
D4("forest green", 34, 139, 34),\
D4("ForestGreen", 34, 139, 34),\
D4("gainsboro", 220, 220, 220),\
D4("ghost white", 248, 248, 255),\
D4("GhostWhite", 248, 248, 255),\
D4("gold", 255, 215, 0),\
D4("gold1", 255, 215, 0),\
D4("gold2", 238, 201, 0),\
D4("gold3", 205, 173, 0),\
D4("gold4", 139, 117, 0),\
D4("goldenrod", 218, 165, 32),\
D4("goldenrod1", 255, 193, 37),\
D4("goldenrod2", 238, 180, 34),\
D4("goldenrod3", 205, 155, 29),\
D4("goldenrod4", 139, 105, 20),\
D4("gray", 190, 190, 190),\
D4("gray0", 0, 0, 0),\
D4("gray1", 3, 3, 3),\
D4("gray10", 26, 26, 26),\
D4("gray100", 255, 255, 255),\
D4("gray11", 28, 28, 28),\
D4("gray12", 31, 31, 31),\
D4("gray13", 33, 33, 33),\
D4("gray14", 36, 36, 36),\
D4("gray15", 38, 38, 38),\
D4("gray16", 41, 41, 41),\
D4("gray17", 43, 43, 43),\
D4("gray18", 46, 46, 46),\
D4("gray19", 48, 48, 48),\
D4("gray2", 5, 5, 5),\
D4("gray20", 51, 51, 51),\
D4("gray21", 54, 54, 54),\
D4("gray22", 56, 56, 56),\
D4("gray23", 59, 59, 59),\
D4("gray24", 61, 61, 61),\
D4("gray25", 64, 64, 64),\
D4("gray26", 66, 66, 66),\
D4("gray27", 69, 69, 69),\
D4("gray28", 71, 71, 71),\
D4("gray29", 74, 74, 74),\
D4("gray3", 8, 8, 8),\
D4("gray30", 77, 77, 77),\
D4("gray31", 79, 79, 79),\
D4("gray32", 82, 82, 82),\
D4("gray33", 84, 84, 84),\
D4("gray34", 87, 87, 87),\
D4("gray35", 89, 89, 89),\
D4("gray36", 92, 92, 92),\
D4("gray37", 94, 94, 94),\
D4("gray38", 97, 97, 97),\
D4("gray39", 99, 99, 99),\
D4("gray4", 10, 10, 10),\
D4("gray40", 102, 102, 102),\
D4("gray41", 105, 105, 105),\
D4("gray42", 107, 107, 107),\
D4("gray43", 110, 110, 110),\
D4("gray44", 112, 112, 112),\
D4("gray45", 115, 115, 115),\
D4("gray46", 117, 117, 117),\
D4("gray47", 120, 120, 120),\
D4("gray48", 122, 122, 122),\
D4("gray49", 125, 125, 125),\
D4("gray5", 13, 13, 13),\
D4("gray50", 127, 127, 127),\
D4("gray51", 130, 130, 130),\
D4("gray52", 133, 133, 133),\
D4("gray53", 135, 135, 135),\
D4("gray54", 138, 138, 138),\
D4("gray55", 140, 140, 140),\
D4("gray56", 143, 143, 143),\
D4("gray57", 145, 145, 145),\
D4("gray58", 148, 148, 148),\
D4("gray59", 150, 150, 150),\
D4("gray6", 15, 15, 15),\
D4("gray60", 153, 153, 153),\
D4("gray61", 156, 156, 156),\
D4("gray62", 158, 158, 158),\
D4("gray63", 161, 161, 161),\
D4("gray64", 163, 163, 163),\
D4("gray65", 166, 166, 166),\
D4("gray66", 168, 168, 168),\
D4("gray67", 171, 171, 171),\
D4("gray68", 173, 173, 173),\
D4("gray69", 176, 176, 176),\
D4("gray7", 18, 18, 18),\
D4("gray70", 179, 179, 179),\
D4("gray71", 181, 181, 181),\
D4("gray72", 184, 184, 184),\
D4("gray73", 186, 186, 186),\
D4("gray74", 189, 189, 189),\
D4("gray75", 191, 191, 191),\
D4("gray76", 194, 194, 194),\
D4("gray77", 196, 196, 196),\
D4("gray78", 199, 199, 199),\
D4("gray79", 201, 201, 201),\
D4("gray8", 20, 20, 20),\
D4("gray80", 204, 204, 204),\
D4("gray81", 207, 207, 207),\
D4("gray82", 209, 209, 209),\
D4("gray83", 212, 212, 212),\
D4("gray84", 214, 214, 214),\
D4("gray85", 217, 217, 217),\
D4("gray86", 219, 219, 219),\
D4("gray87", 222, 222, 222),\
D4("gray88", 224, 224, 224),\
D4("gray89", 227, 227, 227),\
D4("gray9", 23, 23, 23),\
D4("gray90", 229, 229, 229),\
D4("gray91", 232, 232, 232),\
D4("gray92", 235, 235, 235),\
D4("gray93", 237, 237, 237),\
D4("gray94", 240, 240, 240),\
D4("gray95", 242, 242, 242),\
D4("gray96", 245, 245, 245),\
D4("gray97", 247, 247, 247),\
D4("gray98", 250, 250, 250),\
D4("gray99", 252, 252, 252),\
D4("green", 0, 255, 0),\
D4("green yellow", 173, 255, 47),\
D4("green1", 0, 255, 0),\
D4("green2", 0, 238, 0),\
D4("green3", 0, 205, 0),\
D4("green4", 0, 139, 0),\
D4("GreenYellow", 173, 255, 47),\
D4("grey", 190, 190, 190),\
D4("grey0", 0, 0, 0),\
D4("grey1", 3, 3, 3),\
D4("grey10", 26, 26, 26),\
D4("grey100", 255, 255, 255),\
D4("grey11", 28, 28, 28),\
D4("grey12", 31, 31, 31),\
D4("grey13", 33, 33, 33),\
D4("grey14", 36, 36, 36),\
D4("grey15", 38, 38, 38),\
D4("grey16", 41, 41, 41),\
D4("grey17", 43, 43, 43),\
D4("grey18", 46, 46, 46),\
D4("grey19", 48, 48, 48),\
D4("grey2", 5, 5, 5),\
D4("grey20", 51, 51, 51),\
D4("grey21", 54, 54, 54),\
D4("grey22", 56, 56, 56),\
D4("grey23", 59, 59, 59),\
D4("grey24", 61, 61, 61),\
D4("grey25", 64, 64, 64),\
D4("grey26", 66, 66, 66),\
D4("grey27", 69, 69, 69),\
D4("grey28", 71, 71, 71),\
D4("grey29", 74, 74, 74),\
D4("grey3", 8, 8, 8),\
D4("grey30", 77, 77, 77),\
D4("grey31", 79, 79, 79),\
D4("grey32", 82, 82, 82),\
D4("grey33", 84, 84, 84),\
D4("grey34", 87, 87, 87),\
D4("grey35", 89, 89, 89),\
D4("grey36", 92, 92, 92),\
D4("grey37", 94, 94, 94),\
D4("grey38", 97, 97, 97),\
D4("grey39", 99, 99, 99),\
D4("grey4", 10, 10, 10),\
D4("grey40", 102, 102, 102),\
D4("grey41", 105, 105, 105),\
D4("grey42", 107, 107, 107),\
D4("grey43", 110, 110, 110),\
D4("grey44", 112, 112, 112),\
D4("grey45", 115, 115, 115),\
D4("grey46", 117, 117, 117),\
D4("grey47", 120, 120, 120),\
D4("grey48", 122, 122, 122),\
D4("grey49", 125, 125, 125),\
D4("grey5", 13, 13, 13),\
D4("grey50", 127, 127, 127),\
D4("grey51", 130, 130, 130),\
D4("grey52", 133, 133, 133),\
D4("grey53", 135, 135, 135),\
D4("grey54", 138, 138, 138),\
D4("grey55", 140, 140, 140),\
D4("grey56", 143, 143, 143),\
D4("grey57", 145, 145, 145),\
D4("grey58", 148, 148, 148),\
D4("grey59", 150, 150, 150),\
D4("grey6", 15, 15, 15),\
D4("grey60", 153, 153, 153),\
D4("grey61", 156, 156, 156),\
D4("grey62", 158, 158, 158),\
D4("grey63", 161, 161, 161),\
D4("grey64", 163, 163, 163),\
D4("grey65", 166, 166, 166),\
D4("grey66", 168, 168, 168),\
D4("grey67", 171, 171, 171),\
D4("grey68", 173, 173, 173),\
D4("grey69", 176, 176, 176),\
D4("grey7", 18, 18, 18),\
D4("grey70", 179, 179, 179),\
D4("grey71", 181, 181, 181),\
D4("grey72", 184, 184, 184),\
D4("grey73", 186, 186, 186),\
D4("grey74", 189, 189, 189),\
D4("grey75", 191, 191, 191),\
D4("grey76", 194, 194, 194),\
D4("grey77", 196, 196, 196),\
D4("grey78", 199, 199, 199),\
D4("grey79", 201, 201, 201),\
D4("grey8", 20, 20, 20),\
D4("grey80", 204, 204, 204),\
D4("grey81", 207, 207, 207),\
D4("grey82", 209, 209, 209),\
D4("grey83", 212, 212, 212),\
D4("grey84", 214, 214, 214),\
D4("grey85", 217, 217, 217),\
D4("grey86", 219, 219, 219),\
D4("grey87", 222, 222, 222),\
D4("grey88", 224, 224, 224),\
D4("grey89", 227, 227, 227),\
D4("grey9", 23, 23, 23),\
D4("grey90", 229, 229, 229),\
D4("grey91", 232, 232, 232),\
D4("grey92", 235, 235, 235),\
D4("grey93", 237, 237, 237),\
D4("grey94", 240, 240, 240),\
D4("grey95", 242, 242, 242),\
D4("grey96", 245, 245, 245),\
D4("grey97", 247, 247, 247),\
D4("grey98", 250, 250, 250),\
D4("grey99", 252, 252, 252),\
D4("honeydew", 240, 255, 240),\
D4("honeydew1", 240, 255, 240),\
D4("honeydew2", 224, 238, 224),\
D4("honeydew3", 193, 205, 193),\
D4("honeydew4", 131, 139, 131),\
D4("hot pink", 255, 105, 180),\
D4("HotPink", 255, 105, 180),\
D4("HotPink1", 255, 110, 180),\
D4("HotPink2", 238, 106, 167),\
D4("HotPink3", 205, 96, 144),\
D4("HotPink4", 139, 58, 98),\
D4("indian red", 205, 92, 92),\
D4("IndianRed", 205, 92, 92),\
D4("IndianRed1", 255, 106, 106),\
D4("IndianRed2", 238, 99, 99),\
D4("IndianRed3", 205, 85, 85),\
D4("IndianRed4", 139, 58, 58),\
D4("ivory", 255, 255, 240),\
D4("ivory1", 255, 255, 240),\
D4("ivory2", 238, 238, 224),\
D4("ivory3", 205, 205, 193),\
D4("ivory4", 139, 139, 131),\
D4("khaki", 240, 230, 140),\
D4("khaki1", 255, 246, 143),\
D4("khaki2", 238, 230, 133),\
D4("khaki3", 205, 198, 115),\
D4("khaki4", 139, 134, 78),\
D4("lavender", 230, 230, 250),\
D4("lavender blush", 255, 240, 245),\
D4("LavenderBlush", 255, 240, 245),\
D4("LavenderBlush1", 255, 240, 245),\
D4("LavenderBlush2", 238, 224, 229),\
D4("LavenderBlush3", 205, 193, 197),\
D4("LavenderBlush4", 139, 131, 134),\
D4("lawn green", 124, 252, 0),\
D4("LawnGreen", 124, 252, 0),\
D4("lemon chiffon", 255, 250, 205),\
D4("LemonChiffon", 255, 250, 205),\
D4("LemonChiffon1", 255, 250, 205),\
D4("LemonChiffon2", 238, 233, 191),\
D4("LemonChiffon3", 205, 201, 165),\
D4("LemonChiffon4", 139, 137, 112),\
D4("light blue", 173, 216, 230),\
D4("light coral", 240, 128, 128),\
D4("light cyan", 224, 255, 255),\
D4("light goldenrod", 238, 221, 130),\
D4("light goldenrod yellow", 250, 250, 210),\
D4("light gray", 211, 211, 211),\
D4("light green", 144, 238, 144),\
D4("light grey", 211, 211, 211),\
D4("light pink", 255, 182, 193),\
D4("light salmon", 255, 160, 122),\
D4("light sea green", 32, 178, 170),\
D4("light sky blue", 135, 206, 250),\
D4("light slate blue", 132, 112, 255),\
D4("light slate gray", 119, 136, 153),\
D4("light slate grey", 119, 136, 153),\
D4("light steel blue", 176, 196, 222),\
D4("light yellow", 255, 255, 224),\
D4("LightBlue", 173, 216, 230),\
D4("LightBlue1", 191, 239, 255),\
D4("LightBlue2", 178, 223, 238),\
D4("LightBlue3", 154, 192, 205),\
D4("LightBlue4", 104, 131, 139),\
D4("LightCoral", 240, 128, 128),\
D4("LightCyan", 224, 255, 255),\
D4("LightCyan1", 224, 255, 255),\
D4("LightCyan2", 209, 238, 238),\
D4("LightCyan3", 180, 205, 205),\
D4("LightCyan4", 122, 139, 139),\
D4("LightGoldenrod", 238, 221, 130),\
D4("LightGoldenrod1", 255, 236, 139),\
D4("LightGoldenrod2", 238, 220, 130),\
D4("LightGoldenrod3", 205, 190, 112),\
D4("LightGoldenrod4", 139, 129, 76),\
D4("LightGoldenrodYellow", 250, 250, 210),\
D4("LightGray", 211, 211, 211),\
D4("LightGreen", 144, 238, 144),\
D4("LightGrey", 211, 211, 211),\
D4("LightPink", 255, 182, 193),\
D4("LightPink1", 255, 174, 185),\
D4("LightPink2", 238, 162, 173),\
D4("LightPink3", 205, 140, 149),\
D4("LightPink4", 139, 95, 101),\
D4("LightSalmon", 255, 160, 122),\
D4("LightSalmon1", 255, 160, 122),\
D4("LightSalmon2", 238, 149, 114),\
D4("LightSalmon3", 205, 129, 98),\
D4("LightSalmon4", 139, 87, 66),\
D4("LightSeaGreen", 32, 178, 170),\
D4("LightSkyBlue", 135, 206, 250),\
D4("LightSkyBlue1", 176, 226, 255),\
D4("LightSkyBlue2", 164, 211, 238),\
D4("LightSkyBlue3", 141, 182, 205),\
D4("LightSkyBlue4", 96, 123, 139),\
D4("LightSlateBlue", 132, 112, 255),\
D4("LightSlateGray", 119, 136, 153),\
D4("LightSlateGrey", 119, 136, 153),\
D4("LightSteelBlue", 176, 196, 222),\
D4("LightSteelBlue1", 202, 225, 255),\
D4("LightSteelBlue2", 188, 210, 238),\
D4("LightSteelBlue3", 162, 181, 205),\
D4("LightSteelBlue4", 110, 123, 139),\
D4("LightYellow", 255, 255, 224),\
D4("LightYellow1", 255, 255, 224),\
D4("LightYellow2", 238, 238, 209),\
D4("LightYellow3", 205, 205, 180),\
D4("LightYellow4", 139, 139, 122),\
D4("lime green", 50, 205, 50),\
D4("LimeGreen", 50, 205, 50),\
D4("linen", 250, 240, 230),\
D4("magenta", 255, 0, 255),\
D4("magenta1", 255, 0, 255),\
D4("magenta2", 238, 0, 238),\
D4("magenta3", 205, 0, 205),\
D4("magenta4", 139, 0, 139),\
D4("maroon", 176, 48, 96),\
D4("maroon1", 255, 52, 179),\
D4("maroon2", 238, 48, 167),\
D4("maroon3", 205, 41, 144),\
D4("maroon4", 139, 28, 98),\
D4("medium aquamarine", 102, 205, 170),\
D4("medium blue", 0, 0, 205),\
D4("medium orchid", 186, 85, 211),\
D4("medium purple", 147, 112, 219),\
D4("medium sea green", 60, 179, 113),\
D4("medium slate blue", 123, 104, 238),\
D4("medium spring green", 0, 250, 154),\
D4("medium turquoise", 72, 209, 204),\
D4("medium violet red", 199, 21, 133),\
D4("MediumAquamarine", 102, 205, 170),\
D4("MediumBlue", 0, 0, 205),\
D4("MediumOrchid", 186, 85, 211),\
D4("MediumOrchid1", 224, 102, 255),\
D4("MediumOrchid2", 209, 95, 238),\
D4("MediumOrchid3", 180, 82, 205),\
D4("MediumOrchid4", 122, 55, 139),\
D4("MediumPurple", 147, 112, 219),\
D4("MediumPurple1", 171, 130, 255),\
D4("MediumPurple2", 159, 121, 238),\
D4("MediumPurple3", 137, 104, 205),\
D4("MediumPurple4", 93, 71, 139),\
D4("MediumSeaGreen", 60, 179, 113),\
D4("MediumSlateBlue", 123, 104, 238),\
D4("MediumSpringGreen", 0, 250, 154),\
D4("MediumTurquoise", 72, 209, 204),\
D4("MediumVioletRed", 199, 21, 133),\
D4("midnight blue", 25, 25, 112),\
D4("MidnightBlue", 25, 25, 112),\
D4("mint cream", 245, 255, 250),\
D4("MintCream", 245, 255, 250),\
D4("misty rose", 255, 228, 225),\
D4("MistyRose", 255, 228, 225),\
D4("MistyRose1", 255, 228, 225),\
D4("MistyRose2", 238, 213, 210),\
D4("MistyRose3", 205, 183, 181),\
D4("MistyRose4", 139, 125, 123),\
D4("moccasin", 255, 228, 181),\
D4("navajo white", 255, 222, 173),\
D4("NavajoWhite", 255, 222, 173),\
D4("NavajoWhite1", 255, 222, 173),\
D4("NavajoWhite2", 238, 207, 161),\
D4("NavajoWhite3", 205, 179, 139),\
D4("NavajoWhite4", 139, 121, 94),\
D4("navy", 0, 0, 128),\
D4("navy blue", 0, 0, 128),\
D4("NavyBlue", 0, 0, 128),\
D4("old lace", 253, 245, 230),\
D4("OldLace", 253, 245, 230),\
D4("olive drab", 107, 142, 35),\
D4("OliveDrab", 107, 142, 35),\
D4("OliveDrab1", 192, 255, 62),\
D4("OliveDrab2", 179, 238, 58),\
D4("OliveDrab3", 154, 205, 50),\
D4("OliveDrab4", 105, 139, 34),\
D4("orange", 255, 165, 0),\
D4("orange red", 255, 69, 0),\
D4("orange1", 255, 165, 0),\
D4("orange2", 238, 154, 0),\
D4("orange3", 205, 133, 0),\
D4("orange4", 139, 90, 0),\
D4("OrangeRed", 255, 69, 0),\
D4("OrangeRed1", 255, 69, 0),\
D4("OrangeRed2", 238, 64, 0),\
D4("OrangeRed3", 205, 55, 0),\
D4("OrangeRed4", 139, 37, 0),\
D4("orchid", 218, 112, 214),\
D4("orchid1", 255, 131, 250),\
D4("orchid2", 238, 122, 233),\
D4("orchid3", 205, 105, 201),\
D4("orchid4", 139, 71, 137),\
D4("pale goldenrod", 238, 232, 170),\
D4("pale green", 152, 251, 152),\
D4("pale turquoise", 175, 238, 238),\
D4("pale violet red", 219, 112, 147),\
D4("PaleGoldenrod", 238, 232, 170),\
D4("PaleGreen", 152, 251, 152),\
D4("PaleGreen1", 154, 255, 154),\
D4("PaleGreen2", 144, 238, 144),\
D4("PaleGreen3", 124, 205, 124),\
D4("PaleGreen4", 84, 139, 84),\
D4("PaleTurquoise", 175, 238, 238),\
D4("PaleTurquoise1", 187, 255, 255),\
D4("PaleTurquoise2", 174, 238, 238),\
D4("PaleTurquoise3", 150, 205, 205),\
D4("PaleTurquoise4", 102, 139, 139),\
D4("PaleVioletRed", 219, 112, 147),\
D4("PaleVioletRed1", 255, 130, 171),\
D4("PaleVioletRed2", 238, 121, 159),\
D4("PaleVioletRed3", 205, 104, 137),\
D4("PaleVioletRed4", 139, 71, 93),\
D4("papaya whip", 255, 239, 213),\
D4("PapayaWhip", 255, 239, 213),\
D4("peach puff", 255, 218, 185),\
D4("PeachPuff", 255, 218, 185),\
D4("PeachPuff1", 255, 218, 185),\
D4("PeachPuff2", 238, 203, 173),\
D4("PeachPuff3", 205, 175, 149),\
D4("PeachPuff4", 139, 119, 101),\
D4("peru", 205, 133, 63),\
D4("pink", 255, 192, 203),\
D4("pink1", 255, 181, 197),\
D4("pink2", 238, 169, 184),\
D4("pink3", 205, 145, 158),\
D4("pink4", 139, 99, 108),\
D4("plum", 221, 160, 221),\
D4("plum1", 255, 187, 255),\
D4("plum2", 238, 174, 238),\
D4("plum3", 205, 150, 205),\
D4("plum4", 139, 102, 139),\
D4("powder blue", 176, 224, 230),\
D4("PowderBlue", 176, 224, 230),\
D4("purple", 160, 32, 240),\
D4("purple1", 155, 48, 255),\
D4("purple2", 145, 44, 238),\
D4("purple3", 125, 38, 205),\
D4("purple4", 85, 26, 139),\
D4("red", 255, 0, 0),\
D4("red1", 255, 0, 0),\
D4("red2", 238, 0, 0),\
D4("red3", 205, 0, 0),\
D4("red4", 139, 0, 0),\
D4("rosy brown", 188, 143, 143),\
D4("RosyBrown", 188, 143, 143),\
D4("RosyBrown1", 255, 193, 193),\
D4("RosyBrown2", 238, 180, 180),\
D4("RosyBrown3", 205, 155, 155),\
D4("RosyBrown4", 139, 105, 105),\
D4("royal blue", 65, 105, 225),\
D4("RoyalBlue", 65, 105, 225),\
D4("RoyalBlue1", 72, 118, 255),\
D4("RoyalBlue2", 67, 110, 238),\
D4("RoyalBlue3", 58, 95, 205),\
D4("RoyalBlue4", 39, 64, 139),\
D4("saddle brown", 139, 69, 19),\
D4("SaddleBrown", 139, 69, 19),\
D4("salmon", 250, 128, 114),\
D4("salmon1", 255, 140, 105),\
D4("salmon2", 238, 130, 98),\
D4("salmon3", 205, 112, 84),\
D4("salmon4", 139, 76, 57),\
D4("sandy brown", 244, 164, 96),\
D4("SandyBrown", 244, 164, 96),\
D4("sea green", 46, 139, 87),\
D4("SeaGreen", 46, 139, 87),\
D4("SeaGreen1", 84, 255, 159),\
D4("SeaGreen2", 78, 238, 148),\
D4("SeaGreen3", 67, 205, 128),\
D4("SeaGreen4", 46, 139, 87),\
D4("seashell", 255, 245, 238),\
D4("seashell1", 255, 245, 238),\
D4("seashell2", 238, 229, 222),\
D4("seashell3", 205, 197, 191),\
D4("seashell4", 139, 134, 130),\
D4("sienna", 160, 82, 45),\
D4("sienna1", 255, 130, 71),\
D4("sienna2", 238, 121, 66),\
D4("sienna3", 205, 104, 57),\
D4("sienna4", 139, 71, 38),\
D4("sky blue", 135, 206, 235),\
D4("SkyBlue", 135, 206, 235),\
D4("SkyBlue1", 135, 206, 255),\
D4("SkyBlue2", 126, 192, 238),\
D4("SkyBlue3", 108, 166, 205),\
D4("SkyBlue4", 74, 112, 139),\
D4("slate blue", 106, 90, 205),\
D4("slate gray", 112, 128, 144),\
D4("slate grey", 112, 128, 144),\
D4("SlateBlue", 106, 90, 205),\
D4("SlateBlue1", 131, 111, 255),\
D4("SlateBlue2", 122, 103, 238),\
D4("SlateBlue3", 105, 89, 205),\
D4("SlateBlue4", 71, 60, 139),\
D4("SlateGray", 112, 128, 144),\
D4("SlateGray1", 198, 226, 255),\
D4("SlateGray2", 185, 211, 238),\
D4("SlateGray3", 159, 182, 205),\
D4("SlateGray4", 108, 123, 139),\
D4("SlateGrey", 112, 128, 144),\
D4("snow", 255, 250, 250),\
D4("snow1", 255, 250, 250),\
D4("snow2", 238, 233, 233),\
D4("snow3", 205, 201, 201),\
D4("snow4", 139, 137, 137),\
D4("spring green", 0, 255, 127),\
D4("SpringGreen", 0, 255, 127),\
D4("SpringGreen1", 0, 255, 127),\
D4("SpringGreen2", 0, 238, 118),\
D4("SpringGreen3", 0, 205, 102),\
D4("SpringGreen4", 0, 139, 69),\
D4("steel blue", 70, 130, 180),\
D4("SteelBlue", 70, 130, 180),\
D4("SteelBlue1", 99, 184, 255),\
D4("SteelBlue2", 92, 172, 238),\
D4("SteelBlue3", 79, 148, 205),\
D4("SteelBlue4", 54, 100, 139),\
D4("tan", 210, 180, 140),\
D4("tan1", 255, 165, 79),\
D4("tan2", 238, 154, 73),\
D4("tan3", 205, 133, 63),\
D4("tan4", 139, 90, 43),\
D4("thistle", 216, 191, 216),\
D4("thistle1", 255, 225, 255),\
D4("thistle2", 238, 210, 238),\
D4("thistle3", 205, 181, 205),\
D4("thistle4", 139, 123, 139),\
D4("tomato", 255, 99, 71),\
D4("tomato1", 255, 99, 71),\
D4("tomato2", 238, 92, 66),\
D4("tomato3", 205, 79, 57),\
D4("tomato4", 139, 54, 38),\
D4("turquoise", 64, 224, 208),\
D4("turquoise1", 0, 245, 255),\
D4("turquoise2", 0, 229, 238),\
D4("turquoise3", 0, 197, 205),\
D4("turquoise4", 0, 134, 139),\
D4("violet", 238, 130, 238),\
D4("violet red", 208, 32, 144),\
D4("VioletRed", 208, 32, 144),\
D4("VioletRed1", 255, 62, 150),\
D4("VioletRed2", 238, 58, 140),\
D4("VioletRed3", 205, 50, 120),\
D4("VioletRed4", 139, 34, 82),\
D4("wheat", 245, 222, 179),\
D4("wheat1", 255, 231, 186),\
D4("wheat2", 238, 216, 174),\
D4("wheat3", 205, 186, 150),\
D4("wheat4", 139, 126, 102),\
D4("white", 255, 255, 255),\
D4("white smoke", 245, 245, 245),\
D4("WhiteSmoke", 245, 245, 245),\
D4("yellow", 255, 255, 0),\
D4("yellow green", 154, 205, 50),\
D4("yellow1", 255, 255, 0),\
D4("yellow2", 238, 238, 0),\
D4("yellow3", 205, 205, 0),\
D4("yellow4", 139, 139, 0),\
D4("YellowGreen", 154, 205, 50)


#undef D4
#define D4(a,b,c,d) a
static char *cnames[]={Enum_WaveColors};

#undef D4
#define D4(a,b,c,d) b
static unsigned char c_red[]={Enum_WaveColors};

#undef D4
#define D4(a,b,c,d) c
static unsigned char c_grn[]={Enum_WaveColors};

#undef D4
#define D4(a,b,c,d) d
static unsigned char c_blu[]={Enum_WaveColors};

#define C_ARRAY_SIZE (sizeof(c_red)/sizeof(unsigned char))


static int compar(const void *v1, const void *v2)
{
return((int)strcasecmp((char *)v1, *(char **)v2));
}


int get_rgb_from_name(char *str)
{
char **match;
int offset, rgb;

if((match=(char **)bsearch((void *)str, (void *)cnames, C_ARRAY_SIZE, sizeof(char *), compar)))
	{
	offset=match-cnames;
	rgb=((int)c_red[offset]<<16)|((int)c_grn[offset]<<8)|((int)c_blu[offset]);
	return(rgb);
	}
	else
	{
	unsigned char *pnt=(unsigned char *)str;
	int l=strlen(str);
	unsigned char ch;
	int i, rc;

	for(i=0;i<l;i++)
		{
		ch=*(pnt++);
		if     (((ch>='0')&&(ch<='9')) ||
			((ch>='a')&&(ch<='f')) ||
			((ch>='A')&&(ch<='F'))) continue;
			else
			{
			#ifdef _MSC_VER
			fprintf(stderr, "** gtkwave.ini (line %d): '%s' is an unknown color value; ignoring.\n", rc_line_no, str);
			#else
			fprintf(stderr, "** .gtkwaverc (line %d): '%s' is an unknown color value; ignoring.\n", rc_line_no, str);
			#endif
			return(~0);
			}
		}

	sscanf(str,"%x",&rc);
	return(rc&0x00ffffff);
	}
}
