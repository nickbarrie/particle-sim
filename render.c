#include <SDL2/SDL.h>
#include <stdio.h>
#include <math.h>
#include <stdbool.h>
#include <SDL2/SDL_ttf.h>
#include <stdlib.h>
#include <stdint.h> 
#include "vector.h"

// Screen dimensions
int SCREEN_WIDTH = 640;
int SCREEN_HEIGHT = 480;
const float viewportDistance = 1.0f;
bool paused = false;
float  gravity = 0.0f;

Vec3D lightDir = {0.0f,-1.0f, 1.0f}; // Example: light coming from above and behind
float lightIntensity = 1.0f; // Maximum light intensity


typedef struct {
    Vec3D position;
    float pitch;  // Rotation around x-axis
    float yaw;    // Rotation around y-axis
} Camera;

typedef struct {
    Vec3D position;
    Vec3D velocity;
    Uint32 color;
    float radius;
    struct Particle* next;
} Particle;


Uint32 generateRandomColor() {
    Uint8 red = rand() % 256;    // Random value between 0 and 255
    Uint8 green = rand() % 256;  // Random value between 0 and 255
    Uint8 blue = rand() % 256;   // Random value between 0 and 255

    // Combine the components into a single Uint32 color value (assuming an ARGB format with Alpha = 0xFF)
    Uint32 color = (0xFF << 24) | (red << 16) | (green << 8) | blue;

    return color;
}

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
    // Calculate the sum of the radii squared
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
        // Calculate overlap and separate the particles
        float overlap = radiusSum - distance;
        p1->position.x -= normalizedCollisionDirection.x * overlap * 0.5f;
        p1->position.y -= normalizedCollisionDirection.y * overlap * 0.5f;
        p1->position.z -= normalizedCollisionDirection.z * overlap * 0.5f;
        p2->position.x += normalizedCollisionDirection.x * overlap * 0.5f;
        p2->position.y += normalizedCollisionDirection.y * overlap * 0.5f;
        p2->position.z += normalizedCollisionDirection.z * overlap * 0.5f;
        // Project velocities onto the collision direction
        Vec3D w1 = orthogonalProjection(p1->velocity, normalizedCollisionDirection);
        Vec3D w2 = orthogonalProjection(p2->velocity, normalizedCollisionDirection);
        Vec3D u1 = subVector(p1->velocity, w1);
        Vec3D u2 = subVector(p2->velocity, w2);
        // Swap the velocities along the collision direction
        p1->velocity = addVector(u1, w2);
        p2->velocity = addVector(u2, w1);
    }
}

