#include <string.h>
#include <raylib.h>

#include "fft.h"

/* TODO: Make it compatible for windows */
#define MINIAUDIO_IMPLEMENTATION
#include "../external/miniaudio.h"

#define WIDTH 	1080
#define HEIGHT 	(WIDTH * 9 / 16)

#define N (1 << 13)

#define num_bar 128

#ifndef M_PI
#		define M_PI 3.1415926535897932384626433832795028841971693993751058209749445923078164062862089986280348253421170679
#endif

static float sample[N] = { 0 };

typedef enum {
	MODE_CENTER_LINE = 0,
  MODE_UP_AND_BOTTOM,
	MODE_CIRCULAR,
	MODE_BOTTOM_UP,
  MODE_UP_BOTTOM,
	MODE_CIRCLE,
	MAX_MODE
} VisualMode;


void data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount)
{
    if (pInput == NULL) return;

    float *fbuffer = (float *)pInput;
    int channels = pDevice->capture.channels;

    if (frameCount < N) {
        memmove(sample, sample + frameCount, (N - frameCount) * sizeof(float));
    }

    for (unsigned int i = 0; i < frameCount; i++) {
        if (i >= N) break;

        float mixed_sample = 0;
        if (channels == 2) {
            mixed_sample = (fbuffer[i * 2] + fbuffer[i * 2 + 1]) / 2.0f;
        } else {
            mixed_sample = fbuffer[i];
        }

        sample[N - frameCount + i] = mixed_sample;
    }
    (void)pOutput;
}

static inline float lerp(float a, float b, float t)
{
  return a + t * (b - a);
}

static inline float clamp(float value, float min, float max)
{
  if (min == max)  return min;
  if (value < min) return min;
  if (value > max) return max;

  return value;
}

