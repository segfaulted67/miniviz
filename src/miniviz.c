#include <string.h>
#include <raylib.h>

#include "fft.h"

/* TODO: Make it compatible for windows */
#define MINIAUDIO_IMPLEMENTATION
#include "../external/miniaudio.h"

#define WIDTH 1080
#define HEIGHT (WIDTH * 9 / 16)

#define N (1 << 13)

#define num_bar 128

#ifndef M_PI
#		define M_PI 3.1415926535897932384626433832795028841971693993751058209749445923078164062862089986280348253421170679
#endif

static float sample[N] = { 0 };

typedef enum {
	MODE_CENTER_LINE = 0,
	MODE_BOTTOM_UP,
  MODE_UP_BOTTOM,
  MODE_UP_AND_BOTTOM,
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
	VisualMode mode = MODE_CENTER_LINE;

	complex float audio_input[N] = { 0 };
	complex float audio_output[N] = { 0 };

	float display_height[num_bar] = { 0 };

	float padding = 100.0f;
	float x_pos = 0.0f;
	float y_pos = 5.0f;

	float s = 0.85f;

	InitWindow(WIDTH, HEIGHT, "music-visualizer");
	SetTargetFPS(60);

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

	Color color = WHITE;

	while (!WindowShouldClose()) {

		for (int i = 0; i < N; i++) {
				float t = (float)i / (N - 1);
				/* Hann's Function: https://wraycastle.com/blogs/knowledge-base/hanning-window */
				/* w(n) = 0.5 \left(1 - \cos\left(\frac{2\pi n}{N-1}\right)\right) 						 */
				float hann = 0.5f - 0.5f * cos(2.0f * M_PI * t);
				audio_input[i] = sample[i] * hann + I * 0.0f;
		}

		fft(audio_input, audio_output, N);

		BeginDrawing();
		ClearBackground(BLACK);

		float barWidth = (float)(WIDTH - padding) / num_bar;
		float barHeight = 0.0f;

		for (int i = 0; i < num_bar; i++) {
      /* int bin_index = i; */
      /* Gamma-corrected frequency range: https://dlbeer.co.nz/articles/fftvis.html */
      float t = (float)i / num_bar;
      int bin_index = (int)(powf(t, 1.5f) * (N / 4));

      float mag = cabsf(audio_output[bin_index]) * 10.0f;
      /* Logrithmic scaling */
      float log_scaled_mag = log10f(1.0f + mag) * 20.0f;
      if (log_scaled_mag < 0)   log_scaled_mag = 0.0f;

      /* Time smoothing: https://dlbeer.co.nz/articles/fftvis.html 				 */
      /* display_height[i] = (display_height[i] * s) + (mag * (1.0f - s)); */
     	display_height[i] = (display_height[i] * s) + (log_scaled_mag * (1.0f - s));

			x_pos = (padding / 2) + (i * barWidth);

			switch(mode) {
			case MODE_CENTER_LINE:
				barHeight = clamp(display_height[i] * 2.5f, 0.0f, HEIGHT/2 - 20);
				DrawRectangle(x_pos, HEIGHT / 2 - barHeight - y_pos, barWidth + 1.0f, barHeight, color);
				DrawRectangle(x_pos, HEIGHT / 2 + y_pos, barWidth + 1.0f, barHeight, color);
				break;
			case MODE_BOTTOM_UP:
				barHeight = clamp(display_height[i] * 2.5f, 0.0f, HEIGHT - 50);
				DrawRectangle(x_pos, HEIGHT - barHeight - y_pos - 20, barWidth + 1.0f, barHeight, color);
				break;
			case MODE_UP_BOTTOM:
				barHeight = clamp(display_height[i] * 2.5f, 0.0f, HEIGHT - 50);
				DrawRectangle(x_pos, y_pos + 20, barWidth + 1.0f, barHeight, color);
				break;
			case MODE_UP_AND_BOTTOM:
				barHeight = clamp(display_height[i] * 2.5f, 0.0f, HEIGHT / 2 - 40);
				DrawRectangle(x_pos, HEIGHT - barHeight - y_pos - 20, barWidth + 1.0f, barHeight, color);
				DrawRectangle(x_pos, y_pos + 20, barWidth + 1.0f, barHeight, color);
				break;
			case MODE_CIRCLE:
				barHeight = clamp(display_height[i] * 2.5f, 0.0f, HEIGHT - 50);
				DrawCircle(x_pos + (barWidth / 2), HEIGHT - barHeight - y_pos - 20, barWidth / 2 - 1.0f, color);
				break;
			case MAX_MODE:
				break;
			}

		}

		if (IsKeyPressed(KEY_N)) {
			mode = (mode + 1) % MAX_MODE;
		}

		EndDrawing();
	}

	ma_device_uninit(&device);
	CloseWindow();
	return 0;
}
