
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
	cairo_pattern_t* pat = cairo_pattern_create_linear (0.0, 0.0, 0.0, h);
	cairo_pattern_add_color_stop_rgba (pat, 0.00,  .3, .3, .3, 0.5);
	cairo_pattern_add_color_stop_rgba (pat, 0.47,  .6, .6, .6, 0.5);
	cairo_pattern_add_color_stop_rgba (pat, 0.66,  .2, .2, .2, 0.5);
	cairo_pattern_add_color_stop_rgba (pat, 1.00,  .0, .0, .0, 0.5);

	// shadow
	cairo_save (cr);
	cairo_translate (cr, 1, 1);
	wheel_path (cr, w, h);

	for (int i = 8; i > 0; --i) {
		const float alpha = 0.1 - 0.1 * i / 9.f;
		cairo_set_line_width (cr, i);
		cairo_set_source_rgba (cr, .0, .0, .0, alpha);
		cairo_stroke_preserve (cr);
	}
	cairo_new_path (cr);
	cairo_restore (cr);

	// light
	cairo_save (cr);
	cairo_translate (cr, -2, -1);
	wheel_path (cr, w, h);

	for (int i = 6; i > 0; --i) {
		const float alpha = 0.1 - 0.1 * i / 7.f;
		cairo_set_line_width (cr, i);
		cairo_set_source_rgba (cr, .4, .4, .4, alpha);
		cairo_stroke_preserve (cr);
	}
	cairo_new_path (cr);
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

	// shine
	float off = .0330;
	pat = cairo_pattern_create_linear (0.0, 0.0, 5.0, h);
	cairo_pattern_add_color_stop_rgba (pat, 0.00,  .0, .0, .0, 0.0);
	cairo_pattern_add_color_stop_rgba (pat, 0.32 - off,  .75, .75, .75, 0.0);
	cairo_pattern_add_color_stop_rgba (pat, 0.39 - off,  .75, .75, .75, 0.1);
	cairo_pattern_add_color_stop_rgba (pat, 0.45 - off,  .3, .3, .3, 0.0);
	cairo_pattern_add_color_stop_rgba (pat, 1.00,  .0, .0, .0, 0.0);

	cairo_rectangle (cr, 0, 0, w, h);
	cairo_set_source (cr, pat);
	cairo_fill (cr);
	cairo_pattern_destroy (pat);
}

static void draw_bpm_value (cairo_t* cr, PangoLayout* pl, const char* txt, int h, float f) {
	int tw, th;
	cairo_save (cr);
	cairo_set_source_rgba (cr, 1.0, 0.6, 0.2, 1.0);
	pango_layout_set_text (pl, txt, -1);
	pango_layout_get_pixel_size (pl, &tw, &th);
	cairo_scale (cr, 1.0, 1.05 - 0.55 * sqrt (2. * fabs (0.5 - f)));
	cairo_translate (cr, -tw / 2.0, -1 - th / 2.0);
	cairo_translate (cr, 0 , h * 2.05 * (.5 - f));
	pango_cairo_layout_path (cr, pl);
	cairo_fill (cr);
	cairo_restore (cr);
}
