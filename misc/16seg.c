/*
 * Copyright (C) 2016 Robin Gareus <robin@gareus.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <math.h>
#include <cairo/cairo.h>

static void sixteenseg (cairo_t* cr, int h, int s) {
	// h = 424 -- base scale for path
	const float w = h * 3. / 4.; // 318

	const float c_x = w * 0.5; // 159
	const float c_y = h * 0.5; // 212

	// tilt: 80deg
	float dy = 0.5 * h * 0.875;
	float dx = dy / tan (M_PI * 80.f / 180.f);

	float top_x0 = c_x + dx; // 191.7
	float top_y0 = c_y - dy; // 26.5
	float bot_x0 = c_x - dx; // 126.29
	float bot_y0 = c_y + dy; // 397.5

#define STROKE(seg) \
	cairo_close_path (cr); \
	if (s & (1 << seg)) { \
		cairo_set_source_rgba (cr, 1.0, 0.6, 0.2, 1.0); \
	} else { \
		cairo_set_source_rgba (cr, .3, .3, .3, 1); \
	} \
	cairo_fill (cr);

#define SX(X) (X * h / 424.0)
#define SY(Y) (Y * w / 318.0)

	// H center left
	cairo_move_to (cr, c_x - SX(2), c_y);
	cairo_rel_line_to (cr, SX(-20), SY( -13));
	cairo_rel_line_to (cr, SX(-53), SY(   0));
	cairo_rel_line_to (cr, SX(-17), SY(  13));
	cairo_rel_line_to (cr, SX( 11), SY(  13));
	cairo_rel_line_to (cr, SX( 51), SY(   0));
	STROKE (8);

	// H center right
	cairo_move_to (cr, c_x + SX(2), c_y);
	cairo_rel_line_to (cr, SX( 28), SY( -13));
	cairo_rel_line_to (cr, SX( 51), SY(   0));
	cairo_rel_line_to (cr, SX( 11), SY(  13));
	cairo_rel_line_to (cr, SX(-17), SY(  13));
	cairo_rel_line_to (cr, SX(-53), SY(   0));
	STROKE (7);

	// H top left
	cairo_move_to (cr, top_x0 - SX(5), top_y0);
	cairo_rel_line_to (cr, SX(-22), SY(  18));
	cairo_rel_line_to (cr, SX(-55), SY(   0));
	cairo_rel_line_to (cr, SX(-16), SY( -18));
	cairo_rel_line_to (cr, SX( 10), SY( -10));
	cairo_rel_line_to (cr, SX( 75), SY(   0));
	STROKE (15);

	// H top right
	cairo_move_to (cr, top_x0 + SX(4), top_y0);
	cairo_rel_line_to (cr, SX( 16), SY(  18));
	cairo_rel_line_to (cr, SX( 55), SY(   0));
	cairo_rel_line_to (cr, SX( 22), SY( -18));
	cairo_rel_line_to (cr, SX( -8), SY( -10));
	cairo_rel_line_to (cr, SX(-75), SY(   0));
	STROKE (14);

	// H bottom left
	cairo_move_to (cr, bot_x0 - SX(2), bot_y0); // start in center
	cairo_rel_line_to (cr, SX(-16), SY( -18));
	cairo_rel_line_to (cr, SX(-55), SY(   0));
	cairo_rel_line_to (cr, SX(-22), SY(  18));
	cairo_rel_line_to (cr, SX(  7), SY(   9));
	cairo_rel_line_to (cr, SX( 75), SY(   0));
	STROKE (0);

	// H bottom right
	cairo_move_to (cr, bot_x0 + SX(5), bot_y0);
	cairo_rel_line_to (cr, SX( 22), SY( -18));
	cairo_rel_line_to (cr, SX( 55), SY(   0));
	cairo_rel_line_to (cr, SX( 16), SY(  18));
	cairo_rel_line_to (cr, SX(-11), SY(   9));
	cairo_rel_line_to (cr, SX(-75), SY(   0));
	STROKE (11);

	// V h-center top
	cairo_move_to (cr, c_x + SX(2), c_y - SY(8));
	cairo_rel_line_to (cr, SX( 21), SY( -44));
	cairo_rel_line_to (cr, SX( 20), SY(-113));
	cairo_rel_line_to (cr, SX(-11), SY( -15));
	cairo_rel_line_to (cr, SX(-17), SY(  15));
	cairo_rel_line_to (cr, SX(-20), SY( 115));
	STROKE (5);

	// V h-center bottom
	cairo_move_to (cr, c_x - SX(1), c_y + SY(6));
	cairo_rel_line_to (cr, SX(-21), SY(  44));
	cairo_rel_line_to (cr, SX(-20), SY( 115));
	cairo_rel_line_to (cr, SX( 11), SY(  15));
	cairo_rel_line_to (cr, SX( 17), SY( -15));
	cairo_rel_line_to (cr, SX( 20), SY(-113));
	STROKE (2);

	// --------------------

	// V left-top
	cairo_move_to (cr, c_x - SX(95), c_y - SY(5));
	cairo_rel_line_to (cr, SX( 16), SY( -14));
	cairo_rel_line_to (cr, SX( 25), SY(-143));
	cairo_rel_line_to (cr, SX(-16), SY( -19));
	cairo_rel_line_to (cr, SX(-10), SY(   9));
	cairo_rel_line_to (cr, SX(-26), SY( 155));
	STROKE (9);

	// V left-bottom
	cairo_move_to (cr, c_x - SX(97), c_y + SY(4));
	cairo_rel_line_to (cr, SX(-16), SY(  14));
	cairo_rel_line_to (cr, SX(-26), SY( 155));
	cairo_rel_line_to (cr, SX(  6), SY(   7));
	cairo_rel_line_to (cr, SX( 23), SY( -18));
	cairo_rel_line_to (cr, SX( 25), SY(-143));
	STROKE (10);

	// V right-bottom
	cairo_move_to (cr, c_x + SX(96), c_y + SY(5));
	cairo_rel_line_to (cr, SX(-16), SY(  14));
	cairo_rel_line_to (cr, SX(-25), SY( 143));
	cairo_rel_line_to (cr, SX( 16), SY(  18));
	cairo_rel_line_to (cr, SX( 11), SY(  -8));
	cairo_rel_line_to (cr, SX( 26), SY(-155));
	STROKE (12);

	// V right-top
	cairo_move_to (cr, c_x + SX(98), c_y - SY(4));
	cairo_rel_line_to (cr, SX( 16), SY( -14));
	cairo_rel_line_to (cr, SX( 26), SY(-155));
	cairo_rel_line_to (cr, SX( -6), SY(  -8));
	cairo_rel_line_to (cr, SX(-23), SY(  18));
	cairo_rel_line_to (cr, SX(-25), SY( 143));
	STROKE (13);

	// --------------------

	// D top-left
	cairo_move_to (cr, c_x - SX(2), c_y - SY(5));
	cairo_rel_line_to (cr, SX( -9), SY( -68));
	cairo_rel_line_to (cr, SX(-21), SY( -64));
	cairo_rel_line_to (cr, SX(-19), SY( -22));
	cairo_rel_line_to (cr, SX( -5), SY(  32));
	cairo_rel_line_to (cr, SX( 36), SY( 109));
	STROKE (6);

	// D top-right
	cairo_move_to (cr, c_x + SX(4), c_y - SY(5));
	cairo_rel_line_to (cr, SX( 36), SY( -76));
	cairo_rel_line_to (cr, SX( 34), SY( -51));
	cairo_rel_line_to (cr, SX( 33), SY( -28));
	cairo_rel_line_to (cr, SX( -7), SY(  37));
	cairo_rel_line_to (cr, SX(-71), SY( 104));
	STROKE (4);

	// D bottom-left
	cairo_move_to (cr, c_x - SX(4), c_y + SY(4));
	cairo_rel_line_to (cr, SX(-33), SY(  74));
	cairo_rel_line_to (cr, SX(-40), SY(  59));
	cairo_rel_line_to (cr, SX(-28), SY(  22));
	cairo_rel_line_to (cr, SX(  6), SY( -36));
	cairo_rel_line_to (cr, SX( 71), SY(-104));
	STROKE (1);

	// D bottom-right
	cairo_move_to (cr, c_x + SX(2), c_y + SY(4));
	cairo_rel_line_to (cr, SX(  9), SY(  68));
	cairo_rel_line_to (cr, SX( 21), SY(  64));
	cairo_rel_line_to (cr, SX( 19), SY(  22));
	cairo_rel_line_to (cr, SX(  5), SY( -32));
	cairo_rel_line_to (cr, SX(-36), SY(-109));
	STROKE (3);
}

/*
 *      8000         4000
 *      ----   20    ----
 *     |\       |      / | 2000
 * 200 | \40    |   10/  |
 *
 *  100 ---          ---- 80
 *
 * 400 | /2     |     8\ | 1000
 *     |/       |       \|
 *      ----    4    ----
 *        1           800
 */

