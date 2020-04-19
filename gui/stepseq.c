/* robtk stepseq gui
 *
 * Copyright 2016 Robin Gareus <robin@gareus.org>
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
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <assert.h>

#include "src/stepseq.h"
#include "gui/velocity_button.h"
#include "gui/custom_knob.h"

#define RTK_URI SEQ_URI
#define RTK_GUI "ui"

#ifndef MAX
#define MAX(A,B) ((A) > (B)) ? (A) : (B)
#endif

#ifndef MIN
#define MIN(A,B) ((A) < (B)) ? (A) : (B)
#endif

typedef struct {
	LV2UI_Write_Function write;
	LV2UI_Controller controller;

	PangoFontDescription* font[2];

	RobWidget*   rw; // top-level container
	RobWidget*   ctbl; // control element table

	RobTkVBtn*   btn_grid[N_NOTES * N_STEPS];
	RobTkSelect* sel_note[N_NOTES];
	RobTkLbl*    lbl_note[N_NOTES];
	RobTkPBtn*   btn_clear[N_NOTES + N_STEPS + 1];
	RobTkCBtn*   btn_sync;
	RobTkSelect* sel_drum;
	RobTkSelect* sel_mchn;
	RobTkCnob*   spn_div;
	RobTkCnob*   spn_bpm;
	RobTkCnob*   spn_swing;
	RobTkPBtn*   btn_panic;
	RobTkSep*    sep_h0;
	RobTkLbl*    lbl_chn;
	RobTkLbl*    lbl_div;
	RobTkLbl*    lbl_bpm;
	RobTkLbl*    lbl_swg;

	cairo_pattern_t* swg_bg;
	cairo_surface_t* bpm_bg;
	cairo_surface_t* div_bg;

	float user_bpm;
	bool disable_signals;
	const char* nfo;
} SeqUI;

///////////////////////////////////////////////////////////////////////////////

static const char* mdrums[] = {
      "Kick 2",    // "Bass Drum 2"  #35
      "Kick 1",    // "Bass Drum 1"
      "RimShot",   // "Side Stick/Rimshot"
      "Snare 1",   // "Snare Drum 1"
      "Clap",      // "Hand Clap"
      "Snare 2",   // "Snare Drum 2"
      "Low Tom 2", // "Low Tom 2"
      "HH Closed", // "Closed Hi-hat"
      "Low Tom 1", // "Low Tom 1"
      "HH Pedal",  // "Pedal Hi-hat"
      "Mid Tom 2", // "Mid Tom 2"
      "HH Open",   // "Open Hi-hat"
      "Mid Tom 1", // "Mid Tom 1"
      "Hi Tom 2",  // "High Tom 2"
      "Crash 1",   // "Crash Cymbal 1"
      "Hi Tom 1",  // "High Tom 1"
      "Ride 1",    // "Ride Cymbal 1"
      "China Cym", // "Chinese Cymbal"
      "Ride Bell", // "Ride Bell"
      "Tambour.",  // "Tambourine"
      "Splash",    // "Splash Cymbal"
      "Cowbell",   // "Cowbell"
      "Crash 2",   // "Crash Cymbal 2"
      "Slap",      // "Vibra Slap"
      "Ride 2",    // "Ride Cymbal 2"
      "Bongo Hi",  // "High Bongo"
      "Bongo Lo",  // "Low Bongo"
      "Conga Mt.", // "Mute High Conga"
      "Conga Op.", // "Open High Conga"
      "Conga Low", // "Low Conga"
      "Timbale H", // "High Timbale"
      "Timbale L", // "Low Timbale"
      "Agogo H",   // "High Agogô"
      "Agogo L",   // "Low Agogô"
      "Cabasa",    // "Cabasa"
      "Maracas",   // "Maracas"
      "Whistle S", // "Short Whistle"
      "Whistle L", // "Long Whistle"
      "Guiro S",   // "Short Güiro"
      "Guiro L",   // "Long Güiro"
      "Claves",    // "Claves"
      "Woodblk H", // "High Wood Block"
      "Woodblk L", // "Low Wood Block"
      "Cuica Mt.", // "Mute Cuíca"
      "Cuica Op.", // "Open Cuíca"
      "Tri. Mt.",  // "Mute Triangle"
      "Tri. Op."   // "Open Triangle"
};

static const char* avldrums[] = {
	"KickDrum", // 36
	"SnareSide",
	"Snare",
	"HandClap",
	"SnareEdge",
	"Floor Tom",
	"HH Closed",
	"FTom Edge",
	"HH Pedal",
	"TomCtr",
	"HH Semi",
	"Tom Edge",
	"HH Swish",
	"Crash 1",
	"Crash 1Ck",
	"Ride Tip",
	"Ride Chk",
	"Ride Bell",
	"Tambour.",
	"Splash",
	"Cowbell",
	"Crash 2",
	"Crash 2Ck",
	"Ride Shnk",
	"Crash 3",
	"Maracas" // 61
};

static const char* notename[] = {
	"C", "Db", "D", "Eb", "E", "F", "Gb", "G", "Ab", "A", "Bb", "B"
};

/* factory default mappings, nd2 user manual p.10, nd3 user manual p.12 */
static const char* norddrum[] = {
	"Channel 1", // 60
	notename[61%12],
	"Channel 2",
	notename[63%12],
	"Channel 3",
	"Channel 4",
	notename[66%12],
	"Channel 5",
	notename[68%12],
	"Channel 6" // 69
};

