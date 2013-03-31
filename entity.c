/*
        Copyright (C) 2010 Stephen M. Cameron
	Author: Stephen M. Cameron

        This file is part of Spacenerds In Space.

        Spacenerds in Space is free software; you can redistribute it and/or modify
        it under the terms of the GNU General Public License as published by
        the Free Software Foundation; either version 2 of the License, or
        (at your option) any later version.

        Spacenerds in Space is distributed in the hope that it will be useful,
        but WITHOUT ANY WARRANTY; without even the implied warranty of
        MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
        GNU General Public License for more details.

        You should have received a copy of the GNU General Public License
        along with Spacenerds in Space; if not, write to the Free Software
        Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/
#include <stdio.h>
#include <math.h>
#include <gtk/gtk.h>

#include "mathutils.h"
#include "matrix.h"
#include "vertex.h"
#include "triangle.h"
#include "mesh.h"
#include "stl_parser.h"
#include "entity.h"

#include "my_point.h"
#include "snis_font.h"
#include "snis_typeface.h"

#include "snis_graph.h"

#define MAX_ENTITIES 5000

static unsigned long ntris, nents, nlines;

struct entity {
	struct mesh *m;
	float x, y, z; /* world coords */
};

struct camera_info {
	float x, y, z;		/* position of camera */
	float lx, ly, lz;	/* where camera is looking */
	float near, far, width, height;
	int xvpixels, yvpixels;
} camera;

static int nentities = 0;
static struct entity entity_list[MAX_ENTITIES];

static float rx, ry, rz;

struct entity *add_entity(struct mesh *m, float x, float y, float z)
{
	// if (nentities < MAX_ENTITIES) {

	if (nentities < 1) {
		entity_list[nentities].m = m;
		entity_list[nentities].x = 0; // x;
		entity_list[nentities].y = 0; // y;
		entity_list[nentities].z = 2; // z;
		nentities++;
		return &entity_list[nentities - 1];
	}
	return NULL;
}

static int is_backface(int x1, int y1, int x2, int y2, int x3, int y3)
{
	int twicearea;

	twicearea =	(x1 * y2 - x2 * y1) +
			(x2 * y3 - x3 * y2) +
			(x3 * y1 - x1 * y3);
	return twicearea >= 0;
}

void __attribute__((unused))
	wireframe_render_triangle(GtkWidget *w, GdkGC *gc, struct triangle *t)
{
	struct vertex *v1, *v2, *v3;
	int x1, y1, x2, y2, x3, y3;

	ntris++;
	nlines += 3;
	v1 = t->v[0];
	v2 = t->v[1];
	v3 = t->v[2];

	x1 = (int) (v1->wx * camera.xvpixels / 2) + camera.xvpixels / 2;
	x2 = (int) (v2->wx * camera.xvpixels / 2) + camera.xvpixels / 2;
	x3 = (int) (v3->wx * camera.xvpixels / 2) + camera.xvpixels / 2;
	y1 = (int) (v1->wy * camera.yvpixels / 2) + camera.yvpixels / 2;
	y2 = (int) (v2->wy * camera.yvpixels / 2) + camera.yvpixels / 2;
	y3 = (int) (v3->wy * camera.yvpixels / 2) + camera.yvpixels / 2;

	if (is_backface(x1, y1, x2, y2, x3, y3))
		return;

	sng_current_draw_line(w->window, gc, x1, y1, x2, y2); 
	sng_current_draw_line(w->window, gc, x2, y2, x3, y3); 
	sng_current_draw_line(w->window, gc, x3, y3, x1, y1); 
}

static void scan_convert_sorted_triangle(GtkWidget *w, GdkGC *gc,
			int x1, int y1, int x2, int y2, int x3, int y3)
{
	float xa, xb, y;
	float dxdy1, dxdy2;
	int i;

	if (y1 > y2 || y2 > y3)
		printf("you dun fucked up\n");

	if (y2 == y1)
		dxdy1 = x2 - x1;
	else
		dxdy1 = (float) (x2 - x1) / (float) (y2 - y1);

	if (y3 == y1)
		dxdy2 = x3 - x1;
	else
		dxdy2 = (float) (x3 - x1) / (float) (y3 - y1);

	xa = x1;
	xb = x1;
	y = y1;

	for (i = y1; i < y2; i++) {
		sng_device_line(w->window, gc, (int) xa, (int) y, (int) xb, (int) y);
		xa += dxdy1;
		xb += dxdy2;
		y += 1;
	}

	if (y2 == y3)
		dxdy1 = x3 - x2;
	else
		dxdy1 = (float) (x3 - x2) / (float) (y3 - y2);

	xa = x2;
	y = y2;
	for (i = y2; i <= y3; i++) {
		sng_device_line(w->window, gc, (int) xa, (int) y, (int) xb, (int) y);
		xa += dxdy1;
		xb += dxdy2;
		y += 1;
	}
}

