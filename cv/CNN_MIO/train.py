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

from __future__ import absolute_import
from __future__ import division
from __future__ import print_function

from datetime import datetime
import time

import tensorflow as tf

import model_build

FLAGS = tf.app.flags.FLAGS

tf.app.flags.DEFINE_string('train_dir', '/tmp/model_build_train',
                           """Directory where to write event logs """
                           """and checkpoint.""")
tf.app.flags.DEFINE_integer('max_steps', 5000,
                            """Number of batches to run.""")
tf.app.flags.DEFINE_boolean('log_device_placement', True,
                            """Whether to log device placement.""")
tf.app.flags.DEFINE_integer('log_frequency', 10,
                            """How often to log results to the console.""")


def train():

  # This function originally was too complex and has several layers of abstraction , so I rewrote it.   
  with tf.Graph().as_default():
    global_step = tf.train.get_or_create_global_step()

    # Force input pipeline to CPU:0 to avoid operations sometimes ending up on
    # GPU and resulting in a slow down.
    with tf.device('/cpu:0'):
      #images, labels = model_build.distorted_inputs()
      iterator, features_placeholder, labels_placeholder, np_features, np_labels = model_build.distorted_inputs()
      images_labels = iterator.get_next()
      images, labels = images_labels['image'], images_labels['label']
    # Build a Graph that computes the logits predictions from the
    # inference model.
    logits = model_build.inference(images)

    # Calculate loss.
    loss = model_build.loss(logits, labels)

        # Build a Graph that trains the model with one batch of examples and
        # updates the model parameters.
    train_op = model_build.train(loss, global_step)

    #summary = tf.summary.merge_all()

    init = tf.global_variables_initializer()
    saver = tf.train.Saver()
    config=tf.ConfigProto(log_device_placement=FLAGS.log_device_placement)
    sess = tf.Session(config=config)
    # Instantiate a SummaryWriter to output summaries and the Graph.
    #summary_writer = tf.summary.FileWriter(FLAGS.log_dir, sess.graph)

    sess.run(init)

    for step in range(FLAGS.max_steps):
        start_time = time.time()
        sess.run(iterator.initializer, feed_dict={features_placeholder:np_features, labels_placeholder:np_labels})
        train_op_value, loss_value, logits_value, labels_value = sess.run([train_op,loss, logits, labels])
        duration = time.time() - start_time
        if (step%100 == 0 or (step+1) == FLAGS.max_steps) : 
            #print('Step, loss, sec, labels', step, loss_value,duration, labels_value)
            print('Step, loss, sec\n', step, loss_value,duration)
            if (step%1000 == 0 or (step+1) == FLAGS.max_steps):
                ckpt_fname = 'mio_model.ckpt'
                saver.save(sess,ckpt_fname,global_step=step)

            # Update the events file.
            #summary_str = sess.run(summary, feed_dict={features_placeholder:np_features, labels_placeholder:np_labels})
            #summary_writer.add_summary(summary_str, step)
            #summary_writer.flush()


   
def main(argv=None):  # pylint: disable=unused-argument
  if tf.gfile.Exists(FLAGS.train_dir):
    tf.gfile.DeleteRecursively(FLAGS.train_dir)
  tf.gfile.MakeDirs(FLAGS.train_dir)
  train()


if __name__ == '__main__':
  tf.app.run()
