# Mini-ERA: Utilities

This directory contains utility programs for the Mini-ERA.

The primary tools here are for trace generation to drive the Mini-ERA (and to a lesser extent the Visualizer).
This includes two primary tools to generate traces:
 - tracegen : generates mini-era input traces
 - debug_tracegen : adds debugging verbose output to the tracegen tool.

There is also a compiled version to output Visualizer-compatible traces
 - vistracegen
 - debug_vistracegen
These are the same program, but output a very slightly different trace that includes a denotation that the focus car is always in the center lane, and therefore produce a trace compatible with the visualizer (but not reflecting any focus car actions).

## Installation 

This code is a part of the mini-era distribution, and should be acquired through a mini-era github clone:
```
git clone https://github.com/IBM/mini-era.git
```

Once the repository is cloned:
```
cd mini-era/utils
make all
```

## Execution
The current trace-generation programs use compile-time parameters, so they have not command-line parameters (at present; this will change).
The invocation is therefore straightforward:
```
tracegen
```

## Output

The output of the ```tracegen``` program is a mini-era input trace; this should probably be redirected into a file, e.g.
```
tracegen > my_new_trace_file.txt
```
The output is conformant to the trace input described in the Mini-ERA README.md file.

The ```vistracegen``` performs the same way, producing a Visualizer-compatible trace, e.g.
```
vistracegen > my_new_vis_trace_file
```


## Options

There are a number of options coded into the trace generator.  The tracegen program acutally simulates a virtual world (very simplistically) and follwos
some rules about when to inject new obstacle objects into the three main lanes (Left, Center, Right) and then tracks these obstacles as the focus
car progresses forward (overtaking them).

Most of the options are specified in the ```sim_environs.c``` files.  Some of the notable ones are described here:

### Injecting New Obstacles
We have a primary control to determine whether we restrict to a single obstacles in a lane or we allow multiple per lane simultaneously.
Currently, the Mini-Era only supports a single obstacle per lane, but this is high on the list for updates.
 - one_obstacle_per_lane = true; // false = unlimited
One can generate a Visualization trace using ```vistracegen``` with multiple obstacles per lane, and then visualize the obstacles proceeding
through the environment (though not the reaction of the focus car, until Mini-ERA supports multiple obstacles per lane).

We generally use a random integer between [0,99] to select from options.
These are used in a compound set of IF statements:
 - NEW_OBJ_THRESHOLD : RAND > this to create new obstacle object
 - NEW_OBJ_CAR_THRESHOLD : RAND < this -- it is a car
 - NEW_OBJ_TRUCK_THRESHOLD : RAND >= car and < this -- it is a truck
 - NEW_OBJ_BIKE_THRESHOLD : RAND >= truck and < this, it is a bike (else a person)

### Object Properties
The following defines and arrays are used to determine the speed of the obstacle.  Each obstacle type has its own list of possible speeds,
and a set of selection thresholds to select which speed:
 - NUM_CAR_SPEEDS      5
 - NUM_TRUCK_SPEEDS    4
 - NUM_BIKE_SPEEDS     3
 - NUM_PERSON_SPEEDS   2

The speeds are defined in one array, and the array of thresholds in another. The general selection is ```RAND < threshold -> use that speed```:
 - car_sp_thds[NUM_CAR_SPEEDS]       = { 15, 65, 85, 95, 100 }; // The thresholds for each speed (RAND: 0-99)
 - car_speeds[NUM_CAR_SPEEDS]        = { 45, 40, 35, 30, 25 };  // The possible speeds
 - truck_sp_thds[NUM_TRUCK_SPEEDS]   = { 20, 65, 95, 100 }; // The thresholds for each speed (RAND: 0-99)
 - truck_speeds[NUM_TRUCK_SPEEDS]    = { 40, 35, 30, 25 };
 - bike_sp_thds[NUM_BIKE_SPEEDS]     = { 30, 75, 100 }; // The thresholds for each speed (RAND: 0-99)
 - bike_speeds[NUM_BIKE_SPEEDS]      = { 30, 25, 20 };
 - person_sp_thds[NUM_PERSON_SPEEDS] = { 50, 100 }; // The thresholds for each speed (RAND: 0-99)
 - person_speeds[NUM_PERSON_SPEEDS]  = { 15, 10 };


## Contacts and Current Maintainers

 - J-D Wellman (wellman@us.ibm.com)
