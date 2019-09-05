# Mini-ERA Visualizer

Visualization for Mini-Era using pygame.

# Overview
The visualizer is a simple pygame-based 5-lane simulation of the focus car (drawn in red) driving on a straight 3-lane highway at some speed, and overtaking various other vehicles (obstacle objects) on the road.  These obstacle objects will occupy positions initially ahead of the focus car, and can include other cars (drawn in blue), trucks, and bikes.  All of these obstacle objects will be moving slower than the focus car, and therefore progress down the screen closer and closer to the focus car, which must eventually take actions to avoid collisions.

# Invocation
The visualizer is a python program, invocation is as follows:
```
  python visualizer.py -t <tracefile> -d <frame_delay>
```

The tracefile is the input to the visualizer, and there is **NO** default value so a tracefile **MUST** be specified.
The frame delay is a delay value between frames (in milliseconds), with a default of 500 (i.e. 2 frames per second).

There are some example trace files proveded, which correspond to the example trace files of the main Mini-ERA program,
i.e. the visualizer trace ```tt00.viz``` corresponds to the Mini-ERA input trace ```tt00.new``` and similarly with the
```tt01.*``` and ```tt02.*``` traces (and also the ```test_trace1.new``` and ```test_trace1.viz``` files).


Note: there is a second version of the visualizer program for single-step operation, called ```visualizer_ss.py``` which waits on each trace step for the user to hit the enter key before advancing the visualizer state.  This has limited value in general, but can be useful to study the details of a particular sequence or portion of a trace visualization, and is quite helpful when trying to take interesting visualizer screen captures.
The single-step version is invoked in much the same way, but does not support a frame delay parameter.
The visualizer is a python program, invocation is as follows:
```
  python visualizer_ss.py -t <tracefile>
```

# Input Trace Format
The input trace format is very similar to the mini-era format, but also incudes the current lane position of the focus car:
```
  L,X:y,X:y,X:y
```
Here, the L is the lane indicator, and each X is an object type (N = none, C = car, T = truck, B = bike) and y is a distance (in the range [0, 550]).
Each lane X:y is sepatated from the next by a comma character, so the first X:y is the left lane, the second X:y is the center lane, and the third is the right lane. A simple example:
```
  2,C:350,N:550,T:150
```
This specifies a time step where the focus car is in lane 2 (Center lane) and there is a car in the left lane at distance 350, and a truck in the right lane at distance 150 (and nothing in the center lane).

The visualizer also supports multiple obstacle objects per lane, which are then separated by space characters.  An example:
```
  2,C:350 B:450,N:550,T:150 C:300 B:450
```
This specifies a time step where the focus car is in lane 2 (Center lane) and there is a car in the left lane at distance 350 and a Bike farther off in the left lane (At distance 450), a truck in the right lane at distance 150 with a car beyond it (at distance 300) and a bike beyond that (at distance 450).  There is still nothing visible in the center lane.

# Contacts and Current Maintainers

 - Alice Song (Alice.Song1@ibm.com)
 - J-D Wellman (wellman@us.ibm.com)
