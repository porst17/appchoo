/*
appchoo - Application Chooser with Timeout
Written in 2012 by <Ahmet Inan> <ainan@mathematik.uni-freiburg.de>
To the extent possible under law, the author(s) have dedicated all copyright and related and neighboring rights to this software to the public domain worldwide. This software is distributed without any warranty.
You should have received a copy of the CC0 Public Domain Dedication along with this software. If not, see <http://creativecommons.org/publicdomain/zero/1.0/>.
*/

#include <getopt.h>
#include <SDL.h>
#include <SDL_image.h>

int fit_image(SDL_Surface *image, int w, int h)
{
	if (!image)
		return 0;
	int bytes = image->format->BytesPerPixel;
	if (bytes != 3 && bytes != 4)
		return 0;
	if (w >= image->w && h >= image->h)
		return 1;
	int fx = (image->w + (w-1)) / w;
	int fy = (image->h + (h-1)) / h;
	int f = fx > fy ? fx : fy;
	int pitch = image->pitch;
	image->clip_rect.w = image->w /= f;
	image->clip_rect.h = image->h /= f;
	image->pitch = (image->w * bytes + 3) & (~3);
	uint8_t *pixels = image->pixels;
	for (int y = 0; y < image->h; y++) {
		for (int x = 0; x < image->w; x++) {
			for (int c = 0; c < bytes; c++) {
				uint32_t sum = 0;
				for (int j = 0; j < f; j++)
					for (int i = 0; i < f; i++)
						 sum += pixels[(y*f+j) * pitch + (x*f+i) * bytes + c];
				pixels[y * image->pitch + x * bytes + c] = sum / (f*f);
			}
		}
	}
	return 1;
}

void center_image(SDL_Rect *dest, SDL_Rect *src)
{
	if (dest->w > src->w) {
		dest->x += dest->w / 2 - src->w / 2;
		dest->w = src->w;
	} else {
		src->x += src->w / 2 - dest->w / 2;
		src->w = dest->w;
	}
	if (dest->h > src->h) {
		dest->y += dest->h / 2 - src->h / 2;
		dest->h = src->h;
	} else {
		src->y += src->h / 2 - dest->h / 2;
		src->h = dest->h;
	}
}

int handle_corner(int w, int h, int x, int y, int r2, int c)
{
	int cx = c & 1 ? w - 1 : 0;
	int cy = c & 2 ? h - 1 : 0;
	int d2 = (cx - x) * (cx - x) + (cy - y) * (cy - y);
	return r2 > d2;
}

void handle_events(SDL_Surface *screen, SDL_Rect *rects, char **apps, int num, char corners[4][2048], int r2)
{
	static int mouse_x = 0;
	static int mouse_y = 0;
	int button = 0;
	static uint32_t left_mouse_down = false;
	SDL_Event event;
	while (SDL_PollEvent(&event)) {
		switch (event.type) {
			case SDL_KEYDOWN:
				switch (event.key.keysym.sym) {
					case SDLK_q:
						exit(1);
						break;
					case SDLK_ESCAPE:
						exit(1);
						break;
					default:
						break;
				}
				break;
			case SDL_MOUSEMOTION:
				mouse_x = event.motion.x;
				mouse_y = event.motion.y;
				break;
			case SDL_MOUSEBUTTONDOWN:
				switch (event.button.button) {
					case SDL_BUTTON_LEFT:
						left_mouse_down = true;
						break;
					default:
						break;
				}
				break;
			case SDL_MOUSEBUTTONUP:
				switch (event.button.button) {
					case SDL_BUTTON_LEFT:
						if( left_mouse_down )
							button = 1;
						left_mouse_down = false;
						break;
					default:
						break;
				}
				break;
			case SDL_QUIT:
				exit(1);
				break;
			default:
				break;
		}
	}
	if (!button)
		return;

	for (int i = 0; i < 4; i++) {
		if (*corners[i] && handle_corner(screen->w, screen->h, mouse_x, mouse_y, r2, i)) {
			fputs(corners[i], stdout);
			exit(0);
		}
	}

	for (int i = 0; i < num; i++) {
		if (rects[i].x <= mouse_x && mouse_x < (rects[i].x + rects[i].w) && rects[i].y <= mouse_y && mouse_y < (rects[i].y + rects[i].h)) {
			fputs(apps[i], stdout);
			exit(0);
		}
	}

}

