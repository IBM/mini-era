# Copyright (c) 2011-2020 Columbia University, System Level Design Group
# SPDX-License-Identifier: Apache-2.0

from __future__ import print_function

import numpy as np
import pandas as pd

#from sklearn.preprocessing import LabelEncoder
#from sklearn.preprocessing import OneHotEncoder
#from sklearn.preprocessing import StandardScaler
#from sklearn.model_selection import train_test_split

import argparse

#def run_onehot_encoder(data_df):
#    label_encoder = LabelEncoder()
#    onehot_encoder = OneHotEncoder(sparse=False, categories='auto')
#    data = label_encoder.fit_transform(data_df)
#    data = data.reshape(len(data), 1)
#    data = onehot_encoder.fit_transform(data)
#    return data

def load_dataset(args):

    X_test = np.load(args.test_images)[3000+args.range_begin:3000+args.range_end]
    y_test = np.load(args.test_labels)[3000+args.range_begin:3000+args.range_end]

    #y_test = run_onehot_encoder(y_test)

    print(X_test.shape)
    print('INFO:', X_test.shape[0], 'test images')

    return X_test, y_test

def save_header_dataset(X, y, n_images, filename, X_dtype='float32', y_dtype='int32'):
    print('INFO: Saving ', n_images, ' images in ',  filename)

    fd = open(filename, "w+")

    # Image array
    fd.write('float testset_input_data[] = {\n')
    data = X[0:n_images].flatten()
    fd.write('%f' % data[0])
    for d in data[1:]:
        fd.write(', %f' % d)
    fd.write('\n};\n')

    # Label array
    fd.write('unsigned testset_input_labels[] = {\n')
    data = y
    fd.write('%d' % data[0])
    for d in data[1:]:
        fd.write(', %d' % d)
    fd.write('\n};\n')

    # Number of images and labels
    fd.write('unsigned testset_input_count = %d;\n' % n_images)
    fd.write('unsigned testset_label_count = %d;\n' % n_images)

    fd.write('unsigned input_nrows = %d;\n' % X.shape[1])
    fd.write('unsigned input_ncols = %d;\n' % X.shape[2])
    fd.write('unsigned input_nchnls = %d;\n' % X.shape[3])
    fd.write('unsigned label_size = %d;\n' % 5)

    fd.write('bool  testset_data_is_normalized = %d;\n' % 1)

    fd.write('unsigned testset_fxd_point_W = %d;\n' % 18)
    fd.write('unsigned testset_fxd_point_I_W18 = %d;\n' % 8)
    fd.write('unsigned testset_fxd_point_I_W32 = %d;\n' % 22)

    fd.close()

def main(args):

    # Get dataset.
    X_test, y_test = load_dataset(args)

    save_header_dataset(X_test, y_test, X_test.shape[0], args.output_dir + '/' + args.model_name + '_data.h', X_dtype='float32', y_dtype='int32')

if __name__ == '__main__':
    test_images_default = './test_images.npy'
    test_labels_default = './test_labels.npy'
    model_name_default = 'model'
    range_begin_default = 0
    range_end_default = 16
    output_dir_default = '.'

    parser = argparse.ArgumentParser(description='Convert .npy files to C/C++ header files')
    parser.add_argument('--test_images', help='Test images [' + str(test_images_default) + ']', action='store', dest='test_images', type=str, default=test_images_default)
    parser.add_argument('--test_labels', help='Test labels [' + str(test_labels_default) + ']', action='store', dest='test_labels', type=str, default=test_labels_default)
    parser.add_argument('--output_dir', help='Output directory [' + str(output_dir_default) + ']', action='store', dest='output_dir', type=str, default=output_dir_default)
    parser.add_argument('--model_name', help='Model name [' + str(model_name_default) + ']', action='store', dest='model_name', type=str, default=model_name_default)
    parser.add_argument('--range_begin', help='Range begin [' + str(range_begin_default) + ']', action='store', dest='range_begin', type=int, default=range_begin_default)
    parser.add_argument('--range_end', help='Range end [' + str(range_end_default) + ']', action='store', dest='range_end', type=int, default=range_end_default)
    args = parser.parse_args()

    main(args)

