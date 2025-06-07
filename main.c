#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <SDL2/SDL.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// SSD1306 simulation
#define SSD1306_WIDTH 128
#define SSD1306_HEIGHT 64
#define SSD1306_BUFFER_SIZE (SSD1306_WIDTH * SSD1306_HEIGHT / 8)

static uint8_t ssd1306_buffer[SSD1306_BUFFER_SIZE];

// 3D Math structures
typedef struct {
    float x, y, z;
} Vector3;

typedef struct {
    float x, y;
} Vector2;

// Display scaling for PC
#define SCALE 8
#define WINDOW_WIDTH (SSD1306_WIDTH * SCALE)
#define WINDOW_HEIGHT (SSD1306_HEIGHT * SCALE)

// Animation and camera
static float camera_angle = 0.0f;
static Vector3 camera_pos = {0, 0, 150};

// Star field - fixed positions in 3D space
#define NUM_STARS 50
static Vector3 stars[NUM_STARS];
static int stars_initialized = 0;

// SSD1306 Buffer Functions
uint8_t* SSD1306_GetBuffer(void) {
    return ssd1306_buffer;
}

void SSD1306_Clear(void) {
    memset(ssd1306_buffer, 0, SSD1306_BUFFER_SIZE);
}

void SSD1306_SetPixel(int x, int y, uint8_t color) {
    if (x < 0 || x >= SSD1306_WIDTH || y < 0 || y >= SSD1306_HEIGHT) return;
    
    int byte_idx = x + (y / 8) * SSD1306_WIDTH;
    int bit_idx = y % 8;
    
    if (color) {
        ssd1306_buffer[byte_idx] |= (1 << bit_idx);
    } else {
        ssd1306_buffer[byte_idx] &= ~(1 << bit_idx);
    }
}

uint8_t SSD1306_GetPixel(int x, int y) {
    if (x < 0 || x >= SSD1306_WIDTH || y < 0 || y >= SSD1306_HEIGHT) return 0;
    
    int byte_idx = x + (y / 8) * SSD1306_WIDTH;
    int bit_idx = y % 8;
    
    return (ssd1306_buffer[byte_idx] >> bit_idx) & 1;
}

// Forward declaration
Vector3 transformPointLookAt(Vector3 point, Vector3 camera, Vector3 target);

// 3D to 2D projection with look-at camera
Vector2 project3D(Vector3 point, Vector3 camera, Vector3 target) {
    Vector2 result;
    
    // Transform point with look-at camera
    Vector3 transformed = transformPointLookAt(point, camera, target);
    
    float distance = 80.0f;
    if (transformed.z <= 0) {
        // Point is behind camera
        result.x = -1000.0f;
        result.y = -1000.0f;
        return result;
    }
    
    float perspective = distance / transformed.z;
    result.x = transformed.x * perspective + SSD1306_WIDTH / 2;
    result.y = transformed.y * perspective + SSD1306_HEIGHT / 2;
    return result;
}

// Rotate point around Y-axis
Vector3 rotateY(Vector3 point, float angle) {
    Vector3 result;
    float cos_a = cosf(angle);
    float sin_a = sinf(angle);
    
    result.x = point.x * cos_a - point.z * sin_a;
    result.y = point.y;
    result.z = point.x * sin_a + point.z * cos_a;
    
    return result;
}

// Rotate point around X-axis
Vector3 rotateX(Vector3 point, float angle) {
    Vector3 result;
    float cos_a = cosf(angle);
    float sin_a = sinf(angle);
    
    result.x = point.x;
    result.y = point.y * cos_a - point.z * sin_a;
    result.z = point.y * sin_a + point.z * cos_a;
    
    return result;
}

