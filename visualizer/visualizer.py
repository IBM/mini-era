import os, sys, getopt
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
DARK_GREY = (131, 131, 131)
LIGHT_GREY = (191, 191, 191)
TANGERINE = (255, 210, 74)
grass_color = (168, 235, 113)
car_color = (162, 167, 184) # blue-grey
motor_color = (74, 53, 53) # brown
bike_color = (1, 20, 59) # navy

# Define screen info
screen_width = 500
screen_height = 500
lane_width = screen_width / 7 # 3 lanes + 2 grass sides
road_width = lane_width * 5 # 3 lanes
size = (screen_width, screen_height)
screen = pygame.display.set_mode(size)
bgY = 0 # for scrolling grass
bgY2 = -screen_height # for scrolling grass

# Dictionary for images loaded
img_lib = {}

# Define road objects' start position
x_lhaz  = 100
x_left  = 170 # 137.5 # places object in middle of left lane.
x_mid   = 237.5 # 237.5 # places object in middle of mid lane
x_right = 310 # 337.5 # places object in middle of right lane
x_rhaz  = 380

x_main_car = x_mid # your car's x position, starts in mid lane
y_main_car = 440 # your car's y position

# Frame update rate for moving down
MOVE_DOWN = 100 # 500 # every 500ms

obj_list = []

five_lane_trace = False;


# HELPER FUNCTIONS
def set_background(five_lane_trace):
    """
    Objective: Initializes Visualizer's screen.
    Draws grass, road, and tree background.
    """
    # Clear screen
    screen.fill(grass_color)

    # Draw background
    if (five_lane_trace) :
        pygame.draw.rect(screen, LIGHT_GREY , pygame.Rect(lane_width, 0, road_width, screen_height)) # road
    else:
        pygame.draw.rect(screen, DARK_GREY, pygame.Rect(lane_width, 0, road_width, screen_height)) # road
    pygame.draw.rect(screen, LIGHT_GREY, pygame.Rect(2*lane_width, 0, 3*lane_width, screen_height)) # road
    pygame.draw.line(screen, TANGERINE, [lane_width*2, 0], [lane_width*2, screen_height], 3) # first road line
    pygame.draw.line(screen, TANGERINE, [lane_width*3, 0], [lane_width*3, screen_height], 3) # second road line
    pygame.draw.line(screen, TANGERINE, [lane_width*4, 0], [lane_width*4, screen_height], 3) # third road line
    pygame.draw.line(screen, TANGERINE, [lane_width*5, 0], [lane_width*5, screen_height], 3) # fourth road line
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

def parse_trace(filename, five_lanes):  
    """
    Objective: Parces trace files
    Trace_Format:  One epoch per line: my_lane, Left-Lane Objs, Mid-Lane Objs, Right-Lane-Objs\n
    Returns: 4 lists corresponding to 'columns' of trace file
        my_lane   : list of lane positions for my car for each epoch
        left_lane : list containing left lane's trace info for each epoch; ex) [Cdist Bdist, N0, Tdist Bdist Cdist, ... ]
        mid_lane  : list containing middle lane's trace info for each epoch
        right_lane: list containing right lane's trace info for each epoch
    Parameters:
        filename: path name of the trace file
    """ 
    my_lane    = []
    lhaz_lane  = []
    left_lane  = []
    mid_lane   = []
    right_lane = []
    rhaz_lane = []

    with open(filename) as f:
        lines = f.readlines()
    for line in lines:
        tokens = line.split(",")
        my_lane.append(tokens[0])
        if five_lanes :
            lhaz_lane.append(tokens[1])
            left_lane.append(tokens[2])
            mid_lane.append(tokens[3])
            right_lane.append(tokens[4])
            rhaz_lane.append(tokens[5].strip("\n"))
        else:
            left_lane.append(tokens[1])
            mid_lane.append(tokens[2])
            right_lane.append(tokens[3].strip("\n"))
    return my_lane, lhaz_lane, left_lane, mid_lane, right_lane, rhaz_lane

# def get_object(bit_str):
#     """
#     Objective: Get the type of object in the lane
#     Returns: String of first 2 bits of the trace info; ex) '01' for car
#     Parameters:
#         bit_str: 12-bit lengthed trace
#     """
#     obj = bit_str[0:2]
#     return obj # return first 2 bits

def get_dist(dist_str):
    """
    Objective: Calculate how far away an object is from the main car
    Returns: Int of distance away in pixels
    Parameters:
        bit_str: 12-bit lengthed trace
    """
    # Define distance scale
    dist_scale = 1 # 500 / 1023
    int_dist = int(dist_str) # in units out of 1023
    pix_dist = int_dist * dist_scale # distance away in pixels, y-position is (500 - pix_dist)
    #DEBUG print dist_str, int_dist, dist_scale, pix_dist
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
    if obj == 'C': # Car
        screen.blit(get_img('images/blue-car.png'), (x, y))
    if obj == 'B': # Bike
        screen.blit(get_img('images/motorcycle.png'), (x, y))
    if obj == 'T': # Truck
        screen.blit(get_img('images/truck.png'), (x, y))
    if obj == 'P': # Person
        screen.blit(get_img('images/runner.png'), (x, y))


