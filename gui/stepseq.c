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
	RobTkPBtn*   btn_clear[N_NOTES + N_STEPS + 1];
	RobTkCBtn*   btn_sync;
	RobTkCBtn*   btn_drum;
	RobTkSelect* sel_div;
	RobTkSelect* sel_mchn;
	RobTkCnob*   spn_bpm;
	RobTkCnob*   spn_swing;
	RobTkPBtn*   btn_panic;
	RobTkLbl*    dpy_step;
	RobTkLbl*    lbl_step;
	RobTkLbl*    lbl_chn;

	cairo_pattern_t* swg_bg;
	cairo_surface_t* bpm_bg;

	bool disable_signals;
	const char* nfo;
} SeqUI;

///////////////////////////////////////////////////////////////////////////////

static const float c_dlf[4] = {0.8, 0.8, 0.8, 1.0}; // dial faceplate fg

#include "gui/bpmwheel.h"

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
	PangoLayout* pl = pango_cairo_create_layout (cr);
	pango_layout_set_font_description (pl, ui->font[0]);
	cairo_save (cr);
	cairo_set_source_rgba (cr, 0.0, 0.0, 0.0, 1.0);
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

	float w = d->w_width;
	float h = d->w_height;

	const float v_min = d->min;
	const float v_max = d->max;
	const float v_cur = d->cur;

	rounded_rectangle (cr, 1.5, 1.5, w - 3, h - 3, 5);
	cairo_set_line_width (cr, 1.0);
	cairo_set_source_rgba (cr, .0, .0, .0, 1.0);
	cairo_stroke_preserve (cr);
	cairo_clip (cr);

	float vh = h * (v_cur - v_min) / (v_max - v_min);
	cairo_rectangle (cr, 0, h - vh, w, vh);
	cairo_set_source (cr, ui->swg_bg);
	cairo_fill (cr);

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
	const float val = robtk_select_get_item (ui->sel_div);
	ui->write (ui->controller, PORT_DIVIDER, sizeof (float), 0, (const void*) &val);
	return TRUE;
}

