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

opt_type='sgd'
batch_size = 32
epochs = 50
num_classes = 5
img_size = 32

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
    features = np.load("./training_images.npy")
    labels_t = np.load("./training_labels.npy")
    shuffle_buffer_size = 6000
  else:  
    features = np.load("./test_images.npy")
    labels_t = np.load("./test_labels.npy")
    shuffle_buffer_size = 1000
  assert features.shape[0] == labels_t.shape[0]
  labels = keras.utils.to_categorical(labels_t, num_classes=num_classes)
  #labels = labels_t 
  return features,labels

if __name__ == "__main__":   

  K.set_image_data_format('channels_first')


  features,labels = _get_images_labels('train')
  test_features,test_labels = _get_images_labels('test')
  model = mio_model()

  if opt_type == 'sgd':
    sgd = SGD(lr=0.01, decay=1e-6, momentum=0.9, nesterov=True)
    model.compile(loss='categorical_crossentropy', optimizer=sgd, metrics=['accuracy'])
  else: 
    model.compile(optimizer=keras.optimizers.Adam(), loss=keras.losses.categorical_crossentropy, metrics=['accuracy'])

  model.fit(features, labels, batch_size=batch_size, epochs=epochs , verbose=1, callbacks=[ModelCheckpoint('model.h5')], shuffle=True)
  score = model.evaluate(test_features, test_labels, batch_size=batch_size)
  print("Inference loss:", score[0], "Inference Accuracy:", score[1])

  #model = load_model('model.h5')

  model.summary()
    
    # translate_to_approxhpvm(model, "data/lenet_hpvm/", X_test, Y_test, num_classes)

