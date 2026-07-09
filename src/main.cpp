#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <math.h>

#define internal static
#define global_variable static
#define local_persist static

global_variable bool is_running = True;
global_variable bool is_fullscreen = False;
global_variable int bytes_per_pixel = 4;

const char *welcome_string = "WINDOW NAME";

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef struct {
	Display *display;
	Window window;
	GC gc;
	Visual *visual;
	XImage *image;
	uint8 *pixels;
	uint8 screen = DefaultScreen(&display);
	int width, height;
	int depth;
} app_state;

typedef struct {
	float x;
	float y;
	float z;
} vertices;

typedef struct {
	int a;
	int b;
} edges_struct;

edges_struct edges[12] = {
	{0,1}, {1,2}, {2,3}, {3,0},
	{4,5}, {5,6}, {6,7}, {7,4},
	{0,4}, {1,5}, {2,6}, {3,7}
};

internal void put_pixel(app_state *app, int x, int y, int r, int g, int b) {

	if (x < 0 || y < 0 || x >= app->width || y >= app->height)
	return;

	int idx = (y * app->image->bytes_per_line) + (x * bytes_per_pixel);
	app->pixels[idx]     = b;
	app->pixels[idx + 1] = g;
	app->pixels[idx + 2] = r;
	app->pixels[idx + 3] = 0;
}

internal void draw_line(app_state *app, int x0, int y0, int x1, int y1, int r, int g, int b) {

	int dx = abs(x1 - x0);
	int sx = (x0 < x1) ? 1 : -1;
	int dy = -abs(y1 - y0);
	int sy = (y0 < y1) ? 1 : -1;
	int err = dx + dy;

	for (;;) {
		put_pixel(app, x0, y0, r, g, b);

		if (x0 == x1 && y0 == y1)
			break;

		int e2 = 2 * err;

		if (e2 >= dy) {
			err += dy;
			x0 += sx;
		}

		if (e2 <= dx) {
			err += dx;
			y0 += sy;
		}
	}
}


internal int create_image(app_state *app, int width, int height) {

	int bytes_per_line = width * 4;
	int allocation_size = height * bytes_per_line; 
	app->pixels = (uint8*)malloc(allocation_size);
	app->width = width;
	app->height = height;
	
	app->image = XCreateImage(app->display, app->visual, app->depth, 
	    		     ZPixmap, 0, (char*)app->pixels, 
	    		     width, height, 32, bytes_per_line);
	//if (!app->image) {
	    //free(app->pixels);
	    //return 0;
	//}
	//Don't think this check is needed
	return 1;
}

internal void render(app_state *app, int XOffSet) {

	if (!app->image) {return;}

	for (int y = 0; y < app->height; ++y) {
		for (int x = 0; x < app->width; ++x) {
			int idx = (y * app->image->bytes_per_line) + (x * bytes_per_pixel);

			app->pixels[idx] = 0;
			app->pixels[idx + 1] = 0;
			app->pixels[idx + 2] = 0;
			app->pixels[idx + 3] = 0;	
	    }
	}


	vertices v[8] = {
		{-1, -1, -1},
		{ 1, -1, -1},
		{ 1,  1, -1},
		{-1,  1, -1},
		{-1, -1,  1},
		{ 1, -1,  1},
		{ 1,  1,  1},
		{-1,  1,  1}
	};

	float cos_Offset = cos(XOffSet * 0.111);
	float sin_Offset = sin(XOffSet * 0.111);


	for (int i = 0; i < 8; ++i) {

		int x = v[i].x;
		int y = v[i].y;
		int z = v[i].z;

		float x1 = x * cos_Offset - z * sin_Offset;
		float z1 = x * sin_Offset + z * cos_Offset;
		float y2 = y * cos_Offset - z1 * sin_Offset;
		float z2 = y * sin_Offset+ z1 * cos_Offset;

		v[i].x = x1;
		v[i].y = y2;
		v[i].z = z2 + 4.0;
	}

	for (int i = 0; i < 12; i++) {

		int a = edges[i].a;
		int b = edges[i].b;

		int x1 = ((v[a].x / v[a].z) * 420.0 + app->width * 0.5);
		int y1 = ((-v[a].y / v[a].z) * 420.0 + app->height * 0.5);
		int x2 = ((v[b].x / v[b].z) * 420.0 + app->width * 0.5);
		int y2 = ((-v[b].y / v[b].z) * 420.0 + app->height * 0.5);

		draw_line(app, x1, y1, x2, y2, 50, 150, 200);

	}

	XPutImage(app->display, app->window, app->gc, app->image,
		  0, 0, 0, 0, app->width, app->height);
}

