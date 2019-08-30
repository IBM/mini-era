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
road_width = lane_width * 3 # 3 lanes
size = (screen_width, screen_height)
screen = pygame.display.set_mode(size)
bgY = 0 # for scrolling grass
bgY2 = -screen_height # for scrolling grass

# Dictionary for images loaded
img_lib = {}

# Define road objects' start position
x_left = 137.5 # places object in middle of left lane
y_left = 0 
x_mid = 237.5 # places object in middle of mid lane
y_mid = 0 
x_right = 337.5 # places object in middle of right lane
y_right = 0 
x_main_car = x_mid # your car's x position, starts in mid lane
y_main_car = 440 # your car's y position

# Initialize object types
obj_l = '11'
obj_m = '11'
obj_r = '11'

# Frame update rate for moving down
MOVE_DOWN = 500 # every 500ms


# HELPER FUNCTIONS
def set_background():
    """
    Objective: Initializes Visualizer's screen.
    Draws grass, road, and tree background.
    """
    # Clear screen
    screen.fill(grass_color)

    # Draw background
    pygame.draw.rect(screen, LIGHT_GREY, pygame.Rect(lane_width, 0, road_width, screen_height)) # road
    pygame.draw.line(screen, TANGERINE, [lane_width*2, 0], [lane_width*2, screen_height], 3) # first road line
    pygame.draw.line(screen, TANGERINE, [lane_width*3, 0], [lane_width*3, screen_height], 3) # second road line
    screen.blit(get_img('images/trees_bg.png'), (0, bgY)) # scrolling grass bg
    screen.blit(get_img('images/trees_bg.png'), (0, bgY2))

def get_img(path):
    """
    Returns: loaded image
    Parameters: 
        path: path of the image
    """
    global img_lib
    image = img_lib.get(path)
    # Check if image is already loaded
    if image == None:
        image = pygame.image.load(path)
        img_lib[path] = image
    return image

def parse_trace(filename):  
    """
    Objective: Parces trace files
    Returns: 3 lists corresponding to 'columns' of trace file
        left_lane: list containing left lane's trace info for each epoch; ex) [011010111100, 011011101110, 011100100000, ...]
        mid_lane: list containing middle lane's trace info for each epoch
        right_lane: list containing right lane's trace info for each epoch
    Parameters:
        filename: path name of the trace file
    """ 
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
    """
    Objective: Get the type of object in the lane
    Returns: String of first 2 bits of the trace info; ex) '01' for car
    Parameters:
        bit_str: 12-bit lengthed trace
    """
    obj = bit_str[0:2]
    return obj # return first 2 bits

def get_dist(bit_str):
    """
    Objective: Calculate how far away an object is from the main car
    Returns: Int of distance away in pixels
    Parameters:
        bit_str: 12-bit lengthed trace
    """
    # Define distance scale
    dist_scale = 500 / 1023
    
    dist = bit_str[2:] # get last 10 bits
    int_dist = int(dist, 2) # in units out of 1023
    pix_dist = int_dist * dist_scale # distance away in pixels, y-position is (500 - pix_dist)
    return int(pix_dist)

# def get_color(bits):
#     """
#     Objective: Get object's corresponding color
#     Returns: RGB color tuple
#     Parameters:
#         bits: 12-bit lengthed trace
#     """
#     obj = get_object(bits)
#     if obj == "01":
#         return car_color
#     elif obj == "10":
#         return motor_color
#     elif obj == "11":
#         return bike_color
#     else:
#         return LIGHT_GREY # background, return road's color

# def draw_obj(screen, color, x, y, w, h):
#     """
#     Objective: Draw a rectangle on the screen
#     Parameters:
#         screen: variable containing the visualizer's screen
#         color: RGB color tuple
#         x: rectangle's starting x position
#         y: rectangle's starting y position
#         w: width of rectangle
#         h: height of rectangle
#     """
#     pygame.draw.rect(screen, color, pygame.Rect(x, y, w, h))

def blit_obj(screen, obj, x, y):
    """
    Objective: Draw different objects (car, motorcycle, truck)
    Parameters:
        screen: variable containing the visualizer's screen
        obj: String of 2 bits representing the object in the lane
        x: object's starting x position
        y: object's starting y position
    """
    if obj == '01': # car
        screen.blit(get_img('images/blue-car.png'), (x, y))
    if obj == '10': # motorcycle
        screen.blit(get_img('images/motorcycle.png'), (x, y))
    if obj == '11': # truck
        screen.blit(get_img('images/truck.png'), (x, y))


# MAIN FUNCTION
def main():
    """
    Objective: Illustrate the Visualizer
    """

    # Set variables
    done = False # while loop condition

    global x_left, y_left, x_mid, y_mid, x_right, y_right, x_main_car
    global obj_l, obj_m, obj_r
    global MOVE_DOWN

    # Establish clock
    clock = pygame.time.Clock()

    # Create events
    move_down_event = pygame.USEREVENT + 1 

    # Set timers
    pygame.time.set_timer(move_down_event, MOVE_DOWN)

    # Get traces
    # Trace format: 3 columns per epoch with the form XY
    #               where X is a 2-bit string representing object type ('01' car, '10' motorcycle, '11' truck) and
    #               where Y is a 10-bit string representing distance from car (0 to 1023 in binary)
    tr = 'traces/trace_ex.txt'
    left, mid, right = parse_trace(tr)   
    left.reverse() # reverse list order so popping gives chronological order
    mid.reverse()
    right.reverse()

    # MAIN LOOP
    while not done:
        
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                # Catch event when window is closed, exit while loop
                done = True 
            if event.type == move_down_event:
                # Do all the object moving during "down" event
                # So that the screen will refresh at the framerate defined by MOVE_DOWN

                if len(left) == 0:
                    # Stop once all entries of trace have been visualized
                    break

                # Get next trace bits for each lane
                left_bits = left.pop()
                mid_bits = mid.pop()
                right_bits = right.pop()

                # Parse trace bits into tuple: (pixel distance from tip of car, bits for object type)
                left_tup = (get_dist(left_bits), get_object(left_bits))
                mid_tup = (get_dist(mid_bits), get_object(mid_bits))
                right_tup = (get_dist(right_bits), get_object(right_bits))

                # Update y positions and object types
                y_left = 500 - left_tup[0] # (500 - distance) = y position
                y_mid = 500 - mid_tup[0]
                y_right = 500 - right_tup[0]
                obj_l = left_tup[1]
                obj_m = mid_tup[1]
                obj_r = right_tup[1]

                # Update lane position (x position) of your car
                x_main_car = x_main_car # change this if car should switch lanes


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

        # Draw objects every epoch
        screen.blit(get_img('images/red-car.png'), (x_main_car, y_main_car)) # your car
        blit_obj(screen, obj_l, x_left, y_left-55) # left lane
        blit_obj(screen, obj_m, x_mid, y_mid-55) # middle lane
        blit_obj(screen, obj_r, x_right, y_right-55) # right lane

        # Update screen to display changes
        pygame.display.flip()
        clock.tick(30)

    pygame.quit()

if __name__ == '__main__':
    main()
    
