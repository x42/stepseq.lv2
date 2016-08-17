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

	RobTkCBtn*   btn_grid[N_NOTES * N_STEPS];
	RobTkSelect* sel_note[N_NOTES];
	RobTkPBtn*   btn_clear[N_NOTES + N_STEPS + 1];
	RobTkCBtn*   btn_sync;
	RobTkSelect* sel_div;
	RobTkDial*   spn_bpm;
	RobTkSelect* sel_mchn;
	RobTkPBtn*   btn_panic;
	RobTkLbl*    lbl_step;

	cairo_surface_t* bpm_bg;

	bool disable_signals;
	const char* nfo;
} SeqUI;

///////////////////////////////////////////////////////////////////////////////

static const float c_dlf[4] = {0.8, 0.8, 0.8, 1.0}; // dial faceplate fg

/*** knob faceplates ***/
static void prepare_faceplates (SeqUI* ui) {
	cairo_t* cr;
	float xlp, ylp;

#define INIT_DIAL_SF(VAR, W, H) \
	VAR = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 2 * (W), 2 * (H)); \
	cr = cairo_create (VAR); \
	cairo_scale (cr, 2.0, 2.0); \
	CairoSetSouerceRGBA(c_trs); \
	cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE); \
	cairo_rectangle (cr, 0, 0, W, H); \
	cairo_fill (cr); \
	cairo_set_operator (cr, CAIRO_OPERATOR_OVER); \

#define DIALDOTS(V, XADD, YADD) \
	float ang = (-.75 * M_PI) + (1.5 * M_PI) * (V); \
	xlp = GED_CX + XADD + sinf (ang) * (GED_RADIUS + 3.0); \
	ylp = GED_CY + YADD - cosf (ang) * (GED_RADIUS + 3.0); \
	cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND); \
	CairoSetSouerceRGBA(c_dlf); \
	cairo_set_line_width(cr, 2.5); \
	cairo_move_to(cr, rint(xlp)-.5, rint(ylp)-.5); \
	cairo_close_path(cr); \
	cairo_stroke(cr);

#define RESPLABLEL(V) \
	{ \
	DIALDOTS(V, 4.5, 15.5) \
	xlp = rint (GED_CX + 4.5 + sinf (ang) * (GED_RADIUS + 9.5)); \
	ylp = rint (GED_CY + 15.5 - cosf (ang) * (GED_RADIUS + 9.5)); \
	}

	INIT_DIAL_SF(ui->bpm_bg, GED_WIDTH + 8, GED_HEIGHT + 20);

	RESPLABLEL(0.00);
	write_text_full(cr,  "40", ui->font[0], xlp+6, ylp, 0, 1, c_dlf);
	RESPLABLEL(.16);
	//write_text_full(cr,  "68", ui->font[0], xlp,   ylp, 0, 1, c_dlf);
	RESPLABLEL(.33);
	//write_text_full(cr,  "96", ui->font[0], xlp,   ylp, 0, 1, c_dlf);
	RESPLABLEL(0.5);
	write_text_full(cr, "124", ui->font[0], xlp,   ylp, 0, 2, c_dlf);
	RESPLABLEL(.66);
	//write_text_full(cr, "152", ui->font[0], xlp,   ylp, 0, 3, c_dlf);
	RESPLABLEL(.83);
	//write_text_full(cr, "180", ui->font[0], xlp,   ylp, 0, 3, c_dlf);
	RESPLABLEL(1.0);
	write_text_full(cr, "208", ui->font[0], xlp-6, ylp, 0, 3, c_dlf);
	cairo_destroy (cr);
}

