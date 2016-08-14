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
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <cairo/cairo.h>

static void rounded_rectangle (cairo_t* cr, double x, double y, double w, double h, double r) {
	double degrees = M_PI / 180.0;

	cairo_new_sub_path (cr);
	cairo_arc (cr, x + w - r, y + r, r, -90 * degrees, 0 * degrees);
	cairo_arc (cr, x + w - r, y + h - r, r, 0 * degrees, 90 * degrees);
	cairo_arc (cr, x + r, y + h - r, r, 90 * degrees, 180 * degrees);
	cairo_arc (cr, x + r, y + r, r, 180 * degrees, 270 * degrees);
	cairo_close_path (cr);
}

static float random1 () {
	return rand () / (float)RAND_MAX;
}

static float random2 () {
	return 1.f - 2.f * random1 ();
}

static void make_box (cairo_t* cr, int w, int h) {
	int b = 16;
	int t = 6;
	int r0 = 17;
	int r1 = 6;

	// outer box (borders/shade)
	rounded_rectangle (cr, 0.5, 0.5, w - 1, h - 1, r0);
	cairo_set_source_rgba (cr, .93, .93, .93, 1.0);
	cairo_fill_preserve (cr);
	cairo_clip (cr);

	// payload
	rounded_rectangle (cr, b, t, w - 2 * b, h - 2 * b, r1);
	cairo_set_source_rgba (cr, .95, .95, .95, 1.0);
	cairo_fill (cr);

	cairo_pattern_t* pat;

	// right side shadow
	pat = cairo_pattern_create_linear (0.0, 0.0, b, 0.0);
	cairo_pattern_add_color_stop_rgba (pat, 0.00, .9, .9, .9, 0.0);
	cairo_pattern_add_color_stop_rgba (pat, 0.20, .0, .0, .0, 0.1);
	cairo_pattern_add_color_stop_rgba (pat, 0.60, .0, .0, .0, 0.4);
	cairo_pattern_add_color_stop_rgba (pat, 1.00, .0, .0, .0, 0.6);

	cairo_save (cr);
	cairo_translate (cr, w - b, 0);
	cairo_rectangle (cr, 0, 0, b, h);
	cairo_set_source (cr, pat);
	cairo_fill (cr);
	cairo_restore (cr);
	cairo_pattern_destroy (pat);

	// bottom shadow
	int bb = 2 * b - t;
	pat = cairo_pattern_create_linear (0.0, 0.0, 0, bb);
	cairo_pattern_add_color_stop_rgba (pat, 0.00, .9, .9, .9, 0.0);
	cairo_pattern_add_color_stop_rgba (pat, 0.10, .0, .0, .0, 0.1);
	cairo_pattern_add_color_stop_rgba (pat, 0.30, .0, .0, .0, 0.4);
	cairo_pattern_add_color_stop_rgba (pat, 1.00, .0, .0, .0, 0.7);

	cairo_save (cr);
	cairo_translate (cr, 0, h - bb);
	cairo_rectangle (cr, 0, 0, w, bb);
	cairo_set_source (cr, pat);
	cairo_fill (cr);
	cairo_restore (cr);
	cairo_pattern_destroy (pat);

	// bottom edge
	pat = cairo_pattern_create_linear (0.0, 0.0, 0, bb);
	cairo_pattern_add_color_stop_rgba (pat, 0.00,  .0, .0, .0, 0.0);
	cairo_pattern_add_color_stop_rgba (pat, 0.20,  .0, .0, .0, 0.15);
	cairo_pattern_add_color_stop_rgba (pat, 0.30,  .0, .0, .0, 0.0);
	cairo_pattern_add_color_stop_rgba (pat, 1.00,  .0, .0, .0, 0.0);

	cairo_save (cr);
	cairo_translate (cr, 0, h - bb - 4); // TODO unhardcode
	cairo_rectangle (cr, b + 1.5, 0, w - b, bb);
	cairo_set_source (cr, pat);
	cairo_fill (cr);
	cairo_restore (cr);
	cairo_pattern_destroy (pat);

	// left light
	pat = cairo_pattern_create_linear (0.0, 0.0, b, 0.0);
	cairo_pattern_add_color_stop_rgba (pat, 0.00,  1, 1, 1, 0.9);
	cairo_pattern_add_color_stop_rgba (pat, 1.00,  1, 1, 1, 0.0);

	cairo_rectangle (cr, 0, 0, b, h);
	cairo_set_source (cr, pat);
	cairo_fill (cr);
	cairo_pattern_destroy (pat);

	// top light
	pat = cairo_pattern_create_linear (0.0, 0.0, 0.0, t);
	cairo_pattern_add_color_stop_rgba (pat, 0.00,  1, 1, 1, 0.9);
	cairo_pattern_add_color_stop_rgba (pat, 1.00,  1, 1, 1, 0.0);

	cairo_rectangle (cr, 0, 0, w, t);
	cairo_set_source (cr, pat);
	cairo_fill (cr);
	cairo_pattern_destroy (pat);

	// top-edge highlight
	pat = cairo_pattern_create_linear (0.0, 0.0, 0, t);
	cairo_pattern_add_color_stop_rgba (pat, 0.00,  1, 1, 1, 0.0);
	cairo_pattern_add_color_stop_rgba (pat, 0.50,  1, 1, 1, 1.0);
	cairo_pattern_add_color_stop_rgba (pat, 1.00,  1, 1, 1, 0.0);

	cairo_rectangle (cr, b + 0.5, 4, w - 2 * b - 1, t);
	cairo_set_source (cr, pat);
	cairo_fill (cr);
	cairo_pattern_destroy (pat);

	// bottom left corner
	cairo_arc_negative (cr, 0.5 + r0, 0.5 + h - bb - r0 - 3, r0 + 3, M_PI, 0.5 * M_PI);
	cairo_set_line_width (cr, 3.0);
	cairo_set_source_rgba (cr, 0, 0, 0, 0.07);
	cairo_stroke (cr);

	cairo_arc_negative (cr, 0.5 + r0, 0.5 + h - bb - r0 + 6, r0 + 1, M_PI, 0.5 * M_PI);
	cairo_set_line_width (cr, 3.0);
	cairo_set_source_rgba (cr, 0, 0, 0, 0.1);
	cairo_stroke (cr);

	// bottom left corner shade
	cairo_arc (cr, 0.5 + r0, 0.5 + h - r0, r0, 0.5 * M_PI, M_PI);
	cairo_arc_negative (cr, 0.5 + r0, 0.5 + h - bb - r0, r0 + 4, M_PI, 0.5 * M_PI);

	pat = cairo_pattern_create_linear (b, h - bb - r0, b * .5, h);
	cairo_pattern_add_color_stop_rgba (pat, 0.50,  1, 1, 1, 0.0);
	cairo_pattern_add_color_stop_rgba (pat, 1.00,  0, 0, 0, 0.4);
	cairo_set_source (cr, pat);
	cairo_fill (cr);
}

