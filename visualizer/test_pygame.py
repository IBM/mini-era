import sys, pygame, os
import time

pygame.init()

# Define colors
WHITE = (255, 255, 255)
BLACK = (0, 0, 0)
RED = (255, 0, 0)
GREEN = (0, 255, 0)
BLUE = (0, 0, 255)
LIGHT_GREY = (191, 191, 191)
TANGERINE = (255, 210, 74)
grass_color = (168, 235, 113)
car_color = (162, 167, 184) # blue-grey
motor_color = (74, 53, 53) # brown
bike_color = (1, 20, 59) # navy

size = (500, 500)
screen = pygame.display.set_mode(size)
done = False

# Define road objects' start position
x_left = 137.5
y_left = 0 # changes
x_mid = 237.5
y_mid = 0 # changes
x_right = 337.5
y_right = 0 # changes

# Define distance scale
dist_scale = 500 / 1023

# Frame update
MOVE_DOWN = 250

clock = pygame.time.Clock()

# Create events
move_down_event = pygame.USEREVENT + 1

# Set timers
pygame.time.set_timer(move_down_event, MOVE_DOWN)

while not done:
    # clock.tick(60)
    for event in pygame.event.get():
        if event.type == pygame.QUIT:
            done = True
        if event.type == move_down_event:
            y_left += 10

    # Clear screen
    screen.fill(grass_color)

    # Draw background
    pygame.draw.rect(screen, LIGHT_GREY, pygame.Rect(100, 0, 300, 500)) # road
    pygame.draw.line(screen, TANGERINE, [200, 0], [200, 500], 3) # road line
    pygame.draw.line(screen, TANGERINE, [300, 0], [300, 500], 3) # road line
    pygame.draw.rect(screen, RED, pygame.Rect(237.5, 475, 25, 25)) # main car

    # Draw objects
    car = pygame.draw.rect(screen, car_color, pygame.Rect(x_left, y_left, 25, 25)) # car
    # pygame.draw.rect(screen, motor_color, pygame.Rect(x_mid, y_mid, 25, 25)) # motorcycle
    # pygame.draw.rect(screen, bike_color, pygame.Rect(x_right, y_right, 25, 25)) # bike

    # Update screen
    pygame.display.flip()
    clock.tick(60)

pygame.quit()