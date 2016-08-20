/* dial widget with custom display
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

#ifndef _ROB_TK_CNOB_H_
#define _ROB_TK_CNOB_H_

typedef struct _RobTkCnob {
	RobWidget *rw;

	float min;
	float max;
	float acc;
	float cur;
	float dfl;
	float alt;
	float base_mult;
	float scroll_mult;
	float dead_zone_delta;
	int   n_detents;
	float *detent;

	float drag_x, drag_y, drag_c;
	bool dragging;
	bool sensitive;
	bool prelight;

	bool (*cb) (RobWidget* w, void* handle);
	void* handle;

	void (*draw) (struct _RobTkCnob* d, cairo_t *cr, void* handle);
	void* draw_handle;


	cairo_pattern_t* dpat;
	cairo_surface_t* bg;
	float bg_scale;

	float w_width, w_height;

	float dcol[4][4];
} RobTkCnob;

static bool robtk_cnob_expose_event (RobWidget* handle, cairo_t* cr, cairo_rectangle_t* ev) {
	RobTkCnob* d = (RobTkCnob *)GET_HANDLE(handle);
	cairo_rectangle (cr, ev->x, ev->y, ev->width, ev->height);
	cairo_clip (cr);
	cairo_scale (cr, d->rw->widget_scale, d->rw->widget_scale);

	float c[4];
	get_color_from_theme(1, c);
	cairo_set_source_rgb (cr, c[0], c[1], c[2]);
	cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
	cairo_rectangle (cr, 0, 0, d->w_width, d->w_height);
	cairo_fill(cr);
	cairo_set_operator (cr, CAIRO_OPERATOR_OVER);

	cairo_save (cr);
	if (d->draw) {
		d->draw (d, cr, d->draw_handle);
	}

	if (d->prelight && d->sensitive) {
		cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
		cairo_set_source_rgba (cr, 1.0, 1.0, 1.0, .1);
		cairo_rectangle (cr, 0, 0, d->w_width, d->w_height);
		cairo_fill(cr);
	}
	cairo_restore (cr);
	return TRUE;
}

static void robtk_cnob_update_value(RobTkCnob * d, float val) {
	if (val < d->min) val = d->min;
	if (val > d->max) val = d->max;
	val = d->min + rintf((val - d->min) / d->acc) * d->acc;
	if (val != d->cur) {
		d->cur = val;
		if (d->cb) d->cb(d->rw, d->handle);
		queue_draw(d->rw);
	}
}

static RobWidget* robtk_cnob_mousedown(RobWidget* handle, RobTkBtnEvent *ev) {
	RobTkCnob * d = (RobTkCnob *)GET_HANDLE(handle);
	if (!d->sensitive) { return NULL; }
	if (ev->state & ROBTK_MOD_SHIFT) {
		robtk_cnob_update_value(d, d->dfl);
	} else if (ev->button == 3) {
		if (d->cur == d->dfl) {
			robtk_cnob_update_value(d, d->alt);
		} else {
			d->alt = d->cur;
			robtk_cnob_update_value(d, d->dfl);
		}
	} else if (ev->button == 1) {
		d->dragging = TRUE;
		d->drag_x = ev->x;
		d->drag_y = ev->y;
		d->drag_c = d->cur;
	}
	queue_draw(d->rw);
	return handle;
}

static RobWidget* robtk_cnob_mouseup(RobWidget* handle, RobTkBtnEvent *ev) {
	RobTkCnob * d = (RobTkCnob *)GET_HANDLE(handle);
	if (!d->sensitive) {
		d->dragging = FALSE;
		return NULL;
	}
	d->dragging = FALSE;
	queue_draw(d->rw);
	return NULL;
}

static RobWidget* robtk_cnob_mousemove(RobWidget* handle, RobTkBtnEvent *ev) {
	RobTkCnob * d = (RobTkCnob *)GET_HANDLE(handle);
	if (!d->dragging) return NULL;

	if (!d->sensitive) {
		d->dragging = FALSE;
		queue_draw(d->rw);
		return NULL;
	}

	const float mult = (ev->state & ROBTK_MOD_CTRL) ? (d->base_mult * 0.1) : d->base_mult;
	float diff = ((ev->x - d->drag_x) - (ev->y - d->drag_y));
	if (diff == 0) {
		return handle;
	}

	for (int i = 0; i < d->n_detents; ++i) {
		const float px_deadzone = 34.f - d->n_detents; // px
		if ((d->cur - d->detent[i]) * (d->cur - d->detent[i] + diff * mult) < 0) {
			/* detent */
			const int tozero = (d->cur - d->detent[i]) * mult;
			int remain = diff - tozero;
			if (abs (remain) > px_deadzone) {
				/* slow down passing the default value */
				remain += (remain > 0) ? px_deadzone * -.5 : px_deadzone * .5;
				diff = tozero + remain;
				d->dead_zone_delta = 0;
			} else {
				robtk_cnob_update_value(d, d->detent[i]);
				d->dead_zone_delta = remain / px_deadzone;
				d->drag_x = ev->x;
				d->drag_y = ev->y;
				goto out;
			}
		}

		if (fabsf (rintf((d->cur - d->detent[i]) / mult) + d->dead_zone_delta) < 1) {
			robtk_cnob_update_value(d, d->detent[i]);
			d->dead_zone_delta += diff / px_deadzone;
			d->drag_x = ev->x;
			d->drag_y = ev->y;
			goto out;
		}
	}

	diff = rintf(diff * mult * (d->max - d->min) / d->acc ) * d->acc;

	if (diff != 0) {
		d->dead_zone_delta = 0;
	}
	robtk_cnob_update_value(d, d->drag_c + diff);

out:
	if (d->drag_c != d->cur) {
		d->drag_x = ev->x;
		d->drag_y = ev->y;
		d->drag_c = d->cur;
	}

	return handle;
}

