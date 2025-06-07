#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <SDL2/SDL.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Fixed-point math (16.16 format)
#define FIXED_SHIFT 16
#define FIXED_ONE (1 << FIXED_SHIFT)
#define FLOAT_TO_FIXED(x) ((int32_t)((x) * FIXED_ONE))
#define FIXED_TO_INT(x) ((x) >> FIXED_SHIFT)
#define INT_TO_FIXED(x) ((x) << FIXED_SHIFT)
#define FIXED_MUL(a, b) (((int64_t)(a) * (b)) >> FIXED_SHIFT)
#define FIXED_DIV(a, b) (((int64_t)(a) << FIXED_SHIFT) / (b))

// Fixed-point constants
#define FIXED_PI FLOAT_TO_FIXED(M_PI)
#define FIXED_PI_2 FLOAT_TO_FIXED(M_PI / 2.0)
#define FIXED_180 FLOAT_TO_FIXED(180.0)

// SSD1306 simulation
#define SSD1306_WIDTH 128
#define SSD1306_HEIGHT 64
#define SSD1306_BUFFER_SIZE (SSD1306_WIDTH * SSD1306_HEIGHT / 8)

static uint8_t ssd1306_buffer[SSD1306_BUFFER_SIZE];

// 3D Math structures using fixed-point
typedef struct {
    int32_t x, y, z;
} Vector3;

typedef struct {
    int32_t x, y;
} Vector2;

// Display scaling for PC
#define SCALE 8
#define WINDOW_WIDTH (SSD1306_WIDTH * SCALE)
#define WINDOW_HEIGHT (SSD1306_HEIGHT * SCALE)

// Fixed-point sine lookup table (256 entries for 0 to 2*PI)
#define SIN_TABLE_SIZE 256
static const int32_t sin_table[SIN_TABLE_SIZE] = {
    0, 1608, 3216, 4821, 6424, 8022, 9616, 11204, 12785, 14359, 15924, 17479, 19024, 20557, 22078, 23586,
    25080, 26558, 28020, 29466, 30893, 32302, 33692, 35061, 36410, 37736, 39040, 40320, 41576, 42806, 44011, 45190,
    46341, 47464, 48559, 49624, 50660, 51665, 52639, 53581, 54491, 55368, 56212, 57022, 57798, 58538, 59244, 59914,
    60547, 61145, 61705, 62228, 62714, 63162, 63572, 63944, 64277, 64571, 64827, 65043, 65220, 65358, 65457, 65516,
    65536, 65516, 65457, 65358, 65220, 65043, 64827, 64571, 64277, 63944, 63572, 63162, 62714, 62228, 61705, 61145,
    60547, 59914, 59244, 58538, 57798, 57022, 56212, 55368, 54491, 53581, 52639, 51665, 50660, 49624, 48559, 47464,
    46341, 45190, 44011, 42806, 41576, 40320, 39040, 37736, 36410, 35061, 33692, 32302, 30893, 29466, 28020, 26558,
    25080, 23586, 22078, 20557, 19024, 17479, 15924, 14359, 12785, 11204, 9616, 8022, 6424, 4821, 3216, 1608,
    0, -1608, -3216, -4821, -6424, -8022, -9616, -11204, -12785, -14359, -15924, -17479, -19024, -20557, -22078, -23586,
    -25080, -26558, -28020, -29466, -30893, -32302, -33692, -35061, -36410, -37736, -39040, -40320, -41576, -42806, -44011, -45190,
    -46341, -47464, -48559, -49624, -50660, -51665, -52639, -53581, -54491, -55368, -56212, -57022, -57798, -58538, -59244, -59914,
    -60547, -61145, -61705, -62228, -62714, -63162, -63572, -63944, -64277, -64571, -64827, -65043, -65220, -65358, -65457, -65516,
    -65536, -65516, -65457, -65358, -65220, -65043, -64827, -64571, -64277, -63944, -63572, -63162, -62714, -62228, -61705, -61145,
    -60547, -59914, -59244, -58538, -57798, -57022, -56212, -55368, -54491, -53581, -52639, -51665, -50660, -49624, -48559, -47464,
    -46341, -45190, -44011, -42806, -41576, -40320, -39040, -37736, -36410, -35061, -33692, -32302, -30893, -29466, -28020, -26558,
    -25080, -23586, -22078, -20557, -19024, -17479, -15924, -14359, -12785, -11204, -9616, -8022, -6424, -4821, -3216, -1608
};

