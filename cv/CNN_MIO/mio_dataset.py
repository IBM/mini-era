import cv2
import numpy as np
import tensorflow as tf
import pdb
from os import listdir
from os.path import isfile, join

train_dir= "/opt/datasets/MIO-TCD-Classification/train/"
classes = ["background", "bicycle", "bus", "car", "pedestrian"] ## coded as 1,2,3,4,5

NUM_CLASSES= len(classes)
IMG_WIDTH=32
IMG_HEIGHT =32
NUM_TRAINING_IMAGES_IN_CLASS = 2000  ## choose 10k images per class as of now
NUM_TRAINING_IMAGES=NUM_TRAINING_IMAGES_IN_CLASS*NUM_CLASSES
NUM_TEST_IMAGES_IN_CLASS = 200  
NUM_TEST_IMAGES=NUM_TEST_IMAGES_IN_CLASS*NUM_CLASSES

np_features = np.zeros((NUM_TRAINING_IMAGES,IMG_HEIGHT, IMG_WIDTH,3), np.uint8)
np_labels = np.zeros((NUM_TRAINING_IMAGES),np.uint64)
np_test_features = np.zeros((NUM_TEST_IMAGES,IMG_HEIGHT, IMG_WIDTH,3), np.uint8)
np_test_labels = np.zeros((NUM_TEST_IMAGES),np.uint64)
cid=0
for c in classes:
    img_dir = train_dir + c
    img_fnames = [(img_dir+"/"+f) for f in listdir(img_dir) if isfile(img_dir + "/" + f)]
    for i in range(0,NUM_TRAINING_IMAGES_IN_CLASS):
        print("class", c, "image", i)
        img = cv2.imread(img_fnames[i])
        img = cv2.resize(img, (IMG_HEIGHT,IMG_WIDTH))
        np_img = np.array(img, np.uint8)
        np_features[cid*NUM_TRAINING_IMAGES_IN_CLASS+i,:,:,:]=np_img
        np_labels[cid*NUM_TRAINING_IMAGES_IN_CLASS+i] = cid+1
    cid+=1
np.save('training_images.npy', np_features)
np.save('training_labels.npy', np_labels)


cid=0
for c in classes:
    img_dir = train_dir + c
    img_fnames = [(img_dir+"/"+f) for f in listdir(img_dir) if isfile(img_dir + "/" + f)]
    for i in range(NUM_TRAINING_IMAGES_IN_CLASS, NUM_TRAINING_IMAGES_IN_CLASS+NUM_TEST_IMAGES_IN_CLASS):
        print("class", c, "image", i)
        img = cv2.imread(img_fnames[i])
        img = cv2.resize(img, (IMG_HEIGHT,IMG_WIDTH))
        np_img = np.array(img, np.uint8)
        iindex= i-NUM_TRAINING_IMAGES_IN_CLASS
        np_test_features[cid*NUM_TEST_IMAGES_IN_CLASS+iindex,:,:,:]=np_img
        np_test_labels[cid*NUM_TEST_IMAGES_IN_CLASS+iindex] = cid+1
    cid+=1
np.save('test_images.npy', np_test_features)
np.save('test_labels.npy', np_test_labels)