def usage_and_exit(exit_code):
    print("usage: %s OPTIONS" % (sys.argv[0]));
    print(" OPTIONS: -h or --help  : print this usage info");
    print("          -t <TF> or --trace=<TF> : specifies the input trace file <TF>");
    print("          -d <N>  or --delay=<N>  : specifies the delay (in ms) between frames");
    print("          -5      or --five--lane : indicates the trace has obstacles in all five lanes");
    print("          -3      or --three-lane : indicates the trace has obstacles in only three lanes");
    sys.exit(exit_code)

    
# MAIN FUNCTION
def main(argv):
    """
    Objective: Illustrate the Visualizer
    """
    # Set variables
    done = False # while loop condition

    global x_lhaz, x_left, x_mid, x_right, x_rhaz, x_main_car
    global MOVE_DOWN
    global obj_list
    global five_lane_trace
    
    tracefile = '' # NO Default value
    # parse command line arguments
    # So far getopt seems to not work here...
    for i in range(0, len(argv[1:])):
        ii = i+1
        #print argv[ii]
        if ((argv[ii] == "-h") | (argv[ii] == "--help")) :
            usage_and_exit(2)
        elif ((argv[ii] == "-5") | (argv[ii] == "--five-lane")) :
            five_lane_trace = True;
            print("%s using Five-Lane trace\n" % (argv[0]));
        elif ((argv[ii] == "-3") | (argv[ii] == "--three-lane")) :
            five_lane_trace = False;
            print("%s using Three-Lane trace\n" % (argv[0]));
        elif ((argv[ii] == "-t") | (argv[ii] == "--trace")) :
            tracefile = argv[ii+1];
            print("%s tracefile = %s\n" % (argv[0], tracefile));
        elif ((argv[ii] == "-d") | (argv[ii] == "--delay")) :
            MOVE_DOWN = int(argv[ii+1])
            print("%s per frame delay is %u ms\n" % (argv[0], MOVE_DOWN));

    if (tracefile == '') :
        print("You must specify a trace file (using -t or --tracefile=)");
        usage_and_exit(4);
        
    print("Reading trace %s\n" % tracefile);
    print("%s using Five-Lane trace = %u\n" % (argv[0], five_lane_trace));

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
    mine, lhaz, left, mid, right, rhaz  = parse_trace(tracefile, five_lane_trace)
    print("Sizes of mine %u lhaz %u left %u mid %u right %u rhaz %u\n" % (len(mine), len(lhaz), len(left), len(mid), len(right), len(rhaz)))
    mine.reverse()
    lhaz.reverse()
    left.reverse() # reverse list order so popping gives chronological order
    mid.reverse()
    right.reverse()
    rhaz.reverse()

    x_per_lane = (x_lhaz, x_left, x_mid, x_right, x_rhaz)


    # MAIN LOOP
    while not done:
        
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                # Catch event when window is closed, exit while loop
                done = True 
            if event.type == move_down_event:
                # Do all the object moving during "down" event
                # So that the screen will refresh at the frame delay defined by MOVE_DOWN

                if len(left) == 0:
                    # Stop once all entries of trace have been visualized
                    done = True
                    break

                # Get next trace data for each lane
                my_data    = mine.pop()
                left_data  = left.pop()
                mid_data   = mid.pop()
                right_data = right.pop()
                if (five_lane_trace):
                    lhaz_data  = lhaz.pop()
                    rhaz_data  = rhaz.pop()

                #DEBUG print my_data, left_data, mid_data, right_data
                # Update lane position (x position) of your car
                x_main_car = x_per_lane[int(my_data)] # 100*int(my_data) + 37.5; # change this if car should switch lanes

                # Create list of objects to display
                #   Parse trace entries into tuples: (x-position, pixel distance from tip of car, object type)
                obj_list = []
                if (five_lane_trace):
                    lhobjs = lhaz_data.split(" ")
                    for obj in lhobjs:
                        (trtype, trdist) = obj.split(":")
                        trtup  = (x_lhaz, 500 - get_dist(trdist), trtype) # get_object(trtype))
                        obj_list.append(trtup);

                lobjs = left_data.split(" ")
                for obj in lobjs:
                    (trtype, trdist) = obj.split(":")
                    trtup  = (x_left, 500 - get_dist(trdist), trtype) # get_object(trtype))
                    obj_list.append(trtup);

                mobjs = mid_data.split(" ")
                for obj in mobjs:
                    (trtype, trdist) = obj.split(":")
                    trtup  = (x_mid, 500 - get_dist(trdist), trtype) #get_object(trtype))
                    obj_list.append(trtup);

                robjs = right_data.split(" ")
                for obj in robjs:
                    (trtype, trdist) = obj.split(":")
                    trtup  = (x_right, 500 - get_dist(trdist), trtype) #get_object(trtype))
                    obj_list.append(trtup);

                if (five_lane_trace):
                    rhobjs = rhaz_data.split(" ")
                    for obj in rhobjs:
                        (trtype, trdist) = obj.split(":")
                        trtup  = (x_rhaz, 500 - get_dist(trdist), trtype) # get_object(trtype))
                        obj_list.append(trtup);


        # Set background
        set_background(five_lane_trace)

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
        for obj in obj_list:
            blit_obj(screen, obj[2], obj[0], obj[1]-55)
            #DEBUG print obj[2], obj[0], obj[1]

        # Update screen to display changes
        pygame.display.flip()
        clock.tick(30)

    pygame.quit()

if __name__ == '__main__':
    main(sys.argv)
    