int32_t fixed_sin(int32_t angle) {
    // Normalize angle to 0 to 2*PI range
    while (angle < 0) angle += 2 * FIXED_PI;
    while (angle >= 2 * FIXED_PI) angle -= 2 * FIXED_PI;
    
    // Convert angle to table index (0 to SIN_TABLE_SIZE-1)
    int32_t index = FIXED_MUL(angle, FLOAT_TO_FIXED(SIN_TABLE_SIZE)) / (2 * FIXED_PI);
    if (index >= SIN_TABLE_SIZE) index = SIN_TABLE_SIZE - 1;
    
    return sin_table[index];
}

int32_t fixed_cos(int32_t angle) {
    // cos(x) = sin(x + PI/2)
    return fixed_sin(angle + FIXED_PI_2);
}

int32_t fixed_sqrt(int32_t value) {
    if (value <= 0) return 0;
    
    // Newton-Raphson method for square root
    int32_t guess = value >> 1;
    if (guess == 0) guess = 1;
    
    for (int i = 0; i < 10; i++) {
        int32_t new_guess = (guess + FIXED_DIV(value, guess)) >> 1;
        if (new_guess >= guess) break;
        guess = new_guess;
    }
    return guess;
}

// Animation and camera
static int32_t camera_angle = 0;
static Vector3 camera_pos = {0, 0, 0}; // Will be set in renderScene

// Saturn and Ring Configuration
#define SATURN_RADIUS 20.0f
#define RING_1_INNER 18.0f
#define RING_1_OUTER 20.0f
#define RING_2_INNER 22.0f
#define RING_2_OUTER 24.0f
#define RING_3_INNER 26.0f
#define RING_3_OUTER 28.0f
#define RING_TILT_DEGREES 27.0f

// Camera Configuration
#define CAMERA_DISTANCE 70.0f
#define CAMERA_ROTATION_TIME_SECONDS 20.0f
#define TARGET_FPS 30.0f
#define PROJECTION_DISTANCE 80.0f

// Star field - fixed positions in 3D space
#define NUM_STARS 100
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
    
    int32_t distance = FLOAT_TO_FIXED(PROJECTION_DISTANCE);
    if (transformed.z <= 0) {
        // Point is behind camera
        result.x = INT_TO_FIXED(-1000);
        result.y = INT_TO_FIXED(-1000);
        return result;
    }
    
    int32_t perspective = FIXED_DIV(distance, transformed.z);
    result.x = FIXED_MUL(transformed.x, perspective) + INT_TO_FIXED(SSD1306_WIDTH / 2);
    result.y = FIXED_MUL(transformed.y, perspective) + INT_TO_FIXED(SSD1306_HEIGHT / 2);
    return result;
}

// Rotate point around Y-axis
Vector3 rotateY(Vector3 point, int32_t angle) {
    Vector3 result;
    int32_t cos_a = fixed_cos(angle);
    int32_t sin_a = fixed_sin(angle);
    
    result.x = FIXED_MUL(point.x, cos_a) - FIXED_MUL(point.z, sin_a);
    result.y = point.y;
    result.z = FIXED_MUL(point.x, sin_a) + FIXED_MUL(point.z, cos_a);
    
    return result;
}

