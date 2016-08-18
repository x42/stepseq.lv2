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

static void draw_text (cairo_t* cr, PangoLayout* pl, const char* txt) {
	int tw, th;
	cairo_save (cr);
	cairo_set_source_rgba (cr, 0.0, 0.0, 0.0, 1.0);
	pango_layout_set_text (pl, txt, -1);
	// rotate 90deg ? maybe
	pango_layout_get_pixel_size (pl, &tw, &th);
	cairo_translate (cr, -tw / 2.0, -th / 2.0);
	pango_cairo_layout_path (cr, pl);
	cairo_fill (cr);
	cairo_restore (cr);
}

static void draw_swinger (cairo_t* cr, int w, int h, int n_steps) {
	int i;

	PangoLayout* pl = pango_cairo_create_layout (cr);
	PangoFontDescription *desc = pango_font_description_from_string ("Sans 14px"); // XXX use MOD font
	pango_layout_set_font_description (pl, desc);
	pango_font_description_free (desc);

	cairo_pattern_t* pat = cairo_pattern_create_linear (0.0, 0.0, 3.0, h);
#if 0
	cairo_pattern_add_color_stop_rgba (pat, 0.00, 1.0, 0.0, 0.0, 1.0);
	cairo_pattern_add_color_stop_rgba (pat, 0.33, 1.0, 0.6, 0.2, 1.0);
	cairo_pattern_add_color_stop_rgba (pat, 0.60, 0.0, 1.0, 0.0, 1.0);
	cairo_pattern_add_color_stop_rgba (pat, 1.00, 0.0, 0.0, 1.0, 1.0);
#else
	cairo_pattern_add_color_stop_rgba (pat, 0.00, 1.0, 0.6, 0.2, 1.0);
	cairo_pattern_add_color_stop_rgba (pat, 0.33, 1.0, 0.6, 0.2, 1.0);
	cairo_pattern_add_color_stop_rgba (pat, 1.00, 0.5, 0.3, 0.1, 1.0);
#endif

	const int nsm1 = n_steps - 1;
	for (i = 0; i < n_steps; ++i) {
		cairo_save (cr);
		cairo_translate (cr, i * w, 0);
		cairo_set_source_rgba (cr, 1.0, 1.0, 1.0, 1.0);
		cairo_rectangle (cr, 0, 0, w, h);
		cairo_fill (cr);

		float vh = h * i / (n_steps - 1.f);
		cairo_rectangle (cr, 0, h - vh, w, vh);
		cairo_set_source (cr, pat);
		cairo_fill (cr);

		cairo_translate (cr, w * .5, h * .5);
		if (i == 0) {
			draw_text (cr, pl, "1:1");
		}
		if (  6 * nsm1 == 15 * i) {
			draw_text (cr, pl, "3:2");
		}
		if ( 10 * nsm1 == 15 * i) {
			draw_text (cr, pl, "2:1");
		}
		if ( 15 * nsm1 == 15 * i) {
			draw_text (cr, pl, "3:1");
		}

		cairo_restore (cr);
	}

	g_object_unref (pl);

}

int main (int argc, char **argv) {
	int w = 30;
	int h = 42;
	/* need to be able to represent
	 * 0, 0.2, 1/3 and 0.5
	 * common denominator 30:
	 * 0, 6, 10, 15
	 */
	int n_steps = 31;

	cairo_surface_t* cs = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, n_steps * w, h);
	cairo_t* cr = cairo_create (cs);

	draw_swinger (cr, w, h, n_steps);

	cairo_surface_write_to_png (cs, "../modgui/swinger.png");
	cairo_destroy (cr);
	cairo_surface_destroy (cs);
	return 0;
}