static void scan_convert_triangle(GtkWidget *w, GdkGC *gc, struct triangle *t)
{
	struct vertex *v1, *v2, *v3;
	int x1, y1, x2, y2, x3, y3;
	int xa, ya, xb, yb, xc, yc;

	ntris++;
	nlines += 3;
	v1 = t->v[0];
	v2 = t->v[1];
	v3 = t->v[2];

	x1 = (int) (v1->wx * camera.xvpixels / 2) + camera.xvpixels / 2;
	x2 = (int) (v2->wx * camera.xvpixels / 2) + camera.xvpixels / 2;
	x3 = (int) (v3->wx * camera.xvpixels / 2) + camera.xvpixels / 2;
	y1 = (int) (v1->wy * camera.yvpixels / 2) + camera.yvpixels / 2;
	y2 = (int) (v2->wy * camera.yvpixels / 2) + camera.yvpixels / 2;
	y3 = (int) (v3->wy * camera.yvpixels / 2) + camera.yvpixels / 2;

	if (is_backface(x1, y1, x2, y2, x3, y3))
		return;

	/* sort from lowest y to highest y */
	ya = sng_device_y(y1);
	yb = sng_device_y(y2);
	yc = sng_device_y(y3);
	if (ya <= yb) {
		if (ya <= yc) {
			/* order is 1, ... */
			xa = sng_device_x(x1);
			if (yb <= yc) {
				/* order is 1, 2, 3 */
				xb = sng_device_x(x2);
				xc = sng_device_x(x3);
			} else {
				/* order is 1, 3, 2 */
				xb = sng_device_x(x3);
				yb = sng_device_y(y3);
				xc = sng_device_x(x2);
				yc = sng_device_y(y2);
			}
		} else { /* we know c < a <= b, so order is 3, 1, 2... */
			xa = sng_device_x(x3);
			ya = sng_device_y(y3);
			xb = sng_device_x(x1);
			yb = sng_device_y(y1);
			xc = sng_device_x(x2);
			yc = sng_device_y(y2);
		}
	} else {
		/* a > b */
		if (yb > yc) { /* a > b >= c, so order is 3, 2, 1 */
			xa = sng_device_x(x3);
			ya = sng_device_y(y3);
			xb = sng_device_x(x2);
			yb = sng_device_y(y2);
			xc = sng_device_x(x1);
			yc = sng_device_y(y1);
		} else { /* c >= b && b < a */
			if (ya <= yc) {
				/* c >= b && b < a && a <= c : 2, 1, 3 */
				xa = sng_device_x(x2);
				ya = sng_device_y(y2);
				xb = sng_device_x(x1);
				yb = sng_device_y(y1);
				xc = sng_device_x(x3);
				yc = sng_device_y(y3);
			} else {
				/* a > b && c >=b && c < a: 2, 3, 1 */ 
				xa = sng_device_x(x2);
				ya = sng_device_y(y2);
				xb = sng_device_x(x3);
				yb = sng_device_y(y3);
				xc = sng_device_x(x1);
				yc = sng_device_y(y1);
			}
		}
	}
	/* now device coord vertices xa, ya, xb, yb, xc, yc are sorted by y value */
	scan_convert_sorted_triangle(w, gc, xa, ya, xb, yb, xc, yc);
}

void wireframe_render_entity(GtkWidget *w, GdkGC *gc, struct entity *e)
{
	int i;

	for (i = 0; i < e->m->ntriangles; i++)
		wireframe_render_triangle(w, gc, &e->m->t[i]);
	nents++;
}