static const int seg16hextable[] = {
	/* */ 0x0000, /*!*/ 0x3001, /*"*/ 0x2020, /*#*/ 0x07a4,
	/*$*/ 0xdba5, /*%*/ 0x9bb6, /*&*/ 0x8d69, /*'*/ 0x0020,
	/*(*/ 0x0018, /*)*/ 0x0042, /***/ 0x01fe, /*+*/ 0x01a4,
	/*,*/ 0x0002, /*-*/ 0x0180, /*.*/ 0x0800, /*/*/ 0x0012,
	/*0*/ 0xfe13, /*1*/ 0x3010, /*2*/ 0xed81, /*3*/ 0xf881,
	/*4*/ 0x3380, /*5*/ 0xdb81, /*6*/ 0xdf81, /*7*/ 0xc014,
	/*8*/ 0xff81, /*9*/ 0xfb80, /*:*/ 0x0024, /*;*/ 0x0022,
	/*<*/ 0x0118, /*=*/ 0x0980, /*>*/ 0x00c2, /*?*/ 0xe084,
	/*@*/ 0xee91, /*A*/ 0xf780, /*B*/ 0xf8a5, /*C*/ 0xce01,
	/*D*/ 0xf825, /*E*/ 0xcf01, /*F*/ 0xc700, /*G*/ 0xde81,
	/*H*/ 0x3780, /*I*/ 0xc825, /*J*/ 0x3c01, /*K*/ 0x0718,
	/*L*/ 0x0e01, /*M*/ 0x3650, /*N*/ 0x3648, /*O*/ 0xfe01,
	/*P*/ 0xe780, /*Q*/ 0xfe09, /*R*/ 0xe788, /*S*/ 0xdb81,
	/*T*/ 0xc024, /*U*/ 0x3e01, /*V*/ 0x0612, /*W*/ 0x360a,
	/*X*/ 0x005a, /*Y*/ 0x2384, /*Z*/ 0xc813, /*[*/ 0x4824,
	/*\*/ 0x0048, /*]*/ 0x8025, /*^*/ 0x000A, /*_*/ 0x0801,
	/*`*/ 0x0040, /*a*/ 0x0d05, /*b*/ 0x0705, /*c*/ 0x0501,
	/*d*/ 0x0525, /*e*/ 0x0d03, /*f*/ 0x41a4, /*g*/ 0x8325,
	/*h*/ 0x0704, /*i*/ 0x0004, /*j*/ 0x0425, /*k*/ 0x003c,
	/*l*/ 0x0024, /*m*/ 0x1584, /*n*/ 0x0504, /*o*/ 0x0505,
	/*p*/ 0x8720, /*q*/ 0x8324, /*r*/ 0x0500, /*s*/ 0x8305,
	/*t*/ 0x01a4, /*u*/ 0x0405, /*v*/ 0x0402, /*w*/ 0x140a,
	/*x*/ 0x005a, /*y*/ 0x0054, /*z*/ 0x0103, /*{*/ 0x4924,
	/*|*/ 0x0600, /*}*/ 0x80a5, /*~*/ 0x0050, /* */ 0xffff
};

