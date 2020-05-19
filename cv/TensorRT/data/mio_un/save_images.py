import cv2
import numpy as np
import tensorflow as tf
import pdb
from os import listdir
from os.path import isfile, join

train_dir= "/opt/datasets/MIO-TCD-Classification/train/"
classes = ["background", "bus", "car", "pedestrian", "articulated_truck"] ## coded as 0,1,2,3,4
import pdb
NUM_CLASSES= len(classes)
IMG_WIDTH=28
IMG_HEIGHT =28
NUM_CHANNELS=1
NUM_TRAINING_IMAGES_IN_CLASS = 5000  ## choose 10k images per class as of now
NUM_TRAINING_IMAGES=NUM_TRAINING_IMAGES_IN_CLASS*NUM_CLASSES
NUM_TEST_IMAGES_IN_CLASS = 1 
#NUM_TEST_IMAGES_IN_CLASS = 1 
NUM_TEST_IMAGES=NUM_TEST_IMAGES_IN_CLASS*NUM_CLASSES

ind=0
cid=0
for c in classes:
    img_dir = train_dir + c
    img_fnames = [(img_dir+"/"+f) for f in listdir(img_dir) if isfile(img_dir + "/" + f)]
    for i in range(NUM_TRAINING_IMAGES_IN_CLASS, NUM_TRAINING_IMAGES_IN_CLASS+NUM_TEST_IMAGES_IN_CLASS):
        iindex = i-NUM_TRAINING_IMAGES_IN_CLASS
        if i < len(img_fnames):
          print("class", c, "image", i)
          img = cv2.imread(img_fnames[i])
          img = cv2.resize(img, (IMG_HEIGHT,IMG_WIDTH))
          gimg = cv2.cvtColor(img,cv2.COLOR_BGR2GRAY)
          #nimg = cv2.normalize(gimg, None, 0, 1, cv2.NORM_MINMAX, dtype=cv2.CV_32F)
          cv2.imwrite(c+".jpg", gimg)
          ind+=1
    cid+=1
print(ind)