// Create proper look-at transformation
Vector3 transformPointLookAt(Vector3 point, Vector3 camera, Vector3 target) {
    // Calculate forward vector (from camera to target)
    Vector3 forward = {
        target.x - camera.x,
        target.y - camera.y,
        target.z - camera.z
    };
    
    // Normalize forward vector
    float len = sqrtf(forward.x*forward.x + forward.y*forward.y + forward.z*forward.z);
    if (len > 0.001f) {
        forward.x /= len;
        forward.y /= len;
        forward.z /= len;
    } else {
        forward.x = 0; forward.y = 0; forward.z = 1;
    }
    
    // Up vector (world up)
    Vector3 up = {0, 1, 0};
    
    // Calculate right vector (cross product of forward and up)
    Vector3 right = {
        forward.y * up.z - forward.z * up.y,
        forward.z * up.x - forward.x * up.z,
        forward.x * up.y - forward.y * up.x
    };
    
    // Normalize right vector
    len = sqrtf(right.x*right.x + right.y*right.y + right.z*right.z);
    if (len > 0.001f) {
        right.x /= len;
        right.y /= len;
        right.z /= len;
    }
    
    // Recalculate up vector (cross product of right and forward)
    up.x = right.y * forward.z - right.z * forward.y;
    up.y = right.z * forward.x - right.x * forward.z;
    up.z = right.x * forward.y - right.y * forward.x;
    
    // Transform point to camera space
    Vector3 relative = {point.x - camera.x, point.y - camera.y, point.z - camera.z};
    
    // Apply look-at transformation matrix
    Vector3 result;
    result.x = relative.x * right.x + relative.y * right.y + relative.z * right.z;
    result.y = relative.x * up.x + relative.y * up.y + relative.z * up.z;
    result.z = relative.x * forward.x + relative.y * forward.y + relative.z * forward.z;
    
    return result;
}

// Initialize star field
void initStars(void) {
    for (int i = 0; i < NUM_STARS; i++) {
        // Random positions in a large sphere around the origin
        stars[i].x = (float)(rand() % 400 - 200);
        stars[i].y = (float)(rand() % 400 - 200);
        stars[i].z = (float)(rand() % 400 - 200);
        
        // Ensure stars are far enough from Saturn
        float dist = sqrtf(stars[i].x*stars[i].x + stars[i].y*stars[i].y + stars[i].z*stars[i].z);
        if (dist < 50) {
            stars[i].x *= 2.0f;
            stars[i].y *= 2.0f;
            stars[i].z *= 2.0f;
        }
    }
    stars_initialized = 1;
}

// Draw circle (for Saturn sphere)
void drawCircle(int cx, int cy, int radius) {
    for (int y = -radius; y <= radius; y++) {
        for (int x = -radius; x <= radius; x++) {
            if (x*x + y*y <= radius*radius) {
                SSD1306_SetPixel(cx + x, cy + y, 1);
            }
        }
    }
}

// Draw ring (ellipse) with 3D points and proper projection
void drawRing3D(Vector3 center, float inner_radius, float outer_radius, float tilt_angle, Vector3 camera, Vector3 target) {
    // Generate ring points in 3D space
    for (int angle = 0; angle < 360; angle += 3) {
        float rad = (float)angle * M_PI / 180.0f;
        
        // Create points for inner and outer ring
        for (float r = inner_radius; r <= outer_radius; r += 1.0f) {
            Vector3 ring_point = {
                center.x + r * cosf(rad),
                center.y,
                center.z + r * sinf(rad)
            };
            
            // Apply ring tilt around X-axis
            ring_point = rotateX(ring_point, tilt_angle);
            
            // Project to screen
            Vector2 projected = project3D(ring_point, camera, target);
            
            if (projected.x >= 0 && projected.x < SSD1306_WIDTH && 
                projected.y >= 0 && projected.y < SSD1306_HEIGHT) {
                SSD1306_SetPixel((int)projected.x, (int)projected.y, 1);
            }
        }
    }
}