static void robtk_cnob_enter_notify(RobWidget *handle) {
	RobTkCnob * d = (RobTkCnob *)GET_HANDLE(handle);
	if (!d->prelight) {
		d->prelight = TRUE;
		queue_draw(d->rw);
	}
}

static void robtk_cnob_leave_notify(RobWidget *handle) {
	RobTkCnob * d = (RobTkCnob *)GET_HANDLE(handle);
	if (d->prelight) {
		d->prelight = FALSE;
		queue_draw(d->rw);
	}
}


static RobWidget* robtk_cnob_scroll(RobWidget* handle, RobTkBtnEvent *ev) {
	RobTkCnob * d = (RobTkCnob *)GET_HANDLE(handle);
	if (!d->sensitive) { return NULL; }
	if (d->dragging) { d->dragging = FALSE; }

	const float delta = (ev->state & ROBTK_MOD_CTRL) ? d->acc : d->scroll_mult * d->acc;

	float val = d->cur;
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
	robtk_cnob_update_value(d, val);
	return NULL;
}

static void create_cnob_pattern(RobTkCnob * d, const float c_bg[4]) {
	if (d->dpat) {
		cairo_pattern_destroy(d->dpat);
	}

	cairo_pattern_t* pat = cairo_pattern_create_linear (0.0, 0.0, 0.0, d->w_height);
	d->dpat = pat;
}

/******************************************************************************
 * RobWidget stuff
 */

static void
robtk_cnob_size_request(RobWidget* handle, int *w, int *h) {
	RobTkCnob * d = (RobTkCnob *)GET_HANDLE(handle);
	*w = d->w_width * d->rw->widget_scale;
	*h = d->w_height * d->rw->widget_scale;
}

/******************************************************************************
 * public functions
 */

static RobTkCnob*
robtk_cnob_new (float min, float max, float step, int width, int height)
{

	assert(max > min);
	assert(step > 0);
	assert( (max - min) / step >= 1.0);

	RobTkCnob* d = (RobTkCnob *) calloc (1, sizeof (RobTkCnob));

	d->w_width = width;
	d->w_height = height;

	d->rw = robwidget_new(d);
	ROBWIDGET_SETNAME(d->rw, "dial");
	robwidget_set_expose_event(d->rw, robtk_cnob_expose_event);
	robwidget_set_size_request(d->rw, robtk_cnob_size_request);
	robwidget_set_mouseup(d->rw, robtk_cnob_mouseup);
	robwidget_set_mousedown(d->rw, robtk_cnob_mousedown);
	robwidget_set_mousemove(d->rw, robtk_cnob_mousemove);
	robwidget_set_mousescroll(d->rw, robtk_cnob_scroll);
	robwidget_set_enter_notify(d->rw, robtk_cnob_enter_notify);
	robwidget_set_leave_notify(d->rw, robtk_cnob_leave_notify);

	d->min = min;
	d->max = max;
	d->acc = step;
	d->cur = min;
	d->dfl = min;
	d->alt = min;
	d->n_detents = 0;
	d->dead_zone_delta = 0;
	d->sensitive = TRUE;
	d->prelight = FALSE;
	d->dragging = FALSE;
	d->drag_x = d->drag_y = 0;
	d->base_mult = (((d->max - d->min) / d->acc) < 12) ? (d->acc * 12.0 / (d->max - d->min)) : 1.0;
	d->base_mult *= 0.004; // 250px
	d->scroll_mult = 1.0;
	d->bg_scale = 1.0;
	return d;
}

static void robtk_cnob_destroy(RobTkCnob *d) {
	robwidget_destroy(d->rw);
	cairo_pattern_destroy(d->dpat);
	free(d->detent);
	free(d);
}

static void robtk_cnob_set_alignment(RobTkCnob *d, float x, float y) {
	robwidget_set_alignment(d->rw, x, y);
}

static RobWidget* robtk_cnob_widget(RobTkCnob *d) {
	return d->rw;
}

static void robtk_cnob_set_callback(RobTkCnob *d, bool (*cb) (RobWidget* w, void* handle), void* handle) {
	d->cb = cb;
	d->handle = handle;
}

static void robtk_cnob_expose_callback (RobTkCnob *d, void (*cb) (RobTkCnob* d, cairo_t *cr, void* handle), void* handle) {
	d->draw = cb;
	d->draw_handle = handle;
}

static void robtk_cnob_set_default(RobTkCnob *d, float v) {
	v = d->min + rintf((v-d->min) / d->acc ) * d->acc;
	assert(v >= d->min);
	assert(v <= d->max);
	d->dfl = v;
	d->alt = v;
}

static void robtk_cnob_set_value(RobTkCnob *d, float v) {
	robtk_cnob_update_value(d, v);
}

static void robtk_cnob_set_sensitive(RobTkCnob *d, bool s) {
	if (d->sensitive != s) {
		d->sensitive = s;
		queue_draw(d->rw);
	}
}

static float robtk_cnob_get_value(RobTkCnob *d) {
	return (d->cur);
}

static void robtk_cnob_set_scroll_mult(RobTkCnob *d, float v) {
	d->scroll_mult = v;
}

static void robtk_cnob_set_detents(RobTkCnob *d, const int n, const float *p) {
	free(d->detent);
	assert (n < 15); // XXX
	d->n_detents = n;
	d->detent = (float*) malloc(n * sizeof(float));
	memcpy(d->detent, p, n * sizeof(float));
}

static void robtk_cnob_set_detent_default(RobTkCnob *d, bool v) {
	free(d->detent);
	d->n_detents = 1;
	d->detent = (float*) malloc(sizeof(float));
	d->detent[0] = d->dfl;
}
#endif
