import os, sys

# Generate a trace
# obj_type = ['C', 'M', 'T']
screen_height = 500

def slow(speed, start_dis, tr, obj):
    # objects would be moving down
    # y position increasing
    # distance decreasing

    inc = 0
    new_dis = start_dis
    obj_type = ''

    if speed == 1:
        # slowest speed
        # y position increases faster
        inc = 15 # 15 pixels
    elif speed == 2:
        inc = 10
    elif speed == 3:
        inc = 5

    if obj == 0:
        obj_type = 'C' # car
    elif obj == 1:
        obj_type = 'M' # motorcycle
    elif obj == 2:
        obj_type = 'T' # truck

    while new_dis >=0:
        tr_entry = obj_type + str(new_dis)
        tr.append(tr_entry)
        new_dis -= inc

def fast(speed, start_dis, tr, obj):
    # objects would be moving up
    # y position decreasing
    # distance increasing

    inc = 0
    new_dis = start_dis
    obj_type = ''

    if speed == 1:
        # slowest speed
        # y position decreasing slower
        inc = 5
    elif speed == 2:
        inc = 10
    elif speed == 3:
        inc = 15

    if obj == 0:
        obj_type = 'C' # car
    elif obj == 1:
        obj_type = 'M' # motorcycle
    elif obj == 2:
        obj_type = 'T' # truck

    while new_dis <= 550:
        tr_entry = obj_type + str(new_dis)
        tr.append(tr_entry)
        new_dis += inc 

def cruise(start_dis, tr, obj):
    # objects moving up and down

    inc = 0
    new_dis = start_dis
    obj_type = ''

    if obj == 0:
        obj_type = 'C' # car
    elif obj == 1:
        obj_type = 'M' # motorcycle
    elif obj == 2:
        obj_type = 'T' # truck

    tr_entry = obj_type + str(new_dis)
    tr.append(tr_entry)

def main():
    left_tr = []

    fast(1, 100, left_tr, 0)
    fast(3, 0, left_tr, 1)

    right_tr = []
    slow(2, 350, right_tr, 0)
    slow(1, 550, right_tr, 1)


    return left_tr, right_tr

if __name__ == '__main__':
    left, right = main()
    print(left)
    print(len(left))
    print(right)
    print(len(right))
    