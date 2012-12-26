#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>

#include "snis_font.h"
#include "snis_typeface.h"
#include "snis_graph.h"
#include "wwviaudio.h"

#define SLIDERS_DEFINE_GLOBALS
#include "snis_sliders.h"
#undef SLIDERS_DEFINE_GLOBALS

struct slider {
	int x, y, length;
	int color;
	double value, input;
	char label[20], label1[5], label2[5];
	double r1, r2;
	slider_monitor_function sample;
	slider_clicked_function clicked;
	int active_displaymode;
	volatile int *displaymode;
};

static int slider_sound = -1;

struct slider *snis_slider_init(int x, int y, int length, int color,
		char *label, char *l1, char *l2, double r1, double r2,
		slider_monitor_function gmf, slider_clicked_function clicked, 
		int active_displaymode, volatile int *displaymode)
{
	struct slider *s;

	s = malloc(sizeof(*s));
	s->x = x;
	s->y = y;
	s->length = length;
	s->color = color;
	strncpy(s->label, label, sizeof(s->label) - 1);
	strncpy(s->label1, l1, sizeof(s->label1) - 1);
	strncpy(s->label2, l2, sizeof(s->label2) - 1);
	s->r1 = r1;
	s->r2 = r2;
	s->sample = gmf;
	s->clicked = clicked;
	s->value = s->sample() / (s->r2 - s->r1);
	s->active_displaymode = active_displaymode;
	s->displaymode = displaymode;
	return s;
}

void slider_draw(GtkWidget *w, GdkGC *gc, struct slider *s)
{
	double v;
	int width, tx1;
	int bar_color;

#define SLIDER_HEIGHT 15
#define SLIDER_POINTER_HEIGHT 8
#define SLIDER_POINTER_WIDTH 5

	v = s->sample();
	s->value = (v - s->r1) / (s->r2 - s->r1);
	v = s->sample();
	tx1 = (int) (s->value * s->length) + s->x;
	if (!s->clicked) {
		if (v < 25.0) {
			bar_color = RED;
		} else {
			if (v < 50.0)  {
				bar_color = AMBER;
			} else {
				bar_color = DARKGREEN;
			}
		}
	}
	sng_set_foreground(s->color);
	sng_current_draw_rectangle(w->window, gc, 0, s->x, s->y, s->length, SLIDER_HEIGHT);
	width = s->value * s->length - 1;
	if (width < 0)
		width = 0;
	if (!s->clicked)
		sng_set_foreground(bar_color);
	sng_current_draw_rectangle(w->window, gc, 1, s->x + 1, s->y + 1, width, SLIDER_HEIGHT - 2);
	if (!s->clicked)
		sng_set_foreground(s->color);

	tx1 = (int) (s->value * s->length) + s->x;

	if (s->clicked) {
		sng_current_draw_line(w->window, gc, tx1, s->y, tx1 - SLIDER_POINTER_WIDTH, s->y - SLIDER_POINTER_HEIGHT); 
		sng_current_draw_line(w->window, gc, tx1, s->y, tx1 + SLIDER_POINTER_WIDTH, s->y - SLIDER_POINTER_HEIGHT); 
		sng_current_draw_line(w->window, gc, tx1 - SLIDER_POINTER_WIDTH, s->y - SLIDER_POINTER_HEIGHT,
						tx1 + SLIDER_POINTER_WIDTH, s->y - SLIDER_POINTER_HEIGHT); 
		sng_current_draw_line(w->window, gc, tx1, s->y + SLIDER_HEIGHT,
				tx1 - SLIDER_POINTER_WIDTH, s->y + SLIDER_HEIGHT + SLIDER_POINTER_HEIGHT); 
		sng_current_draw_line(w->window, gc, tx1, s->y + SLIDER_HEIGHT,
				tx1 + SLIDER_POINTER_WIDTH, s->y + SLIDER_HEIGHT + SLIDER_POINTER_HEIGHT); 
		sng_current_draw_line(w->window, gc, tx1 - SLIDER_POINTER_WIDTH, s->y + SLIDER_HEIGHT + SLIDER_POINTER_HEIGHT,
				tx1 + SLIDER_POINTER_WIDTH, s->y + SLIDER_HEIGHT + SLIDER_POINTER_HEIGHT); 
	}
	sng_abs_xy_draw_string(w, gc, s->label, TINY_FONT, s->x + s->length + 5, s->y + 2 * SLIDER_HEIGHT / 3); 
}

#define MAXSLIDERS 20
static int nsliders = 0;
static struct slider *sliderlist[MAXSLIDERS];

void snis_add_slider(struct slider *s)
{
	if (nsliders >= MAXSLIDERS)
		return;
	sliderlist[nsliders] = s;
	nsliders++;
}

double snis_slider_get_value(struct slider *s)
{	
	return s->value;
}

double snis_slider_get_input(struct slider *s)
{	
	return s->input;
}

void snis_draw_sliders(GtkWidget *w, GdkGC *gc)
{
	int i;

	for (i = 0; i < nsliders; i++)
		if (sliderlist[i]->active_displaymode == *sliderlist[i]->displaymode)
			slider_draw(w, gc, sliderlist[i]);
}

void snis_sliders_button_press(int x, int y)
{
	int i;
	struct slider *s;

	for (i = 0; i < nsliders; i++) {
		s = sliderlist[i];
		if (s->active_displaymode == *s->displaymode) {
			if (x < s->x || x > s->x + s->length || 
				y < s->y || y > s->y + SLIDER_HEIGHT)
				continue;
			s->input = ((double) x - (double) s->x) / (double) s->length;
			if (s->clicked) {
				s->clicked(s);
				if (slider_sound != -1) 
					wwviaudio_add_sound(slider_sound);
			}
		}
	}
}

void snis_slider_set_sound(int sound)
{
	slider_sound = sound;
}

void snis_slider_set_input(struct slider *s, double input)
{
	s->input = input;
}
