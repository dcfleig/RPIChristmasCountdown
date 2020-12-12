// -*- mode: c++; c-basic-offset: 2; indent-tabs-mode: nil; -*-
// Small example how to use the library.
// For more examples, look at demo-main.cc
//
// This code is public domain
// (but note, that the led-matrix library this depends on is GPL v2)

#include <unistd.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <iostream>
#include <FreeImage.h>
#include <FreeImagePlus.h>
#include <string>
#include <time.h>

#include "boost/date_time/gregorian/gregorian.hpp"
#include "boost/date_time/posix_time/posix_time.hpp"
#include "led-matrix.h"
#include "threaded-canvas-manipulator.h"
#include "graphics.h"

//#include "sparkle.h"

using rgb_matrix::Canvas;
using rgb_matrix::GPIO;
using rgb_matrix::RGBMatrix;

using namespace rgb_matrix;
using namespace std;
using namespace boost::posix_time;
using namespace boost::gregorian;

volatile bool interrupt_received = false;
static void InterruptHandler(int signo)
{
	interrupt_received = true;
}

int letter_spacing = 0;

const char *countdownImages_fn = "/home/dfleig/RPIChristmasCountdown/img/countdown.png";
FIBITMAP *countdownImages = NULL;
const int countdown_frame_width = 128;

const char *landscape_fn = "/home/dfleig/RPIChristmasCountdown/img/landscape512x32.png";
FIBITMAP *landscape = NULL;

const char *merryChristmas_fn = "/home/dfleig/RPIChristmasCountdown/img/MerryChristmas.png";
FIBITMAP *merryChristmas = NULL;

const char *santa_fn = "/home/dfleig/RPIChristmasCountdown/img/santa128x11.png";
FIBITMAP *santa = NULL;
const int santa_frame_width = 16;
const int santa_frame_height = 11;
const int santa_frame_count = 7;

const char *headerFont = "/home/dfleig/RPIChristmasCountdown/fonts/6x9.bdf";
const char *timerFont = "/home/dfleig/RPIChristmasCountdown/fonts/9x18.bdf";

enum mode
{
	countdown_1,
	countdown_2,
	expired,
	idle
};

static int usage(const char *progname)
{
	fprintf(stderr, "usage: %s -d <party start time> [optional matrix parameters]\n",
			progname);
	fprintf(stderr, "Options:\n");
	fprintf(stderr,
			"\t-d <date-time-string>  : Ex. \"2018-12-24 16:30:00.000\"\n");
	rgb_matrix::PrintMatrixFlags(stderr);

	return 1;
}

// ----------------------------------------------------------

static void DrawOnCanvas(Canvas *canvas, FIBITMAP *dib, int top, int left)
{

	BYTE *bits = (BYTE *)FreeImage_GetBits(dib);
	int dib_width = FreeImage_GetWidth(dib);
	int dib_height = FreeImage_GetHeight(dib);
	int dib_pitch = FreeImage_GetPitch(dib);

	for (int y = dib_height + top; y > top; y--)
	{

		if (y > canvas->height())
			continue;
		BYTE *pixel = (BYTE *)bits;

		for (int x = left; x < dib_width + left; x++)
		{
			if (x > canvas->width())
				continue;
			if (pixel[FI_RGBA_RED] || pixel[FI_RGBA_GREEN] || pixel[FI_RGBA_BLUE])
				canvas->SetPixel(x, y - 1, pixel[FI_RGBA_RED], pixel[FI_RGBA_GREEN], pixel[FI_RGBA_BLUE]);
			pixel += 3;
		}
		// next line
		bits += dib_pitch;
	}
}

// ----------------------------------------------------------

/** Generic image loader
        @param lpszPathName Pointer to the full file name
        @param flag Optional load flag constant
        @return Returns the loaded dib if successful, returns NULL otherwise
*/
FIBITMAP *GenericLoader(const char *lpszPathName, int flag)
{
	FREE_IMAGE_FORMAT fif = FIF_UNKNOWN;

	// check the file signature and deduce its format
	// (the second argument is currently not used by FreeImage)
	fif = FreeImage_GetFileType(lpszPathName, 0);
	if (fif == FIF_UNKNOWN)
	{
		// no signature ?
		// try to guess the file format from the file extension
		fif = FreeImage_GetFIFFromFilename(lpszPathName);
	}
	// check that the plugin has reading capabilities ...
	if ((fif != FIF_UNKNOWN) && FreeImage_FIFSupportsReading(fif))
	{
		// ok, let's load the file
		FIBITMAP *dib = FreeImage_Load(fif, lpszPathName, flag);
		// unless a bad file format, we are done !
		return dib;
	}
	return NULL;
}