///////////////////////////////////////////////////////////////////////////////

struct MyGimpImage {
	unsigned int   width;
	unsigned int   height;
	unsigned int   bytes_per_pixel;
	unsigned char  pixel_data[];
};

/* load gimp-exported .c image into cairo surface */
static void img2surf (struct MyGimpImage const* img, cairo_surface_t** ds) {
	unsigned int x,y;
	unsigned char* d;
	cairo_surface_t* s;
	int stride = cairo_format_stride_for_width (CAIRO_FORMAT_ARGB32, img->width);

	d = (unsigned char *) malloc (stride * img->height);
	s = cairo_image_surface_create_for_data(d,
			CAIRO_FORMAT_ARGB32, img->width, img->height, stride);

	cairo_surface_flush (s);
	for (y = 0; y < img->height; ++y) {
		const int y0 = y * stride;
		const int ys = y * img->width * img->bytes_per_pixel;
		for (x = 0; x < img->width; ++x) {
			const int xs = x * img->bytes_per_pixel;
			const int xd = x * 4;

			if (img->bytes_per_pixel == 3) {
			d[y0 + xd + 3] = 0xff;
			} else {
			d[y0 + xd + 3] = img->pixel_data[ys + xs + 3]; // A
			}
			d[y0 + xd + 2] = img->pixel_data[ys + xs];     // R
			d[y0 + xd + 1] = img->pixel_data[ys + xs + 1]; // G
			d[y0 + xd + 0] = img->pixel_data[ys + xs + 2]; // B
		}
	}
	cairo_surface_mark_dirty (s);

	*ds = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, img->width, img->height);
	cairo_t *cr = cairo_create (*ds);
	cairo_set_source_surface (cr, s, 0, 0);
	cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
	cairo_paint (cr);
	cairo_destroy (cr);
	free (d);
}

///////////////////////////////////////////////////////////////////////////////

static const float c_dlf[4] = {0.8, 0.8, 0.8, 1.0}; // dial faceplate fg

#include "gui/bpmwheel.h"
#include "gui/divisions.h"

/*** knob faceplates ***/
static void prepare_faceplates (SeqUI* ui)
{
	float w = ui->spn_bpm->w_width * 2.f;
	float h = ui->spn_bpm->w_height * 2.f;
	ui->bpm_bg = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, w, h);
	cairo_t* cr = cairo_create (ui->bpm_bg);
	draw_wheel (cr, w, h);
	cairo_destroy (cr);

	h = ui->spn_swing->w_height;
	ui->swg_bg = cairo_pattern_create_linear (0.0, 0.0, 3.0, h);
	cairo_pattern_add_color_stop_rgba (ui->swg_bg, 0.00, 1.0, 0.6, 0.2, 1.0);
	cairo_pattern_add_color_stop_rgba (ui->swg_bg, 0.33, 1.0, 0.6, 0.2, 1.0);
	cairo_pattern_add_color_stop_rgba (ui->swg_bg, 1.00, 0.5, 0.3, 0.1, 1.0);

	img2surf((struct MyGimpImage const *)&note_image, &ui->div_bg);
}

#if 0
static void display_annotation (SeqUI* ui, RobTkCnob* d, cairo_t* cr, const char* txt) {
	int tw, th;
	cairo_save (cr);
	PangoLayout* pl = pango_cairo_create_layout (cr);
	pango_layout_set_font_description (pl, ui->font[0]);
	pango_layout_set_text (pl, txt, -1);
	pango_layout_get_pixel_size (pl, &tw, &th);
	cairo_translate (cr, d->w_width / 2, d->w_height - 2);
	cairo_translate (cr, -tw / 2.0, -th);
	cairo_set_source_rgba (cr, .0, .0, .0, .7);
	rounded_rectangle (cr, -1, -1, tw+3, th+1, 3);
	cairo_fill (cr);
	CairoSetSouerceRGBA (c_wht);
	pango_cairo_show_layout (cr, pl);
	g_object_unref (pl);
	cairo_restore (cr);
	cairo_new_path (cr);
}

static void cnob_annotation_bpm (RobTkCnob* d, cairo_t* cr, void* data) {
	SeqUI* ui = (SeqUI*)data;
	char txt[16];
	snprintf (txt, 16, "%5.1f BPM", d->cur);
	display_annotation (ui, d, cr, txt);
}
#endif
///////////////////////////////////////////////////////////////////////////////

