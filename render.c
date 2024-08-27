#include <SDL2/SDL.h>
#include <stdio.h>
#include <math.h>
#include <stdbool.h>
#include <SDL2/SDL_ttf.h>

// Screen dimensions
const int SCREEN_WIDTH = 640;
const int SCREEN_HEIGHT = 480;
const float viewportDistance = 1.0f;

// Structure to represent a 3D point
typedef struct {
    float x, y, z;
} Vec3D;

// Structure to represent a camera
typedef struct {
    Vec3D position;
    float pitch;  // Rotation around x-axis
    float yaw;    // Rotation around y-axis
} Camera;
// Define the structure for a particle
typedef struct {
    Vec3D position;
    Vec3D velocity;
} Particle;

// Function to update particle positions and handle collisions
void updateParticles(Particle* particles, int numParticles, float deltaTime) {
    for (int i = 0; i < numParticles; i++) {
        // Update position based on velocity
        particles[i].position.x += particles[i].velocity.x * deltaTime;
        particles[i].position.y += particles[i].velocity.y * deltaTime;
        particles[i].position.z += particles[i].velocity.z * deltaTime;

        // Check for collision with cube walls and bounce
        if (particles[i].position.x <= -0.5f || particles[i].position.x >= 0.5f)
            particles[i].velocity.x *= -1.0f;
        if (particles[i].position.y <= -0.5f || particles[i].position.y >= 0.5f)
            particles[i].velocity.y *= -1.0f;
        if (particles[i].position.z <= -0.5f || particles[i].position.z >= 0.5f)
            particles[i].velocity.z *= -1.0f;
    }
}

// Function to apply rotation based on yaw and pitch
Vec3D rotate(Vec3D point, float pitch, float yaw) {
    Vec3D rotated;

    // Rotate around y-axis (yaw)
    rotated.x = cosf(yaw) * point.x - sinf(yaw) * point.z;
    rotated.z = sinf(yaw) * point.x + cosf(yaw) * point.z;
    rotated.y = point.y;

    // Rotate around x-axis (pitch)
    float tempY = rotated.y;
    rotated.y = cosf(pitch) * tempY - sinf(pitch) * rotated.z;
    rotated.z = sinf(pitch) * tempY + cosf(pitch) * rotated.z;

    return rotated;
}

// Function to project 3D points to 2D points, considering the camera position and rotation
void project(Camera camera, Vec3D point3D, int* x2D, int* y2D) {
    // Translate point based on camera position
    Vec3D translated = { point3D.x - camera.position.x, point3D.y - camera.position.y, point3D.z - camera.position.z };

    // Rotate point based on camera rotation (pitch, yaw)
    Vec3D rotated = rotate(translated, camera.pitch, camera.yaw);

    // Project onto 2D viewport
    *x2D = (int)((rotated.x / (rotated.z + viewportDistance)) * SCREEN_WIDTH / 2 + SCREEN_WIDTH / 2);
    *y2D = (int)((rotated.y / (rotated.z + viewportDistance)) * SCREEN_HEIGHT / 2 + SCREEN_HEIGHT / 2);
}

    // Function to draw a filled circle
void drawFilledCircle(Uint32* pixels, int centerX, int centerY, int radius, Uint32 color) {
    for (int y = -radius; y <= radius; y++) {
        for (int x = -radius; x <= radius; x++) {
            if (x * x + y * y <= radius * radius) {
                int drawX = centerX + x;
                int drawY = centerY + y;
                if (drawX >= 0 && drawX < SCREEN_WIDTH && drawY >= 0 && drawY < SCREEN_HEIGHT) {
                    pixels[drawY * SCREEN_WIDTH + drawX] = color;
                }
            }
        }
    }
}

// Function to render particles as spheres
void renderParticles(Uint32* pixels, Camera camera, Particle* particles, int numParticles, Uint32 color) {
    for (int i = 0; i < numParticles; i++) {
        int x2D, y2D;
        project(camera, particles[i].position, &x2D, &y2D);
        if (x2D >= 0 && x2D < SCREEN_WIDTH && y2D >= 0 && y2D < SCREEN_HEIGHT) {
            // Adjust radius based on the distance from the camera
            float distance = sqrtf(
                (particles[i].position.x - camera.position.x) * (particles[i].position.x - camera.position.x) +
                (particles[i].position.y - camera.position.y) * (particles[i].position.y - camera.position.y) +
                (particles[i].position.z - camera.position.z) * (particles[i].position.z - camera.position.z)
            );
            int radius = (int)(5 / distance); // scale the sphere size
            drawFilledCircle(pixels, x2D, y2D, radius, color);
        }
    }
}