int main(void)
{
	static int current_height = HEIGHT;
	static int current_width = WIDTH;
	static float bass_intensity = 0.0f;

	VisualMode mode = MODE_CENTER_LINE;

	complex float audio_input[N] = { 0 };
	complex float audio_output[N] = { 0 };

	float display_height[num_bar] = { 0 };

	float padding = 50.0f;
	float x_pos = 0.0f;
	float y_pos = 5.0f;
	/* Gaps b/w each bars eg. 1.0 - 0.2 => 20% gap b/w each bars */
	float bar_gaps = 1.0f - 0.1f;

	float s = 0.85f;
	float scale = 2.5f;


	InitWindow(current_width, current_height, "music-visualizer");
	SetTargetFPS(60);

	SetWindowState(FLAG_WINDOW_RESIZABLE);

	#ifdef _WIN32
	/* TODO: Implement miniaudio playback for windows */
	ma_device_config config = ma_device_config_init(ma_device_type_loopback);
  config.wasapi.loopback  = ma_true;
	#else
	ma_device_config config = ma_device_config_init(ma_device_type_capture);
	#endif
	config.capture.format   = ma_format_f32;
	config.capture.channels = 2;
	config.sampleRate       = 48000;
	config.dataCallback     = data_callback;
	config.pUserData        = NULL;

	ma_device device;
	if (ma_device_init(NULL, &config, &device) != MA_SUCCESS) {
			return -1;
	}

	ma_device_start(&device);

	Color color = RAYWHITE;
	Color BGcolor = BLACK;
	Color CircleColor = BLACK;

	while (!WindowShouldClose()) {

		current_width = GetScreenWidth();
		current_height = GetScreenHeight();

		for (int i = 0; i < N; i++) {
				float t = (float)i / (N - 1);
				/* Hann's Function: https://wraycastle.com/blogs/knowledge-base/hanning-window */
				/* w(n) = 0.5 \left(1 - \cos\left(\frac{2\pi n}{N-1}\right)\right) 						 */
				float hann = 0.5f - 0.5f * cos(2.0f * M_PI * t);
				audio_input[i] = sample[i] * hann + I * 0.0f;
		}

		for (int j = 0; j < 10; j++) {
			bass_intensity += display_height[j];
		}
		bass_intensity /= 10.0f;


		fft(audio_input, audio_output, N);

		BeginDrawing();

		unsigned char bass_val = (unsigned char)clamp(bass_intensity * 0.5f, 0, 80);
		BGcolor = (Color){ bass_val, bass_val / 2, bass_val, 255 };

		ClearBackground(BGcolor);
		bass_intensity = 0.0f;

		float barWidth = (float)(current_width - padding) / num_bar;
		float barHeight = 0.0f;

		for (int i = 0; i < num_bar; i++) {

			float hue = (float)i / num_bar * 300.0f;
			color = ColorFromHSV(hue, 0.7f, 0.9f);
      /* int bin_index = i; */
      /* Gamma-corrected frequency range: https://dlbeer.co.nz/articles/fftvis.html */
      float t = (float)i / num_bar;
      int bin_index = (int)(powf(t, 1.5f) * (N / 4));

      float mag = cabsf(audio_output[bin_index]) * 4.0f;
      /* Logrithmic scaling */
      float log_scaled_mag = log10f(mag + 1e-9) * 40.0f;
      if (log_scaled_mag < 0)   log_scaled_mag = 0.0f;

      /* Time smoothing: https://dlbeer.co.nz/articles/fftvis.html 				 */
      /* display_height[i] = (display_height[i] * s) + (mag * (1.0f - s)); */
     	display_height[i] = (display_height[i] * s) + (log_scaled_mag * (1.0f - s));

			x_pos = (padding / 2) + (i * barWidth);

			switch(mode) {
			case MODE_CENTER_LINE:
				barHeight = clamp(display_height[i] * scale, 0.0f, current_height/2 - 20);
				DrawRectangleRounded((Rectangle) { x_pos, current_height / 2 - barHeight - y_pos, barWidth * bar_gaps, barHeight }, 0.5f, 4, color);
				DrawRectangleRounded((Rectangle) { x_pos, current_height / 2 + y_pos, barWidth * bar_gaps, barHeight }, 0.5f, 4, color);
				break;
			case MODE_UP_AND_BOTTOM:
				barHeight = clamp(display_height[i] * scale, 0.0f, current_height / 2 - 40);
				DrawRectangleRounded((Rectangle) { x_pos, current_height - barHeight - y_pos - 20, barWidth * bar_gaps, barHeight }, 0.5f, 4, color);
				DrawRectangleRounded((Rectangle) { x_pos, y_pos + 20, barWidth * bar_gaps, barHeight }, 0.5f, 4, color);
				break;
			case MODE_CIRCULAR:
				float radius = (current_height + current_width) / 8.0f - 100.0f;
				float angle = (float)i / num_bar * 2.0f * M_PI;
				Vector2 center = { current_width / 2, current_height / 2 };

				barHeight = clamp(display_height[i] * scale, 0.0f, 300.0f);
				DrawCircle(center.x, center.y, radius, CircleColor);
				DrawLineEx((Vector2) { center.x + cosf(angle) * radius, center.y + sinf(angle) * radius },
									 (Vector2) { center.x + cosf(angle) * (barHeight + radius), center.y + sinf(angle) * (barHeight + radius) },
										barWidth * bar_gaps,
										color);
				break;
			case MODE_BOTTOM_UP:
				barHeight = clamp(display_height[i] * scale * 1.5f, 0.0f, current_height - 50);
				DrawRectangleRounded((Rectangle) { x_pos, current_height - barHeight - y_pos - 20, barWidth * bar_gaps, barHeight }, 0.5f, 4, color);
				break;
			case MODE_UP_BOTTOM:
				barHeight = clamp(display_height[i] * scale * 1.5f, 0.0f, current_height - 50);
				DrawRectangleRounded((Rectangle) { x_pos, y_pos + 20, barWidth * bar_gaps, barHeight }, 0.5f, 4, color);
				break;
			case MODE_CIRCLE:
				barHeight = clamp(display_height[i] * scale, 0.0f, current_height - 50);
				DrawCircle(x_pos + (barWidth / 2), current_height - barHeight - y_pos - 20, barWidth / 2 - 1.0f, color);
				break;
			case MAX_MODE:
				break;
			}

		}

		if (IsKeyPressed(KEY_N)) {
			mode = (mode + 1) % MAX_MODE;
		}
		if (IsKeyPressed(KEY_P)) {
			mode = (mode == 0) ? MAX_MODE - 1 : (mode - 1) % MAX_MODE;
		}
		if (IsKeyPressed(KEY_ONE)) {
			mode = MODE_CENTER_LINE;
		}
		if (IsKeyPressed(KEY_TWO)) {
			mode = MODE_UP_AND_BOTTOM;
		}
		if (IsKeyPressed(KEY_THREE)) {
			mode = MODE_CIRCULAR;
		}
		if (IsKeyPressed(KEY_FOUR)) {
			mode = MODE_BOTTOM_UP;
		}
		if (IsKeyPressed(KEY_FIVE)) {
			mode = MODE_UP_BOTTOM;
		}
		if (IsKeyPressed(KEY_SIX)) {
			mode = MODE_CIRCLE;
		}

		EndDrawing();
	}

	ma_device_uninit(&device);
	CloseWindow();
	return 0;
}