void updateParticles(Particle* particles, int numParticles, float deltaTime) {
    for (int i = 0; i < numParticles; i++) {
        // Update position based on velocity
        particles[i].position.x += particles[i].velocity.x * deltaTime;
        particles[i].position.y += particles[i].velocity.y * deltaTime;
        particles[i].position.z += particles[i].velocity.z * deltaTime;
        // Handle particle collisions
        for (int j = i + 1; j < numParticles; j++) {
            handleParticleCollision(&particles[i], &particles[j]);
        }


	if(particles[i].position.y <= 0.5f){
            particles[i].velocity.y += gravity;
	}
        // Check for collision with cube walls and bounce
        if (particles[i].position.x <= -0.5f) {
            particles[i].position.x = -0.5f;
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
            float scaleFactor = viewportDistance*SCREEN_HEIGHT/2 /  (distance  + 0.1f * viewportDistance); // +0.1f to avoid division by zero

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

    char velocityComponentText[60]; 
    snprintf(velocityComponentText, sizeof(velocityComponentText), "Velocity comp. (X, Y, Z)"); 

    char componentVelocityText[60];
    snprintf(componentVelocityText, sizeof(componentVelocityText), "(%.2f, %.2f, %.2f)", 
             particle.velocity.x, particle.velocity.y, particle.velocity.z);

    char velocityText[60];
	    snprintf(velocityText, sizeof(velocityText), "Velocity: (%.2f)", magnitudeVec3D(particle.velocity));

    char radiusText[40];
    snprintf(radiusText, sizeof(radiusText), "Radius: %.2f", particle.radius);

    // Draw each line of information
    drawText(renderer, font, posText, x + 10 , y + 10, color);           
    drawText(renderer, font, velocityComponentText, x + 10, y + 40, color);      
    drawText(renderer, font, componentVelocityText, x + 10, y + 70, color);
    drawText(renderer, font,velocityText, x + 10, y + 100, color);      
    drawText(renderer, font, radiusText, x + 10, y + 130, color);   
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
    printf("null");
    return NULL; // Return NULL if no particle was selected
}



int main(int argc, char* args[]) {
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
        return 1;
    }

    // Create window
    SDL_Window* window = SDL_CreateWindow("3D Rendering of Particles in a cube",
                                          SDL_WINDOWPOS_UNDEFINED,
                                          SDL_WINDOWPOS_UNDEFINED,
                                          SCREEN_WIDTH,
                                          SCREEN_HEIGHT,
                                          SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
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
    int infoBoxHeight = 160;

    int cubeEdges = 12;

    Uint32 startTicks, endTicks;

    Particle* particles = (Particle*)malloc(numParticles * sizeof(Particle));



    if (particles == NULL) {
    fprintf(stderr, "Memory allocation failed!\n");
    return 1;
    }


    for (int i = 0; i <  particlesSpawned; i++) {
	    createParticle(&particles[i], 2.0f, generateRandomColor());
        if (i > 0) {
            particles[i-1].next = &particles[i];
        }
    }

    particles[particlesSpawned-1].next =  &particles[0];

    Particle* selectedParticle = &particles[0];

    Particle* head = &particles[0];
    // Main loop
    while (!quit) {
        startTime = SDL_GetTicks();

      	while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT) {
                quit = 1;
            } else if (e.type == SDL_KEYDOWN) {
                switch (e.key.keysym.sym) {
                    case SDLK_LEFT:
                        camera.yaw -= 0.1f;  // Rotate left
                        break;
                    case SDLK_RIGHT:
                        camera.yaw += 0.1f;  // Rotate right
                        break;
                    case SDLK_UP:
                        camera.pitch -= 0.1f;  // Rotate up
                        break;
                    case SDLK_DOWN:
                        camera.pitch += 0.1f;  // Rotate down
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
			moveCamera(&camera, 0.0f,0.0f, -0.1f);  // Move up
			break;
		    case SDLK_z:
			moveCamera(&camera, 0.0f, 0.0f, 0.1f);  // Move down
			break;
		    case SDLK_g:
			if(gravity == 0.0f){
			    gravity = 0.1f;
			}
			else {
		            gravity = 0.0f;
			}
                        break;			
	            case SDLK_p:  // Spawn particle
		       	createParticle(&particles[particlesSpawned], 2.0f, generateRandomColor());
			addParticle(&head, &particles[particlesSpawned]);
			particlesSpawned++;
		       	break;
		    case SDLK_ESCAPE:
			paused = !paused;
			break;
		    case SDLK_n:
			selectedParticle = selectedParticle->next;
	                break;
		}
            }
	  else if (e.type == SDL_MOUSEBUTTONDOWN) {
		    int mouseX, mouseY;
	    	    SDL_GetMouseState(&mouseX, &mouseY);
		    selectedParticle = handleMouseClick(mouseX, mouseY, particles, numParticles, camera);
	    }

	  else if (e.type == SDL_WINDOWEVENT) {
            if (e.window.event == SDL_WINDOWEVENT_RESIZED) {
                SCREEN_WIDTH = e.window.data1;
                SCREEN_HEIGHT = e.window.data2;

		surface = SDL_GetWindowSurface(window);

                pixels = (Uint32*)surface->pixels;
            }
        }

	
        }

        SDL_FillRect(surface, NULL, 0x00000000);


        Uint32 color = (255 << 24) | (255 << 16) | (255 << 8) | 255;  // White


        for (int lineNumber = 0; lineNumber < cubeEdges; lineNumber++) {
            drawLine3D(pixels, camera, cubeVertices[edges[lineNumber][0]], cubeVertices[edges[lineNumber][1]], color);
        }


//	startTicks = SDL_GetTicks();
	renderParticles(pixels, camera, particles, particlesSpawned);

//	endTicks = SDL_GetTicks();
//	printf("Rendering took %d ms\n", endTicks - startTicks);

         if (selectedParticle->next != NULL) {
                drawBoxOutline(pixels, SCREEN_WIDTH-infoBoxWidth -10, 10, infoBoxWidth, infoBoxHeight, selectedParticle->color, 5);
        }

	if(!paused){
//		startTicks = SDL_GetTicks();
		updateParticles(particles, particlesSpawned, 0.016f);
//		endTicks = SDL_GetTicks();
//		printf("Physics update took %d ms\n", endTicks - startTicks);
	}
        SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_RenderCopy(renderer, texture, NULL, NULL);

        SDL_DestroyTexture(texture);

	frameCount++;
        endTime = SDL_GetTicks();
        if (endTime - lastTime >= 1000) {
            fps = frameCount;
            frameCount = 0;
            lastTime = endTime;
	}
	renderCounts(renderer, font, fps, particlesSpawned);


// for some reason text must go later
	if (selectedParticle->next != NULL) {
                displayParticleInfo(renderer, font, *selectedParticle, SCREEN_WIDTH-infoBoxWidth -10, 10);
        }


        SDL_RenderPresent(renderer);

        SDL_Delay(16);
    }

    TTF_CloseFont(font);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();
    free(particles);
    return 0;
}