// Rotate point around X-axis
Vector3 rotateX(Vector3 point, int32_t angle) {
    Vector3 result;
    int32_t cos_a = fixed_cos(angle);
    int32_t sin_a = fixed_sin(angle);
    
    result.x = point.x;
    result.y = FIXED_MUL(point.y, cos_a) - FIXED_MUL(point.z, sin_a);
    result.z = FIXED_MUL(point.y, sin_a) + FIXED_MUL(point.z, cos_a);
    
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
    int32_t len_sq = FIXED_MUL(forward.x, forward.x) + FIXED_MUL(forward.y, forward.y) + FIXED_MUL(forward.z, forward.z);
    int32_t len = fixed_sqrt(len_sq);
    if (len > FLOAT_TO_FIXED(0.001)) {
        forward.x = FIXED_DIV(forward.x, len);
        forward.y = FIXED_DIV(forward.y, len);
        forward.z = FIXED_DIV(forward.z, len);
    } else {
        forward.x = 0; forward.y = 0; forward.z = FIXED_ONE;
    }
    
    // Up vector (world up)
    Vector3 up = {0, FIXED_ONE, 0};
    
    // Calculate right vector (cross product of forward and up)
    Vector3 right = {
        FIXED_MUL(forward.y, up.z) - FIXED_MUL(forward.z, up.y),
        FIXED_MUL(forward.z, up.x) - FIXED_MUL(forward.x, up.z),
        FIXED_MUL(forward.x, up.y) - FIXED_MUL(forward.y, up.x)
    };
    
    // Normalize right vector
    len_sq = FIXED_MUL(right.x, right.x) + FIXED_MUL(right.y, right.y) + FIXED_MUL(right.z, right.z);
    len = fixed_sqrt(len_sq);
    if (len > FLOAT_TO_FIXED(0.001)) {
        right.x = FIXED_DIV(right.x, len);
        right.y = FIXED_DIV(right.y, len);
        right.z = FIXED_DIV(right.z, len);
    }
    
    // Recalculate up vector (cross product of right and forward)
    up.x = FIXED_MUL(right.y, forward.z) - FIXED_MUL(right.z, forward.y);
    up.y = FIXED_MUL(right.z, forward.x) - FIXED_MUL(right.x, forward.z);
    up.z = FIXED_MUL(right.x, forward.y) - FIXED_MUL(right.y, forward.x);
    
    // Transform point to camera space
    Vector3 relative = {point.x - camera.x, point.y - camera.y, point.z - camera.z};
    
    // Apply look-at transformation matrix
    Vector3 result;
    result.x = FIXED_MUL(relative.x, right.x) + FIXED_MUL(relative.y, right.y) + FIXED_MUL(relative.z, right.z);
    result.y = FIXED_MUL(relative.x, up.x) + FIXED_MUL(relative.y, up.y) + FIXED_MUL(relative.z, up.z);
    result.z = FIXED_MUL(relative.x, forward.x) + FIXED_MUL(relative.y, forward.y) + FIXED_MUL(relative.z, forward.z);
    
    return result;
}