// Function to draw a line between two 3D points
void drawLine3D(Uint32* pixels, Camera camera, Vec3D p1, Vec3D p2, Uint32 color) {
    int x1, y1, x2, y2;
    project(camera, p1, &x1, &y1);
    project(camera, p2, &x2, &y2);

    // Bresenham's algorithm
    int dx = abs(x2 - x1), sx = x1 < x2 ? 1 : -1;
    int dy = -abs(y2 - y1), sy = y1 < y2 ? 1 : -1;
    int err = dx + dy, e2;

    while (true) {
        if (x1 >= 0 && x1 < SCREEN_WIDTH && y1 >= 0 && y1 < SCREEN_HEIGHT)
            pixels[y1 * SCREEN_WIDTH + x1] = color;

        if (x1 == x2 && y1 == y2) break;
        e2 = 2 * err;
        if (e2 >= dy) { err += dy; x1 += sx; }
        if (e2 <= dx) { err += dx; y1 += sy; }
    }
}

// Function to move the camera in the direction it is facing
void moveCamera(Camera* camera, float forward, float strafe, float vertical) {
    camera->position.y += vertical;
    camera->position.x += forward * sinf(camera->yaw) + strafe * cosf(camera->yaw);
    camera->position.z += forward * cosf(camera->yaw) - strafe * sinf(camera->yaw);
}

// Function to render FPS counter
void renderFPS(SDL_Renderer* renderer, TTF_Font* font, int fps) {
    SDL_Color color = {255, 255, 255, 255}; // White color
    char fpsText[20];
    snprintf(fpsText, sizeof(fpsText), "FPS: %d", fps);
    
    SDL_Surface* textSurface = TTF_RenderText_Solid(font, fpsText, color);
    if (textSurface == NULL) {
        printf("Unable to render text surface! SDL_ttf Error: %s\n", TTF_GetError());
        return;
    }
    
    SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
    if (textTexture == NULL) {
        printf("Unable to create texture from rendered text! SDL Error: %s\n", SDL_GetError());
        SDL_FreeSurface(textSurface);
        return;
    }
    
    SDL_Rect renderQuad = { 10, 10, textSurface->w, textSurface->h };
    SDL_RenderCopy(renderer, textTexture, NULL, &renderQuad);
    
    SDL_DestroyTexture(textTexture);
    SDL_FreeSurface(textSurface);
}


