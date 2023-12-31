#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600
#define BALL_SIZE 10
#define OBSTACLE_COUNT 8
#define FRICTION 0.95
#define ACCELERATION 0.5
#define FRAME_RATE 60
#define MIN_VELOCITY 0.5

SDL_Window *window;
SDL_Renderer *renderer;
SDL_Texture *backgroundTexture;
SDL_Rect ball, obstacles[OBSTACLE_COUNT], hole;
float ball_velocity_x = 0, ball_velocity_y = 0;
int collisionCount = 0;

int isMouseDragging = 0;
int mouseXClick, mouseYClick;
int draggingDistanceX = 0, draggingDistanceY = 0;

void initSDL() {
    SDL_Init(SDL_INIT_VIDEO);
    window = SDL_CreateWindow("2D Golf Game", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, 0);
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    backgroundTexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, SCREEN_WIDTH, SCREEN_HEIGHT);
}

void loadBackground() {
    SDL_Rect bgRect = { 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT };
    SDL_SetRenderTarget(renderer, backgroundTexture);
    SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255); // Blue color
    SDL_RenderFillRect(renderer, &bgRect);
    SDL_SetRenderTarget(renderer, NULL);
}

void drawArrow(SDL_Renderer *renderer, int startX, int startY, int endX, int endY) {
    SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255); // Yellow color for the arrow

    double distance = sqrt(pow(endX - startX, 2) + pow(endY - startY, 2));
    double arrowLength = fmin(distance, 100.0); // Cap the arrow's maximum length

    int arrowEndX = startX - (arrowLength * (endX - startX)) / distance;
    int arrowEndY = startY - (arrowLength * (endY - startY)) / distance;

    SDL_RenderDrawLine(renderer, startX, startY, arrowEndX, arrowEndY);

    // Draw arrowhead
    int arrowSize = 10;
    double angle = atan2(endY - startY, endX - startX);
    SDL_Point arrowPoints[3];
    arrowPoints[0].x = arrowEndX;
    arrowPoints[0].y = arrowEndY;
    arrowPoints[1].x = arrowEndX + arrowSize * cos(angle - M_PI / 6);
    arrowPoints[1].y = arrowEndY + arrowSize * sin(angle - M_PI / 6);
    arrowPoints[2].x = arrowEndX + arrowSize * cos(angle + M_PI / 6);
    arrowPoints[2].y = arrowEndY + arrowSize * sin(angle + M_PI / 6);

    SDL_RenderDrawLines(renderer, arrowPoints, 3);
}


void render() {
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, backgroundTexture, NULL, NULL);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255); // White color for ball
    SDL_RenderFillRect(renderer, &ball);

    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255); // Red color for obstacles
    for (int i = 0; i < OBSTACLE_COUNT; i++) {
        SDL_RenderFillRect(renderer, &obstacles[i]);
    }

    SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255); // Green color for the hole
    SDL_RenderFillRect(renderer, &hole);

    if (isMouseDragging) {
        // Draw arrow representing shot power while dragging
        drawArrow(renderer, ball.x + BALL_SIZE / 2, ball.y + BALL_SIZE / 2, ball.x + BALL_SIZE / 2 + draggingDistanceX, ball.y + BALL_SIZE / 2 + draggingDistanceY);
    }

    SDL_RenderPresent(renderer);
}

void updatePhysics() {
    ball_velocity_x *= FRICTION;
    ball_velocity_y *= FRICTION;

    // Calculate the next frame's position for the ball
    float nextFrameX = ball.x + ball_velocity_x;
    float nextFrameY = ball.y + ball_velocity_y;

    // Check collision with obstacles
    for (int i = 0; i < OBSTACLE_COUNT; i++) {
        if (nextFrameX < obstacles[i].x + obstacles[i].w &&
            nextFrameX + BALL_SIZE > obstacles[i].x &&
            nextFrameY < obstacles[i].y + obstacles[i].h &&
            nextFrameY + BALL_SIZE > obstacles[i].y) {

            // Handle collision
            float temp = ball_velocity_x;
            ball_velocity_x = ball_velocity_y;
            ball_velocity_y = temp;
            collisionCount++;
            break; // Exit the loop once we detect a collision
        }
    }

    ball.x += ball_velocity_x;
    ball.y += ball_velocity_y;

    // Check collision with walls
    if (ball.x <= 0 || ball.x >= SCREEN_WIDTH - BALL_SIZE) {
        ball_velocity_x = -ball_velocity_x;
        collisionCount++;
    }

    if (ball.y <= 0 || ball.y >= SCREEN_HEIGHT - BALL_SIZE) {
        ball_velocity_y = -ball_velocity_y;
        collisionCount++;
    }

    // Check if the ball reaches the hole
    if (SDL_HasIntersection(&ball, &hole)) {
        // Handle reaching the hole (e.g., display a message or end the game)
        printf("You reached the hole!\n");
        SDL_Delay(2000); // Pause for 2 seconds before ending the game
        exit(0);
    }

    // Apply friction to stop the ball
    if (fabs(ball_velocity_x) < MIN_VELOCITY) {
        ball_velocity_x = 0;
    }

    if (fabs(ball_velocity_y) < MIN_VELOCITY) {
        ball_velocity_y = 0;
    }
}

