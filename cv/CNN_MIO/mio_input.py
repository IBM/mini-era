# Copyright 2015 The TensorFlow Authors. All Rights Reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
# ==============================================================================

"""Routine for decoding the CIFAR-10 binary file format."""

from __future__ import absolute_import
from __future__ import division
from __future__ import print_function

import tensorflow as tf
#import tensorflow_datasets as tfds
import numpy as np
IMAGE_SIZE = 32

NUM_CLASSES = 5
NUM_EXAMPLES_PER_EPOCH_FOR_TRAIN = 500
NUM_EXAMPLES_PER_EPOCH_FOR_EVAL = 1000

# original function 
#def _get_images_labels(batch_size, split, distords=False):
#  """Returns Dataset for given split."""
#  dataset = tfds.load(name='cifar10', split=split)
#  scope = 'data_augmentation' if distords else 'input'
#  with tf.name_scope(scope):
#    dataset = dataset.map(DataPreprocessor(distords), num_parallel_calls=10)
#  # Dataset is small enough to be fully loaded on memory:
#  dataset = dataset.prefetch(-1)
#  dataset = dataset.repeat().batch(batch_size)
#  iterator = dataset.make_one_shot_iterator()
#  images_labels = iterator.get_next()
#  images, labels = images_labels['input'], images_labels['target']
#  tf.summary.image('images', images)
#  return images, labels
#

# my function
def _get_images_labels(dsplit,batch_size,distords):
  if dsplit == 'train':  
    features = np.load("./NPY_FILES/training_images.npy")
    labels = np.load("./NPY_FILES/training_labels.npy")
    shuffle_buffer_size = 6000
  else:  
    features = np.load("./NPY_FILES/test_images.npy")
    labels = np.load("./NPY_FILES/test_labels.npy")
    shuffle_buffer_size = 1000
  assert features.shape[0] == labels.shape[0]
  features_placeholder = tf.placeholder(tf.float32, features.shape)
  labels_placeholder = tf.placeholder(tf.int32, labels.shape)
  dataset = tf.data.Dataset.from_tensor_slices({"image": features_placeholder, "label": labels_placeholder})
  dataset = dataset.map(DataPreprocessor(distords), num_parallel_calls=10)
  dataset = dataset.repeat()  ## repeating the dataset indefinitely ...
  dataset = dataset.shuffle(shuffle_buffer_size)
  dataset = dataset.batch(batch_size)   ## Making buffer size the same as number of example size
  iterator = dataset.make_initializable_iterator()
  return iterator, features_placeholder, labels_placeholder, features, labels



class DataPreprocessor(object):
  """Applies transformations to dataset record."""

  def __init__(self, distords):
    self._distords = distords

  def __call__(self, record):
    """Process img for training or eval."""
    img = record['image']
    lab = record['label']
    img = tf.cast(img, tf.float32)
    #img = tf.cast(img, tf.float16)
    if self._distords:  # training
      # Randomly crop a [height, width] section of the image.
      img = tf.random_crop(img, [IMAGE_SIZE, IMAGE_SIZE, 3])
      # Randomly flip the image horizontally.
      img = tf.image.random_flip_left_right(img)
      # Because these operations are not commutative, consider randomizing
      # the order their operation.
      # NOTE: since per_image_standardization zeros the mean and makes
      # the stddev unit, this likely has no effect see tensorflow#1458.
      img = tf.image.random_brightness(img, max_delta=63)
      img = tf.image.random_contrast(img, lower=0.2, upper=1.8)
    else:  # Image processing for evaluation.
      # Crop the central [height, width] of the image.
      img = tf.image.resize_image_with_crop_or_pad(img, IMAGE_SIZE, IMAGE_SIZE)
    # Subtract off the mean and divide by the variance of the pixels.
    img = tf.image.per_image_standardization(img)
    img = tf.cast(img, tf.float32)
    lab = tf.cast(lab, tf.int32)
    #return dict(input=img, target=record['label'])
    return dict(image=img, label=lab)


def distorted_inputs(batch_size):
  """.

  Args:
    batch_size: Number of images per batch.

  Returns:
    images: Images. 4D tensor of [batch_size, IMAGE_SIZE, IMAGE_SIZE, 3] size.
    labels: Labels. 1D tensor of [batch_size] size.
  """
  #return _get_images_labels(batch_size, tfds.Split.TRAIN, distords=True)
  return _get_images_labels('train', batch_size, distords=False)


def inputs(batch_size):
  """Construct input for CIFAR evaluation using the Reader ops.

  Args:
    eval_data: bool, indicating if one should use the train or eval data set.
    batch_size: Number of images per batch.

  Returns:
    images: Images. 4D tensor of [batch_size, IMAGE_SIZE, IMAGE_SIZE, 3] size.
    labels: Labels. 1D tensor of [batch_size] size.
  """
  #split = tfds.Split.TEST if eval_data == 'test' else tfds.Split.TRAIN
  return _get_images_labels('test', batch_size, distords=False)