static bool cb_bpm (RobWidget* w, void* handle) {
	SeqUI* ui = (SeqUI*)handle;
	if (ui->disable_signals) return TRUE;
	const float val = robtk_cnob_get_value (ui->spn_bpm);
	ui->write (ui->controller, PORT_BPM, sizeof (float), 0, (const void*) &val);
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
	if (ui->disable_signals) return TRUE;
	const float val = robtk_cbtn_get_active (ui->btn_drum) ? 1 : 0;
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

static bool cb_btn_panic_on (RobWidget *w, void* handle) {
	SeqUI* ui = (SeqUI*)handle;
	float val = 1.f;
	ui->write (ui->controller, PORT_PANIC, sizeof (float), 0, (const void*) &val);
	return TRUE;
}

static bool cb_btn_panic_off (RobWidget *w, void* handle) {
	SeqUI* ui = (SeqUI*)handle;
	float val = 0.0;
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
		ui->sel_note[n] = robtk_select_new ();
			for (uint32_t mn = 0; mn < 128; ++mn) {
				char txt[8];
				sprintf (txt, "%d", mn);
				robtk_select_add_item (ui->sel_note[n], mn, txt);
				// hack alert -- should add a .data field to Robwidget
				memcpy (ui->sel_note[n]->rw->name, &n, sizeof(int));
			}

			rob_table_attach (ui->ctbl, GSL_W (ui->sel_note[n]), 0, 1, n, n + 1, 2, 2, RTK_SHRINK, RTK_SHRINK);
			robtk_select_set_callback (ui->sel_note[n], cb_note, ui);

		for (uint32_t s = 0; s < N_STEPS; ++s) {
			uint g = n * N_STEPS + s;
			ui->btn_grid[g] = robtk_vbtn_new ();
			rob_table_attach (ui->ctbl, robtk_vbtn_widget (ui->btn_grid[g]), 1 + s, 2 + s, n, n + 1, 0, 0, RTK_SHRINK, RTK_SHRINK);
			// hack alert -- should add a .data field to Robwidget
			memcpy (ui->btn_grid[g]->rw->name, &g, sizeof(int));
			robtk_vbtn_set_callback (ui->btn_grid[g], cb_grid, ui);
		}

		ui->btn_clear[n] = robtk_pbtn_new ("C");
		robtk_pbtn_set_callback_up (ui->btn_clear[n], cb_btn_reset, ui);
		robtk_pbtn_set_alignment (ui->btn_clear[n], .5, .5);
		rob_table_attach (ui->ctbl, robtk_pbtn_widget (ui->btn_clear[n]), N_STEPS + 1, N_STEPS + 2, n, n + 1, 2, 2, RTK_EXANDF, RTK_SHRINK);
		// hack alert -- should add a .data field to Robwidget
		memcpy (ui->btn_clear[n]->rw->name, &n, sizeof(int));
	}

	for (uint32_t s = 0; s <= N_STEPS; ++s) {
		int n = N_NOTES + s;
		ui->btn_clear[n] = robtk_pbtn_new ("C");
		robtk_pbtn_set_callback_up (ui->btn_clear[n], cb_btn_reset, ui);
		robtk_pbtn_set_alignment (ui->btn_clear[n], .5, .5);
		rob_table_attach (ui->ctbl, robtk_pbtn_widget (ui->btn_clear[n]), s + 1, s + 2, N_NOTES, N_NOTES + 1, 2, 2, RTK_SHRINK, RTK_SHRINK);
		// hack alert -- should add a .data field to Robwidget
		memcpy (ui->btn_clear[n]->rw->name, &n, sizeof(int));
	}


	int cr = 2 + N_NOTES;

	/* sync */
	ui->btn_sync = robtk_cbtn_new ("Sync", GBT_LED_LEFT, false);
	robtk_cbtn_set_callback (ui->btn_sync, cb_sync, ui);

	/* sync */
	ui->btn_drum = robtk_cbtn_new ("Drum Mode", GBT_LED_LEFT, false);
	robtk_cbtn_set_callback (ui->btn_drum, cb_drum, ui);

	/* beat divicer */
	ui->sel_div = robtk_select_new ();
	robtk_select_add_item (ui->sel_div, 0, "32th");
	robtk_select_add_item (ui->sel_div, 1, "16th");
	robtk_select_add_item (ui->sel_div, 2, "8th");
	robtk_select_add_item (ui->sel_div, 3, "Quarter");
	robtk_select_add_item (ui->sel_div, 4, "Half");
	robtk_select_add_item (ui->sel_div, 5, "1 Bar");
	robtk_select_add_item (ui->sel_div, 6, "1.5 Bars");
	robtk_select_add_item (ui->sel_div, 7, "2 Bars");
	robtk_select_add_item (ui->sel_div, 8, "3 Bars");
	robtk_select_add_item (ui->sel_div, 9, "4 Bars");
	robtk_select_set_default_item (ui->sel_div, 3);
	robtk_select_set_callback (ui->sel_div, cb_div, ui);

	/* bpm */
	ui->spn_bpm = robtk_cnob_new (40, 208, 0.2, 64, 44);
	robtk_cnob_set_callback (ui->spn_bpm, cb_bpm, ui);
	robtk_cnob_set_value (ui->spn_bpm, 120.f);
	robtk_cnob_set_default (ui->spn_bpm, 120.f);
	robtk_cnob_expose_callback (ui->spn_bpm, cnob_expose_bpm, ui);
	robtk_cnob_set_scroll_mult (ui->spn_bpm, 5.0);

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
	robtk_pbtn_set_callback_up (ui->btn_panic, cb_btn_panic_off, ui);
	robtk_pbtn_set_callback_down (ui->btn_panic, cb_btn_panic_on, ui);
	robtk_pbtn_set_alignment (ui->btn_panic, .5, .5);

	ui->dpy_step = robtk_lbl_new ("");
	ui->lbl_step = robtk_lbl_new ("Step:");
	ui->lbl_chn  = robtk_lbl_new ("Chn:");
	robtk_lbl_set_alignment (ui->lbl_step, .9, .5);

	rob_table_attach (ui->ctbl, robtk_cbtn_widget (ui->btn_sync),  0, 1, cr + 0, cr + 1, 2, 2, RTK_EXANDF, RTK_SHRINK);
	rob_table_attach (ui->ctbl, robtk_pbtn_widget (ui->btn_panic), 0, 1, cr + 1, cr + 2, 2, 2, RTK_EXANDF, RTK_SHRINK);

	rob_table_attach (ui->ctbl, robtk_lbl_widget (ui->lbl_chn), 1, 2, cr + 1, cr + 2, 0, 0, RTK_EXANDF, RTK_SHRINK);
	rob_table_attach (ui->ctbl, GSL_W (ui->sel_div),  1, 4, cr + 0, cr + 1, 0, 0, RTK_EXANDF, RTK_SHRINK);
	rob_table_attach (ui->ctbl, GSL_W (ui->sel_mchn), 2, 4, cr + 1, cr + 2, 0, 0, RTK_EXANDF, RTK_SHRINK);
	rob_table_attach (ui->ctbl, robtk_cnob_widget (ui->spn_bpm), 4, 6, cr + 0, cr + 2, 0, 0, RTK_SHRINK, RTK_SHRINK);
	rob_table_attach (ui->ctbl, robtk_cnob_widget (ui->spn_swing), 6, 7, cr + 0, cr + 2, 0, 0, RTK_EXANDF, RTK_SHRINK);

	rob_table_attach (ui->ctbl, robtk_cbtn_widget (ui->btn_drum), 7, 10, cr + 0, cr + 1, 2, 2, RTK_EXANDF, RTK_SHRINK);
	rob_table_attach (ui->ctbl, robtk_lbl_widget (ui->lbl_step), 7, 9, cr + 1, cr + 2, 0, 0, RTK_EXANDF, RTK_SHRINK);
	rob_table_attach (ui->ctbl, robtk_lbl_widget (ui->dpy_step), 9, 10, cr + 1, cr + 2, 0, 0, RTK_EXANDF, RTK_SHRINK);

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
		for (uint32_t s = 0; s < N_STEPS; ++s) {
			uint g = n * N_STEPS + s;
			robtk_vbtn_destroy (ui->btn_grid[g]);
		}
	}

	for (uint32_t p = 0; p < N_NOTES + N_STEPS + 1; ++p) {
		robtk_pbtn_destroy (ui->btn_clear[p]);
	}

	robtk_cbtn_destroy (ui->btn_sync);
	robtk_cbtn_destroy (ui->btn_drum);
	robtk_select_destroy (ui->sel_div);
	robtk_select_destroy (ui->sel_mchn);
	robtk_cnob_destroy (ui->spn_bpm);
	robtk_cnob_destroy (ui->spn_swing);
	robtk_pbtn_destroy (ui->btn_panic);
	robtk_lbl_destroy (ui->dpy_step);
	robtk_lbl_destroy (ui->lbl_step);
	robtk_lbl_destroy (ui->lbl_chn);

	cairo_surface_destroy (ui->bpm_bg);
	cairo_pattern_destroy (ui->swg_bg);

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

	*widget = toplevel (ui, ui_toplevel);
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
			// check sens..
			robtk_cnob_set_value (ui->spn_bpm, v);
			break;
		case PORT_DIVIDER:
			robtk_select_set_item (ui->sel_div, v);
			break;
		case PORT_CHN:
			robtk_select_set_value (ui->sel_mchn, v);
			break;
		case PORT_DRUM:
			robtk_cbtn_set_active (ui->btn_drum, v > 0);
			break;
		case PORT_SWING:
			robtk_cnob_set_value (ui->spn_swing, v);
			break;
		case PORT_PANIC:
			break;
		case PORT_HOSTBPM:
			if (v < 0) {
				robtk_cnob_set_sensitive (ui->spn_bpm, true);
			} else {
				robtk_cnob_set_sensitive (ui->spn_bpm, false);
				robtk_cnob_set_value (ui->spn_bpm, v);
			}
			break;
		case PORT_STEP:
			{
				char txt[16];
				sprintf (txt, "%.0f", v);
				robtk_lbl_set_text (ui->dpy_step, txt);
			}
			break;
		default:
			if (port_index < PORT_NOTES + N_NOTES) {
				int n = port_index - PORT_NOTES;
				robtk_select_set_item (ui->sel_note[n], rintf(v));
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