SDL_Cursor *empty_cursor()
{
	static uint8_t null[32];
	return SDL_CreateCursor(null, null, 16, 16, 0, 0);
}

static int hide_cursor = 0;
static int timeout = 0;
static char *to_cmd = "true";
void init(int argc, char **argv)
{
	for (;;) {
		switch (getopt(argc, argv, "hct:d:")) {
			case 'h':
				fprintf(stderr, "usage: %s [-h] (show help) [-c] (hide cursor) [-t NUM] (timeout after NUM seconds) [-d 'CMD'] (emit 'CMD' instead of 'true' on timeout)\n", argv[0]);
				exit(1);
				break;
			case 'c':
				hide_cursor = 1;
				break;
			case 't':
				timeout = atoi(optarg);
				break;
			case 'd':
				to_cmd = optarg;
				break;
			case -1:
				return;
				break;
			default:
				fprintf(stderr, "use '-h' for help.\n");
				exit(1);
				break;
		}
	}
}

int check_corner(char *out, char *in, char *which)
{
	if (in == strstr(in, which)) {
		strncpy(out, in + 4, 2048);
		out[strnlen(out, 2048) - 1] = 0;
		return 1;
	}
	return 0;
}

int main(int argc, char **argv)
{
	init(argc, argv);
	int max = 32;
	int num = 0;
	char imgs[max][2048];
	char *apps[max];
	char corners[4][2048];
	for (int i = 0; i < 4; i++)
		*corners[i] = 0;

	while (num < max && fgets(imgs[num], 2048, stdin)) {
		if ('#' == *imgs[num])
			continue;
		if (check_corner(corners[0], imgs[num], "@NW"))
			continue;
		if (check_corner(corners[1], imgs[num], "@NE"))
			continue;
		if (check_corner(corners[2], imgs[num], "@SW"))
			continue;
		if (check_corner(corners[3], imgs[num], "@SE"))
			continue;
		imgs[num][strnlen(imgs[num], 2048) - 1] = 0;
		char *delim = strchr(imgs[num], ' ');
		apps[num] = delim + 1;
		*delim = 0;
		num++;
	}

	SDL_Init(SDL_INIT_VIDEO);

	const SDL_VideoInfo *const info = SDL_GetVideoInfo();

	int w = info->current_w;
	int h = info->current_h;
	int r2 = (w * w + h * h) / 0x4000;

	SDL_Surface *screen = SDL_SetVideoMode(w, h, 32, SDL_SWSURFACE|SDL_FULLSCREEN);

	if (!screen)
		exit(1);
	if (screen->format->BytesPerPixel != 4)
		exit(1);
	if (screen->w != w || screen->h != h)
		exit(1);

	SDL_WM_SetCaption("Application Chooser", "appchoo");

	if (hide_cursor)
		SDL_SetCursor(empty_cursor());

        SDL_FillRect(screen , NULL , 0xFFFFFF);
	
	int num_x = 1;
	int num_y = 1;

	while (num > (num_x * num_y)) {
		if (num_y < num_x)
			num_y++;
		else
			num_x++;
	}

	SDL_Rect rects[max];
	for (int i = 0; i < num; i++) {
		SDL_Surface *image = IMG_Load(imgs[i]);

		if (!image) {
			fprintf(stderr, "could not load \"%s\"\n", imgs[i]);
			exit(1);
		}

		SDL_Rect dest = screen->clip_rect;

		dest.w /= num_x;
		dest.h /= num_y;

		dest.x += (i % num_x) * dest.w;
		dest.y += ((i / num_x) % num_y) * dest.h;

		if (!fit_image(image, dest.w, dest.h))
			exit(1);

		SDL_Rect src = image->clip_rect;

		center_image(&dest, &src);

		SDL_BlitSurface(image, &src, screen, &dest);
		SDL_FreeSurface(image);
		rects[i] = dest;
	}

	SDL_Flip(screen);

	for (;;) {
		if (timeout && (int)SDL_GetTicks() > (timeout * 1000)) {
			fputs(to_cmd, stdout);
			exit(0);
		}
		SDL_Delay(100);
		handle_events(screen, rects, apps, num, corners, r2);
	}
	return 0;
}

