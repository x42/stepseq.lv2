/* velocity dial + button widget
 *
 * Copyright (C) 2013-2016 Robin Gareus <robin@gareus.org>
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
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef _ROB_TK_VBTN_H_
#define _ROB_TK_VBTN_H_

typedef struct {
	RobWidget* rw;

	bool prelight;

	bool (*cb) (RobWidget* w, void* handle);
	void* handle;

	cairo_pattern_t* btn_enabled;
	cairo_pattern_t* btn_inactive;
	cairo_surface_t* sf_txt;

	char *txt;
	pthread_mutex_t _mutex;

	float scale;

	float cur;
	float drag_x, drag_y, drag_c;
	bool dragging;
	bool clicking;

	float w_width, w_height, l_width, l_height;
} RobTkVBtn;

/******************************************************************************
 * some helpers
 */

static void create_vbtn_pattern(RobTkVBtn * d) {
	float c_bg[4]; get_color_from_theme(1, c_bg);

	if (d->btn_inactive) cairo_pattern_destroy(d->btn_inactive);
	if (d->btn_enabled) cairo_pattern_destroy(d->btn_enabled);

	d->btn_inactive = cairo_pattern_create_linear (0.0, 0.0, d->w_width, d->w_height);
	cairo_pattern_add_color_stop_rgb (d->btn_inactive, 0.00, SHADE_RGB(c_bg, 0.80));
	cairo_pattern_add_color_stop_rgb (d->btn_inactive, 1.00, SHADE_RGB(c_bg, 0.60));

	d->btn_enabled = cairo_pattern_create_linear (0.0, 0.0, d->w_width, d->w_height);
	cairo_pattern_add_color_stop_rgb (d->btn_enabled, 0.00, SHADE_RGB(c_bg, 1.95));
	cairo_pattern_add_color_stop_rgb (d->btn_enabled, 1.00, SHADE_RGB(c_bg, 1.85));
}

static void create_vbtn_text_surface (RobTkVBtn* d) {
	float c_col[4];
	get_color_from_theme(0, c_col);
	pthread_mutex_lock (&d->_mutex);
	PangoFontDescription *font = get_font_from_theme();
	d->scale = d->rw->widget_scale;

	char txt[8];
	snprintf (txt, 8, "%.0f", d->cur);

	create_text_surface3 (&d->sf_txt,
			ceil(d->w_width * d->rw->widget_scale),
			ceil(d->w_height * d->rw->widget_scale),
			floor (d->rw->widget_scale * (d->w_width / 2.0)),
			floor (d->rw->widget_scale * d->w_height / 2.0),
			txt, font, c_col, d->rw->widget_scale);

	pango_font_description_free(font);
	pthread_mutex_unlock (&d->_mutex);
}


static void robtk_vbtn_update_value(RobTkVBtn * d, float val) {
	val = rintf (val);
	if (val <= 0) { val = 0;}
	if (val >= 127) { val = 127;}
	if (val == d->cur) {
		return;
	}
	d->cur = val;
	if (d->cb) d->cb(d->rw, d->handle);
	create_vbtn_text_surface (d);
	queue_draw(d->rw);
}

/******************************************************************************
 * main drawing callback
 */

