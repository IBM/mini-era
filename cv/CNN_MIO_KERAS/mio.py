### This code picks a random image belonging to a object class  as an input, and predicts the label. The random image is assumed to come from a road object trace, which constains objects and distances of objects on each lane. The returned label is used to update the vehicle state.  

import sys
import argparse
import numpy as np
import os
import keras
from keras.models import Sequential, load_model
from keras.layers import Dense, Dropout, Flatten, Activation
from keras.layers import Conv2D, MaxPooling2D, ZeroPadding2D
from keras.callbacks import ModelCheckpoint
from keras.optimizers import SGD

from keras import backend as K
#from frontend.approxhpvm_translator import translate_to_approxhpvm
import pdb

batch_size = 1
num_classes = 5
img_size = 32
num_channels = 3
run_dir = './cv/CNN_MIO_KERAS/'
#run_dir = './'


def mio_model():
    model = Sequential()

    model.add(Conv2D(32, (3, 3), activation='relu', input_shape=(3, img_size, img_size)))
    model.add(Conv2D(32, (3, 3), activation='relu'))
    model.add(MaxPooling2D(pool_size=(2, 2)))
    model.add(Dropout(0.25))

    model.add(Conv2D(64, (3, 3), activation='relu'))
    model.add(Conv2D(64, (3, 3), activation='relu'))
    model.add(MaxPooling2D(pool_size=(2, 2)))
    model.add(Dropout(0.25))

    model.add(Flatten())
    model.add(Dense(256, activation='relu'))
    model.add(Dropout(0.5))
    model.add(Dense(num_classes, activation='softmax'))
    return model

def _get_images_labels(dsplit):
  if dsplit == 'train':  
    features = np.load(run_dir + 'training_images.npy')
    labels_t = np.load(run_dir + 'training_labels.npy')
    shuffle_buffer_size = 6000
  else:  
    features = np.load(run_dir + 'test_images.npy')
    labels_t = np.load(run_dir + 'test_labels.npy')
    shuffle_buffer_size = 1000
  assert features.shape[0] == labels_t.shape[0]
  labels = labels_t 
  return features,labels

def predict(imagetype):
  K.set_image_data_format('channels_first')
  if imagetype == 4:
      imageid = np.random.randint(4000,4999)
  elif imagetype == 3:
      imageid = np.random.randint(3000,3999)
  elif imagetype == 2:
      imageid = np.random.randint(2000,2999)
  elif imagetype == 1:
      imageid = np.random.randint(1000,1999)
  else: 
      imageid = np.random.randint(0,999)
  print('Received image type',imagetype)

  test_image = np.zeros((1, num_channels, img_size, img_size), np.float32)
  test_features,test_labels = _get_images_labels('test')
  test_image[0,:,:,:] = test_features[imageid,:,:,:]
  model = mio_model()
  model.load_weights(run_dir + 'model.h5')

  outputs = model.predict(test_image, batch_size=batch_size)
  predicted_labels_t=(outputs.argmax(1))
  predicted_labels=np.reshape(predicted_labels_t, [predicted_labels_t.size, 1])
  print("Predicted Label:")
  print(predicted_labels[0,0])
  return predicted_labels[0,0]
    
  # translate_to_approxhpvm(model, "data/lenet_hpvm/", X_test, Y_test, num_classes)

def main(argv):

  print('Running command:', str(sys.argv))
  parser = argparse.ArgumentParser()
	
  parser.add_argument("-t", "--objecttype", type=int, help="Class from the road object trace")

  args = parser.parse_args()
  predict(args.objecttype)  

if __name__ == "__main__":   
  main(sys.argv[1:])