static cairo_pattern_t* scratch_pattern (float x, float y, float dx, float dy, float m) {
	int j;
	cairo_pattern_t* pat;
	pat = cairo_pattern_create_linear (x, y, x + dx, y + dy);
	cairo_pattern_add_color_stop_rgba (pat, 0.00, .9, .9, .9, 0.0);
	for (j = 1 ; j < 9; ++j) {
		cairo_pattern_add_color_stop_rgba (pat, j/10.f, .6, .6, .6, .01 + m * random1 ());
	}
	cairo_pattern_add_color_stop_rgba (pat, 1.00, .9, .9, .9, 0.0);
	return pat;
}
	

static void add_scratches (cairo_t* cr, int w, int h) {
	int b = 16;
	int t = 6;
	int r1 = 6;
	rounded_rectangle (cr, b, t, w - 2 * b, h - 2 * b, r1);
	cairo_clip (cr);

	float ww = w - b * 2;
	float hh = h - b * 2;

	float ww2 = ww * .4;
	float hh2 = hh * .4;
	float ww4 = ww2 * .5;
	float hh4 = hh2 * .5;

	cairo_set_line_cap (cr, CAIRO_LINE_CAP_ROUND);

	float maxlen = sqrt (ww2 * ww2 + hh2 * hh2);
	int deep = 0;
	fprintf (stderr, "D: %.1f\n", maxlen);

	cairo_pattern_t* pat;
	int i, j;

	for (i = 0; i < ceil (maxlen); ++i) {
		float x = b + ww * random1 ();
		float y = t + hh * random1 ();
		float dx = ww4 - ww2 * random1 ();
		float dy = hh4 - hh2 * random1 ();


		cairo_move_to (cr, x, y);
		if (random1 () > .5) {
			cairo_rel_line_to (cr, dx, dy);
		} else {
			cairo_rel_curve_to (cr, 0, 0, dx * .5 * random2 (), dy * .5 * random2 (), dx, dy);
		}

		pat = scratch_pattern (x, y, dx, dy, .07);
		cairo_set_source (cr, pat);
		cairo_set_line_width (cr, 0.5 + 1.5 * random1 ());
		cairo_stroke (cr);
		cairo_pattern_destroy (pat);
	}

	// short & deep
	for (i = 0; i < ceil (maxlen * .1); ++i) {
		float x = b + ww * random1 ();
		float y = t + hh * random1 ();
		float dx = .7 * ww4 * random2 ();
		float dy = .7 * ww4 * random2 ();

		float len = sqrt (dx * dx + dy * dy);

		pat = scratch_pattern (x, y, dx, dy, .2);

		cairo_move_to (cr, x, y);
		if (random1 () > .5) {
			cairo_rel_line_to (cr, dx, dy);
		} else {
			cairo_rel_curve_to (cr, 0, 0, dx * .5 * random2 (), dy * .5 * random2 (), dx, dy);
		}

		cairo_set_source (cr, pat);
		cairo_set_line_width (cr, 0.5 + 1.5 * random1 ());

		if (len < maxlen * .04) {
			++deep;
			for (j = 0 ; j < 4; ++j) {
				cairo_stroke_preserve (cr);
				cairo_stroke_preserve (cr);
				cairo_pattern_destroy (pat);
				pat = scratch_pattern (x, y, dx, dy, .9);
				cairo_set_line_width (cr, 2.5 + 1.5 * random1 ());
			}
			cairo_stroke_preserve (cr);
		}
		cairo_stroke (cr);
		cairo_pattern_destroy (pat);
	}
	fprintf (stderr, "ADDED %d deep bump(s)\n", deep);
}

int main (int argc, char **argv) {
	int n_steps = 8;
	int n_notes = 8;
	srand (time (NULL));
	char* fn = "../modgui/box.png";

	if (argc > 1) {
	 fn = argv[1];
	}
	if (argc > 2) {
		n_steps = atoi (argv[2]);
	}
	if (argc > 3) {
		n_notes = atoi (argv[3]);
	}
	
	/* see gridgen.sh */
	int w = 250 + 46 * n_steps;
	int h = 192 + 46 * n_notes;
	fprintf (stderr, "G: %d x %d F: %s\n", w, h, fn);

	cairo_surface_t* cs = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, w, h);
	cairo_t* cr = cairo_create (cs);

	make_box (cr, w, h);
	add_scratches (cr, w, h);

	cairo_surface_write_to_png (cs, fn);
	cairo_destroy (cr);
	cairo_surface_destroy (cs);
	return 0;
}