static void display_annotation (SeqUI* ui, RobTkDial* d, cairo_t* cr, const char* txt) {
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

static void dial_annotation_bpm (RobTkDial* d, cairo_t* cr, void* data) {
	SeqUI* ui = (SeqUI*)data;
	char txt[16];
	snprintf (txt, 16, "%5.1f BPM", d->cur);
	display_annotation (ui, d, cr, txt);
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
	const float val = robtk_dial_get_value (ui->spn_bpm);
	ui->write (ui->controller, PORT_BPM, sizeof (float), 0, (const void*) &val);
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
	const float val = robtk_cbtn_get_active (ui->btn_sync) ? 0 : 1;
	ui->write (ui->controller, PORT_SYNC, sizeof (float), 0, (const void*) &val);
	return TRUE;
}

static bool cb_grid (RobWidget* w, void* handle) {
	SeqUI* ui = (SeqUI*)handle;
	int g;
	memcpy (&g, w->name, sizeof(int)); // hack alert
	if (ui->disable_signals) return TRUE;
	const float val = robtk_cbtn_get_active (ui->btn_grid[g]) ? 127 : 0;
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


///////////////////////////////////////////////////////////////////////////////


static RobWidget* toplevel (SeqUI* ui, void* const top) {
	/* main widget: layout */
	ui->rw = rob_hbox_new (FALSE, 2);
	robwidget_make_toplevel (ui->rw, top);
	robwidget_toplevel_enable_scaling (ui->rw);

	ui->font[0] = pango_font_description_from_string ("Mono 9px");
	ui->font[1] = pango_font_description_from_string ("Mono 10px");

	prepare_faceplates (ui);

	ui->ctbl = rob_table_new (/*rows*/N_NOTES + 2, /*cols*/ N_STEPS + 2, FALSE);

#define GSP_W(PTR) robtk_dial_widget (PTR)
#define GSL_W(PTR) robtk_select_widget (PTR)
#define GCB_W(PTR) robtk_cbtn_widget (PTR)

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
			ui->btn_grid[g] = robtk_cbtn_new ("  ", GBT_LED_OFF, false);
			rob_table_attach (ui->ctbl, GCB_W (ui->btn_grid[g]), 1 + s, 2 + s, n, n + 1, 2, 2, RTK_SHRINK, RTK_SHRINK);
			// hack alert -- should add a .data field to Robwidget
			memcpy (ui->btn_grid[g]->rw->name, &g, sizeof(int));
			robtk_cbtn_set_callback (ui->btn_grid[g], cb_grid, ui);
		}
	}

	int cr = 2 + N_NOTES;

	/* sync */
	ui->btn_sync = robtk_cbtn_new ("Sync", GBT_LED_LEFT, false);
	robtk_cbtn_set_callback (ui->btn_sync, cb_sync, ui);
	rob_table_attach (ui->ctbl, GCB_W (ui->btn_sync), 0, 1, cr, cr + 1, 2, 2, RTK_SHRINK, RTK_SHRINK);

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
	rob_table_attach (ui->ctbl, GSL_W (ui->sel_div), 0, 3, cr + 1, cr + 2, 2, 0, RTK_EXANDF, RTK_SHRINK);

	/* bpm */
	ui->spn_bpm = robtk_dial_new_with_size (40, 208, 1.f / (208.f - 40.f),
			GED_WIDTH + 8, GED_HEIGHT + 20, GED_CX + 4, GED_CY + 15, GED_RADIUS);
	ui->spn_bpm->with_scroll_accel = false;

	robtk_dial_set_callback (ui->spn_bpm, cb_bpm, ui);
	robtk_dial_set_value (ui->spn_bpm, 120.f);
	robtk_dial_set_default (ui->spn_bpm, 120.f);
	robtk_dial_set_scroll_mult (ui->spn_bpm, 5);
	robtk_dial_annotation_callback (ui->spn_bpm, dial_annotation_bpm, ui);
	rob_table_attach (ui->ctbl, GSP_W (ui->spn_bpm), 3, 6, cr + 1, cr + 2, 2, 2, RTK_SHRINK, RTK_SHRINK);
	robtk_dial_set_scaled_surface_scale (ui->spn_bpm, ui->bpm_bg, 2.0);

	/* midi channel */
	ui->sel_mchn = robtk_select_new ();
	for (int mc = 0; mc < 16; ++mc) {
		char buf[8];
		snprintf (buf, 8, "%d", mc + 1);
		robtk_select_add_item (ui->sel_mchn, mc, buf);
	}
	robtk_select_set_default_item (ui->sel_mchn, 0);
	robtk_select_set_callback (ui->sel_mchn, cb_mchn, ui);
	rob_table_attach (ui->ctbl, GSL_W (ui->sel_mchn), 3, 6, cr, cr + 1, 2, 0, RTK_EXANDF, RTK_SHRINK);

	/* panic */
	ui->btn_panic = robtk_pbtn_new ("MIDI Panic");
	robtk_pbtn_set_callback_up (ui->btn_panic, cb_btn_panic_off, ui);
	robtk_pbtn_set_callback_down (ui->btn_panic, cb_btn_panic_on, ui);
	rob_table_attach (ui->ctbl, robtk_pbtn_widget (ui->btn_panic), 6, 9, cr, cr + 1, 2, 0, RTK_EXANDF, RTK_SHRINK);

	ui->lbl_step = robtk_lbl_new ("");
	rob_table_attach (ui->ctbl, robtk_lbl_widget (ui->lbl_step), 6, 9, cr + 1, cr + 2, 2, 0, RTK_EXANDF, RTK_SHRINK);

	/* top-level packing */
	rob_hbox_child_pack (ui->rw, ui->ctbl, FALSE, TRUE);
	return ui->rw;
}

static void gui_cleanup (SeqUI* ui) {
	pango_font_description_free (ui->font[0]);
	pango_font_description_free (ui->font[1]);

	for (uint32_t n = 0; n < N_NOTES; ++n) {
		robtk_select_destroy (ui->sel_note[n]);
		for (uint32_t s = 0; s < N_STEPS; ++s) {
			uint g = n * N_STEPS + s;
			robtk_cbtn_destroy (ui->btn_grid[g]);
		}
	}

	robtk_select_destroy (ui->sel_mchn);
	robtk_select_destroy (ui->sel_div);
	robtk_pbtn_destroy (ui->btn_panic);
	robtk_dial_destroy (ui->spn_bpm);
	robtk_lbl_destroy (ui->lbl_step);

	cairo_surface_destroy (ui->bpm_bg);

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
			robtk_dial_set_value (ui->spn_bpm, v);
			break;
		case PORT_DIVIDER:
			robtk_select_set_item (ui->sel_div, v);
			break;
		case PORT_CHN:
			robtk_select_set_value (ui->sel_mchn, v);
			break;
		case PORT_PANIC:
			break;
		case PORT_STEP:
			{
				char txt[16];
				sprintf (txt, "%.0f", v);
				robtk_lbl_set_text (ui->lbl_step, txt);
			}
			break;
		default:
			if (port_index < PORT_NOTES + N_NOTES) {
				int n = port_index - PORT_NOTES;
				robtk_select_set_item (ui->sel_note[n], rintf(v));
			}
			else if (port_index < PORT_NOTES + N_NOTES + N_NOTES * N_STEPS) {
				int g = port_index - PORT_NOTES - N_NOTES;
				robtk_cbtn_set_active (ui->btn_grid[g], v > 0);
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
