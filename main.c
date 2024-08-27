#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdbool.h>

// Screen dimensions
const int SCREEN_WIDTH = 640;
const int SCREEN_HEIGHT = 480;

// Camera and viewport settings
const float viewportDistance = 500.0f;  // Distance from the camera to the viewport

void drawLine(Uint32* pixels, int x0, int y0, int x1, int y1, Uint32 color) {
    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy, e2;

    while (true) {
        if (x0 >= 0 && x0 < SCREEN_WIDTH && y0 >= 0 && y0 < SCREEN_HEIGHT)
            pixels[y0 * SCREEN_WIDTH + x0] = color;

        if (x0 == x1 && y0 == y1) break;
        e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}


void drawRectangle(Uint32* pixels, int x, int y, int width, int height, Uint32 color) {
    for (int dy = 0; dy < height; ++dy) {
        for (int dx = 0; dx < width; ++dx) {
            int px = x + dx;
            int py = y + dy;
            if (px >= 0 && px < SCREEN_WIDTH && py >= 0 && py < SCREEN_HEIGHT)
                pixels[py * SCREEN_WIDTH + px] = color;
        }
    }
}


int main(int argc, char* args[]) {
    printf("viewportDistance %f\n",viewportDistance);
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
        return 1;
    }

    // Create window
    SDL_Window* window = SDL_CreateWindow("SDL Grid Approximation",
                                          SDL_WINDOWPOS_UNDEFINED,
                                          SDL_WINDOWPOS_UNDEFINED,
                                          SCREEN_WIDTH,
                                          SCREEN_HEIGHT,
                                          SDL_WINDOW_SHOWN);
    if (window == NULL) {
        printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    // Create renderer
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (renderer == NULL) {
        printf("Renderer could not be created! SDL_Error: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // Load the source image
    SDL_Surface* sourceSurface = SDL_LoadBMP("source_image.bmp");
    if (sourceSurface == NULL) {
        printf("Unable to load image! SDL_Error: %s\n", SDL_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

        // Get pixel format info for color conversion
    Uint32 rmask, gmask, bmask, amask;
    int bpp;
    if (!SDL_PixelFormatEnumToMasks(SDL_PIXELFORMAT_ARGB8888, &bpp, &rmask, &gmask, &bmask, &amask)) {
        printf("SDL_PixelFormatEnumToMasks failed! SDL_Error: %s\n", SDL_GetError());
        SDL_FreeSurface(sourceSurface);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // Create a surface for the pixel buffer (same size as the screen)
    SDL_Surface* surface = SDL_CreateRGBSurface(0, SCREEN_WIDTH, SCREEN_HEIGHT, 32, rmask, gmask, bmask, amask);
    if (surface == NULL) {
        printf("Surface could not be created! SDL_Error: %s\n", SDL_GetError());
        SDL_FreeSurface(sourceSurface);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // Get a pointer to the pixel buffer
    Uint32* pixels = (Uint32*)surface->pixels;

    // Define grid size
    int gridWidth = 8;
    int gridHeight = 8;

    // Iterate over each cell in the grid
for (int y = 0; y < SCREEN_HEIGHT; y += gridHeight) {
    for (int x = 0; x < SCREEN_WIDTH; x += gridWidth) {
        // Calculate the average color for the current grid cell
        Uint32 rTotal = 0;
        Uint32 gTotal = 0;
        Uint32 bTotal = 0;
        int pixelCount = 0;

        for (int dy = 0; dy < gridHeight; ++dy) {
            for (int dx = 0; dx < gridWidth; ++dx) {
                // Project 2D screen coordinates into 3D space
                float x3D = x + dx - SCREEN_WIDTH / 2;
                float y3D = y + dy - SCREEN_HEIGHT / 2;
                float z3D = viewportDistance;  // Assuming the point lies on the projection plane

                // Project the 3D point onto the 2D viewport
                float xProjected = (viewportDistance * x3D) / z3D;
                float yProjected = (viewportDistance * -y3D) / z3D;

                // Translate the projected point back to screen coordinates
                int screenX = SCREEN_WIDTH / 2 + (int)xProjected;
                int screenY = SCREEN_HEIGHT / 2 - (int)yProjected;

                if (screenX >= 0 && screenX < SCREEN_WIDTH && screenY >= 0 && screenY < SCREEN_HEIGHT) {
                    // Get pixel color from the source image
                    int srcX = screenX * sourceSurface->w / SCREEN_WIDTH;
                    int srcY = screenY * sourceSurface->h / SCREEN_HEIGHT;

                    Uint32* srcPixels = (Uint32*)sourceSurface->pixels;
                    Uint32 pixelColor = srcPixels[srcY * sourceSurface->w + srcX];

                    // Extract and accumulate color components
                    Uint8 r = (pixelColor & sourceSurface->format->Rmask) >> sourceSurface->format->Rshift;
                    Uint8 g = (pixelColor & sourceSurface->format->Gmask) >> sourceSurface->format->Gshift;
                    Uint8 b = (pixelColor & sourceSurface->format->Bmask) >> sourceSurface->format->Bshift;

                    rTotal += r;
                    gTotal += g;
                    bTotal += b;
                    pixelCount++;
                }
            }
        }

        // Calculate the average color by dividing by the number of pixels
        Uint8 rAvg = rTotal / pixelCount;
        Uint8 gAvg = gTotal / pixelCount;
        Uint8 bAvg = bTotal / pixelCount;

        // Combine the averaged components back into a single color
        Uint32 avgColor = (rAvg << 16) | (gAvg << 8) | bAvg;

        // Set all pixels in the current grid cell to the average color
        for (int dy = 0; dy < gridHeight; ++dy) {
            for (int dx = 0; dx < gridWidth; ++dx) {
                int screenX = x + dx;
                int screenY = y + dy;

                if (screenX >= 0 && screenX < SCREEN_WIDTH && screenY >= 0 && screenY < SCREEN_HEIGHT) {
                    pixels[screenY * SCREEN_WIDTH + screenX] = (255 << 24) | avgColor;
                }
            }
        }
    }
}


    // Create a texture from the surface (pixel buffer)
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (texture == NULL) {
        printf("Texture could not be created! SDL_Error: %s\n", SDL_GetError());
        SDL_FreeSurface(surface);
        SDL_FreeSurface(sourceSurface);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // Clear the screen
    SDL_RenderClear(renderer);


    // Draw a rectangle
    drawRectangle(pixels, 50, 50, 100, 50, (255 << 24) | (255 << 16) | (0 << 8) | 0);  // Red rectangle

    // Draw a line
    drawLine(pixels, 100, 100, 200, 150, (255 << 24) | (0 << 16) | (255 << 8) | 0);  // Green line


    // Copy the texture to the renderer
    SDL_RenderCopy(renderer, texture, NULL, NULL);

    // Update the screen
    SDL_RenderPresent(renderer);

    // Wait for 3 seconds to see the result
    SDL_Delay(9000);

    // Cleanup
    SDL_DestroyTexture(texture);
    SDL_FreeSurface(surface);
    SDL_FreeSurface(sourceSurface);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}

