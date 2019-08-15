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

size = (500, 500)
screen = pygame.display.set_mode(size)
# print("outside")
img_lib = {}
bgY = 0
bgY2 = -500

def set_background():
    
    # Clear screen
    screen.fill(grass_color)

    # Draw background
    pygame.draw.rect(screen, LIGHT_GREY, pygame.Rect(100, 0, 300, 500)) # road
    pygame.draw.line(screen, TANGERINE, [200, 0], [200, 500], 3) # road line
    pygame.draw.line(screen, TANGERINE, [300, 0], [300, 500], 3) # road line
    # pygame.draw.rect(screen, RED, pygame.Rect(237.5, 475, 25, 25)) # main car
    screen.blit(get_img('images/trees_bg.png'), (0, bgY))
    screen.blit(get_img('images/trees_bg.png'), (0, bgY2))

def get_img(path):
    global img_lib
    image = img_lib.get(path)
    if image == None:
        image = pygame.image.load(path)
        img_lib[path] = image
    return image

def parse_trace(filename):
    # Currently: can only see one object in front of you?
    
    left_lane = []
    mid_lane = []
    right_lane = []

    with open(filename) as f:
        lines = f.readlines()
    for line in lines:
        # print(line)
        tokens = line.split(" ")
        left_lane.append(tokens[0])
        mid_lane.append(tokens[1])
        right_lane.append(tokens[2].strip("\n"))
    # print(left_lane)
    # print(mid_lane)
    # print(right_lane)
    return left_lane, mid_lane, right_lane

def get_object(bit_str):
    obj = bit_str[0:2]
    return obj # return first 2 bits

def get_dist(bit_str):
    # Define distance scale
    dist_scale = 500 / 1023
    
    dist = bit_str[2:] # get last 10 bits
    int_dist = int(dist, 2) # in units out of 1023
    pix_dist = int_dist * dist_scale # in pixels, y-position is (500 - pix_dist)
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
    if obj == '01':
        screen.blit(get_img('images/blue-car.png'), (x, y))
    if obj == '10':
        screen.blit(get_img('images/motorcycle.png'), (x, y))
    if obj == '11':
        screen.blit(get_img('images/truck.png'), (x, y))

def main():

    done = False

    # Define road objects' start position
    # Either hard-code as top of screen, or take from first trace
    x_left = 137.5
    y_left = 0 # changes
    c_left = BLACK
    x_mid = 237.5
    y_mid = 0 # changes
    c_mid = BLACK
    x_right = 337.5
    y_right = 0 # changes
    c_right = BLACK
    
    obj_l = '11'
    obj_m = '11'
    obj_r = '11'

    # Frame update
    MOVE_DOWN = 500

    clock = pygame.time.Clock()

    # Create events
    move_down_event = pygame.USEREVENT + 1

    # Set timers
    pygame.time.set_timer(move_down_event, MOVE_DOWN)

    # Parsing
    # lt, md, rt = parse_trace("trace_test.txt")
    # print(lt)
    # print(md)
    # print(rt)

    list_traces = ["trace_test.txt", "trace_test2.txt", "trace_test3.txt"]
    list_traces.reverse() # reverse order so popping gives earliest

    # init: parse first trace
    # first_trace = list_traces.pop()
    # left, mid, right = parse_trace(first_trace)
    # left_lane, mid_lane, right_lane = [], [], []
    # for i in range(len(left)):
    #     left_lane.append((get_dist(left[i]), get_color(left[i])))
    #     mid_lane.append((get_dist(mid[i]), get_color(mid[i])))
    #     right_lane.append((get_dist(right[i]), get_color(right[i])))
        

    # MAIN LOOP
    while not done:
        # clock.tick(60)
        
        for event in pygame.event.get():
##            print("event loop")
            if event.type == pygame.QUIT:
                done = True
            if event.type == move_down_event:
                # print("down loop")

                if len(list_traces) == 0:
                    break
                # Loop through list of traces and parse
                tr = list_traces.pop()
                left, mid, right = parse_trace(tr) # get bits info of each lane for 1 trace
                # dist_l, dist_m, dist_r, color_l, color_m, color_r = [],[],[],[],[],[]
                left_lane, mid_lane, right_lane = [], [], []
                for i in range(len(left)):
                    # left_lane.append((get_dist(left[i]), get_color(left[i])))
                    # mid_lane.append((get_dist(mid[i]), get_color(mid[i])))
                    # right_lane.append((get_dist(right[i]), get_color(right[i])))
                    left_lane.append((get_dist(left[i]), get_object(left[i])))
                    mid_lane.append((get_dist(mid[i]), get_object(mid[i])))
                    right_lane.append((get_dist(right[i]), get_object(right[i])))
                print(left_lane)
                print(mid_lane)
                print(right_lane)

                y_left = 500 - left_lane[0][0]
                y_mid = 500 - mid_lane[0][0]
                y_right = 500 - right_lane[0][0]
                # c_left = left_lane[0][1]
                # c_mid = mid_lane[0][1]
                # c_right = right_lane[0][1]
                obj_l = left_lane[0][1]
                obj_m = mid_lane[0][1]
                obj_r = right_lane[0][1]
                

        # Set background
        set_background()
        global bgY
        global bgY2 
        bgY += 5
        bgY2 += 5
        if bgY > 500:
            bgY = -500
        if bgY2 > 500:
            bgY2 = -500

        # Draw objects, need to set the obj first
        # draw_obj(screen, RED, x_mid, 475, 25, 25) # your car
        screen.blit(get_img('images/red-car.png'), (x_mid, 440)) # your car
        # draw_obj(screen, c_left, x_left, y_left-25, 25, 25)
        # draw_obj(screen, c_mid, x_mid, y_mid-25, 25, 25)
        # draw_obj(screen, c_right, x_right-25, y_right-25, 25, 25)
        blit_obj(screen, obj_l, x_left, y_left-55)
        blit_obj(screen, obj_m, x_mid, y_mid-55)
        blit_obj(screen, obj_r, x_right, y_right-55)
        # print("init draw")

        # Update screen
        pygame.display.flip()
        clock.tick(60)

    pygame.quit()

if __name__ == '__main__':
    # parse_trace("trace_test.txt")
    print("name main")
    main()
    