static void draw_swing_text (SeqUI* ui, cairo_t* cr, const char* txt) {
	int tw, th;
	float c_fg[4]; get_color_from_theme(0, c_fg);
	PangoLayout* pl = pango_cairo_create_layout (cr);
	pango_layout_set_font_description (pl, ui->font[0]);
	cairo_save (cr);
	CairoSetSouerceRGBA(c_fg);
	pango_layout_set_text (pl, txt, -1);
	pango_layout_get_pixel_size (pl, &tw, &th);
	cairo_translate (cr, -tw / 2.0, -th / 2.0);
	pango_cairo_layout_path (cr, pl);
	cairo_fill (cr);
	cairo_restore (cr);
	g_object_unref (pl);
}

static void cnob_expose_swing (RobTkCnob* d, cairo_t* cr, void* data) {
	SeqUI* ui = (SeqUI*)data;
	float c_bg[4]; get_color_from_theme(1, c_bg);

	float w = d->w_width;
	float h = d->w_height;

	const float v_min = d->min;
	const float v_max = d->max;
	const float v_cur = d->cur;

	rounded_rectangle (cr, 1.5, 1.5, w - 3, h - 3, 5);
	// TODO gradient
	cairo_set_source_rgba (cr, SHADE_RGB(c_bg, 0.75), 1.0);
	cairo_fill_preserve (cr);
	cairo_set_line_width (cr, 1.0);
	cairo_set_source_rgba (cr, .0, .0, .0, 1.0);
	cairo_stroke_preserve (cr);
	cairo_clip (cr);

	float vh = h * (v_cur - v_min) / (v_max - v_min);
	cairo_rectangle (cr, 0, h - vh, w, vh);
	cairo_set_source (cr, ui->swg_bg);
	cairo_fill (cr);

	for (int r = 2 * C_RAD; r > 0; --r) {
		const float alpha = 0.1 - 0.1 * r / (2 * C_RAD + 1.f);
		cairo_set_line_width (cr, r);
		cairo_set_source_rgba (cr, .0, .0, .0, alpha);
		cairo_move_to(cr, 0, 1.5);
		cairo_rel_line_to(cr, d->w_width, 0);
		cairo_stroke(cr);
		cairo_move_to(cr, 1.5, 0);
		cairo_rel_line_to(cr, 0, d->w_height);
		cairo_stroke(cr);
	}

	cairo_save (cr);
	cairo_translate (cr, w * .5, h * .5);

	if (rint(30 * v_cur) == 0.0) {
		draw_swing_text (ui, cr, "1:1");
	}
	else if (rint(30 * v_cur) == 6.0) {
		draw_swing_text (ui, cr, "3:2");
	}
	else if (rint(30 * v_cur) == 10.0) {
		draw_swing_text (ui, cr, "2:1");
	}
	else if (rint(30 * v_cur) == 15.0) {
		draw_swing_text (ui, cr, "3:1");
	}

	cairo_restore (cr);

	rounded_rectangle (cr, 1.5, 1.5, w - 3, h - 3, 5);
	cairo_set_line_width (cr, 1.0);
	cairo_set_source_rgba (cr, .0, .0, .0, 1.0);
	cairo_stroke (cr);
}

static void cnob_expose_div (RobTkCnob* d, cairo_t* cr, void* data) {
	SeqUI* ui = (SeqUI*)data;
	float c[4]; get_color_from_theme(1, c);

	float w = d->w_width;
	float h = d->w_height;

	rounded_rectangle (cr, 1.5, 1.5, w - 3, h - 3, C_RAD);
	// TODO gradient
	cairo_set_source_rgba (cr, SHADE_RGB(c, 0.75), 1.0);
	cairo_fill_preserve (cr);
	cairo_set_line_width (cr, 1.0);
	cairo_set_source_rgba (cr, .0, .0, .0, 1.0);
	cairo_stroke_preserve (cr);
	cairo_clip (cr);

	for (int r = 2 * C_RAD; r > 0; --r) {
		const float alpha = 0.1 - 0.1 * r / (2 * C_RAD + 1.f);
		cairo_set_line_width (cr, r);
		cairo_set_source_rgba (cr, .0, .0, .0, alpha);
		cairo_move_to(cr, 0, 1.5);
		cairo_rel_line_to(cr, d->w_width, 0);
		cairo_stroke(cr);
		cairo_move_to(cr, 1.5, 0);
		cairo_rel_line_to(cr, 0, d->w_height);
		cairo_stroke(cr);
	}

	cairo_save (cr);
	cairo_scale (cr, 0.5, 0.5);
	cairo_set_operator (cr, CAIRO_OPERATOR_ADD);
	cairo_set_source_surface(cr, ui->div_bg, -60 * rintf(d->cur), 0);
	cairo_paint (cr);
	cairo_restore (cr);
}

