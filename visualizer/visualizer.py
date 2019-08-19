import os, sys
import time
import pygame

# SETUP
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

# Define screen info
screen_width = 500
screen_height = 500
lane_width = screen_width / 5 # 3 lanes + 2 grass sides
road_width = lane_width * 3
size = (screen_width, screen_height)
screen = pygame.display.set_mode(size)
bgY = 0 # for scrolling grass
bgY2 = -screen_height

# Dictionary for images loaded
img_lib = {}

# Define road objects' start position
x_left = 137.5
y_left = 0 # updates
x_mid = 237.5
y_mid = 0 # updates
x_right = 337.5
y_right = 0 # updates

# Initialize object types
obj_l = '11'
obj_m = '11'
obj_r = '11'

# Frame update rate
MOVE_DOWN = 500 # every 500ms


# HELPER FUNCTIONS
def set_background():
    # Clear screen
    screen.fill(grass_color)

    # Draw background
    pygame.draw.rect(screen, LIGHT_GREY, pygame.Rect(lane_width, 0, road_width, screen_height)) # road
    pygame.draw.line(screen, TANGERINE, [lane_width*2, 0], [lane_width*2, screen_height], 3) # first road line
    pygame.draw.line(screen, TANGERINE, [lane_width*3, 0], [lane_width*3, screen_height], 3) # second road line
    screen.blit(get_img('images/trees_bg.png'), (0, bgY)) # scrolling grass bg
    screen.blit(get_img('images/trees_bg.png'), (0, bgY2))

def get_img(path):
    global img_lib
    image = img_lib.get(path)
    # Check if image is already loaded
    if image == None:
        image = pygame.image.load(path)
        img_lib[path] = image
    return image

def parse_trace(filename):   
    left_lane = []
    mid_lane = []
    right_lane = []

    with open(filename) as f:
        lines = f.readlines()
    for line in lines:
        tokens = line.split(" ")
        left_lane.append(tokens[0])
        mid_lane.append(tokens[1])
        right_lane.append(tokens[2].strip("\n"))
    return left_lane, mid_lane, right_lane

def get_object(bit_str):
    obj = bit_str[0:2]
    return obj # return first 2 bits

def get_dist(bit_str):
    # Define distance scale
    dist_scale = 500 / 1023
    
    dist = bit_str[2:] # get last 10 bits
    int_dist = int(dist, 2) # in units out of 1023
    pix_dist = int_dist * dist_scale # distance away in pixels, y-position is (500 - pix_dist)
    return int(pix_dist)

def get_color(bits):
    obj = get_object(bits)
    if obj == "01":
        return car_color
    elif obj == "10":
        return motor_color
    elif obj == "11":
        return bike_color
    else:
        return LIGHT_GREY # background, return road's color

def draw_obj(screen, color, x, y, w, h):
    pygame.draw.rect(screen, color, pygame.Rect(x, y, w, h))

def blit_obj(screen, obj, x, y):
    # Draw object images
    if obj == '01':
        screen.blit(get_img('images/blue-car.png'), (x, y))
    if obj == '10':
        screen.blit(get_img('images/motorcycle.png'), (x, y))
    if obj == '11':
        screen.blit(get_img('images/truck.png'), (x, y))

def main():
    done = False

    global x_left, y_left, x_mid, y_mid, x_right, y_right
    global obj_l, obj_m, obj_r
    global MOVE_DOWN

    clock = pygame.time.Clock()

    # Create events
    move_down_event = pygame.USEREVENT + 1

    # Set timers
    pygame.time.set_timer(move_down_event, MOVE_DOWN)

    # Get traces
    list_traces = []
    prefix = 'traces/trace'
    ext = '.txt'
    for t in range(10):
        num = t+1
        num_str = str(num)
        list_traces.append(prefix + num_str + ext)
    # print(list_traces)
    list_traces.reverse() # reverse order so popping gives earliest

    tr = 'traces/trace_ex.txt'
    left, mid, right = parse_trace(tr)   
    left.reverse()
    mid.reverse()
    right.reverse()

    # MAIN LOOP
    while not done:
        
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                done = True
            if event.type == move_down_event:
                # print("down loop")

                if len(left) == 0:
                    break

                # get next trace bits for each lane
                left_bits = left.pop()
                mid_bits = mid.pop()
                right_bits = right.pop()

                # parse trace bits into tuple: (pixel distance from tip of car, bits for object type)
                left_tup = (get_dist(left_bits), get_object(left_bits))
                mid_tup = (get_dist(mid_bits), get_object(mid_bits))
                right_tup = (get_dist(right_bits), get_object(right_bits))

                # update y positions and object types
                y_left = 500 - left_tup[0] # (500 - distance) = y position
                y_mid = 500 - mid_tup[0]
                y_right = 500 - right_tup[0]
                obj_l = left_tup[1]
                obj_m = mid_tup[1]
                obj_r = right_tup[1]

\
        # Set background
        set_background()

        # Scrolling background
        global bgY, bgY2
        bgY += 5
        bgY2 += 5
        if bgY > 500:
            bgY = -500
        if bgY2 > 500:
            bgY2 = -500

        # Draw objects, need to set the obj first
        screen.blit(get_img('images/red-car.png'), (x_mid, 440)) # your car
        blit_obj(screen, obj_l, x_left, y_left-55) # left lane
        blit_obj(screen, obj_m, x_mid, y_mid-55) # middle lane
        blit_obj(screen, obj_r, x_right, y_right-55) # right lane

        # Update screen
        pygame.display.flip()
        clock.tick(60)

    pygame.quit()

if __name__ == '__main__':
    main()
    # left_traces, mid_traces, right_traces = parse_trace('traces/trace_ex.txt')
    # print('left traces: ')
    # for l in left_traces:
    #     print(l)
    # print('mid traces: ')
    # for m in mid_traces:
    #     print(m)
    # print('right traces: ')
    # for r in right_traces:
    #     print(r)
    
