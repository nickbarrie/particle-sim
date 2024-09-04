#include <SDL2/SDL.h>
#include <stdio.h>
#include <math.h>
#include <stdbool.h>
#include <SDL2/SDL_ttf.h>
#include <stdlib.h>
#include <stdint.h> 

// Screen dimensions
const int SCREEN_WIDTH = 640;
const int SCREEN_HEIGHT = 480;
const float viewportDistance = 2.0f;

// Define a light source direction (normalized)
Vec3D lightDir = {0.0f, 1.0f, -1.0f}; // Example: light coming from above and behind
float lightIntensity = 1.0f; // Maximum light intensity

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
    Uint32 color;
    float radius;
} Particle;


// Function to perform orthogonal projection of vector 'a' onto vector 'b'
Vec3D orthogonalProjection(Vec3D a, Vec3D b) {
    float bLengthSquared = b.x * b.x + b.y * b.y + b.z * b.z;
    float dotProduct = a.x * b.x + a.y * b.y + a.z * b.z;
    Vec3D projection = {
        (dotProduct / bLengthSquared) * b.x,
        (dotProduct / bLengthSquared) * b.y,
        (dotProduct / bLengthSquared) * b.z
    };
    return projection;
}

Uint32 generateRandomColor() {
    Uint8 red = rand() % 256;    // Random value between 0 and 255
    Uint8 green = rand() % 256;  // Random value between 0 and 255
    Uint8 blue = rand() % 256;   // Random value between 0 and 255

    // Combine the components into a single Uint32 color value (assuming an ARGB format with Alpha = 0xFF)
    Uint32 color = (0xFF << 24) | (red << 16) | (green << 8) | blue;

    return color;
}

// Function to subtract vector 'b' from vector 'a'
Vec3D subVector(Vec3D a, Vec3D b) {
    Vec3D result = { a.x - b.x, a.y - b.y, a.z - b.z };
    return result;
}

// Function to add  vector 'b' to  vector 'a'
Vec3D addVector(Vec3D a, Vec3D b) {
    Vec3D result = { a.x + b.x, a.y +  b.y, a.z + b.z };
    return result;
}

// Function to handle collision between two particles
void handleParticleCollision(Particle* p1, Particle* p2) {
    // Calculate the vector between the centers of the two particles
    Vec3D collisionDirection = {
        p2->position.x - p1->position.x,
        p2->position.y - p1->position.y,
        p2->position.z - p1->position.z
    };

    // Calculate the distance squared between the two particles
    float distanceSquared = 
        collisionDirection.x * collisionDirection.x +
        collisionDirection.y * collisionDirection.y +
        collisionDirection.z * collisionDirection.z;

    // Calculate the sum of the radii squared (assuming radius = 0.1 for both particles)
    float radiusSum = p1->radius + p2->radius;
    float radiusSumSquared = radiusSum * radiusSum;

    // Check if the distance is less than the sum of the radii
    if (distanceSquared <= radiusSumSquared) {
        // Normalize the collision direction
        float distance = sqrtf(distanceSquared);
        Vec3D normalizedCollisionDirection = {
            collisionDirection.x / distance,
            collisionDirection.y / distance,
            collisionDirection.z / distance
        };

        // Project velocities onto the collision direction
        Vec3D w1 = orthogonalProjection(p1->velocity, normalizedCollisionDirection);
        Vec3D w2 = orthogonalProjection(p2->velocity, normalizedCollisionDirection);
        Vec3D u1 = subVector(p1->velocity, w1);
        Vec3D u2 = subVector(p2->velocity, w2);

        // Swap the velocities along the collision direction
            particles[i].velocity.x *= -1.0f;
        } else if (particles[i].position.x >= 0.5f) {
            particles[i].position.x = 0.5f;
            particles[i].velocity.x *= -1.0f;
        }

        if (particles[i].position.y <= -0.5f) {
            particles[i].position.y = -0.5f;
            particles[i].velocity.y *= -1.0f;
        } else if (particles[i].position.y >= 0.5f) {
            particles[i].position.y = 0.5f;
            particles[i].velocity.y *= -1.0f;
        }

        if (particles[i].position.z <= -0.5f) {
            particles[i].position.z = -0.5f;
            particles[i].velocity.z *= -1.0f;
        } else if (particles[i].position.z >= 0.5f) {
            particles[i].position.z = 0.5f;
            particles[i].velocity.z *= -1.0f;
        }
    }
}