#define LETTER_SPACE .875

/** draw a single 16seg display -- by char */
static void ssdc (cairo_t* cr, int h, char c) {
	int s = 0;
	if (c >= 32 && c < 127) {
		s = seg16hextable [c-32];
	}

	sixteenseg (cr, h, s);
}

/** draw null terminated text */
static void ssdt (cairo_t* cr, int h, char* c) {
	while (*c) {
		ssdc (cr, h, *c);
		cairo_translate (cr, h * LETTER_SPACE * 3. / 4., 0);
		++c;
	}
}

/** numbers 0..n_steps */
static void gen_nums (const char* fn, int h, int n_steps) {
	int s;
	int cc = 4;
	int bw = ((LETTER_SPACE * (cc - 1) + 1)) * h * 3 / 4;
	printf ("NUMS: w:%d g:%dx%d f:%s\n", bw, n_steps * bw, h, fn);

	cairo_surface_t* cs = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, n_steps * bw, h);
	cairo_t* cr = cairo_create (cs);

	h -= 3;
	cairo_translate (cr, 2, 1);

	for (s = 0; s < n_steps; ++s) {
		cairo_save (cr);
		cairo_translate (cr, s * bw, 0);

		char txt[7];
		snprintf (txt, 16, "%4d", s);

		ssdt (cr, h, txt);
		cairo_restore (cr);
	}
	cairo_surface_write_to_png (cs, fn);
	cairo_destroy (cr);
	cairo_surface_destroy (cs);
}

