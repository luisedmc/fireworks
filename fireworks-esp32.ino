#include <TFT_eSPI.h>
#include <SPI.h>

#define WIDTH 240
#define HEIGHT 240
#define MAX_PARTICLES 40  // Increased for better effect
#define GRAVITY 0.2       // Reduced for slower fall
#define MAX_LIFETIME 60  
#define NUM_FIREWORKS 3

#define TFT_BLK 15

#define NUM_COLORS 6
uint16_t fireworkColors[NUM_COLORS] = {
  TFT_RED, TFT_YELLOW, TFT_GREEN,
  TFT_BLUE, TFT_MAGENTA, TFT_CYAN
};

// Color fade lookup table
uint16_t fadeColors[8] = {
  TFT_WHITE,    
  TFT_YELLOW,   
  TFT_RED,      
  TFT_MAGENTA,  
  TFT_PURPLE,   
  TFT_NAVY,     
  TFT_MAROON,  
  TFT_BLACK     
};

TFT_eSPI tft = TFT_eSPI();

struct Particle {
  float x, y;
  float dx, dy;
  int lifetime;
  uint16_t color;
  bool active;
  float last_x, last_y;
  float brightness;  // fade effect
  float size;        // particle size
};

struct Firework {
  Particle particles[MAX_PARTICLES];
  int x, y;
  int last_y;
  bool active;
  uint16_t color;
  float explosion_size; 
};

Firework fireworks[NUM_FIREWORKS];
int launch_delays[NUM_FIREWORKS];

// Helper function to create faded color
uint16_t fadeColor(uint16_t color, float brightness) {
  if (brightness <= 0) return TFT_BLACK;

  // Extract RGB components
  uint8_t r = (color >> 11) & 0x1F;
  uint8_t g = (color >> 5) & 0x3F;
  uint8_t b = color & 0x1F;

  // Brightness
  r = (r * brightness);
  g = (g * brightness);
  b = (b * brightness);

  // Reconstruct color
  return (r << 11) | (g << 5) | b;
}

void init_firework(Firework *fw, int x) {
  fw->x = x;
  fw->y = HEIGHT - 1;
  fw->last_y = HEIGHT - 1;
  fw->active = true;
  fw->color = fireworkColors[random(NUM_COLORS)];
  fw->explosion_size = random(40, 80) / 10.0;  // Random explosion size

  for (int i = 0; i < MAX_PARTICLES; i++) {
    fw->particles[i].active = false;
  }
}

void explode_firework(Firework *fw) {
  tft.drawPixel(fw->x, fw->y, TFT_BLACK);

  // Create initial bright flash
  tft.fillCircle(fw->x, fw->y, 3, TFT_WHITE);
  delay(5);

  for (int i = 0; i < MAX_PARTICLES; i++) {
    fw->particles[i].x = fw->x;
    fw->particles[i].y = fw->y;
    fw->particles[i].last_x = fw->x;
    fw->particles[i].last_y = fw->y;

    // Create more natural explosion pattern
    float angle = (float)i * ((2 * PI) / MAX_PARTICLES) + random(-30, 30) / 100.0;
    float speed = random(20, 50) / 10.0 * fw->explosion_size;

    fw->particles[i].dx = cos(angle) * speed;
    fw->particles[i].dy = sin(angle) * speed;
    fw->particles[i].lifetime = MAX_LIFETIME + random(-10, 10);  // Varied lifetime
    fw->particles[i].color = fw->color;
    fw->particles[i].active = true;
    fw->particles[i].brightness = 1.0;     // Start at full brightness
    fw->particles[i].size = random(1, 3);  // Random particle size
  }
  fw->active = false;

  // Clear initial flash
  tft.fillCircle(fw->x, fw->y, 3, TFT_BLACK);
}

