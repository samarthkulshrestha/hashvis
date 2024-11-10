#include <stdint.h>
#define NOB_IMPLEMENTATION
#define NOB_STRIP_PREFIX
#include "nob.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#define WIDTH 800
#define HEIGHT WIDTH

typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
} RGBA32;

static RGBA32 pixels[WIDTH*HEIGHT];

typedef struct {
    float x, y;
} Vec2;

typedef struct {
    float r, g, b;
} Color;

Color gray_gradient(float x, float y) {
    UNUSED(y);
    return (Color) {x, x, x};
}

Color color_gradient(float x, float y) {
    if (x * y >= 0) return (Color){x, y, 1};
    float r = fmodf(x, y);
    return (Color){r, r, r};
}

void render_pixels(Color (*f)(float x, float y)) {
    for (size_t y = 0; y < HEIGHT; ++y) {
        float ny = (float)y / HEIGHT * 2.0f - 1;
        for (size_t x = 0; x < WIDTH; ++x) {
            float nx = (float)x / WIDTH * 2.0f - 1;
            Color c = f(nx, ny);
            size_t index = y * WIDTH + x;
            pixels[index].r = (c.r + 1) / 2 * 255;
            pixels[index].g = (c.g + 1) / 2 * 255;
            pixels[index].b = (c.b + 1) / 2 * 255;
            pixels[index].a = 255;

        }
    }
}

int main(void) {
    // render_pixels(gray_gradient);
    render_pixels(color_gradient);
    const char *output_path = "output.png";
    if (!stbi_write_png(output_path, WIDTH, HEIGHT, 4, pixels, WIDTH * sizeof(RGBA32))) {
        nob_log(ERROR, "could not save image: %s", output_path);
        return 1;
    };
    nob_log(INFO, "generated: %s", output_path);
    return 0;
}