int main() {

	app_state app = {0};
	app.display = XOpenDisplay(NULL);

	if (!app.display) {
		fprintf(stderr, "\nCannot open display\n");
		return 1;
	}

	app.screen = DefaultScreen(app.display);
	app.visual = XDefaultVisual(app.display, app.screen);
	app.depth = XDefaultDepth(app.display, app.screen);

	int SCREEN_WIDTH = DisplayWidth(app.display, app.screen);
	int SCREEN_HEIGHT = DisplayHeight(app.display, app.screen);
	app.width = 800;
	app.height = 500; 
	int XOffSet = 0; 

	app.window = XCreateSimpleWindow(app.display, 
				   RootWindow(app.display, app.screen),
				   0, 0, app.width, app.height, 1,
				   BlackPixel(app.display, app.screen),
				   BlackPixel(app.display, app.screen));

	XSelectInput(app.display, app.window, 
		StructureNotifyMask | ExposureMask | KeyPressMask);


	XMapWindow(app.display, app.window);
	app.gc = XCreateGC(app.display, app.window, 0, NULL); 	
	XFlush(app.display);

	XStoreName(app.display, app.window, welcome_string);

	XSizeHints* sizeHints = XAllocSizeHints();
	sizeHints->flags = PMinSize;
	sizeHints->min_width = 500;
	sizeHints->min_height = 300;
	XSetNormalHints(app.display, app.window, sizeHints);
	XFree(sizeHints);

	if (!create_image(&app, 800, 600)) {
		fprintf(stderr, "\nFailed to create initial image\n");
		goto cleanup;
	}

	while (is_running) {

		render(&app, XOffSet);
		XFlush(app.display);
		XOffSet += 1;

		if (XPending(app.display)) {
			XEvent event;
			XNextEvent(app.display, &event);

			/* TODO:: Write separate functions to reduce the size of a switch statement */
			switch (event.type) {

				case (ConfigureNotify): {
					int current_width = event.xconfigure.width;
					int current_height = event.xconfigure.height;
			    
					if (current_width != app.width || current_height != app.height) {
						if (create_image(&app, current_width, current_height)) {
							XSync(app.display, False);
						}
					}
					    
				} break;

				case (Expose): {
					render(&app, XOffSet);
					XSync(app.display, False);
				} break;

				case (KeyPress): {
					KeySym keysym;
					char keybuf[16];
					XLookupString(&event.xkey, keybuf, sizeof(keybuf), &keysym, NULL);

					if (keysym == XK_Escape) {
						goto cleanup;
						break;
					}
					if (keysym == XK_F11) {
						if (!is_fullscreen) { // create separate function for this code block
							  XMoveResizeWindow(app.display, app.window, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
							  is_fullscreen = True;
						}

						/* This block doesn't work for some reason 						
						else {
							XMoveResizeWindow(app.display, app.window, 100, 100, 1200, 800);
							is_fullscreen = False; 
						}
						*/

						XSync(app.display, False);
					  }

				} break;

				case (DestroyNotify): {
					  goto cleanup;
				} break;
				case FocusIn: {
					  XSetInputFocus(app.display, app.window, RevertToParent, CurrentTime);
				} break;

				default: {

				} break;
			}	 
		}	 
	}	 
	cleanup:
		XDestroyImage(app.image);
		XFreeGC(app.display, app.gc);
		XDestroyWindow(app.display, app.window);
		XCloseDisplay(app.display);
		return 0;  
}
