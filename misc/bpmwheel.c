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
#include <pango/pango.h>
#include <pango/pangocairo.h>

static void wheel_path (cairo_t* cr, int w, int h) {
	float cx = w * .5;
	float cy = h * .5;

	float da = M_PI * .32;
	float sx = 0.15;
	float dx = w * .40;
	float rad = h * .5;

	cairo_matrix_t mtrx;

	cairo_get_matrix (cr, &mtrx);
	cairo_translate (cr, cx - dx, cy);
	cairo_scale (cr, sx, 1);
	cairo_arc (cr, 0, 0, rad, -M_PI - da, -M_PI + da);
	cairo_set_matrix (cr, &mtrx);

	cairo_get_matrix (cr, &mtrx);
	cairo_translate (cr, cx + dx, cy);
	cairo_scale (cr, sx, 1);
	cairo_arc (cr, 0, 0, rad, -da, da);
	cairo_set_matrix (cr, &mtrx);
	cairo_close_path (cr);

}

static void draw_wheel (cairo_t* cr, int w, int h) {
	float cx = w * .5;
	float cy = h * .5;

	cairo_pattern_t* pat = cairo_pattern_create_linear (0.0, 0.0, 0.0, h);
	cairo_pattern_add_color_stop_rgba (pat, 0.00,  .3, .3, .3, 0.5);
	cairo_pattern_add_color_stop_rgba (pat, 0.47,  .6, .6, .6, 0.5);
	cairo_pattern_add_color_stop_rgba (pat, 0.66,  .2, .2, .2, 0.5);
	cairo_pattern_add_color_stop_rgba (pat, 1.00,  .0, .0, .0, 0.5);

	// shadow
	cairo_save (cr);
	cairo_translate (cr, 1, 1);

	wheel_path (cr, w, h);

	cairo_set_line_width (cr, 2.5);
	cairo_set_source_rgba (cr, .1, .1, .1, .5);
	cairo_stroke (cr);
	cairo_restore (cr);

	// light
	cairo_save (cr);
	cairo_translate (cr, -1, -1);

	wheel_path (cr, w, h);

	cairo_set_line_width (cr, 2.5);
	cairo_set_source_rgba (cr, .8, .8, .8, .7);
	cairo_stroke (cr);
	cairo_restore (cr);

	// actual cylinder
	wheel_path (cr, w, h);

	cairo_set_source_rgba (cr, .25, .25, .25, 1);
	cairo_fill_preserve (cr);
	cairo_set_source (cr, pat);
	cairo_fill_preserve (cr);

	cairo_set_source_rgba (cr, 0, 0, 0, 1);

	cairo_set_line_width (cr, 1.25);
	cairo_stroke_preserve (cr);

	cairo_clip (cr);

	cairo_pattern_destroy (pat);
	cairo_translate (cr, cx, cy);
}

static void add_shine (cairo_t* cr, int w, int h, float f) {
	float cx = w * .5;
	float cy = h * .5;

	float off = 1.f - 2.f * fabs (.5 - f);
	off *= .033;

	cairo_pattern_t* pat = cairo_pattern_create_linear (0.0, 0.0, 3.0, h);
	cairo_pattern_add_color_stop_rgba (pat, 0.00,  .0, .0, .0, 0.0);
	cairo_pattern_add_color_stop_rgba (pat, 0.32 - off,  .75, .75, .75, 0.0);
	cairo_pattern_add_color_stop_rgba (pat, 0.39 - off,  .75, .75, .75, 0.1);
	cairo_pattern_add_color_stop_rgba (pat, 0.45 - off,  .3, .3, .3, 0.0);
	cairo_pattern_add_color_stop_rgba (pat, 1.00,  .0, .0, .0, 0.0);

	cairo_translate (cr, -cx, -cy);

	cairo_rectangle (cr, 0, 0, w, h);
	cairo_set_source (cr, pat);
	cairo_fill (cr);
	cairo_pattern_destroy (pat);
}

static void draw_value (cairo_t* cr, PangoLayout* pl, const char* txt, int h, float f) {
	int tw, th;
	cairo_save (cr);
	cairo_set_source_rgba (cr, 1.0, 0.6, 0.2, 1.0);
	pango_layout_set_text (pl, txt, -1);
	pango_layout_get_pixel_size (pl, &tw, &th);
	cairo_scale (cr, 1.0, 1.2 - sqrt (fabs (0.5 - f)));
	cairo_translate (cr, -tw / 2.0, -th / 2.0);
	cairo_translate (cr, 0 , h * 2. * (.5 - f));
	pango_cairo_layout_path (cr, pl);
	cairo_fill (cr);
	cairo_restore (cr);
}

static void draw_wheel_steps (cairo_t* cr, int w, int h, int n_steps) {
	int i, s;
	int substeps = 3;

	float v_min = 40;
	float v_max = 208;

	PangoLayout* pl = pango_cairo_create_layout (cr);
	PangoFontDescription *desc = pango_font_description_from_string ("Sans 17px"); // XXX use MOD font
	pango_layout_set_font_description (pl, desc);
	pango_font_description_free (desc);

	for (i = 0; i < n_steps; ++i) {
		cairo_save (cr);
		cairo_translate (cr, i * w, 0);
		draw_wheel (cr, w, h);
		for (s = 0; s < substeps; ++s) {
			char txt[7];
			float v = v_min + (i + s - 1) * (v_max - v_min) / (n_steps - 1);
			if (v < v_min || v > v_max) {
				continue;
			}
			sprintf (txt, "%.0f", v);
			float off = (1.f + s) / (substeps + 1.f);
			draw_value (cr, pl, txt, h, off);
		}
		add_shine (cr, w, h, (i % 7) / 6.0);
		cairo_restore (cr);
	}

	g_object_unref (pl);

}

int main (int argc, char **argv) {
	int w = 64;
	int h = 44;
	int n_steps = 169;

	cairo_surface_t* cs = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, n_steps * w, h);
	cairo_t* cr = cairo_create (cs);

	draw_wheel_steps (cr, w, h, n_steps);

	cairo_surface_write_to_png (cs, "../modgui/knob.png");
	cairo_destroy (cr);
	cairo_surface_destroy (cs);
	return 0;
}
