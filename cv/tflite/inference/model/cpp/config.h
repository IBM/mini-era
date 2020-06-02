// Copyright (c) 2011-2020 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0

#ifndef CONFIG_H_
#define CONFIG_H_

#include "tensorflow/lite/model.h"
#include "tensorflow/lite/string.h"

namespace tflite {
    namespace tflite_application {

        // BEGIN-MODEL-DEPENDENT
        const std::string APPLICATION_NAME = "MODEL";
        // END-MODEL-DEPENDENT

        struct Settings {
            int image_index = -1;
            int batch_size = 1;
            bool verbose = true;
            string model_file;
#ifdef USE_DATA_READER
            string data_dir;
#endif
            bool trace_profiling = false;
            int loop_count = 10;
            int number_of_results = 3;
            int max_trace_profiling_buffer_entries = 100;
            int warmup_runs = 10;
        };

    }  // namespace tflite_application
}  // namespace tflite

#endif // CONFIG_H_
