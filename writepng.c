/*
 * Copyright 2002-2008 Guillaume Cottenceau, 2015 Aleksander Denisiuk
 *
 * This software may be freely redistributed under the terms
 * of the X11 license.
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

#define PNG_DEBUG 3
#include <png.h>


#define OUT_FILE "initials.png"
#define WIDTH 600
#define HEIGHT 600
#define COLOR_TYPE PNG_COLOR_TYPE_RGB
#define BIT_DEPTH 8


void abort_(const char * s, ...)
{
	va_list args;
	va_start(args, s);
	vfprintf(stderr, s, args);
	fprintf(stderr, "\n");
	va_end(args);
	abort();
}

int x, y;

int width, height;
png_byte color_type;
png_byte bit_depth;

png_structp png_ptr;
png_infop info_ptr;
int number_of_passes;
png_bytep * row_pointers;

void create_png_file()
{
	width = WIDTH;
	height = HEIGHT;
        bit_depth = BIT_DEPTH;
        color_type = COLOR_TYPE;

	row_pointers = (png_bytep*) malloc(sizeof(png_bytep) * height);
	for (y=0; y<height; y++)
		row_pointers[y] = (png_byte*) malloc(width*bit_depth*3);


}


void write_png_file(char* file_name)
{
	/* create file */
	FILE *fp = fopen(file_name, "wb");
	if (!fp)
		abort_("[write_png_file] File %s could not be opened for writing", file_name);


	/* initialize stuff */
	png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

	if (!png_ptr)
		abort_("[write_png_file] png_create_write_struct failed");

	info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr)
		abort_("[write_png_file] png_create_info_struct failed");

	if (setjmp(png_jmpbuf(png_ptr)))
		abort_("[write_png_file] Error during init_io");

	png_init_io(png_ptr, fp);


	/* write header */
	if (setjmp(png_jmpbuf(png_ptr)))
		abort_("[write_png_file] Error during writing header");

	png_set_IHDR(png_ptr, info_ptr, width, height,
		     bit_depth, color_type, PNG_INTERLACE_NONE,
		     PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

	png_write_info(png_ptr, info_ptr);


	/* write bytes */
	if (setjmp(png_jmpbuf(png_ptr)))
		abort_("[write_png_file] Error during writing bytes");

	png_write_image(png_ptr, row_pointers);


	/* end write */
	if (setjmp(png_jmpbuf(png_ptr)))
		abort_("[write_png_file] Error during end of write");

	png_write_end(png_ptr, NULL);

        /* cleanup heap allocation */
	for (y=0; y<height; y++)
		free(row_pointers[y]);
	free(row_pointers);

        fclose(fp);
}


void write_pixel(int x, int y, png_byte cr, png_byte cg, png_byte cb) {
    png_byte* row = row_pointers[y];
    png_byte* ptr = &(row[x*3]);
    ptr[0] = cr;
    ptr[1] = cg;
    ptr[2] = cb;
}

void bresenham(int i1, int j1, int i2, int j2, png_byte cr, png_byte cg, png_byte cb) {
    int m, b, j, P, i;

    if (j2 > j1 && j2 >= j1 && j2 - j1 <= i2 - i1) {
        m = 2 * (j2 - j1);
        b = 0;
        write_pixel(i1, j1, cr, cg, cb);
        j = j1;
        P = i2 - i1;
        for (i = i1 + 1; i < i2; i++) {
            b = b + m;
            if (b > P) {
                j++;
                b = b - 2 * P;
            }
            write_pixel(i, j, cr, cg, cb);
        }
    }
    else if (i2 > i1 && i2 >= i1 && i2 - i1 <= j2 - j1) {
        m = 2 * (i2 - i1);
        b = 0;
        write_pixel(i1, j1, cr, cg, cb);
        i = i1;
        P = j2 - j1;
        for (j = j1 + 1; j < j2; j++) {
            b = b + m;
            if (b > P) {
                i++;
                b = b - 2 * P;
            }
            write_pixel(i, j, cr, cg, cb);
        }
    }
    else if (i2 > i1 && -j2 >= -j1 && -j2 + j1 <= i2 - i1) {
        m = 2 * (-j2 + j1);
        b = 0;
        write_pixel(i1, j1, cr, cg, cb);
        j = j1;
        P = i2 - i1;
        for (i = i1 + 1; i < i2; i++) {
            b = b + m;
            if (b > P) {
                j--;
                b = b - 2 * P;
            }
            write_pixel(i, j, cr, cg, cb);
        }
    }
    else if (-j2 > -j1 && i2 >= i1 && i2 - i1 <= -j2 + j1) {
        m = 2 * (i2 - i1);
        b = 0;
        write_pixel(i1, j1, cr, cg, cb);
        i = i1;
        P = -j2 + j1;
        for (j = j1 - 1; -j < -j2; j--) {
            b = b + m;
            if (b > P) {
                i++;
                b = b - 2 * P;
            }
            write_pixel(i, j, cr, cg, cb);
        }
    }
}