void render_entity(GtkWidget *w, GdkGC *gc, struct entity *e)
{
	int i;
	float cos_theta;
	struct mat41 light = { {0, 1, 0, 1} };
	struct mat41 normal;

	for (i = 0; i < e->m->ntriangles; i++) {
		normal = *(struct mat41 *) &e->m->t[i].n.wx;
		normalize_vector(&normal, &normal);
		normalize_vector(&light, &light);
		cos_theta = mat41_dot_mat41(&light, &normal);
		cos_theta = (cos_theta + 1.0) / 2.0;
		sng_set_foreground((int) fmod((cos_theta * 256.0), 255.0) + GRAY);
		// sng_set_foreground(RED);
		scan_convert_triangle(w, gc, &e->m->t[i]);
	}
	nents++;
}

static void transform_entity(struct entity *e, struct mat44 *transform)
{
	int i;
	struct mat41 *m1, *m2;

	/* calculate the object transform... */
	struct mat44 object_transform, total_transform, tmp_transform;
	struct mat44 object_rotation = {{{ 1, 0, 0, 0 },
					 { 0, 1, 0, 0 },
					 { 0, 0, 1, 0 },
					 { 0, 0, 0, 1 }}}; /* fixme, do this really. */
	struct mat44 object_translation = {{{ 1,    0,     0,    0 },
					    { 0,    1,     0,    0 },
					    { 0,    0,     1,    0 },
					    { e->x, e->y, e->z, 1 }}};
	/* for testing, do small rotation... */
	struct mat44 r1, r2;
	mat44_rotate_x(&object_rotation, rx * M_PI / 180.0, &r1);  
	mat44_rotate_y(&r1, ry * M_PI / 180.0, &r2);  
	mat44_rotate_z(&r2, rz * M_PI / 180.0, &object_rotation);  

	tmp_transform = *transform;
	mat44_product(&tmp_transform, &object_translation, &object_transform);
	mat44_product(&object_transform, &object_rotation, &total_transform);
#if 0
	mat44_product(&object_translation, &object_rotation, &object_transform);
	mat44_product(&object_transform, transform, &total_transform);
#endif
	
	/* Set homogeneous coord to 1 initially for all vertices */
	for (i = 0; i < e->m->nvertices; i++)
		e->m->v[i].w = 1.0;

	/* Do the transformation... */
	for (i = 0; i < e->m->nvertices; i++) {
		m1 = (struct mat41 *) &e->m->v[i].x;
		m2 = (struct mat41 *) &e->m->v[i].wx;
		mat44_x_mat41(&total_transform, m1, m2);
		/* normalize... */
		m2->m[0] /= m2->m[3];
		m2->m[1] /= m2->m[3];
		m2->m[2] /= m2->m[3];

	}
	
	for (i = 0; i < e->m->ntriangles; i++) {
		m1 = (struct mat41 *) &e->m->t[i].n.x;
		m2 = (struct mat41 *) &e->m->t[i].n.wx;
		mat44_x_mat41(&total_transform, m1, m2);
		/* normalize... */
		m2->m[0] /= m2->m[3];
		m2->m[1] /= m2->m[3];
		m2->m[2] /= m2->m[3];
	}
}