// ----------------------------------------------------------

/**
        FreeImage error handler
        @param fif Format / Plugin responsible for the error
        @param message Error message
*/
void FreeImageErrorHandler(FREE_IMAGE_FORMAT fif, const char *message)
{
	printf("\n*** ");
	if (fif != FIF_UNKNOWN)
	{
		printf("%s Format\n", FreeImage_GetFormatFromFIF(fif));
	}
	printf(message);
	printf(" ***\n");
}
// ----------------------------------------------------------

void LoadImages()
{
	// initialize your own FreeImage error handler
	FreeImage_SetOutputMessage(FreeImageErrorHandler);

	// print version & copyright infos
	printf(FreeImage_GetVersion());
	printf("\n");
	printf(FreeImage_GetCopyrightMessage());
	printf("\n");

	countdownImages = GenericLoader(countdownImages_fn, 0);
	cout << "loaded "
		 << countdownImages_fn
		 << " "
		 << FreeImage_GetWidth(countdownImages)
		 << "x"
		 << FreeImage_GetHeight(countdownImages)
		 << "x"
		 << FreeImage_GetBPP(countdownImages)
		 << endl;

	merryChristmas = GenericLoader(merryChristmas_fn, 0);
	cout << "loaded "
		 << merryChristmas_fn
		 << " "
		 << FreeImage_GetWidth(merryChristmas)
		 << "x"
		 << FreeImage_GetHeight(merryChristmas)
		 << "x"
		 << FreeImage_GetBPP(merryChristmas)
		 << endl;

	landscape = GenericLoader(landscape_fn, 0);
	cout << "loaded "
		 << landscape_fn
		 << " "
		 << FreeImage_GetWidth(landscape)
		 << "x"
		 << FreeImage_GetHeight(landscape)
		 << "x"
		 << FreeImage_GetBPP(landscape)
		 << endl;

	santa = GenericLoader(santa_fn, 0);
	cout << "loaded "
		 << santa_fn
		 << " "
		 << FreeImage_GetWidth(santa)
		 << "x"
		 << FreeImage_GetHeight(santa)
		 << "x"
		 << FreeImage_GetBPP(santa)
		 << endl;

}

// int calcPixelWidthForString(const char *utf8_text, Font font) {
// 	int pixelCount = 0;
// 	while (utf8_text) {
// 		pixelCount += font.CharacterWidth(*utf8_text++);
// 	}
// 	return pixelCount;
// }

// ----------------------------------------------------------