void generateRandomObstacles() {
    for (int i = 0; i < OBSTACLE_COUNT; i++) {
        do {
            obstacles[i].w = rand() % 50 + 30; // Random obstacle width between 30 and 80 to make them larger
            obstacles[i].h = rand() % 50 + 30; // Random obstacle height between 30 and 80 to make them larger
            obstacles[i].x = rand() % (SCREEN_WIDTH - obstacles[i].w);
            obstacles[i].y = rand() % (SCREEN_HEIGHT - obstacles[i].h);
        } while (SDL_HasIntersection(&ball, &obstacles[i])); // Check for intersection with the ball
    }
}

void generateRandomHole() {
    do {
        hole.w = BALL_SIZE; // Make the hole the same size as the ball for simplicity
        hole.h = BALL_SIZE;
        hole.x = rand() % (SCREEN_WIDTH - hole.w);
        hole.y = rand() % (SCREEN_HEIGHT - hole.h);
    } while (SDL_HasIntersection(&ball, &hole)); // Check intersection with the ball

    // Ensure the hole doesn't spawn inside an obstacle
    for (int i = 0; i < OBSTACLE_COUNT; i++) {
        while (SDL_HasIntersection(&hole, &obstacles[i])) {
            hole.x = rand() % (SCREEN_WIDTH - hole.w);
            hole.y = rand() % (SCREEN_HEIGHT - hole.h);
        }
    }
}

int main(int argc, char* argv[]) {
    srand(time(NULL));

    initSDL();
    loadBackground();

    ball.w = ball.h = BALL_SIZE;
    ball.x = SCREEN_WIDTH / 2;
    ball.y = SCREEN_HEIGHT / 2;

    generateRandomObstacles();
    generateRandomHole();

    int isRunning = 1;
    SDL_Event event;
    unsigned int frameStartTime, frameEndTime, frameTime;

    while (isRunning) {
        frameStartTime = SDL_GetTicks();

        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_QUIT:
                    isRunning = 0;
                    break;
                case SDL_MOUSEBUTTONDOWN:
                    if (event.button.button == SDL_BUTTON_LEFT) {
                        isMouseDragging = 1;
                        SDL_GetMouseState(&mouseXClick, &mouseYClick);
                    }
                    break;
                case SDL_MOUSEMOTION:
                    if (isMouseDragging) {
                        int mouseX, mouseY;
                        SDL_GetMouseState(&mouseX, &mouseY);
                        draggingDistanceX = mouseX - mouseXClick;
                        draggingDistanceY = mouseY - mouseYClick;
                    }
                    break;
                case SDL_MOUSEBUTTONUP:
                    if (event.button.button == SDL_BUTTON_LEFT) {
                        isMouseDragging = 0;
                        ball_velocity_x = -draggingDistanceX * 0.1;
                        ball_velocity_y = -draggingDistanceY * 0.1;
                        draggingDistanceX = 0;
                        draggingDistanceY = 0;
                    }
                    break;
                default:
                    break;
            }
        }

        updatePhysics();
        render();

        frameEndTime = SDL_GetTicks();
        frameTime = frameEndTime - frameStartTime;

        if (frameTime < 1000 / FRAME_RATE) {
            SDL_Delay((1000 / FRAME_RATE) - frameTime);
        }
    }

    SDL_DestroyTexture(backgroundTexture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    printf("Total collisions: %d\n", collisionCount);

    return 0;
}