static void cnob_expose_bpm (RobTkCnob* d, cairo_t* cr, void* data) {
	SeqUI* ui = (SeqUI*)data;

	PangoLayout* pl = pango_cairo_create_layout (cr);
	pango_layout_set_font_description (pl, ui->font[1]);

	float w = d->w_width;
	float h = d->w_height;

	cairo_save (cr);
	cairo_scale (cr, 0.5, 0.5);
	cairo_set_source_surface(cr, ui->bpm_bg, 0, 0);
	cairo_paint (cr);

	cairo_restore (cr);

	cairo_translate (cr, 0, 1);
	wheel_path (cr, w, h - 2);
	cairo_clip (cr);
	cairo_save (cr);
	cairo_translate (cr, w * .5f, h * .5f);

	const float v_min = d->min;
	const float v_max = d->max;
	const float v_cur = d->cur;

	for (int s = 0; s < 5; ++s) {
			char txt[7];
			float v = v_cur + s - 2;
			if (floor(v) < v_min || floor(v) > v_max) {
				continue;
			}
			sprintf (txt, "%.0f", floor(v));
			float off =  s / (4.f);
			off -= fmodf (v_cur, 1.0) / 4.f;
			if (off <  .05 || off >  .95) {
				continue;
			}
			draw_bpm_value (cr, pl, txt, d->w_height, off);
	}
	cairo_restore (cr);
	g_object_unref (pl);
	// keep clip (for overlay)
}

static void set_note_txt (SeqUI* ui, int n) {
	const int mn = rintf (robtk_select_get_value (ui->sel_note[n]));
	const int dm = rintf (robtk_select_get_value (ui->sel_drum));

	if (dm == 1 && mn >= 35 && mn <= 81) {
		/* general midi */
		robtk_lbl_set_text (ui->lbl_note[n], mdrums[mn-35]);
	} else if (dm == 2 && mn >= 36 && mn <= 61) {
		/* avldrums */
		robtk_lbl_set_text (ui->lbl_note[n], avldrums[mn-36]);
	} else if (dm == 3 && mn >= 60 && mn <= 69) {
		/* nord drum 2 & 3 */
		robtk_lbl_set_text (ui->lbl_note[n], norddrum[mn-60]);
	} else {
		char txt[16];
		sprintf (txt, "%-2s%d ", notename[mn%12], -1 + mn / 12);
		robtk_lbl_set_text (ui->lbl_note[n], txt);
	}
}
///////////////////////////////////////////////////////////////////////////////

/*** knob & button callbacks ****/

static bool cb_mchn (RobWidget* w, void* handle) {
	SeqUI* ui = (SeqUI*)handle;
	if (ui->disable_signals) return TRUE;
	const float val = robtk_select_get_value (ui->sel_mchn);
	ui->write (ui->controller, PORT_CHN, sizeof (float), 0, (const void*) &val);
	return TRUE;
}

static bool cb_div (RobWidget* w, void* handle) {
	SeqUI* ui = (SeqUI*)handle;
	if (ui->disable_signals) return TRUE;
	const float val = robtk_cnob_get_value (ui->spn_div);
	ui->write (ui->controller, PORT_DIVIDER, sizeof (float), 0, (const void*) &val);
	return TRUE;
}

static bool cb_bpm (RobWidget* w, void* handle) {
	char txt[31];
	SeqUI* ui = (SeqUI*)handle;
	if (ui->disable_signals) return TRUE;
	const float val = robtk_cnob_get_value (ui->spn_bpm);
	ui->user_bpm = val;
	ui->write (ui->controller, PORT_BPM, sizeof (float), 0, (const void*) &val);
	snprintf(txt, 31, "%.1f BPM", val);
	robtk_lbl_set_text (ui->lbl_bpm, txt);
	return TRUE;
}

static bool cb_swing (RobWidget* w, void* handle) {
	SeqUI* ui = (SeqUI*)handle;
	if (ui->disable_signals) return TRUE;
	const float val = robtk_cnob_get_value (ui->spn_swing);
	ui->write (ui->controller, PORT_SWING, sizeof (float), 0, (const void*) &val);
	return TRUE;
}

static bool cb_note (RobWidget* w, void* handle) {
	SeqUI* ui = (SeqUI*)handle;
	int n;
	memcpy (&n, w->name, sizeof(int)); // hack alert
	if (ui->disable_signals) return TRUE;
	const float val = robtk_select_get_value (ui->sel_note[n]);
	ui->write (ui->controller, PORT_NOTES + n, sizeof (float), 0, (const void*) &val);
	set_note_txt (ui, n);
	return TRUE;
}