static bool robtk_vbtn_expose_event(RobWidget* handle, cairo_t* cr, cairo_rectangle_t* ev) {
	RobTkVBtn * d = (RobTkVBtn *)GET_HANDLE(handle);

	if (d->scale != d->rw->widget_scale) {
		create_vbtn_text_surface(d);
	}
	if (pthread_mutex_trylock (&d->_mutex)) {
		queue_draw(d->rw);
		return TRUE;
	}

	cairo_rectangle (cr, ev->x, ev->y, ev->width, ev->height);
	cairo_clip (cr);

	cairo_scale (cr, d->rw->widget_scale, d->rw->widget_scale);

	float c[4];
	get_color_from_theme(1, c);

	cairo_set_operator (cr, CAIRO_OPERATOR_OVER);

	rounded_rectangle(cr, 2.5, 2.5, d->w_width - 4, d->w_height - 4, C_RAD);
	if (d->cur > 0) {
		cairo_set_source_rgb(cr, SHADE_RGB(c, 1.95));
		cairo_set_source (cr, d->btn_enabled);
		cairo_fill_preserve (cr);
		float fc = d->cur / 127.f;
		cairo_set_source_rgba (cr, fc, .6 * fc, .2 * fc, .6);
	} else {
		cairo_set_source (cr, d->btn_inactive);
	}
	cairo_fill_preserve (cr);

	cairo_set_line_width (cr, .75);
	cairo_set_source_rgba (cr, .0, .0, .0, 1.0);
	cairo_stroke_preserve (cr);
	cairo_clip (cr);

	for (int r = 2 * C_RAD; r > 0; --r) {
		const float alpha = 0.1 - 0.1 * r / (2 * C_RAD + 1.f);
		cairo_set_line_width (cr, r);
		cairo_set_source_rgba (cr, SHADE_RGB(c, 3.0), alpha);
		cairo_move_to(cr, 0, 2.5);
		cairo_rel_line_to(cr, d->w_width, 0);
		cairo_stroke(cr);
		cairo_move_to(cr, 2.5, 0);
		cairo_rel_line_to(cr, 0, d->w_height);
		cairo_stroke(cr);

		cairo_set_source_rgba (cr, .0, .0, .0, alpha);
		cairo_move_to(cr, 2.5, d->w_height - 1.5);
		cairo_rel_line_to(cr, d->w_width - 4, 0);
		cairo_stroke(cr);
		cairo_move_to(cr, d->w_width - 2.5, 1.5);
		cairo_rel_line_to(cr, 0, d->w_height - 4);
		cairo_stroke(cr);
	}

	const float xalign = 0; // rint((d->w_width - d->l_width) * d->rw->xalign);
	const float yalign = 0; // rint((d->w_height - d->l_height) * d->rw->yalign);

	cairo_save (cr);
	cairo_scale (cr, 1.0 / d->rw->widget_scale, 1.0 / d->rw->widget_scale);
	cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
	cairo_set_source_surface(cr, d->sf_txt, xalign, yalign);
	cairo_paint (cr);
	cairo_restore (cr);

	if (d->prelight) {
		cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
		if (ISBRIGHT(c)) {
			cairo_set_source_rgba (cr, 0.0, 0.0, 0.0, .1);
		} else {
			cairo_set_source_rgba (cr, 1.0, 1.0, 1.0, .1);
		}

		rounded_rectangle(cr, 2.5, 2.5, d->w_width - 4, d->w_height - 4, C_RAD);
		cairo_fill(cr);
	}
	pthread_mutex_unlock (&d->_mutex);
	return TRUE;
}


/******************************************************************************
 * UI callbacks
 */

static RobWidget* robtk_vbtn_mousedown(RobWidget *handle, RobTkBtnEvent *event) {
	RobTkVBtn * d = (RobTkVBtn *)GET_HANDLE(handle);
	if (!d->prelight) { return NULL; }

	d->dragging = TRUE;
	d->clicking = TRUE;
	d->drag_x = event->x;
	d->drag_y = event->y;
	d->drag_c = d->cur;
	queue_draw(d->rw);
	return handle;
}

static RobWidget* robtk_vbtn_mouseup(RobWidget *handle, RobTkBtnEvent *event) {
	RobTkVBtn * d = (RobTkVBtn *)GET_HANDLE(handle);
	d->dragging = FALSE;
	if (!d->prelight) { return NULL; }
	if (event->button != 1) { return NULL; }
	if (d->clicking) {
		robtk_vbtn_update_value(d, d->cur == 0 ? 127 : 0);
	}
	queue_draw(d->rw);
	return NULL;
}

static RobWidget* robtk_vbtn_mousemove(RobWidget* handle, RobTkBtnEvent *ev) {
	RobTkVBtn * d = (RobTkVBtn *)GET_HANDLE(handle);
	if (!d->dragging) return NULL;

	float mult = (ev->state & ROBTK_MOD_CTRL) ? 0.25 : .97;

	float diff = mult * ((ev->x - d->drag_x) - (ev->y - d->drag_y));
	if (fabsf (diff) < 1.0) {
		return handle;
	}

	d->clicking = FALSE;
	robtk_vbtn_update_value(d, d->drag_c + diff);

	if (d->drag_c != d->cur) {
		d->drag_x = ev->x;
		d->drag_y = ev->y;
		d->drag_c = d->cur;
	}
	return handle;
}