/// Draw 8 mirrored pixels around an origin point (oi, oj)
void write_8_pixels(int oi, int oj, int x, int y, png_byte cr, png_byte cg, png_byte cb) {
    write_pixel(oi + x, oj + y, cr, cg, cb);
    write_pixel(oi + y, oj + x, cr, cg, cb);
    write_pixel(oi + x, oj - y, cr, cg, cb);
    write_pixel(oi + y, oj - x, cr, cg, cb);
    write_pixel(oi - x, oj + y, cr, cg, cb);
    write_pixel(oi - y, oj + x, cr, cg, cb);
    write_pixel(oi - x, oj - y, cr, cg, cb);
    write_pixel(oi - y, oj - x, cr, cg, cb);
}

/// Draw circle with radius r around an origin point (oi, oj)
void circle(int oi, int oj, int r, png_byte cr, png_byte cg, png_byte cb) {
    int i = 0, j = r, f = 5 - 4 * r;

    write_8_pixels(oi, oj, i, j, cr, cg, cb);
    while (i < j) {
        if (f > 0) {
            f = f + 8 * i - 8 * j + 20;
            j--;
        } else {
            f = f + 8 * i + 12;
        }
        i++;
        write_8_pixels(oi, oj, i, j, cr, cg, cb);
    }
}

int is_color(int x, int y, png_byte cr, png_byte cg, png_byte cb) {
    png_byte* row = row_pointers[y];
    png_byte* ptr = &(row[x*3]);
    return ptr[0] == cr && ptr[1] == cg && ptr[2] == cb;
}

int is_inside(int x, int y) {
    return x >= 0 && x < WIDTH && y >= 0 && y < HEIGHT;
}

/// Flood fill
/// - (i, j) - starting position
/// - (ocr, ocg, ocb) - color to be replaced
/// - (ncr, ncg, ncb) - new color
void fill(
    int i, int j,
    png_byte ocr, png_byte ocg, png_byte ocb,
    png_byte ncr, png_byte ncg, png_byte ncb
) {
    // Allocate up to HEIGHT * WIDTH pixel coordinates
    int S[HEIGHT * WIDTH][2] = { 0 };
    int S_len = 0;

    write_pixel(i, j, ncr, ncg, ncb);
    // Push center pixel to stack
    S[S_len][0] = i;
    S[S_len][1] = j;
    S_len++;

    while (S_len != 0) {
        // Pop from stack
        S_len--;
        i = S[S_len][0];
        j = S[S_len][1];

        // Check pixels in all directions and push to stack if they were filled
        if (is_inside(i - 1, j) && is_color(i - 1, j, ocr, ocg, ocb)) {
            write_pixel(i - 1, j, ncr, ncg, ncb);
            S[S_len][0] = i - 1;
            S[S_len][1] = j;
            S_len++;
        }
        if (is_inside(i, j - 1) && is_color(i, j - 1, ocr, ocg, ocb)) {
            write_pixel(i, j - 1, ncr, ncg, ncb);
            S[S_len][0] = i;
            S[S_len][1] = j - 1;
            S_len++;
        }
        if (is_inside(i + 1, j) && is_color(i + 1, j, ocr, ocg, ocb)) {
            write_pixel(i - 1, j, ncr, ncg, ncb);
            S[S_len][0] = i + 1;
            S[S_len][1] = j;
            S_len++;
        }
        if (is_inside(i, j + 1) && is_color(i, j + 1, ocr, ocg, ocb)) {
            write_pixel(i - 1, j, ncr, ncg, ncb);
            S[S_len][0] = i;
            S[S_len][1] = j + 1;
            S_len++;
        }
    }
}

/// Shortcut for drawing a red line between two points with shifted coordinates
#define red_line(i1, j1, i2, j2) bresenham(i1 + 100, j1 + 130, i2 + 100, j2 + 130, 255, 0, 0)
void draw_initials() {
    // "S"
	red_line(10, 100, 200, 10);
	red_line(10, 100, 180, 191); // +1 and -1 so they fully connect
	red_line(10, 300, 180, 189);

	red_line(10, 320, 10, 300);

	red_line(10, 320, 210, 189);
	red_line(40, 100, 210, 191);
	red_line(40, 100, 200, 30);

	red_line(200, 30, 200, 10);

	// "K"
	red_line(250, 320, 250, 10);
	red_line(250, 10, 270, 10);
	red_line(250, 320, 270, 320);

	red_line(270, 140, 270, 10);
	red_line(270, 320, 270, 170);

	red_line(270, 140, 350, 10);
	red_line(270, 170, 350, 320);

	red_line(350, 10, 370, 10);
	red_line(350, 320, 370, 320);

	red_line(280, 155, 370, 10);
	red_line(280, 155, 370, 320);
}

void process_file(void)
{
	for (y=0; y<height; y++) {
		png_byte* row = row_pointers[y];
		for (x=0; x<width; x++) {
			png_byte* ptr = &(row[x*3]);
			ptr[0] = 0;
			ptr[1] = 255;
			ptr[2] = 128;
		}
	}

	circle(WIDTH / 2, HEIGHT / 2, 250, 0, 0, 0);
	fill(WIDTH / 2, HEIGHT / 2, 0, 255, 128, 128, 0, 255); // Circle
	draw_initials();
	fill(115, 230, 128, 0, 255, 0, 255, 0); // "S"
	fill(355, HEIGHT / 2, 128, 0, 255, 0, 255, 0); // "K"
}


int main(int argc, char **argv)
{
	create_png_file();
	process_file();
	write_png_file(OUT_FILE);

        return 0;
}