int main(int argc, char* args[]) {
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
        return 1;
    }

    // Create window
    SDL_Window* window = SDL_CreateWindow("3D Rendering with POV Camera Movement in SDL",
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

    // Create a surface for the pixel buffer (same size as the screen)
    SDL_Surface* surface = SDL_CreateRGBSurface(0, SCREEN_WIDTH, SCREEN_HEIGHT, 32, 0, 0, 0, 0);
    if (surface == NULL) {
        printf("Surface could not be created! SDL_Error: %s\n", SDL_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // Initialize SDL_ttf
    if (TTF_Init() == -1) {
        printf("TTF_Init: %s\n", TTF_GetError());
        return 1;
    }

    // Load a font
    TTF_Font* font = TTF_OpenFont("Consolas.ttf", 24);
    if (!font) {
        printf("TTF_OpenFont: %s\n", TTF_GetError());
        return 1;
    }
    // Get a pointer to the pixel buffer
    Uint32* pixels = (Uint32*)surface->pixels;

    // Define the vertices of a 3D cube
    Vec3D cubeVertices[8] = {
        {-0.5f, -0.5f, -0.5f},
        { 0.5f, -0.5f, -0.5f},
        { 0.5f,  0.5f, -0.5f},
        {-0.5f,  0.5f, -0.5f},
        {-0.5f, -0.5f,  0.5f},
        { 0.5f, -0.5f,  0.5f},
        { 0.5f,  0.5f,  0.5f},
        {-0.5f,  0.5f,  0.5f}
    };

    // Define the edges of the cube (pairs of vertices)
    int edges[12][2] = {
        {0, 1}, {1, 2}, {2, 3}, {3, 0},  // Back face
        {4, 5}, {5, 6}, {6, 7}, {7, 4},  // Front face
        {0, 4}, {1, 5}, {2, 6}, {3, 7}   // Connecting edges
    };

    // Define the camera
    Camera camera = {{0.0f, 0.0f, -3.0f}, 0.0f, 0.0f};

      // Timing for FPS counter
    Uint32 startTime, endTime, frameCount = 0;
    Uint32 lastTime = SDL_GetTicks();
    int fps = 0;

    SDL_Color textColor = {255, 255, 255, 255};

    // Main loop flag
    int quit = 0;
    SDL_Event e;
 // Create particlesi
 
    int particlesSpawned = 100;
    int numParticles = 10000;
    Particle particles[numParticles];
    for (int i = 0; i <  particlesSpawned; i++) {
        particles[i].position = (Vec3D){rand() / (float)RAND_MAX - 0.5f, rand() / (float)RAND_MAX - 0.5f, rand() / (float)RAND_MAX - 0.5f};
        particles[i].velocity = (Vec3D){(rand() / (float)RAND_MAX - 0.5f) * 0.1f, (rand() / (float)RAND_MAX - 0.5f) * 0.1f, (rand() / (float)RAND_MAX - 0.5f) * 0.1f};
    }

    // Main loop
    while (!quit) {
        startTime = SDL_GetTicks();  // Get the start time of the frame        // Handle events
      
      	while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT) {
                quit = 1;
            } else if (e.type == SDL_KEYDOWN) {
                switch (e.key.keysym.sym) {
                    case SDLK_LEFT:
                        camera.yaw += 0.1f;  // Rotate left
                        break;
                    case SDLK_RIGHT:
                        camera.yaw -= 0.1f;  // Rotate right
                        break;
                    case SDLK_UP:
                        camera.pitch += 0.1f;  // Rotate up
                        break;
                    case SDLK_DOWN:
                        camera.pitch -= 0.1f;  // Rotate down
                        break;
                    case SDLK_w:
                        moveCamera(&camera, 0.1f, 0.0f, 0.0f);  // Move forward
                        break;
                    case SDLK_s:
                        moveCamera(&camera, -0.1f, 0.0f,0.0f);  // Move backward
                        break;
                    case SDLK_a:
                        moveCamera(&camera, 0.0f, -0.1f, 0.0f);  // Strafe left
                        break;
                    case SDLK_d:
                        moveCamera(&camera, 0.0f, 0.1f, 0.0f);  // Strafe right
                        break;
		    case SDLK_SPACE:
			moveCamera(&camera, 0.0f,0.0f, -0.1f);
			break;
		    case SDLK_z:
			moveCamera(&camera, 0.0f, 0.0f, 0.1f);
			break;
	            case SDLK_p:
                        particles[particlesSpawned].position = (Vec3D){rand() / (float)RAND_MAX - 0.5f, rand() / (float)RAND_MAX - 0.5f, rand() / (float)RAND_MAX - 0.5f};
                        particles[particlesSpawned].velocity = (Vec3D){(rand() / (float)RAND_MAX - 0.5f) * 0.1f, (rand() / (float)RAND_MAX - 0.5f) * 0.1f, (rand() / (float)RAND_MAX - 0.5f) * 0.1f};
                        particlesSpawned++;
		       	break;
		}
            }
        }

        // Clear the pixel buffer
        SDL_FillRect(surface, NULL, 0x00000000);

       	// Update particles
        updateParticles(particles, particlesSpawned, 0.016f);

        // Draw the cube
        Uint32 color = (255 << 24) | (255 << 16) | (255 << 8) | 255;  // White
        Uint32 redColor = (255 << 16) | (255 << 24);
        // Render the particles
        renderParticles(pixels, camera, particles, particlesSpawned, redColor);

        // Draw the cube
        for (int i = 0; i < 12; i++) {
            drawLine3D(pixels, camera, cubeVertices[edges[i][0]], cubeVertices[edges[i][1]], color);
        }

        // Update the texture with the current pixel buffer
        SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_RenderCopy(renderer, texture, NULL, NULL);
        SDL_RenderPresent(renderer);

        // Clean up the texture
        SDL_DestroyTexture(texture);

	frameCount++;
    // Update FPS counter every second
        endTime = SDL_GetTicks();
        if (endTime - lastTime >= 1000) {
            fps = frameCount;
            frameCount = 0;
            lastTime = endTime;
        }        // Render FPS counter
        renderFPS(renderer, font, fps);

        // Present the screen
        SDL_RenderPresent(renderer);

        // Update FPS counter every second
        endTime = SDL_GetTicks();
        if (endTime - lastTime >= 1000) {
            fps = frameCount;
            frameCount = 0;
            lastTime = endTime;
        }
        // Delay to control frame rate
        SDL_Delay(16);
    }

    TTF_CloseFont(font);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();

    return 0;
}

