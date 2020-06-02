# Copyright (c) 2011-2020 Columbia University, System Level Design Group
# SPDX-License-Identifier: Apache-2.0

from __future__ import print_function
import sys
import tensorflow as tf


def main(argv):
    tf_model_file = ''
    tflite_model_file = ''

    if (len(argv) != 3):
        print('ERROR: usage:', argv[0], '<tf-model.h5> <tflite-model.tflite>')
        return 1
    else:
        tf_model_file = argv[1]
        tflite_model_file = argv[2]

        tf_model = tf.keras.models.load_model(tf_model_file)
        converter = tf.lite.TFLiteConverter.from_keras_model(tf_model)
        tflite_model = converter.convert()
        open(tflite_model_file, 'wb').write(tflite_model)
        return 0

if __name__ == "__main__":
    main(sys.argv[0:])