int main(int argc, char *argv[])
{
	// It is always good to set up a signal handler to cleanly exit when we
	// receive a CTRL-C for instance. The DrawOnCanvas() routine is looking
	// for that.
	signal(SIGTERM, InterruptHandler);
	signal(SIGINT, InterruptHandler);

	RGBMatrix::Options defaults;
	defaults.hardware_mapping = "regular"; // or e.g. "adafruit-hat"
	defaults.rows = 32;
	defaults.cols = 64;
	defaults.chain_length = 2;
	defaults.parallel = 1;
	defaults.show_refresh_rate = false;
	defaults.brightness = 60;
	rgb_matrix::RuntimeOptions runtime_opt;

	// First things first: extract the command line flags that contain
	// relevant matrix options.
	if (!ParseOptionsFromFlags(&argc, &argv, &defaults, &runtime_opt))
	{
		return usage(argv[0]);
	}

	int opt;
	const char *party_time_string = "2018-12-24 16:30:00.000";
	while ((opt = getopt(argc, argv, "d:")) != -1)
	{
		switch (opt)
		{
		case 'd':
			party_time_string = strdup(optarg);
			break;
		default:
			return usage(argv[0]);
		}
	}

	RGBMatrix *canvas = CreateMatrixFromOptions(defaults, runtime_opt);

	FrameCanvas *offscreen_canvas = canvas->CreateFrameCanvas();

	//BunchOfSparkles sparkles(offscreen_canvas,20);

	if (canvas == NULL)
	{
		printf("Could not initialize RGBMatrix.");
		return 1;
	}

	// Load the images into memory
	LoadImages();

	/*
   * Load font. This needs to be a filename with a bdf bitmap font.
   */
	rgb_matrix::Font hFont;
	if (!hFont.LoadFont(headerFont))
	{
		fprintf(stderr, "Couldn't load font '%s'\n", headerFont);
		return 1;
	}

	rgb_matrix::Font tFont;
	if (!tFont.LoadFont(timerFont))
	{
		fprintf(stderr, "Couldn't load font '%s'\n", timerFont);
		return 1;
	}
	rgb_matrix::Font *tfont_outline_font = NULL;
	tfont_outline_font = tFont.CreateOutlineFont();

	const Color yellow(235, 235, 0);
	const Color red(255, 0, 0);
	const Color green(0, 205, 0);
	const Color white(235, 235, 235);
	Color fontColor = green;

	FIBITMAP *view = NULL;
	FIBITMAP *santa_view = NULL;
	FIBITMAP *countdown_view = NULL;

	// Load the time information
	ptime partyTime(time_from_string(party_time_string));

	ptime t(second_clock::local_time());
	time_duration remaining = partyTime - t;

	cout << "local: " << to_simple_string(t) << endl;
	cout << "party: " << to_simple_string(partyTime) << endl;
	cout << "remaining hours: " << remaining.hours() << endl;

	char days[4], hours[4], mins[4], secs[4];
	int direction = 1, counter = 0, speed = 4, santa_frame = 0, santa_x = 64, santa_y = 16, santa_path = 0;
	unsigned x = 0;
	bool path_start = true;
	mode display_mode = countdown_1;
	//mode display_mode = expired;

	// Top-right corners of each interval section
	// Allows tighter spacing than the usual char kerning
	int day_x = 2;
	int hr_x = day_x + (canvas->width() / 4);
	int min_x = hr_x + (canvas->width() / 4);
	int sec_x = min_x + (canvas->width() / 4);

	// Center the vertical position of the text
	int char_y = canvas->height() - (tFont.baseline() / 2) - 10;

	int green_ctr=0;

	while (!interrupt_received)
	{
		offscreen_canvas->Clear();

		counter++;

		if (x >= FreeImage_GetWidth(landscape) - offscreen_canvas->width())
		{
			x = 0;
			counter = 0;
		}

		if (counter % speed == 0)
		{
			x = x + direction;
		}

		switch (display_mode)
		{
		case countdown_1:
			view = FreeImage_CreateView(landscape, x, 0, x + offscreen_canvas->width(), offscreen_canvas->height());
			DrawOnCanvas(offscreen_canvas, view, 0, 0); // Using the canvas.
			FreeImage_Unload(view);

			// Format the interval parts
			snprintf(days, 4, "%02dd", int(abs(remaining.hours() / 24)));
			snprintf(hours, 4, "%02dh", int(abs(remaining.hours() % 24)));
			snprintf(mins, 4, "%02dm", int(remaining.minutes()));
			snprintf(secs, 4, "%02ds", int(remaining.seconds()));

			if (remaining < seconds(60))
			{
				fontColor = Color(0,green_ctr++,0);
				if (green_ctr > 254) green_ctr = 0;
			}

			// Days
			rgb_matrix::DrawText(offscreen_canvas, *tfont_outline_font,
								 day_x - 1, char_y,
								 white, NULL, days,
								 letter_spacing - 2);
			rgb_matrix::DrawText(offscreen_canvas, tFont,
								 day_x, char_y,
								 fontColor, NULL, days,
								 letter_spacing);

			// Hours
			rgb_matrix::DrawText(offscreen_canvas, *tfont_outline_font,
								 hr_x - 1, char_y,
								 white, NULL, hours,
								 letter_spacing - 2);
			rgb_matrix::DrawText(offscreen_canvas, tFont,
								 hr_x, char_y,
								 fontColor, NULL, hours,
								 letter_spacing);

			// Minutes
			rgb_matrix::DrawText(offscreen_canvas, *tfont_outline_font,
								 min_x - 1, char_y,
								 white, NULL, mins,
								 letter_spacing - 2);
			rgb_matrix::DrawText(offscreen_canvas, tFont,
								 min_x, char_y,
								 fontColor, NULL, mins,
								 letter_spacing);

			// Seconds
			rgb_matrix::DrawText(offscreen_canvas, *tfont_outline_font,
								 sec_x - 1, char_y,
								 white, NULL, secs,
								 letter_spacing - 2);
			rgb_matrix::DrawText(offscreen_canvas, tFont,
								 sec_x, char_y,
								 fontColor, NULL, secs,
								 letter_spacing);

			remaining = partyTime - second_clock::local_time();

			if (remaining < seconds(11))
			{
				display_mode = countdown_2;
			}

			break;
		case countdown_2:
			offscreen_canvas->Clear();
			santa_x = 0 - santa_frame_width;
			santa_y = 0 - santa_frame_height;
			direction = 0;

			remaining = partyTime - second_clock::local_time();

			if (remaining == seconds(0))
			{
				display_mode = expired;
				break;
			}

			countdown_view = FreeImage_CreateView(countdownImages, (remaining.seconds() - 1) * countdown_frame_width, 0, (remaining.seconds()) * countdown_frame_width, 32);
			DrawOnCanvas(offscreen_canvas, countdown_view, 0, 0);
			FreeImage_Unload(countdown_view);

			break;
		case expired:
			santa_x = 0 - santa_frame_width;
			santa_y = 0 - santa_frame_height;
			direction = 0;
			offscreen_canvas->Clear();
			DrawOnCanvas(offscreen_canvas, merryChristmas, 0, 0);

			remaining = second_clock::local_time() - partyTime;
			
			//sparkles.Draw();

			// if (remaining > seconds(60))
			// {
			// 	display_mode = idle;
			// }
			break;
		case idle:
			view = FreeImage_CreateView(landscape, x, 0, x + offscreen_canvas->width(), offscreen_canvas->height());
			DrawOnCanvas(offscreen_canvas, view, 0, 0); // Using the canvas.
			FreeImage_Unload(view);
			direction = 1;
			speed = 3;
			break;
		}

		// Update Santa animation
		switch (santa_path)
		{
		case 0:
			if (path_start)
			{
				santa_x = 0 - santa_frame_width;
				santa_y = 1 + (rand() % (int)(20 - 1 + 1));
				path_start = false;
			}
			santa_x = santa_x + (0 + (rand() % (int)(2 - 0 + 1) * direction));
			santa_y = santa_y + (-1 + (rand() % (int)(1 - -1 + 1) * direction));
			if (santa_x >= offscreen_canvas->width() + santa_frame_width)
				santa_path = 99;
			if (santa_y >= offscreen_canvas->height() - santa_frame_height)
				santa_y--;
			if (santa_x < 0 - santa_frame_width)
				santa_path = 99;
			if (santa_y < 0 - santa_frame_height)
				santa_path = 99;
			break;
		case 1:
			if (path_start)
			{
				santa_x = 0 - santa_frame_width;
				santa_y = 1 + (rand() % (int)(20 - 1 + 1));
				path_start = false;
			}
			if (counter % 4 == 0)
				santa_x = santa_x + 1 * direction;
			santa_y = 4 * sin(santa_x / 10) + 5;
			if ((santa_x > offscreen_canvas->width() + santa_frame_width))
				santa_path = 99;
			break;
		case 2:
			if (path_start)
			{
				santa_x = 0 - santa_frame_width;
				santa_y = 1 + (rand() % (int)(20 - 1 + 1));
				path_start = false;
			}
			if (counter % 4 == 0)
				santa_x = santa_x + 1 * direction;
			santa_y = 4 * sin(santa_x / 20) + 10;
			if (santa_x > offscreen_canvas->width() + santa_frame_width)
				santa_path = 99;
			break;
		default:
			santa_path = 0 + (rand() % (int)(10 - 0 + 1));
			path_start = true;
		}

		if (counter % 4 == 0)
			santa_frame++;
		if (santa_frame > santa_frame_count)
			santa_frame = 0;

		if (direction > 0)
		{
			santa_view = FreeImage_CreateView(santa, santa_frame_width * santa_frame, 0, santa_frame_width * (santa_frame + 1), santa_frame_height);
			DrawOnCanvas(offscreen_canvas, santa_view, santa_y, santa_x);
			FreeImage_Unload(santa_view);
		}

		offscreen_canvas = canvas->SwapOnVSync(offscreen_canvas);
	};

	// Animation finished. Shut down the RGB matrix.
	canvas->Clear();

	return 0;
}
