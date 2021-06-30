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

"""A binary to train CIFAR-10 using a single GPU.

Accuracy:
model_build_train.py achieves ~86% accuracy after 100K steps (256 epochs of
data) as judged by model_build_eval.py.

Speed: With batch_size 128.

System        | Step Time (sec/batch)  |     Accuracy
------------------------------------------------------------------
1 Tesla K20m  | 0.35-0.60              | ~86% at 60K steps  (5 hours)
1 Tesla K40m  | 0.25-0.35              | ~86% at 100K steps (4 hours)

Usage:
Please see the tutorial and website for how to download the CIFAR-10
data set, compile the program and train the model.

http://tensorflow.org/tutorials/deep_cnn/
"""
from __future__ import absolute_import
from __future__ import division
from __future__ import print_function

from datetime import datetime
import time
import numpy as np
import tensorflow as tf


def main(argv=None):  # pylint: disable=unused-argument
  features_placeholder = tf.placeholder(tf.float32, shape=[100,2,2])
  labels_placeholder = tf.placeholder(tf.float32, shape=[100,1])
  dataset = tf.data.Dataset.from_tensor_slices({'features': features_placeholder, 'labels' : labels_placeholder})
  data = np.random.sample((100,2,2))
  target = np.random.sample((100,1))
  iterator = dataset.make_initializable_iterator() # create the iterator
  images_labels = iterator.get_next()
  features = images_labels['features']
  labels = images_labels['labels']
  result = tf.add(features, features)
  with tf.Session() as sess:
          # feed the placeholder with data
    sess.run(iterator.initializer, feed_dict={features_placeholder: data, labels_placeholder: target}) 
    print(sess.run([features, result])) # output [ 0.52374458  0.71968478]rain()


if __name__ == '__main__':
  tf.app.run()