// Render the scene
void renderScene(void) {
    SSD1306_Clear();
    
    // Initialize stars on first run
    if (!stars_initialized) {
        initStars();
    }
    
    // Camera orbits in X-Z plane (horizontal orbit)
    camera_pos.x = 100.0f * cosf(camera_angle);
    camera_pos.y = 0.0f;  // Keep camera at same height
    camera_pos.z = 100.0f * sinf(camera_angle);
    
    // Saturn is always at the origin
    Vector3 saturn_center = {0, 0, 0};
    
    // Render stars first (background)
    for (int i = 0; i < NUM_STARS; i++) {
        Vector2 star_2d = project3D(stars[i], camera_pos, saturn_center);
        
        if (star_2d.x >= 0 && star_2d.x < SSD1306_WIDTH && 
            star_2d.y >= 0 && star_2d.y < SSD1306_HEIGHT) {
            SSD1306_SetPixel((int)star_2d.x, (int)star_2d.y, 1);
        }
    }
    
    // Saturn body (sphere) - draw individual points in 3D
    int saturn_radius = 12;
    for (int phi = 0; phi < 180; phi += 10) {
        for (int theta = 0; theta < 360; theta += 10) {
            float phi_rad = (float)phi * M_PI / 180.0f;
            float theta_rad = (float)theta * M_PI / 180.0f;
            
            Vector3 sphere_point = {
                saturn_center.x + saturn_radius * sinf(phi_rad) * cosf(theta_rad),
                saturn_center.y + saturn_radius * cosf(phi_rad),
                saturn_center.z + saturn_radius * sinf(phi_rad) * sinf(theta_rad)
            };
            
            Vector2 projected = project3D(sphere_point, camera_pos, saturn_center);
            if (projected.x >= 0 && projected.x < SSD1306_WIDTH && 
                projected.y >= 0 && projected.y < SSD1306_HEIGHT) {
                SSD1306_SetPixel((int)projected.x, (int)projected.y, 1);
            }
        }
    }
    
    // Saturn rings - tilted like real Saturn (about 27 degrees)
    float ring_tilt = 27.0f * M_PI / 180.0f;
    
    // Draw multiple rings with different radii
    drawRing3D(saturn_center, 18, 20, ring_tilt, camera_pos, saturn_center);
    drawRing3D(saturn_center, 22, 24, ring_tilt, camera_pos, saturn_center);
    drawRing3D(saturn_center, 26, 28, ring_tilt, camera_pos, saturn_center);
}

// SDL Display Functions
void renderToSDL(SDL_Renderer* renderer) {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    
    for (int y = 0; y < SSD1306_HEIGHT; y++) {
        for (int x = 0; x < SSD1306_WIDTH; x++) {
            if (SSD1306_GetPixel(x, y)) {
                SDL_Rect rect = {x * SCALE, y * SCALE, SCALE, SCALE};
                SDL_RenderFillRect(renderer, &rect);
            }
        }
    }
    
    SDL_RenderPresent(renderer);
}

int main(int argc, char* argv[]) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
        return 1;
    }
    
    SDL_Window* window = SDL_CreateWindow("Saturn Renderer Prototype",
                                         SDL_WINDOWPOS_UNDEFINED,
                                         SDL_WINDOWPOS_UNDEFINED,
                                         WINDOW_WIDTH,
                                         WINDOW_HEIGHT,
                                         SDL_WINDOW_SHOWN);
    
    if (!window) {
        printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }
    
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        printf("Renderer could not be created! SDL_Error: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    
    printf("Saturn Renderer Prototype\n");
    printf("Controls: ESC to quit\n");
    printf("Display: %dx%d pixels (simulating SSD1306)\n", SSD1306_WIDTH, SSD1306_HEIGHT);
    
    int running = 1;
    SDL_Event e;
    
    while (running) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT || (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE)) {
                running = 0;
            }
        }
        
        // Update camera animation (slow orbit)
        camera_angle += 0.01f;
        
        // Render the scene
        renderScene();
        
        // Display
        renderToSDL(renderer);
        
        SDL_Delay(50); // ~20 FPS
    }
    
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    
    return 0;
} 