static bool cb_sync (RobWidget* w, void* handle) {
	SeqUI* ui = (SeqUI*)handle;
	if (ui->disable_signals) return TRUE;
	const float val = robtk_cbtn_get_active (ui->btn_sync) ? 1 : 0;
	ui->write (ui->controller, PORT_SYNC, sizeof (float), 0, (const void*) &val);
	return TRUE;
}

static bool cb_drum (RobWidget* w, void* handle) {
	SeqUI* ui = (SeqUI*)handle;
	for (int n = 0; n < N_NOTES; ++n) {
		set_note_txt (ui, n);
	}
	if (ui->disable_signals) return TRUE;
	const float val = robtk_select_get_value (ui->sel_drum);
	ui->write (ui->controller, PORT_DRUM, sizeof (float), 0, (const void*) &val);
	return TRUE;
}

static bool cb_grid (RobWidget* w, void* handle) {
	SeqUI* ui = (SeqUI*)handle;
	int g;
	memcpy (&g, w->name, sizeof(int)); // hack alert
	if (ui->disable_signals) return TRUE;
	const float val = robtk_vbtn_get_value (ui->btn_grid[g]);
	ui->write (ui->controller, PORT_NOTES + N_NOTES + g, sizeof (float), 0, (const void*) &val);
	return TRUE;
}

static bool cb_btn_panic_press (RobWidget *w, void* handle) {
	SeqUI* ui = (SeqUI*)handle;
	float val = 1;
	ui->write (ui->controller, PORT_PANIC, sizeof (float), 0, (const void*) &val);
	return TRUE;
}

static bool cb_btn_panic_release (RobWidget *w, void* handle) {
	SeqUI* ui = (SeqUI*)handle;
	float val = 0;
	ui->write (ui->controller, PORT_PANIC, sizeof (float), 0, (const void*) &val);
	return TRUE;
}

static bool cb_btn_reset (RobWidget *w, void* handle) {
	SeqUI* ui = (SeqUI*)handle;
	if (ui->disable_signals) return TRUE;
	int g;
	memcpy (&g, w->name, sizeof(int)); // hack alert
	if (g < N_NOTES) {
		// row 'g'
		for (uint32_t p = 0; p < N_STEPS; ++p) {
			uint32_t po = N_NOTES * g + p;
			robtk_vbtn_set_value (ui->btn_grid[po], 0);
		}
	} else if (g < N_NOTES + N_STEPS) {
		// column 'g'
		for (uint32_t p = 0; p < N_NOTES; ++p) {
			uint32_t po = (g - N_NOTES) + N_STEPS * p;
			robtk_vbtn_set_value (ui->btn_grid[po], 0);
		}
	} else {
		for (uint32_t p = 0; p < N_NOTES * N_STEPS; ++p) {
			robtk_vbtn_set_value (ui->btn_grid[p], 0);
		}
	}
	return TRUE;
}

///////////////////////////////////////////////////////////////////////////////