// Initialize star field
void initStars(void) {
    for (int i = 0; i < NUM_STARS; i++) {
        // Random positions in a large sphere around the origin
        stars[i].x = INT_TO_FIXED(rand() % 400 - 200);
        stars[i].y = INT_TO_FIXED(rand() % 400 - 200);
        stars[i].z = INT_TO_FIXED(rand() % 400 - 200);
        
        // Ensure stars are far enough from Saturn
        int32_t dist_sq = FIXED_MUL(stars[i].x, stars[i].x) + FIXED_MUL(stars[i].y, stars[i].y) + FIXED_MUL(stars[i].z, stars[i].z);
        int32_t dist = fixed_sqrt(dist_sq);
        if (dist < FLOAT_TO_FIXED(50)) {
            stars[i].x = FIXED_MUL(stars[i].x, INT_TO_FIXED(2));
            stars[i].y = FIXED_MUL(stars[i].y, INT_TO_FIXED(2));
            stars[i].z = FIXED_MUL(stars[i].z, INT_TO_FIXED(2));
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
void drawRing3D(Vector3 center, int32_t inner_radius, int32_t outer_radius, int32_t tilt_angle, Vector3 camera, Vector3 target) {
    // Generate ring points in 3D space
    for (int angle = 0; angle < 360; angle += 3) {
        int32_t rad = FIXED_DIV(FIXED_MUL(INT_TO_FIXED(angle), FIXED_PI), FIXED_180);
        
        // Create points for inner and outer ring
        for (int32_t r = inner_radius; r <= outer_radius; r += FIXED_ONE) {
            Vector3 ring_point = {
                center.x + FIXED_MUL(r, fixed_cos(rad)),
                center.y,
                center.z + FIXED_MUL(r, fixed_sin(rad))
            };
            
            // Apply ring tilt around X-axis
            ring_point = rotateX(ring_point, tilt_angle);
            
            // Project to screen
            Vector2 projected = project3D(ring_point, camera, target);
            
            int proj_x = FIXED_TO_INT(projected.x);
            int proj_y = FIXED_TO_INT(projected.y);
            if (proj_x >= 0 && proj_x < SSD1306_WIDTH && 
                proj_y >= 0 && proj_y < SSD1306_HEIGHT) {
                SSD1306_SetPixel(proj_x, proj_y, 1);
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
    camera_pos.x = FIXED_MUL(FLOAT_TO_FIXED(CAMERA_DISTANCE), fixed_cos(camera_angle));
    camera_pos.y = 0;  // Keep camera at same height
    camera_pos.z = FIXED_MUL(FLOAT_TO_FIXED(CAMERA_DISTANCE), fixed_sin(camera_angle));
    
    // Saturn is always at the origin
    Vector3 saturn_center = {0, 0, 0};
    
    // Render stars first (background)
    for (int i = 0; i < NUM_STARS; i++) {
        Vector2 star_2d = project3D(stars[i], camera_pos, saturn_center);
        
        int star_x = FIXED_TO_INT(star_2d.x);
        int star_y = FIXED_TO_INT(star_2d.y);
        if (star_x >= 0 && star_x < SSD1306_WIDTH && 
            star_y >= 0 && star_y < SSD1306_HEIGHT) {
            SSD1306_SetPixel(star_x, star_y, 1);
        }
    }
    
    // Saturn body (solid sphere) - project center and draw filled circle
    Vector2 saturn_center_2d = project3D(saturn_center, camera_pos, saturn_center);
    
    // Calculate the projected radius using camera distance (consistent sizing)
    int32_t dx = camera_pos.x - saturn_center.x;
    int32_t dy = camera_pos.y - saturn_center.y;
    int32_t dz = camera_pos.z - saturn_center.z;
    int32_t distance_sq = FIXED_MUL(dx, dx) + FIXED_MUL(dy, dy) + FIXED_MUL(dz, dz);
    int32_t camera_distance = fixed_sqrt(distance_sq);
    
    int32_t saturn_radius_3d = FLOAT_TO_FIXED(SATURN_RADIUS);
    int32_t perspective_scale = FIXED_DIV(FLOAT_TO_FIXED(PROJECTION_DISTANCE), camera_distance); // Same as in project3D
    int projected_radius = FIXED_TO_INT(FIXED_MUL(saturn_radius_3d, perspective_scale));
    
    // Draw filled circle for Saturn
    int center_x = FIXED_TO_INT(saturn_center_2d.x);
    int center_y = FIXED_TO_INT(saturn_center_2d.y);
    
    if (center_x >= -projected_radius && center_x < SSD1306_WIDTH + projected_radius &&
        center_y >= -projected_radius && center_y < SSD1306_HEIGHT + projected_radius) {
        
        for (int y = -projected_radius; y <= projected_radius; y++) {
            for (int x = -projected_radius; x <= projected_radius; x++) {
                if (x*x + y*y <= projected_radius*projected_radius) {
                    int screen_x = center_x + x;
                    int screen_y = center_y + y;
                    
                    if (screen_x >= 0 && screen_x < SSD1306_WIDTH && 
                        screen_y >= 0 && screen_y < SSD1306_HEIGHT) {
                        SSD1306_SetPixel(screen_x, screen_y, 1);
                    }
                }
            }
        }
    }
    
    // Saturn rings - tilted like real Saturn
    int32_t ring_tilt = FIXED_DIV(FIXED_MUL(FLOAT_TO_FIXED(RING_TILT_DEGREES), FIXED_PI), FIXED_180);
    
    // Draw multiple rings with different radii
    drawRing3D(saturn_center, FLOAT_TO_FIXED(RING_1_INNER), FLOAT_TO_FIXED(RING_1_OUTER), ring_tilt, camera_pos, saturn_center);
    drawRing3D(saturn_center, FLOAT_TO_FIXED(RING_2_INNER), FLOAT_TO_FIXED(RING_2_OUTER), ring_tilt, camera_pos, saturn_center);
    drawRing3D(saturn_center, FLOAT_TO_FIXED(RING_3_INNER), FLOAT_TO_FIXED(RING_3_OUTER), ring_tilt, camera_pos, saturn_center);
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
        
        // Update camera animation (configurable rotation time and FPS)
        // rotation_time * fps = total_frames for 2*PI radians
        // So increment = 2*PI / total_frames = PI / (rotation_time * fps / 2)
        int32_t total_frames = FLOAT_TO_FIXED(CAMERA_ROTATION_TIME_SECONDS * TARGET_FPS);
        camera_angle += FIXED_DIV(2 * FIXED_PI, total_frames);
        
        // Render the scene
        renderScene();
        
        // Display
        renderToSDL(renderer);
        
        SDL_Delay((int)(1000.0f / TARGET_FPS)); // Configurable FPS
    }
    
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    
    return 0;
} 