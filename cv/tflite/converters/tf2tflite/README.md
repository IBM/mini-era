<!--
Copyright (c) 2011-2020 Columbia University, System Level Design Group
SPDX-License-Identifier: Apache-2.0
-->

# TensorFlow Lite Converter

The `converter.py` script uses the TensorFlow Lite Converter Python API to convert TensorFlow models into the TensorFlow Lite format.

We provide a `Makefile` for your convenience.

- To generate the TFLite model (`.tflite`, flatbuffer) and show the model files (both `.h5` and `.tflite`):
  ```
  make model
  ```
  The visualization requires [Netron](https://github.com/lutzroeder/Netron).

- To remove the TFLite flatbuffer:
  ```
  make clean
  ```

- To visualize the model with [Netron](https://github.com/lutzroeder/Netron):
  ```
  make visualize
  ```
 
For more details see the [official documentation](https://www.tensorflow.org/lite/convert)