static RobWidget* toplevel (SeqUI* ui, void* const top) {
	/* main widget: layout */
	ui->rw = rob_hbox_new (FALSE, 2);
	robwidget_make_toplevel (ui->rw, top);
	robwidget_toplevel_enable_scaling (ui->rw);

	ui->font[0] = pango_font_description_from_string ("Sans 12px");
	ui->font[1] = pango_font_description_from_string ("Sans 17px");

	ui->ctbl = rob_table_new (/*rows*/N_NOTES + 2, /*cols*/ N_STEPS + 2, FALSE);

#define GSL_W(PTR) robtk_select_widget (PTR)

	for (uint32_t n = 0; n < N_NOTES; ++n) {
		ui->lbl_note[n] = robtk_lbl_new ("##|G#-8|#");
		robtk_lbl_set_min_geometry (ui->lbl_note[n], ui->lbl_note[n]->w_width, ui->lbl_note[n]->w_height);
		robtk_lbl_set_alignment (ui->lbl_note[n], .5, 0);
		ui->sel_note[n] = robtk_select_new ();
			for (uint32_t mn = 0; mn < 128; ++mn) {
				char txt[8];
				sprintf (txt, "%d", mn);
				robtk_select_add_item (ui->sel_note[n], mn, txt);
				// hack alert -- should add a .data field to Robwidget
				memcpy (ui->sel_note[n]->rw->name, &n, sizeof(int));
			}
			robtk_select_set_callback (ui->sel_note[n], cb_note, ui);

			rob_table_attach (ui->ctbl, GSL_W (ui->sel_note[n]), 0, 1, 2*n, 2*n + 1, 2, 0, RTK_SHRINK, RTK_SHRINK);
			rob_table_attach (ui->ctbl, robtk_lbl_widget (ui->lbl_note[n]), 0, 1, 2*n + 1, 2*n + 2, 2, 0, RTK_SHRINK, RTK_EXANDF);

		for (uint32_t s = 0; s < N_STEPS; ++s) {
			uint32_t g = n * N_STEPS + s;
			ui->btn_grid[g] = robtk_vbtn_new ();
			rob_table_attach (ui->ctbl, robtk_vbtn_widget (ui->btn_grid[g]), 1 + s, 2 + s, 2*n, 2*n + 2, 0, 0, RTK_SHRINK, RTK_SHRINK);
			// hack alert -- should add a .data field to Robwidget
			memcpy (ui->btn_grid[g]->rw->name, &g, sizeof(int));
			robtk_vbtn_set_callback (ui->btn_grid[g], cb_grid, ui);
		}

		ui->btn_clear[n] = robtk_pbtn_new ("C");
		robtk_pbtn_set_callback_up (ui->btn_clear[n], cb_btn_reset, ui);
		robtk_pbtn_set_alignment (ui->btn_clear[n], .5, .5);
		rob_table_attach (ui->ctbl, robtk_pbtn_widget (ui->btn_clear[n]), N_STEPS + 1, N_STEPS + 2, 2*n, 2*n + 2, 2, 2, RTK_EXANDF, RTK_SHRINK);
		// hack alert -- should add a .data field to Robwidget
		memcpy (ui->btn_clear[n]->rw->name, &n, sizeof(int));
	}

	for (uint32_t s = 0; s <= N_STEPS; ++s) {
		int n = N_NOTES + s;
		ui->btn_clear[n] = robtk_pbtn_new ("C");
		robtk_pbtn_set_callback_up (ui->btn_clear[n], cb_btn_reset, ui);
		robtk_pbtn_set_alignment (ui->btn_clear[n], .5, .5);
		rob_table_attach (ui->ctbl, robtk_pbtn_widget (ui->btn_clear[n]), s + 1, s + 2, 2 * N_NOTES, 2 * N_NOTES + 1, 2, 2, RTK_SHRINK, RTK_SHRINK);
		// hack alert -- should add a .data field to Robwidget
		memcpy (ui->btn_clear[n]->rw->name, &n, sizeof(int));
	}

	ui->sep_h0   = robtk_sep_new(TRUE);
	rob_table_attach (ui->ctbl, robtk_sep_widget(ui->sep_h0), 0, N_STEPS + 2, 2 * N_NOTES + 1, 2 * N_NOTES + 2, 4, 8, RTK_EXANDF, RTK_SHRINK);

	int cr = 2 + 2 * N_NOTES;

	/* sync */
	ui->btn_sync = robtk_cbtn_new ("Host Sync", GBT_LED_LEFT, false);
	robtk_cbtn_set_callback (ui->btn_sync, cb_sync, ui);

	/* sync */
	ui->sel_drum = robtk_select_new ();
	robtk_select_add_item (ui->sel_drum, 0, "Chromatic");
	robtk_select_add_item (ui->sel_drum, 1, "GM/GS Drums");
	robtk_select_add_item (ui->sel_drum, 2, "AVL Drums");
	robtk_select_add_item (ui->sel_drum, 3, "Nord Drum 2/3");
	robtk_select_set_default_item (ui->sel_drum, 0);
	robtk_select_set_callback (ui->sel_drum, cb_drum, ui);

	/* beat divicer */
	ui->spn_div = robtk_cnob_new (0, 9, 1, 30, 42);
	robtk_cnob_set_callback (ui->spn_div, cb_div, ui);
	robtk_cnob_set_value (ui->spn_div, 3.f);
	robtk_cnob_set_default (ui->spn_div, 3.f);
	robtk_cnob_expose_callback (ui->spn_div, cnob_expose_div, ui);
	robtk_cnob_set_scroll_mult (ui->spn_div, 1.0);

	/* bpm */
	ui->spn_bpm = robtk_cnob_new (40, 208, 0.1, 64, 44);
	robtk_cnob_set_callback (ui->spn_bpm, cb_bpm, ui);
	robtk_cnob_set_value (ui->spn_bpm, 120.f);
	robtk_cnob_set_default (ui->spn_bpm, 120.f);
	robtk_cnob_expose_callback (ui->spn_bpm, cnob_expose_bpm, ui);
	robtk_cnob_set_scroll_mult (ui->spn_bpm, 10.0);
	ui->user_bpm = 120;

	/* schwing */
	ui->spn_swing = robtk_cnob_new (0, 0.5, 1.f / 30.f, 30, 42);
	robtk_cnob_set_callback (ui->spn_swing, cb_swing, ui);
	robtk_cnob_set_value (ui->spn_swing, 0.f);
	robtk_cnob_set_default (ui->spn_swing, 0.f);
	robtk_cnob_expose_callback (ui->spn_swing, cnob_expose_swing, ui);
	robtk_cnob_set_scroll_mult (ui->spn_swing, 1.0);
	//float swing_detents[4] =  {0, 0.2, 1.0 / 3.0, 0.5};
	//robtk_cnob_set_detents (ui->spn_swing, 4, swing_detents);

	/* midi channel */
	ui->sel_mchn = robtk_select_new ();
	for (int mc = 0; mc < 16; ++mc) {
		char buf[8];
		snprintf (buf, 8, "%d", mc + 1);
		robtk_select_add_item (ui->sel_mchn, mc, buf);
	}
	robtk_select_set_default_item (ui->sel_mchn, 0);
	robtk_select_set_callback (ui->sel_mchn, cb_mchn, ui);

	/* panic */
	ui->btn_panic = robtk_pbtn_new ("Panic");
	robtk_pbtn_set_callback_down (ui->btn_panic, cb_btn_panic_press, ui);
	robtk_pbtn_set_callback_up (ui->btn_panic, cb_btn_panic_release, ui);
	robtk_pbtn_set_alignment(ui->btn_panic, 0.5, 0.5);

	/* labels */
	ui->lbl_chn  = robtk_lbl_new ("Midi Chn.");
	ui->lbl_div  = robtk_lbl_new ("Step");
	ui->lbl_bpm  = robtk_lbl_new ("888.8 BPM");
	ui->lbl_swg  = robtk_lbl_new ("Swing");

	/* Layout */

	rob_table_attach (ui->ctbl, robtk_cbtn_widget (ui->btn_sync),  0,  2, cr + 0, cr + 1, 2, 0, RTK_EXANDF, RTK_SHRINK);
	rob_table_attach (ui->ctbl, GSL_W (ui->sel_drum),              0,  2, cr + 1, cr + 2, 2, 0, RTK_EXANDF, RTK_SHRINK);

	rob_table_attach (ui->ctbl, robtk_cnob_widget (ui->spn_div),   3,  4, cr + 0, cr + 2, 0, 0, RTK_EXANDF, RTK_SHRINK);
	rob_table_attach (ui->ctbl, robtk_cnob_widget (ui->spn_bpm),   4,  6, cr + 0, cr + 2, 0, 0, RTK_SHRINK, RTK_SHRINK);
	rob_table_attach (ui->ctbl, robtk_cnob_widget (ui->spn_swing), 6,  7, cr + 0, cr + 2, 0, 0, RTK_EXANDF, RTK_SHRINK);

	rob_table_attach (ui->ctbl, robtk_lbl_widget (ui->lbl_div),    3,  4, cr + 2, cr + 3, 0, 0, RTK_EXANDF, RTK_SHRINK);
	rob_table_attach (ui->ctbl, robtk_lbl_widget (ui->lbl_bpm),    4,  6, cr + 2, cr + 3, 0, 0, RTK_EXANDF, RTK_SHRINK);
	rob_table_attach (ui->ctbl, robtk_lbl_widget (ui->lbl_swg),    6,  7, cr + 2, cr + 3, 0, 0, RTK_EXANDF, RTK_SHRINK);

	rob_table_attach (ui->ctbl, robtk_pbtn_widget (ui->btn_panic), 8, 10, cr + 0, cr + 1, 2, 0, RTK_EXANDF, RTK_SHRINK);
	rob_table_attach (ui->ctbl, GSL_W (ui->sel_mchn),              8, 10, cr + 1, cr + 2, 2, 0, RTK_EXANDF, RTK_SHRINK);
	rob_table_attach (ui->ctbl, robtk_lbl_widget (ui->lbl_chn),    8, 10, cr + 2, cr + 3, 0, 0, RTK_EXANDF, RTK_SHRINK);

	/* top-level packing */
	rob_hbox_child_pack (ui->rw, ui->ctbl, FALSE, TRUE);

	prepare_faceplates (ui);

	return ui->rw;
}