/** MIDI note names */
static void gen_note (const char* fn, int h) {
	int s;
	int n_steps = 128;

	int cc = 4;
	int bw = ((LETTER_SPACE * (cc - 1) + 1)) * h * 3 / 4;
	printf ("NOTE: w:%d g:%dx%d f:%s\n", bw, n_steps * bw, h, fn);

	cairo_surface_t* cs = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, n_steps * bw, h);
	cairo_t* cr = cairo_create (cs);

	h -= 3;
	cairo_translate (cr, 2, 1);

	for (s = 0; s < n_steps; ++s) {
		cairo_save (cr);
		cairo_translate (cr, s * bw, 0);

		char txt[7];
		int note = s % 12;
		int oct = s / 12 - 1;

		switch (note) {
			case  0: sprintf (txt, "C %+d", oct); break;
			case  1: sprintf (txt, "Db%+d", oct); break;
			case  2: sprintf (txt, "D %+d", oct); break;
			case  3: sprintf (txt, "Eb%+d", oct); break;
			case  4: sprintf (txt, "E %+d", oct); break;
			case  5: sprintf (txt, "F %+d", oct); break;
			case  6: sprintf (txt, "Gb%+d", oct); break;
			case  7: sprintf (txt, "G %+d", oct); break;
			case  8: sprintf (txt, "Ab%+d", oct); break;
			case  9: sprintf (txt, "A %+d", oct); break;
			case 10: sprintf (txt, "Bb%+d", oct); break;
			case 11: sprintf (txt, "b %+d", oct); break;
			default: break;
		}

		ssdt (cr, h, txt);
		cairo_restore (cr);
	}
	cairo_surface_write_to_png (cs, fn);
	cairo_destroy (cr);
	cairo_surface_destroy (cs);
}

static const char *mdrums[] = {
      "Kik2", // "Bass Drum 2"  #35
      "Kik1", // "Bass Drum 1"
      "Rims", // "Side Stick/Rimshot"
      "Sna1", // "Snare Drum 1"
      "Clap", // "Hand Clap"
      "Sna2", // "Snare Drum 2"
      "Tom2", // "Low Tom 2"
      "HH C", // "Closed Hi-hat"
      "Tom1", // "Low Tom 1"
      "HH P", // "Pedal Hi-hat"
      "Tom2", // "Mid Tom 2"
      "HH O", // "Open Hi-hat"
      "Tom1", // "Mid Tom 1"
      "Tom2", // "High Tom 2"
      "Crs1", // "Crash Cymbal 1"
      "Tom1", // "High Tom 1"
      "Ride", // "Ride Cymbal 1"
      "Chin", // "Chinese Cymbal"
      "RidB", // "Ride Bell"
      "Tamp", // "Tambourine"
      "Spla", // "Splash Cymbal"
      "Cowb", // "Cowbell"
      "Crs2", // "Crash Cymbal 2"
      "Slap", // "Vibra Slap"
      "Rid2", // "Ride Cymbal 2"
      "HBon", // "High Bongo"
      "LBon", // "Low Bongo"
      "MCon", // "Mute High Conga"
      "OCon", // "Open High Conga"
      "LCon", // "Low Conga"
      "HTim", // "High Timbale"
      "LTim", // "Low Timbale"
      "HAgo", // "High Agogô"
      "LAgo", // "Low Agogô"
      "Caba", // "Cabasa"
      "Mara", // "Maracas"
      "S Ws", // "Short Whistle"
      "L Ws", // "Long Whistle"
      "SGui", // "Short Güiro"
      "LGui", // "Long Güiro"
      "Clav", // "Claves"
      "H WB", // "High Wood Block"
      "L WB", // "Low Wood Block"
      "MCui", // "Mute Cuíca"
      "OCui", // "Open Cuíca"
      "MTri", // "Mute Triangle"
      "OTri"  // "Open Triangle"
};

/** General MIDI Drum names */
static void gen_drum (const char* fn, int h) {
	int s;
	int cc = 4;
	int bw = ((LETTER_SPACE * (cc - 1) + 1)) * h * 3 / 4;
	int n_steps = 128;
	printf ("DRUM: w:%d g:%dx%d f:%s\n", bw, n_steps * bw, h, fn);

	cairo_surface_t* cs = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, n_steps * bw, h);
	cairo_t* cr = cairo_create (cs);

	h -= 3;
	cairo_translate (cr, 2, 1);

	for (s = 0; s < n_steps; ++s) {
		cairo_save (cr);
		cairo_translate (cr, s * bw, 0);

		char txt[7];
		if (s < 32 || s > 81) {
			snprintf (txt, 16, "D%3d", s);
		} else {
			snprintf (txt, 16, "%4s", mdrums[s-32]);
		}

		ssdt (cr, h, txt);
		cairo_restore (cr);
	}
	cairo_surface_write_to_png (cs, fn);
	cairo_destroy (cr);
	cairo_surface_destroy (cs);
}

int main (int argc, char **argv) {
	gen_note ("../modgui/m_note.png", 42);
	gen_nums ("../modgui/m_nums.png", 42, 128);
	gen_drum ("../modgui/m_drum.png", 42);
	return 0;
}