// Function to project 3D points to 2D points, considering the camera position and rotation
void project(Camera camera, Vec3D point3D, int* x2D, int* y2D) {
    // Translate point based on camera position
    Vec3D translated = {
        point3D.x - camera.position.x,
        point3D.y - camera.position.y,
        point3D.z - camera.position.z
    };

    // Rotate point based on camera rotation (pitch, yaw)
    Vec3D rotated = rotate(translated, camera.pitch, camera.yaw);

    // Apply perspective projection with correct FOV and distance scaling
    float focalLength = 1.0f;  // Controls the field of view
    float zFactor = (rotated.z + viewportDistance); // Adjust based on how far the object is

    if (zFactor > 0) {
        *x2D = (int)((focalLength * rotated.x / zFactor) * (SCREEN_WIDTH / 2)) + (SCREEN_WIDTH / 2);
        *y2D = (int)((focalLength * rotated.y / zFactor) * (SCREEN_HEIGHT / 2)) + (SCREEN_HEIGHT / 2);
    } else {
        *x2D = SCREEN_WIDTH / 2;
        *y2D = SCREEN_HEIGHT / 2;
    }
}

// Function to draw a filled circle with simple shading
void drawFilledCircleWithShading(Uint32* pixels, int centerX, int centerY, int radius, Uint32 color, Camera camera) {
    for (int y = -radius; y <= radius; y++) {
        for (int x = -radius; x <= radius; x++) {
            if (x * x + y * y <= radius * radius) {
                int drawX = centerX + x;
                int drawY = centerY + y;
                if (drawX >= 0 && drawX < SCREEN_WIDTH && drawY >= 0 && drawY < SCREEN_HEIGHT) {

                    // Calculate the normal at this point on the sphere's surface
                    Vec3D normal = {x / (float)radius, y / (float)radius, sqrtf(1.0f - (x * x + y * y) / (float)(radius * radius))};

                    // Rotate normal based on camera rotation
                    Vec3D transformedNormal = rotate(normal, camera.pitch, camera.yaw);

                    // Calculate the dot product of the normal and the light direction
                    float dot = transformedNormal.x * lightDir.x + transformedNormal.y * lightDir.y + transformedNormal.z * lightDir.z;
                    if (dot < 0) dot = 0; // Ensure no negative light intensity

		    // Adjust the color intensity based on the dot product and light intensity
		    Uint8 r = (Uint8)fminf(255.0f, ((color >> 16) & 0xFF) * dot * lightIntensity);
		    Uint8 g = (Uint8)fminf(255.0f, ((color >> 8) & 0xFF) * dot * lightIntensity);
		    Uint8 b = (Uint8)fminf(255.0f, (color & 0xFF) * dot * lightIntensity);

                    Uint32 shadedColor = (0xFF << 24) | (r << 16) | (g << 8) | b;
                    // Combine the new color components
                  

                    // Set the pixel with the shaded color
                    pixels[drawY * SCREEN_WIDTH + drawX] = shadedColor;
                }
            }
        }
    }
}
// Function to render particles as spheres
void renderParticles(Uint32* pixels, Camera camera, Particle* particles, int numParticles) {
    for (int i = 0; i < numParticles; i++) {
        int x2D, y2D;
        project(camera, particles[i].position, &x2D, &y2D);

        if (x2D >= 0 && x2D < SCREEN_WIDTH && y2D >= 0 && y2D < SCREEN_HEIGHT) {
            // Calculate distance from the camera
            float distance = sqrtf(
                (particles[i].position.x - camera.position.x) * (particles[i].position.x - camera.position.x) +
                (particles[i].position.y - camera.position.y) * (particles[i].position.y - camera.position.y) +
                (particles[i].position.z - camera.position.z) * (particles[i].position.z - camera.position.z)
            );
            // Adjust the sphere size based on distance
            // Here, we assume a base size and scale it with distance.
            float scaleFactor = viewportDistance*200/  (distance + 0.5f * viewportDistance); // +0.1f to avoid division by zero

            int radius = (int)(particles[i].radius * scaleFactor); // Scale radius by distance
//	    if (radius < 5) radius = 5;  // Minimum radius
  //          if (radius > 100) radius = 100; // Maximum radius

        drawFilledCircleWithShading(pixels, x2D, y2D, radius, particles[i].color, camera);
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

float generateRandomFloat() {
    return (float)rand() / (float)RAND_MAX;
}

// Function to move the camera in the direction it is facing
void moveCamera(Camera* camera, float forward, float strafe, float vertical) {
    camera->position.y += vertical;
    camera->position.x += forward * sinf(camera->yaw) + strafe * cosf(camera->yaw);
    camera->position.z += forward * cosf(camera->yaw) - strafe * sinf(camera->yaw);
}

// General-purpose function to draw text on the screen
void drawText(SDL_Renderer* renderer, TTF_Font* font, const char* text, int x, int y, SDL_Color color) {
    SDL_Surface* textSurface = TTF_RenderText_Solid(font, text, color);
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

    SDL_Rect renderQuad = { x, y, textSurface->w, textSurface->h };
    SDL_RenderCopy(renderer, textTexture, NULL, &renderQuad);

    SDL_DestroyTexture(textTexture);
    SDL_FreeSurface(textSurface);
}

// Example usage in renderCounts function
void renderCounts(SDL_Renderer* renderer, TTF_Font* font, int fps, int particles) {
    SDL_Color color = {255, 255, 255, 255}; // White color
    char fpsText[20];
    char particleText[20];
    snprintf(fpsText, sizeof(fpsText), "FPS: %d", fps);
    snprintf(particleText, sizeof(particleText),"Particles: %d", particles);

    drawText(renderer, font, fpsText, 10, 10, color);
    drawText(renderer, font, particleText, 10, 40, color);
}

void drawBoxOutline(Uint32* pixels, int x, int y, int width, int height, Uint32 color, int thickness) {
    // Draw top and bottom edges
    for (int t = 0; t < thickness; t++) {
        for (int i = x - t; i <= x + width + t; i++) {
            if (y - t >= 0 && y - t < SCREEN_HEIGHT && i >= 0 && i < SCREEN_WIDTH)
                pixels[(y - t) * SCREEN_WIDTH + i] = color;
            if (y + height + t >= 0 && y + height + t < SCREEN_HEIGHT && i >= 0 && i < SCREEN_WIDTH)
                pixels[(y + height + t) * SCREEN_WIDTH + i] = color;
        }
    }

    // Draw left and right edges
    for (int t = 0; t < thickness; t++) {
        for (int i = y - t; i <= y + height + t; i++) {
            if (x - t >= 0 && x - t < SCREEN_WIDTH && i >= 0 && i < SCREEN_HEIGHT)
                pixels[i * SCREEN_WIDTH + (x - t)] = color;
            if (x + width + t >= 0 && x + width + t < SCREEN_WIDTH && i >= 0 && i < SCREEN_HEIGHT)
                pixels[i * SCREEN_WIDTH + (x + width + t)] = color;
        }
    }
}


void displayParticleInfo(SDL_Renderer* renderer, TTF_Font* font, Particle particle, int x, int y) {
    SDL_Color color = {255, 255, 255, 255}; // White color

    // Prepare strings for each piece of particle information
    char posText[60];
    snprintf(posText, sizeof(posText), "Pos: (%.2f, %.2f, %.2f)", 
             particle.position.x, particle.position.y, particle.position.z);

    char velText[60];
    snprintf(velText, sizeof(velText), "Vel: (%.2f, %.2f, %.2f)", 
             particle.velocity.x, particle.velocity.y, particle.velocity.z);

    char radiusText[40];
    snprintf(radiusText, sizeof(radiusText), "Radius: %.2f", particle.radius);

    // Draw each line of information
    drawText(renderer, font, posText, x + 10 , y + 10, color);           // Position text at (x, y)
    drawText(renderer, font, velText, x + 10, y + 40, color);      // Velocity text slightly below position
    drawText(renderer, font, radiusText, x + 10, y + 70, color);   // Radius text slightly below velocity
}


void createParticle(Particle* particle, float velocity, Uint32 color){
	particle->position = (Vec3D){rand() / (float)RAND_MAX - 0.5f, rand() / (float)RAND_MAX - 0.5f, rand() / (float)RAND_MAX - 0.5f};
        particle->velocity = (Vec3D){(rand() / (float)RAND_MAX - 0.5f) * velocity, (rand() / (float)RAND_MAX - 0.5f) * velocity, (rand() / (float)RAND_MAX - 0.5f) * velocity};
        particle->radius = generateRandomFloat()/10;
        particle->color = color;
	particle->next = NULL;
}

void addParticle(Particle** head, Particle* newParticle) {
    if (*head == NULL) {
        *head = newParticle;
        newParticle->next = newParticle;
    } else {
        Particle* current = *head;

        while (current->next != *head) {
            current = current->next;
        }

        current->next = newParticle;

        newParticle->next = *head;

    }
}


Particle* handleMouseClick(int mouseX, int mouseY, Particle* particles, int numParticles, Camera camera) {
    Particle* currentParticle = NULL; // Initialize to NULL

    for (int i = 0; i < numParticles; i++) {
        int x2D, y2D;
        project(camera, particles[i].position, &x2D, &y2D);

        // Adjust radius based on the distance from the camera
        float distance = sqrtf(
            (particles[i].position.x - camera.position.x) * (particles[i].position.x - camera.position.x) +
            (particles[i].position.y - camera.position.y) * (particles[i].position.y - camera.position.y) +
            (particles[i].position.z - camera.position.z) * (particles[i].position.z - camera.position.z)
        );
        int radius = (int)(20 / distance); // Scale the sphere size

        // Check if the click is within the sphere
        int dx = mouseX - x2D;
        int dy = mouseY - y2D;
        if (dx * dx + dy * dy <= radius * radius) {
            currentParticle = &particles[i]; // Use address-of operator to return a pointer
            return currentParticle; // Exit the loop once a particle is selected
        }
    }

    return NULL; // Return NULL if no particle was selected
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
    Uint32* pixels = (Uint32*)surface->pixels;

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

    int edges[12][2] = {
        {0, 1}, {1, 2}, {2, 3}, {3, 0},  // Back face
        {4, 5}, {5, 6}, {6, 7}, {7, 4},  // Front face
        {0, 4}, {1, 5}, {2, 6}, {3, 7}   // Connecting edges
    };

    Camera camera = {{0.0f, 0.0f, -3.0f}, 0.0f, 0.0f};

    Uint32 startTime, endTime, frameCount = 0;
    Uint32 lastTime = SDL_GetTicks();
    int fps = 0;

    SDL_Color textColor = {255, 255, 255, 255};

    // Main loop flag
    int quit = 0;
    SDL_Event e;

    // Create particles
    int particlesSpawned = 2;
    int numParticles = 10000;


    int infoBoxWidth = 350;
    int infoBoxHeight = 100;

    int cubeEdges = 12;
    Particle* particles = (Particle*)malloc(numParticles * sizeof(Particle));



    if (particles == NULL) {
    fprintf(stderr, "Memory allocation failed!\n");
    return 1;
    }