static void gui_cleanup (SeqUI* ui) {
	pango_font_description_free (ui->font[0]);
	pango_font_description_free (ui->font[1]);

	for (uint32_t n = 0; n < N_NOTES; ++n) {
		robtk_select_destroy (ui->sel_note[n]);
		robtk_lbl_destroy (ui->lbl_note[n]);
		for (uint32_t s = 0; s < N_STEPS; ++s) {
			uint32_t g = n * N_STEPS + s;
			robtk_vbtn_destroy (ui->btn_grid[g]);
		}
	}

	for (uint32_t p = 0; p < N_NOTES + N_STEPS + 1; ++p) {
		robtk_pbtn_destroy (ui->btn_clear[p]);
	}

	robtk_cbtn_destroy (ui->btn_sync);
	robtk_select_destroy (ui->sel_drum);
	robtk_select_destroy (ui->sel_mchn);
	robtk_cnob_destroy (ui->spn_div);
	robtk_cnob_destroy (ui->spn_bpm);
	robtk_cnob_destroy (ui->spn_swing);
	robtk_pbtn_destroy (ui->btn_panic);
	robtk_sep_destroy (ui->sep_h0);
	robtk_lbl_destroy (ui->lbl_chn);
	robtk_lbl_destroy (ui->lbl_div);
	robtk_lbl_destroy (ui->lbl_bpm);
	robtk_lbl_destroy (ui->lbl_swg);

	cairo_surface_destroy (ui->bpm_bg);
	cairo_pattern_destroy (ui->swg_bg);
	cairo_surface_destroy (ui->div_bg);

	rob_table_destroy (ui->ctbl);
	rob_box_destroy (ui->rw);
}

