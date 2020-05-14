import cv2
import numpy as np
import tensorflow as tf
import pdb
from os import listdir
from os.path import isfile, join

train_dir= "/opt/datasets/MIO-TCD-Classification/train/"
classes = ["background", "bus", "car", "pedestrian", "articulated_truck"] ## coded as 0,1,2,3,4

NUM_CLASSES= len(classes)
IMG_WIDTH=32
IMG_HEIGHT =32
NUM_CHANNELS=3
NUM_TRAINING_IMAGES_IN_CLASS = 5000  ## choose 10k images per class as of now
NUM_TRAINING_IMAGES=NUM_TRAINING_IMAGES_IN_CLASS*NUM_CLASSES
NUM_TEST_IMAGES_IN_CLASS = 1000  
NUM_TEST_IMAGES=NUM_TEST_IMAGES_IN_CLASS*NUM_CLASSES

np_features = np.zeros((NUM_TRAINING_IMAGES, NUM_CHANNELS, IMG_HEIGHT, IMG_WIDTH), np.float32)
np_labels = np.zeros((NUM_TRAINING_IMAGES,1),np.uint32)
np_test_features = np.zeros((NUM_TEST_IMAGES, NUM_CHANNELS, IMG_HEIGHT, IMG_WIDTH), np.float32)
np_test_labels = np.zeros((NUM_TEST_IMAGES,1),np.uint32)

cid=0
ind=0
for c in classes:
    img_dir = train_dir + c
    img_fnames = [(img_dir+"/"+f) for f in listdir(img_dir) if isfile(img_dir + "/" + f)]
    for i in range(0,NUM_TRAINING_IMAGES_IN_CLASS):
        print("class", c, "image", i)
        img = cv2.imread(img_fnames[i])
        img = cv2.resize(img, (IMG_HEIGHT,IMG_WIDTH))
        nimg = np.zeros((IMG_HEIGHT,IMG_WIDTH))
        nimg = cv2.normalize(img, nimg, 0, 1, cv2.NORM_MINMAX, dtype=cv2.CV_32F)
        np_img_reshape = nimg.reshape([NUM_CHANNELS, IMG_HEIGHT, IMG_WIDTH])
        for ch in range(0,NUM_CHANNELS):
            np_img_reshape[ch, 0:IMG_HEIGHT, 0:IMG_WIDTH] = nimg[:,:,ch]


        np_features[ind,:,:,:]=np_img_reshape
        np_labels[ind,:] = cid
        ind+=1
    cid+=1
np.save('training_images.npy', np_features)
np.save('training_labels.npy', np_labels)

print(ind)

cid=0
ind=0
for c in classes:
    img_dir = train_dir + c
    img_fnames = [(img_dir+"/"+f) for f in listdir(img_dir) if isfile(img_dir + "/" + f)]
    for i in range(NUM_TRAINING_IMAGES_IN_CLASS, NUM_TRAINING_IMAGES_IN_CLASS+NUM_TEST_IMAGES_IN_CLASS):
        iindex = i-NUM_TRAINING_IMAGES_IN_CLASS
        if i < len(img_fnames):
          print("class", c, "image", i)
          img = cv2.imread(img_fnames[i])
          img = cv2.resize(img, (IMG_HEIGHT,IMG_WIDTH))
          nimg = np.zeros((IMG_HEIGHT,IMG_WIDTH))
          nimg = cv2.normalize(img, nimg, 0, 1, cv2.NORM_MINMAX,dtype=cv2.CV_32F)
          np_img_reshape = nimg.reshape([NUM_CHANNELS, IMG_HEIGHT, IMG_WIDTH])
          for ch in range(0,NUM_CHANNELS):
            np_img_reshape[ch, 0:IMG_HEIGHT, 0:IMG_WIDTH] = nimg[:,:,ch]
          np_test_features[ind,:,:,:]=np_img_reshape
          np_test_labels[ind,:] = cid
          ind+=1
    cid+=1
print(ind)
np.save('test_images.npy', np_test_features)
np.save('test_labels.npy', np_test_labels)

