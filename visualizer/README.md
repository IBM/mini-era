# Mini-ERA Visualizer

Visualization for Mini-Era using pygame.

# Overview
The visualizer is a simple pygame-based 5-lane simulation of the focus car (drawn in red) driving on a straight 3-lane highway at some speed, and overtaking various other vehicles (obstacle objects) on the road.  These obstacle objects will occupy positions initially ahead of the focus car, and can include other cars (drawn in blue), trucks, and bikes.  All of these obstacle objects will be moving slower than the focus car, and therefore progress down the screen closer and closer to the focus car, which must eventually take actions to avoid collisions.

The visualizer is driven by an input trace (see examples in the ```traces``` subdirectory) which are extracted from a Mini-ERA run.  The main visualizer progrma supports both the trace-driven Mini-ERA runs and the simulation-based Mini-ERA runs (in either 3-lane obstacles form or the new 5-lanes of obstacles form).  Details are given below.
 
# Invocation
The visualizer is a python program, invocation is as follows:
```
  python visualizer.py -t <viz_tracefile> -d <frame_delay>
```

The tracefile is the input to the visualizer, and there is **NO** default value so a tracefile **MUST** be specified.
The frame delay is a delay value between frames (in milliseconds), with a default of 500 (i.e. 2 frames per second).

There are some example trace files proveded, which correspond to the example trace files of the main Mini-ERA program,
i.e. the visualizer trace ```tt00.viz``` corresponds to the Mini-ERA input trace ```tt00.new``` and similarly with the
```tt01.*``` and ```tt02.*``` traces (and also the ```test_trace1.new``` and ```test_trace1.viz``` files).

## Usage
The general usage can be obstained with the `-h` option:
```
> python visualizer.py 
pygame 1.9.6
Hello from the pygame community. https://www.pygame.org/contribute.html
You must specify a trace file (using -t or --tracefile=)
usage: visualizer.py OPTIONS
 OPTIONS: -h or --help  : print this usage info
          -t <TF> or --trace=<TF> : specifies the input trace file <TF>
          -d <N>  or --delay=<N>  : specifies the delay (in ms) between frames
          -5      or --five--lane : indicates the trace has obstacles in all five lanes
          -3      or --three-lane : indicates the trace has obstacles in only three lanes
```

In the usage, one can specify the input visualizer tracefile (required), the delay between time-step displays (in the pygame engine; smaller delay indicates faster visualizer updates/advances), and either the '-3' or '-5' option which indicates whether the input trace indicates obstacles only within 3 lanes (Left, Middle, Right) or across all five lanes (including the far left Left-Hazard and far right Right-Hazard lanes).   

The use of the five-lane and three-lane must be coordincated with the proper input trace, i.e. it is an indicator as to whether the input trace includes obstacles only for 3 lanes, or for 5 lanes.

## Other Visualizers
Note: there is a second version of the visualizer program for single-step operation, called ```visualizer_ss.py``` which waits on each trace step for the user to hit the enter key before advancing the visualizer state.  This has limited value in general, but can be useful to study the details of a particular sequence or portion of a trace visualization, and is quite helpful when trying to take interesting visualizer screen captures.
The single-step version is invoked in much the same way, but does not support a frame delay parameter.
The visualizer is a python program, invocation is as follows:
```
  python visualizer_ss.py -t <tracefile>
```

# Input Trace Format
The input trace format is very similar to the mini-era format, but also incudes the current lane position of the focus car.  For the standard, default 3-lanes of obstacle vehicles trace:
```
  L,X:y,X:y,X:y
```
Here, the L is the lane indicator, and each X is an object type (N = none, C = car, T = truck, B = bike) and y is a distance (in the range [0, 550]).
Each lane X:y is separated from the next by a comma character, so the first X:y is the left lane, the second X:y is the center lane, and the third is the right lane. A simple example:

```
  2,C:350,N:550,T:150
```
This specifies a time step where the focus car is in lane 2 (Center lane) and there is a car in the Left lane at distance 350, and a truck in the Right lane at distance 150 (and nothing in the center lane).

The visualizer also supports multiple obstacle objects per lane, which are then separated by space characters.  An example:
```
  2,C:350 B:450,N:550,T:150 C:300 B:450
```
This specifies a time step where the focus car is in lane 2 (Center lane) and there is a car in the Left lane at distance 350 and a Bike farther off in the Left lane (At distance 450), a truck in the Right lane at distance 150 with a car beyond it (at distance 300) and a bike beyond that (at distance 450).  There is still nothing visible in the center lane.

For the 3-lane trace, the commas separate the contents of the Obstacle lanes (and the red car's lane position) such the the first number is the red car's lane, the next set of values after that first comma and before the second are obstacels in the Left lane (but not the lef-hazard lane, which has no obstacles allowed in the 3-lane format) and so forth.  The format provides specification for obstacles in the Left, Middle and Right lanes.

For a 5-lane input trace, this format is extended to include two more lane entries, one before the Left lane (to correspond to the Left-Hazard Lane) and one after the Right lane (corresponding to the Right-Hazard Lane).
An exmaple of a time-step from a 5-lane trace is:
```
  2,T:150 c:400,N:550,C:275 C:450,T:150 C:300,B:450
```
which specifies a Truck and a Car in the Left-Haard lane, nothing in the Left lane, two cars in the Middle lane, a Truck and a Car in the Right lane, and a Bike in the Right-Hazard lane.


# Contacts and Current Maintainers

 - Alice Song (Alice.Song1@ibm.com)
 - J-D Wellman (wellman@us.ibm.com)
