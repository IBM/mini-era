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

batch_size = 32
num_classes = 5
img_size = 32

#run_dir = './cv/CNN_MIO_KERAS/'
run_dir = './'


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

if __name__ == "__main__":   

  K.set_image_data_format('channels_first')


  test_features,test_labels = _get_images_labels('test')
  model = mio_model()

  model.load_weights(run_dir + 'model.h5')

  outputs = model.predict(test_features, batch_size=batch_size)
  predicted_labels_t=(outputs.argmax(1))
  predicted_labels=np.reshape(predicted_labels_t, [predicted_labels_t.size, 1])
  inference_accuracy = 100*np.sum(test_labels == predicted_labels)/predicted_labels.size
  print("Inference accuracy", inference_accuracy)
    
  # translate_to_approxhpvm(model, "data/lenet_hpvm/", X_test, Y_test, num_classes)