void render_entities(GtkWidget *w, GdkGC *gc)
{
	int i;

	struct mat44 identity = {{{ 1, 0, 0, 0 }, /* identity matrix */
				    { 0, 1, 0, 0 },
				    { 0, 0, 1, 0 },
				    { 0, 0, 0, 1 }}};
	struct mat44 perspective_transform;
	struct mat44 cameralook_transform;
	struct mat44 tmp_transform;
	struct mat44 cameralocation_transform;
	struct mat44 total_transform;
	
	struct mat41 look_direction;

	struct mat41 up = { { 0, 1, 0, 0 } };
	struct mat41 camera_x, x_cross_look;
	struct mat41 *v; /* camera relative y axis (up/down) */ 
	struct mat41 *n; /* camera relative z axis (into view plane) */
	struct mat41 *u; /* camera relative x axis (left/right) */

	nents = 0;
	ntris = 0;
	nlines = 0;
	/* Translate to camera position... */
	mat44_translate(&identity, -camera.x, -camera.y, -camera.z,
				&cameralocation_transform);

	/* Calculate look direction, look direction, ... */
	look_direction.m[0] = (camera.lx - camera.x);
	look_direction.m[1] = (camera.ly - camera.y);
	look_direction.m[2] = (camera.lz - camera.z);
	look_direction.m[3] = 1.0;
	normalize_vector(&look_direction, &look_direction);
	n = &look_direction;

	/* Calculate x direction relative to camera, "camera_x" */
	mat41_cross_mat41(&up, &look_direction, &camera_x);
	normalize_vector(&camera_x, &camera_x);
	u = &camera_x;

	/* Calculate camera relative x axis */
	v = &x_cross_look;
	mat41_cross_mat41(n, u, v);
	/* v should not need normalizing as n and u are already
	 * unit, and are perpendicular */
	normalize_vector(v, v);

	/* Make a rotation matrix...
	   | ux uy uz 0 |
	   | vx vy vz 0 |
	   | nx ny nz 0 |
	   |  0  0  0 1 |
	 */
	cameralook_transform.m[0][0] = u->m[0];
	cameralook_transform.m[0][1] = v->m[0];
	cameralook_transform.m[0][2] = n->m[0];
	cameralook_transform.m[0][3] = 0.0;
	cameralook_transform.m[1][0] = u->m[1];
	cameralook_transform.m[1][1] = v->m[1];
	cameralook_transform.m[1][2] = n->m[1];
	cameralook_transform.m[1][3] = 0.0;
	cameralook_transform.m[2][0] = u->m[2];
	cameralook_transform.m[2][1] = v->m[2];
	cameralook_transform.m[2][2] = n->m[2];
	cameralook_transform.m[2][3] = 0.0;
	cameralook_transform.m[3][0] = 0.0;
	cameralook_transform.m[3][1] = 0.0;
	cameralook_transform.m[3][2] = 0.0; 
	cameralook_transform.m[3][3] = 1.0;

	/* Make perspective transform... */
	perspective_transform.m[0][0] = (2 * camera.near) / camera.width;
	perspective_transform.m[0][1] = 0.0;
	perspective_transform.m[0][2] = 0.0;
	perspective_transform.m[0][3] = 0.0;
	perspective_transform.m[1][0] = 0.0;
	perspective_transform.m[1][1] = (2.0 * camera.near) / camera.height;
	perspective_transform.m[1][2] = 0.0;
	perspective_transform.m[1][3] = 0.0;
	perspective_transform.m[2][0] = 0.0;
	perspective_transform.m[2][1] = 0.0;
	perspective_transform.m[2][2] = -(camera.far + camera.near) /
						(camera.far - camera.near);
	perspective_transform.m[2][3] = -1.0;
	perspective_transform.m[3][0] = 0.0;
	perspective_transform.m[3][1] = 0.0;
	perspective_transform.m[3][2] = (-2 * camera.far * camera.near) /
						(camera.far - camera.near);
	perspective_transform.m[3][3] = 0.0;

#if 1
	mat44_product(&perspective_transform, &cameralook_transform, &tmp_transform);
	mat44_product(&tmp_transform, &cameralocation_transform, &total_transform);
#else
	mat44_product(&cameralocation_transform, &cameralook_transform, &tmp_transform);
	mat44_product(&tmp_transform, &perspective_transform, &total_transform);
#endif
	   
	for (i = 0; i < nentities; i++)
		transform_entity(&entity_list[i], &total_transform);
	for (i = 0; i < nentities; i++)
		render_entity(w, gc, &entity_list[i]);
	// printf("ntris = %lu, nlines = %lu, nents = %lu\n", ntris, nlines, nents);
	rx = fmod(rx + 0.3, 360.0);
	ry = fmod(ry + 0.15, 360.0);
	rz = fmod(rz + 0.6, 360.0);
}

void camera_set_pos(float x, float y, float z)
{
	camera.x = x;
	camera.y = y;
	camera.z = z;
}

void camera_look_at(float x, float y, float z)
{
	camera.lx = x;
	camera.ly = y;
	camera.lz = z;
}

void camera_set_parameters(float near, float far, float width, float height,
				int xvpixels, int yvpixels)
{
	camera.near = -near;
	camera.far = -far;
	camera.width = width;
	camera.height = height;	
	camera.xvpixels = xvpixels;
	camera.yvpixels = yvpixels;
}