static RobWidget* robtk_vbtn_scroll(RobWidget* handle, RobTkBtnEvent *ev) {
	RobTkVBtn * d = (RobTkVBtn *)GET_HANDLE(handle);
	if (d->dragging) { d->dragging = FALSE; }

	float val = d->cur;
	const float delta = (ev->state & ROBTK_MOD_CTRL) ? 1 : 8;

	switch (ev->direction) {
		case ROBTK_SCROLL_RIGHT:
		case ROBTK_SCROLL_UP:
			val += delta;
			break;
		case ROBTK_SCROLL_LEFT:
		case ROBTK_SCROLL_DOWN:
			val -= delta;
			break;
		default:
			break;
	}
	robtk_vbtn_update_value(d, val);
	return NULL;
}

static void robtk_vbtn_enter_notify(RobWidget *handle) {
	RobTkVBtn * d = (RobTkVBtn *)GET_HANDLE(handle);
	if (!d->prelight) {
		d->prelight = TRUE;
		queue_draw(d->rw);
	}
}

static void robtk_vbtn_leave_notify(RobWidget *handle) {
	RobTkVBtn * d = (RobTkVBtn *)GET_HANDLE(handle);
	if (d->prelight) {
		d->prelight = FALSE;
		queue_draw(d->rw);
	}
}

/******************************************************************************
 * RobWidget stuff
 */

static void
priv_vbtn_size_request(RobWidget* handle, int *w, int *h) {
	RobTkVBtn* d = (RobTkVBtn*)GET_HANDLE(handle);
	*w = 46 * d->rw->widget_scale;
	*h = 46 * d->rw->widget_scale;
}

static void
priv_vbtn_size_allocate(RobWidget* handle, int w, int h) {
	RobTkVBtn* d = (RobTkVBtn*)GET_HANDLE(handle);
	bool recreate_patterns = FALSE;
	if (h != d->w_height * d->rw->widget_scale) recreate_patterns = TRUE;
	d->w_width = w / d->rw->widget_scale;
	d->w_height = h / d->rw->widget_scale;
	if (recreate_patterns) create_vbtn_pattern(d);
	robwidget_set_size(handle, w, h);
}


/******************************************************************************
 * public functions
 */

static RobTkVBtn * robtk_vbtn_new() {
	RobTkVBtn *d = (RobTkVBtn *) malloc(sizeof(RobTkVBtn));

	d->cb = NULL;
	d->handle = NULL;
	d->sf_txt = NULL;
	d->btn_enabled = NULL;
	d->btn_inactive = NULL;
	d->prelight = FALSE;
	d->txt = strdup("");
	d->scale = 1.0;

	d->dragging = FALSE;
	d->clicking = FALSE;

	d->cur = 0;
	pthread_mutex_init (&d->_mutex, 0);

	d->w_width = 48;
	d->w_height = 48;
	d->l_width = 0;
	d->l_height = 0;

	d->rw = robwidget_new(d);

	create_vbtn_text_surface(d);

	robwidget_set_alignment(d->rw, 0, .5);
	ROBWIDGET_SETNAME(d->rw, "cbtn");

	robwidget_set_size_request(d->rw, priv_vbtn_size_request);
	robwidget_set_size_allocate(d->rw, priv_vbtn_size_allocate);
	robwidget_set_expose_event(d->rw, robtk_vbtn_expose_event);
	robwidget_set_mousedown(d->rw, robtk_vbtn_mousedown);
	robwidget_set_mouseup(d->rw, robtk_vbtn_mouseup);
	robwidget_set_mousemove(d->rw, robtk_vbtn_mousemove);
	robwidget_set_mousescroll(d->rw, robtk_vbtn_scroll);
	robwidget_set_enter_notify(d->rw, robtk_vbtn_enter_notify);
	robwidget_set_leave_notify(d->rw, robtk_vbtn_leave_notify);

	create_vbtn_pattern(d);

	return d;
}

static void robtk_vbtn_destroy(RobTkVBtn *d) {
	robwidget_destroy(d->rw);
	cairo_pattern_destroy(d->btn_enabled);
	cairo_pattern_destroy(d->btn_inactive);
	cairo_surface_destroy(d->sf_txt);
	pthread_mutex_destroy(&d->_mutex);
	free(d->txt);
	free(d);
}

static RobWidget * robtk_vbtn_widget(RobTkVBtn *d) {
	return d->rw;
}

static void robtk_vbtn_set_callback(RobTkVBtn *d, bool (*cb) (RobWidget* w, void* handle), void* handle) {
	d->cb = cb;
	d->handle = handle;
}

static float robtk_vbtn_get_value(RobTkVBtn *d) {
	return (d->cur);
}

static void robtk_vbtn_set_value(RobTkVBtn *d, float v) {
	robtk_vbtn_update_value(d, v);
}
#endif