void update_particles(Firework *fw) {
  for (int i = 0; i < MAX_PARTICLES; i++) {
    if (fw->particles[i].active) {
      // Clear previous position
      if (fw->particles[i].size > 1) {
        tft.fillCircle(fw->particles[i].last_x, fw->particles[i].last_y,
                       fw->particles[i].size, TFT_BLACK);
      } else {
        tft.drawPixel(fw->particles[i].last_x, fw->particles[i].last_y, TFT_BLACK);
      }

      // Decrease lifetime first
      fw->particles[i].lifetime--;

      // Check if particle should be deactivated
      if (fw->particles[i].lifetime <= 0 || fw->particles[i].x < 0 || fw->particles[i].x >= WIDTH || fw->particles[i].y < 0 || fw->particles[i].y >= HEIGHT || fw->particles[i].brightness <= 0.1) {  // Add brightness threshold
        fw->particles[i].active = false;
        continue;  // Skip the rest of the update for this particle
      }

      // Save current position
      fw->particles[i].last_x = fw->particles[i].x;
      fw->particles[i].last_y = fw->particles[i].y;

      // Update position with easing
      fw->particles[i].dx *= 0.98;  // Add air resistance
      fw->particles[i].dy *= 0.98;
      fw->particles[i].x += fw->particles[i].dx;
      fw->particles[i].y += fw->particles[i].dy;
      fw->particles[i].dy += GRAVITY;

      // Update brightness based on lifetime
      float life_ratio = (float)fw->particles[i].lifetime / MAX_LIFETIME;
      fw->particles[i].brightness = life_ratio * life_ratio;  // Quadratic fade

      // Only draw if particle is still active and bright enough
      if (fw->particles[i].brightness > 0.1) {
        uint16_t color = fadeColor(fw->particles[i].color, fw->particles[i].brightness);
        if (fw->particles[i].size > 1) {
          tft.fillCircle(fw->particles[i].x, fw->particles[i].y,
                         fw->particles[i].size, color);
        } else {
          tft.drawPixel(fw->particles[i].x, fw->particles[i].y, color);
        }
      }
    }
  }
}

void setup() {
  randomSeed(analogRead(0));

  pinMode(TFT_BLK, OUTPUT);
  digitalWrite(TFT_BLK, HIGH);

  tft.init();
  tft.setRotation(0);
  tft.fillScreen(TFT_BLACK);

  for (int i = 0; i < NUM_FIREWORKS; i++) {
    fireworks[i].active = false;
    launch_delays[i] = random(20);
  }
}

void loop() {
  for (int i = 0; i < NUM_FIREWORKS; i++) {
    if (!fireworks[i].active && launch_delays[i] <= 0) {
      init_firework(&fireworks[i], random(WIDTH));
    } else if (!fireworks[i].active) {
      launch_delays[i]--;
      continue;
    }

    if (fireworks[i].active) {
      // Clear previous rocket position
      tft.drawPixel(fireworks[i].x, fireworks[i].last_y, TFT_BLACK);

      fireworks[i].last_y = fireworks[i].y;

      // Launch sequence with trail effect
      if (fireworks[i].y > HEIGHT / 3) {
        fireworks[i].y -= 3;  // Faster launch
        // Draw rocket trail
        for (int trail = 0; trail < 3; trail++) {
          int trail_y = fireworks[i].y + trail * 2;
          if (trail_y < HEIGHT) {
            tft.drawPixel(fireworks[i].x, trail_y,
                          fadeColor(TFT_WHITE, 1.0 - (trail * 0.3)));
          }
        }
      } else {
        explode_firework(&fireworks[i]);
      }
    }

    update_particles(&fireworks[i]);

    // Check if firework is completely finished
    int active_particles = 0;
    for (int j = 0; j < MAX_PARTICLES; j++) {
      if (fireworks[i].particles[j].active) {
        active_particles++;
      }
    }

    if (!fireworks[i].active && active_particles == 0) {
      launch_delays[i] = random(20);
    }
  }

  delay(16);  // Approximately 60 FPS
}