/******************************************************************************
 * RobTk + LV2
 */

#define LVGL_RESIZEABLE

static void ui_enable (LV2UI_Handle handle) { }
static void ui_disable (LV2UI_Handle handle) { }

static enum LVGLResize
plugin_scale_mode (LV2UI_Handle handle)
{
	return LVGL_LAYOUT_TO_FIT;
}

static LV2UI_Handle
instantiate (
		void* const               ui_toplevel,
		const LV2UI_Descriptor*   descriptor,
		const char*               plugin_uri,
		const char*               bundle_path,
		LV2UI_Write_Function      write_function,
		LV2UI_Controller          controller,
		RobWidget**               widget,
		const LV2_Feature* const* features)
{
	SeqUI* ui = (SeqUI*) calloc (1, sizeof (SeqUI));
	if (!ui) {
		return NULL;
	}

	if (strcmp (plugin_uri, RTK_URI)) {
		free (ui);
		return NULL;
	}

	ui->nfo        = robtk_info (ui_toplevel);
	ui->write      = write_function;
	ui->controller = controller;

	ui->disable_signals = true;
	*widget = toplevel (ui, ui_toplevel);
	ui->disable_signals = false;
	return ui;
}

static void
cleanup (LV2UI_Handle handle)
{
	SeqUI* ui = (SeqUI*)handle;
	gui_cleanup (ui);
	free (ui);
}

/* receive information from DSP */
static void
port_event (LV2UI_Handle handle,
		uint32_t     port_index,
		uint32_t     buffer_size,
		uint32_t     format,
		const void*  buffer)
{
	SeqUI* ui = (SeqUI*)handle;
	if (format != 0 || port_index <= PORT_MIDI_OUT) return;

	const float v = *(float*)buffer;
	ui->disable_signals = true;

	switch (port_index) {
		case PORT_SYNC:
			robtk_cbtn_set_active (ui->btn_sync, v > 0);
			break;
		case PORT_BPM:
			ui->user_bpm = v;
			if (ui->spn_bpm->sensitive) {
				char txt[31];
				snprintf(txt, 31, "%.1f BPM", v);
				robtk_lbl_set_text (ui->lbl_bpm, txt);
				robtk_cnob_set_value (ui->spn_bpm, v); // TODO check sensitivity -- PORT_HOSTBPM
			}
			break;
		case PORT_HOSTBPM:
			if (v <= 0) {
				robtk_cnob_set_sensitive (ui->spn_bpm, true);
				// restore user-set BPM (display)
				port_event (handle, PORT_BPM, buffer_size, 0, &ui->user_bpm);
			} else {
				char txt[31];
				robtk_cnob_set_sensitive (ui->spn_bpm, false);
				robtk_cnob_set_value (ui->spn_bpm, v);
				snprintf(txt, 31, "%.1f BPM", v);
				robtk_lbl_set_text (ui->lbl_bpm, txt);
			}
			if (v != 0) {
				// [potential] host sync
				robtk_cbtn_set_color_on (ui->btn_sync, .3, .8, .1);
				robtk_cbtn_set_color_off (ui->btn_sync, .1, .3, .1);
			}
			break;
		case PORT_DIVIDER:
			robtk_cnob_set_value (ui->spn_div, v);
			break;
		case PORT_CHN:
			robtk_select_set_value (ui->sel_mchn, v);
			break;
		case PORT_DRUM:
			robtk_select_set_value (ui->sel_drum, v);
			break;
		case PORT_SWING:
			robtk_cnob_set_value (ui->spn_swing, v);
			break;
		case PORT_PANIC:
			break;
		case PORT_STEP:
			{
				unsigned int step = rintf (v - 1.f);
				for (uint32_t p = 0; p < N_NOTES * N_STEPS; ++p) {
					robtk_vbtn_set_highlight (ui->btn_grid[p], (p % N_STEPS) == step);
				}
			}
			break;
		default:
			if (port_index < PORT_NOTES + N_NOTES) {
				int n = port_index - PORT_NOTES;
				robtk_select_set_item (ui->sel_note[n], rintf(v));
				set_note_txt (ui, n);
			}
			else if (port_index < PORT_NOTES + N_NOTES + N_NOTES * N_STEPS) {
				int g = port_index - PORT_NOTES - N_NOTES;
				robtk_vbtn_set_value (ui->btn_grid[g], v);
			}
			break;
	}

	ui->disable_signals = false;
}

static const void*
extension_data (const char* uri)
{
	return NULL;
}
