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
print("outside")

def set_background():
    
    # Clear screen
    screen.fill(grass_color)

    # Draw background
    pygame.draw.rect(screen, LIGHT_GREY, pygame.Rect(100, 0, 300, 500)) # road
    pygame.draw.line(screen, TANGERINE, [200, 0], [200, 500], 3) # road line
    pygame.draw.line(screen, TANGERINE, [300, 0], [300, 500], 3) # road line
    pygame.draw.rect(screen, RED, pygame.Rect(237.5, 475, 25, 25)) # main car

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

def main():

    done = False

    # Define road objects' start position
    # Either hard-code as top of screen, or take from first trace
    x_left = 137.5
    y_left = 0 # changes
    x_mid = 237.5
    y_mid = 0 # changes
    x_right = 337.5
    y_right = 0 # changes

    # Frame update
    MOVE_DOWN = 500

    clock = pygame.time.Clock()

    # Create events
    move_down_event = pygame.USEREVENT + 1

    # Set timers
    pygame.time.set_timer(move_down_event, MOVE_DOWN)

    # Parsing
    lt, md, rt = parse_trace("trace_test.txt")
    print(lt)
    print(md)
    print(rt)

    # l_dist = []
    # l_color = []
    # for i in left:
    #     l_dist.append(get_dist(i))
    #     l_color.append(get_color(i))
    # l_dist.reverse()
    # l_color.reverse()
    # print(l_dist)
    # print(l_color)

    list_traces = ["trace_test.txt", "trace_test2.txt", "trace_test3.txt"]
    list_traces.reverse() # reverse order so popping gives earliest

    # MAIN LOOP
    while not done:
        clock.tick(60)
        for event in pygame.event.get():
##            print("event loop")
            if event.type == pygame.QUIT:
                done = True
            if event.type == move_down_event:
                # length = len(l_dist)
                # if length == 0:
                #     break
                print("down loop")

                if len(list_traces) == 0:
                    break
                # Loop through list of traces and parse
                tr = list_traces.pop()
                left, mid, right = parse_trace(tr) # get bits info of each lane for 1 trace
                # dist_l, dist_m, dist_r, color_l, color_m, color_r = [],[],[],[],[],[]
                left_lane, mid_lane, right_lane = [], [], []
                for i in range(len(left)):
                    left_lane.append((get_dist(left[i]), get_color(left[i])))
                    mid_lane.append((get_dist(mid[i]), get_color(mid[i])))
                    right_lane.append((get_dist(right[i]), get_color(right[i])))
                    # dist_l.append(get_dist(left[i])) # get dist of all obj in left lane
                    # dist_m.append(get_dist(mid[i]))
                    # dist_r.append(get_dist(right[i]))
                    # color_l.append(get_color(left[i]))
                    # color_m.append(get_color(mid[i]))
                    # color_r.append(get_color(right[i]))
                print(left_lane)
                print(mid_lane)
                print(right_lane)

                # for j in range(len(left_lane)):
                #     left_d = left_lane[j][0] # y pos
                #     left_c = left_lane[j][1]
                #     mid_d = mid_lane[j][0]
                #     mid_c = mid_lane[j][1]
                #     right_d = right_lane[j][0] 
                #     right_c = right_lane[j][1]

                #     draw_obj(screen, left_c, x_left, y_left, 25, 25)
                #     draw_obj(screen, mid_c, x_mid, y_mid, 25, 25)
                #     draw_obj(screen, right_c, x_right, y_right, 25, 25)
                #     time.sleep(0.5)

                #     y_left = 500 - left_d
                #     y_mid = 500 - mid_d
                #     y_right = 500 - right_d

                # d = l_dist.pop()
                # c = l_color.pop()
                # draw_obj(screen, c, x_left, y_left, 25, 25)
                # y_left = 500 - d
##                print("increment: ", d)

                # try just drawing 1 obj at a time per lane
                draw_obj(screen, left_lane[0][1], x_left, y_left, 25, 25)
                # draw_obj(screen, mid_lane[1][1], x_mid, y_mid, 25, 25)
                # draw_obj(screen, right_lane[0][1], x_right, y_right, 25, 25)
                y_left = 500 - left_lane[0][0]
                y_mid = 500 - mid_lane[1][0]
                y_right = 500 - right_lane[0][0]

        # Set background
        set_background()

        # Draw objects, need to set the obj first
        draw_obj(screen, RED, x_mid, 475, 25, 25) # your car
        draw_obj(screen, BLUE, x_left, y_left, 25, 25)

        # Update screen
        pygame.display.flip()
        # clock.tick(60)

    pygame.quit()

if __name__ == '__main__':
    # parse_trace("trace_test.txt")
    print("name main")
    main()
    
