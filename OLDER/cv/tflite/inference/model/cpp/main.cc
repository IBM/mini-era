// Copyright (c) 2011-2020 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0

#include "config.h"

#include <getopt.h>
#include <sys/time.h>
#include <iomanip>
#include <iostream>
#include <queue>
//#include <ctime>

#ifdef USE_OPENCV
#ifdef __x86_64
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#endif
#endif

//#include "absl/memory/memory.h"
#include "tensorflow/lite/profiling/profiler.h"
#include "tensorflow/lite/tools/evaluation/utils.h"

#include "tensorflow/lite/model.h"
#include "tensorflow/lite/interpreter.h"
#include "tensorflow/lite/kernels/register.h"
//#include "tensorflow/lite/optional_debug_tools.h"

// BEGIN-MODEL-DEPENDENT
#ifdef USE_DATA_READER
#error "Data reader is undefined"
#else
#include "model_data.h"
#endif
// END-MODEL-DEPENDENT

#if defined(BUILD_MODEL_FROM_MMAP_BUFFER)
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#endif

#ifdef BUILD_MODEL_FROM_MMAP_BUFFER
#include <sys/mman.h>
#endif

#define LOG_INFO std::cout << "INFO: "
#define LOG_ERROR std::cerr << "ERROR: "
#define LOG_WARNING std::cerr << "WARNING: "

namespace tflite {
    namespace tflite_application {

        double get_us(struct timeval t) { return (t.tv_sec * 1000000 + t.tv_usec); }

        void get_top_n(float* prediction, unsigned batch_id, int prediction_size, size_t num_results, float threshold, std::vector<std::pair<float, int>>* top_results, bool input_floating) {
            // Will contain top N results in ascending order.
            std::priority_queue<std::pair<float, int>, std::vector<std::pair<float, int>>, std::greater<std::pair<float, int>>> top_result_pq;

            const long count = prediction_size;
            for (int i = 0; i < count; ++i) {
                float value;
                value = prediction[batch_id*prediction_size + i];

                // Only add it if it beats the threshold and has a chance at being in
                // the top N.
                if (value < threshold) {
                    continue;
                }

                top_result_pq.push(std::pair<float, int>(value, i));

                // If at capacity, kick the smallest value out.
                if (top_result_pq.size() > num_results) {
                    top_result_pq.pop();
                }
            }

            // Copy to output vector and reverse into descending order.
            while (!top_result_pq.empty()) {
                top_results->push_back(top_result_pq.top());
                top_result_pq.pop();
            }
            std::reverse(top_results->begin(), top_results->end());
        }

//        using TfLiteDelegatePtr = tflite::Interpreter::TfLiteDelegatePtr;
//        using TfLiteDelegatePtrMap = std::map<std::string, TfLiteDelegatePtr>;

        void PrintProfilingInfo(const profiling::ProfileEvent* e, uint32_t op_index, TfLiteRegistration registration) {
            // output something like
            // time (ms) , Node xxx, OpCode xxx, symblic name
            //      5.352, Node   5, OpCode   4, DEPTHWISE_CONV_2D

            LOG_INFO << std::fixed << std::setw(10) << std::setprecision(3)
                << (e->end_timestamp_us - e->begin_timestamp_us) / 1000.0
                << ", Node " << std::setw(3) << std::setprecision(3) << op_index
                << ", OpCode " << std::setw(3) << std::setprecision(3)
                << registration.builtin_code << ", "
                << EnumNameBuiltinOperator(
                        static_cast<BuiltinOperator>(registration.builtin_code))
                << std::endl;
        }

#if defined(BUILD_MODEL_FROM_MMAP_BUFFER)
        size_t GetFilesize(const char* filename) {
            struct stat st;
            stat(filename, &st);
            return st.st_size;
        }
#endif

        std::string GetTypeAsString(TfLiteType type) {
            switch(type) {
                case kTfLiteNoType: return "kTfLiteNoType";
                case kTfLiteFloat32: return "kTfLiteFloat32";
                case kTfLiteInt32: return "kTfLiteInt32";
                case kTfLiteUInt8: return "kTfLiteUInt8";
                case kTfLiteInt64: return "kTfLiteInt64";
                case kTfLiteString: return "kTfLiteString";
                case kTfLiteBool: return "kTfLiteBool";
                case kTfLiteInt16: return "kTfLiteInt16";
                case kTfLiteComplex64: return "kTfLiteComplex64";
                case kTfLiteInt8: return "kTfLiteInt8";
                case kTfLiteFloat16: return "kTfLiteFloat16";
                default: return "unknown";
            }
        }

        void *GetTensorDataPtr(std::unique_ptr<tflite::Interpreter> &interpreter, int tensor_id) {
            TfLiteTensor *tensor = interpreter->tensor(tensor_id);
            TfLiteType type = tensor->type;
            switch(type) {
                //case kTfLiteNoType: return "kTfLiteNoType";
                case kTfLiteFloat32: return (void*) (interpreter->typed_tensor<float>(tensor_id));
                                     //case kTfLiteInt32: return "kTfLiteInt32";
                                     //case kTfLiteUInt8: return "kTfLiteUInt8";
                                     //case kTfLiteInt64: return "kTfLiteInt64";
                                     //case kTfLiteString: return "kTfLiteString";
                                     //case kTfLiteBool: return "kTfLiteBool";
                                     //case kTfLiteInt16: return "kTfLiteInt16";
                                     //case kTfLiteComplex64: return "kTfLiteComplex64";
                                     //case kTfLiteInt8: return "kTfLiteInt8";
                                     //case kTfLiteFloat16: return "kTfLiteFloat16";
                default: return NULL;
            }
        }

        std::string GetAllocationTypeAsString(TfLiteAllocationType type) {
            switch(type) {
                case kTfLiteMemNone: return "kTfLiteMemNone";
                case kTfLiteMmapRo: return "kTfLiteMmapRo";
                case kTfLiteArenaRw: return "kTfLiteArenaRw";
                case kTfLiteArenaRwPersistent: return "kTfLiteArenaRwPersistent";
                case kTfLiteDynamic: return "kTfLiteDynamic";
                default: return "unknown";
            }
        }

        std::string GetDimensionsAsString(TfLiteIntArray *array) {
            int size = array->size;
            std::string dims = "[";
            for (int i = 0; i < size; i++) {
                dims += std::to_string(array->data[i]);
                if (i+1 < size) dims += ",";
            }
            dims += "]";
            return dims;
        }

        void PrintTensorInfo(std::unique_ptr<tflite::Interpreter> &interpreter, int tensor_id) {
            TfLiteTensor *tensor = interpreter->tensor(tensor_id);
            std::string tensor_name(tensor->name);
            TfLiteType tensor_type = tensor->type;
            std::string tensor_type_as_string = GetTypeAsString(tensor_type);
            uint8_t *tensor_data_ptr = (uint8_t*)GetTensorDataPtr(interpreter, tensor_id);
            size_t tensor_size = tensor->bytes;
            TfLiteAllocationType tensor_allocation_type = tensor->allocation_type;
            TfLiteIntArray *tensor_dims = tensor->dims;
            LOG_INFO
                << "    - id: " << tensor_id
                << ", name: " << tensor_name
                << ", type: " << tensor_type_as_string
                << ", vaddr: " << (void*)tensor_data_ptr
                << ", size (B): " << tensor_size
                << ", allocation type: " << GetAllocationTypeAsString(tensor_allocation_type)
                << ", dimensions: " << GetDimensionsAsString(tensor_dims)
                << std::endl;
        }

        void PrintModelInfo(std::unique_ptr<tflite::Interpreter> &interpreter) {
            LOG_INFO << "Model Info:" << std::endl;
            LOG_INFO << "  - Number of tensors: " << interpreter->tensors_size() << "\n";
            LOG_INFO << "  - Number of nodes: " << interpreter->nodes_size() << "\n";
            LOG_INFO << "  - Number of input tensors: " << interpreter->inputs().size() << "\n";
            LOG_INFO << "  - Number of output tensors: " << interpreter->outputs().size() << "\n";

            const std::vector<int> input_ids = interpreter->inputs();
            LOG_INFO << "  - Input tensors:\n";
            for (int id: input_ids)
                PrintTensorInfo(interpreter, id);

            const std::vector<int> output_ids = interpreter->outputs();
            LOG_INFO << "  - Output tensors:\n";
            for (int id: output_ids)
                PrintTensorInfo(interpreter, id);

#if 0
            int t_size = interpreter->tensors_size();
            LOG_INFO << "  - All tensors:\n";
            for (int id = 0; id < t_size; id++) {
                if (interpreter->tensor(id)->name)
                    PrintTensorInfo(interpreter, id);
            }
#endif
        }

        void RunInference(Settings* s) {
            LOG_INFO << "\n";

            if (s->model_file.empty()) {
                LOG_ERROR << "No model file.\n";
                exit(-1);
            }
#ifdef USE_DATA_READER
            if (s->data_dir.empty()) {
                LOG_ERROR << "No data directory.\n";
                exit(-1);
            }
#error "Data reader is undefined"
            // BEGIN-MODEL-DEPENDENT
            // END-MODEL-DEPENDENT
#endif

#if defined(BUILD_MODEL_FROM_MMAP_BUFFER)
            LOG_INFO << "Build TFLite Model from mmap buffer (" << s->model_file << ")" << std::endl;

            int fd = open(s->model_file.c_str(), O_RDONLY, 0);
            if (!fd) {
                LOG_ERROR << "Failed to open file " << s->model_file << std::endl;
                exit(-1);
            }

            size_t filesize = GetFilesize(s->model_file.c_str());
            char* mmap_flatbuffer = (char*) mmap(NULL, filesize, PROT_READ, MAP_SHARED, fd, 0);
            if (mmap_flatbuffer == MAP_FAILED) {
                LOG_ERROR << "Failed to mmap file " << s->model_file << std::endl;
                exit(-1);
            }

            std::unique_ptr<tflite::FlatBufferModel> model;

            model = tflite::FlatBufferModel::BuildFromBuffer(mmap_flatbuffer, strlen(mmap_flatbuffer));
            if (!model) {
                LOG_ERROR << "Failed to build model from buffer " << s->model_file << std::endl;
                exit(-1);
            }
#else
            LOG_INFO << "Build TFLite Model from file (" << s->model_file << ")" << std::endl;

            // Load the model from file into memory as a FlatBufferModel.
            std::unique_ptr<tflite::FlatBufferModel> model;

            model = tflite::FlatBufferModel::BuildFromFile(s->model_file.c_str());
            if (!model) {
                LOG_ERROR << "Failed to build model from file (" << s->model_file << ")" << std::endl;
                exit(-1);
            }
#endif

            model->error_reporter();

            // Build an Interpreter based on an existing FlatBufferModel.
            tflite::ops::builtin::BuiltinOpResolver resolver;
            std::unique_ptr<tflite::Interpreter> interpreter;

            tflite::InterpreterBuilder builder(*model, resolver);
            builder(&interpreter);
            if (!interpreter) {
                LOG_ERROR << "Failed to construct interpreter" << std::endl;
                exit(-1);
            }

            // Fix batch size (in case).
            if (s->batch_size <= 0)
                s->batch_size = 1;
            if (((int)testset_input_count) - (s->batch_size + s->image_index) < 0)
                s->batch_size = testset_input_count - s->image_index;

            // Batching
            std::vector<int> input_shape = {s->batch_size, (int)input_nrows, (int)input_ncols, (int)input_nchnls};
            interpreter->ResizeInputTensor(interpreter->inputs()[0], input_shape);

            // Allocate and resize input tensor buffers.
            if (interpreter->AllocateTensors() != kTfLiteOk) {
                LOG_ERROR << "Failed to allocate tensors!";
            }

            // NOTE: Important to print information after the tensors have been allocated.
            if (s->verbose)
                PrintModelInfo(interpreter);

            // Fill input buffers.
            float* input_buffer = interpreter->typed_input_tensor<float>(0);
#ifdef USE_DATA_READER
            // BEGIN-MODEL-DEPENDENT
            // END-MODEL-DEPENDENT
#else
            LOG_INFO << "\n";
            // BEGIN-MODEL-DEPENDENT
            LOG_INFO << "Load dataset (mlp1layer_data.h)" << std::endl;
            // END-MODEL-DEPENDENT
            LOG_INFO << "Dataset info:" << std::endl;
            LOG_INFO << "  - Test images: # " << testset_input_count << std::endl;
            LOG_INFO << "  - Test labels: # " << testset_label_count << std::endl;
            LOG_INFO << "  - Are images normalized? (0=No, 1=Yes): " << testset_data_is_normalized << std::endl;
            if (s->image_index < 0 || s->image_index > testset_input_count) {
                srand (time(NULL));
                s->image_index = rand() % testset_input_count;
                LOG_INFO << "  - Test image index (rand): " << s->image_index << std::endl;
            } else {
                LOG_INFO << "  - Test image index: " << s->image_index << std::endl;
            }
            LOG_INFO << "  - Batch size: " << s->batch_size << std::endl;

            for (int b = 0; b < s->batch_size; b++) {
                unsigned data_offset = (s->image_index + b) * input_nrows*input_ncols*input_nchnls;
                unsigned buffer_offset = b * input_nrows*input_ncols*input_nchnls;
                for (int i = 0; i < input_nrows*input_ncols*input_nchnls; i++) {
                    if (testset_data_is_normalized) {
                        input_buffer[buffer_offset + i] = (testset_input_data[data_offset + i]);
                    } else {
                        input_buffer[buffer_offset + i] = (testset_input_data[data_offset + i] / 255.);
                    }
                    //LOG_INFO << "[" << buffer_offset + i << "]" << " <- [" << data_offset + i << "]" << std::endl;
                }
            }

#endif

            profiling::Profiler *profiler = NULL;
            if (s->trace_profiling) {
                LOG_INFO << "Enable the trace profiler" << std::endl;
                profiler = new profiling::Profiler(s->max_trace_profiling_buffer_entries);
                interpreter->SetProfiler(profiler);
                profiler->StartProfiling();
            }

            if (s->loop_count >= 1)
                for (int i = 0; i < s->warmup_runs; i++) {
                    if (interpreter->Invoke() != kTfLiteOk) {
                        LOG_ERROR << "Failed to invoke tflite!\n";
                    }
                }

            struct timeval start_time, stop_time;
            gettimeofday(&start_time, nullptr);
            for (int i = 0; i < s->loop_count; i++) {
                if (interpreter->Invoke() != kTfLiteOk) {
                    LOG_ERROR << "Failed to invoke tflite!\n";
                }
            }
            gettimeofday(&stop_time, nullptr);

            if (s->trace_profiling) {
                profiler->StopProfiling();
                auto profile_events = profiler->GetProfileEvents();
                LOG_INFO << "\n";
                LOG_INFO << "Profiler event count: " << profile_events.size() << std::endl;
                for (int i = 0; i < profile_events.size(); i++) {
                    auto op_index = profile_events[i]->event_metadata;
                    const auto node_and_registration = interpreter->node_and_registration(op_index);
                    const TfLiteRegistration registration = node_and_registration->second;
                    PrintProfilingInfo(profile_events[i], op_index, registration);
                }
                free(profiler);
            }

            LOG_INFO << "\n";

            // Read output tensor buffers.
            float* output_buffer = interpreter->typed_output_tensor<float>(0);

#if 0
            LOG_INFO << "Prediction" << std::endl;
            for (int i = 0; i < label_size; i++) {
                LOG_INFO << "  [" << i << ", @"<< (output_buffer+i) << "]: " << output_buffer[i] << ", hex: 0x" << std::hex << *((uint32_t*)&(output_buffer[i])) << std::dec << std::endl;
            }
#endif
            TfLiteIntArray* output_dims = interpreter->tensor(0)->dims;

            // Assume output dims to be something like (batch_size, 1, ... , output_size)
            auto output_size = output_dims->data[output_dims->size - 1];
            auto batch_size = output_dims->data[0];

            for (unsigned b = 0; b < batch_size; b++) {
                const float threshold = 0.001f;
                std::vector<std::pair<float, int>> top_results;

                get_top_n(interpreter->typed_output_tensor<float>(0), b, output_size, s->number_of_results, threshold, &top_results, true);
                LOG_INFO << "  - Output: " << b+1 << " (of " << s->batch_size << ")"<< std::endl;

                // BEGIN-MODEL-DEPENDENT
#ifdef USE_DATA_READER
#error "Data reader is undefined"
                LOG_INFO << "    - Expected: " << dataset.test_labels[s->image_index + b] << std::endl;
#else
                LOG_INFO << "    - Expected: " << testset_input_labels[s->image_index + b] << std::endl;
#endif
                // END-MODEL-DEPENDENT
                if (top_results.size() == 0)
                    LOG_WARNING << "*** There are no results with confidence above the minimum threshold! ***" << std::endl;
                for (const auto& result : top_results) {
                    const float confidence = result.first;
                    const int index = result.second;
                    LOG_INFO << "    - Prediction: " << index << " (" << confidence*100 << "\% confidence)" << std::endl;
                    //LOG_INFO << "    - Prediction: " << confidence << std::endl;

                }
#ifdef USE_OPENCV
#ifdef __x86_64
                const cv::Mat image(cv::Size(input_nrows, input_ncols), CV_32FC3, input_buffer);
                const cv::Mat resized(cv::Size(image.cols*8, image.rows*8), CV_32FC3);
                cv::resize(image, resized, cv::Size(image.cols*8, image.rows*8), cv::INTER_NEAREST);
                // BEGIN-MODEL-DEPENDENT
                cv::imshow("MODEL", resized);
                // END-MODEL-DEPENDENT
                cv::waitKey(0);
#endif
#endif
            }

            LOG_INFO << "\n";
            LOG_INFO << "Prediction invoked " << s->loop_count << " times (w/ " << s->warmup_runs << " warmup runs)" << std::endl;
            LOG_INFO << "  - Average time " << (get_us(stop_time) - get_us(start_time)) / (s->loop_count * 1000) << " ms" << std::endl;

#ifdef BUILD_MODEL_FROM_MMAP_BUFFER
            munmap(mmap_flatbuffer, filesize);
#endif
        }

        void display_usage() {
            LOG_INFO
                << "\n--image_index, -i: use a specific image in the test set\n"
                << "--batch_size, -b: set a batch size\n"
                << "--verbose, -v: [0|1] print more information\n"
                << "--model_file, -m: model file\n"
#ifdef USE_DATA_READER
                << "--data_dir, -d: data directory\n"
#endif
                << "--count, -c: loop interpreter->Invoke() for certain times\n"
                << "--trace_profiling, -p: [0|1], profiling or not\n"
                << "--num_results, -r: number of results to show\n"
                << "--max_trace_profiling_buffer_entries, -e max profiler entries\n"
                << "--warmup_runs, -w: number of warmup runs\n"
                << "\n";
        }

        int Main(int argc, char** argv) {
            Settings s;

            int c;
            while (1) {
                static struct option long_options[] = {
                    {"image_index", required_argument, nullptr, 'i'},
                    {"batch_size", required_argument, nullptr, 'b'},
                    {"verbose", required_argument, nullptr, 'v'},
                    {"model_file", required_argument, nullptr, 'm'},
#ifdef USE_DATA_READER
                    {"data_dir", required_argument, nullptr, 'd'},
#endif
                    {"count", required_argument, nullptr, 'c'},
                    {"trace_profiling", required_argument, nullptr, 'p'},
                    {"num_results", required_argument, nullptr, 'r'},
                    {"max_trace_profiling_buffer_entries", required_argument, nullptr, 'e'},
                    {"warmup_runs", required_argument, nullptr, 'w'},
                    {nullptr, 0, nullptr, 0}};

                /* getopt_long stores the option index here. */
                int option_index = 0;

                c = getopt_long(argc, argv,
#ifdef USE_DATA_READER
                        "i:b:v:m:d:c:p:r:e:w:",
#else
                        "i:b:v:m:c:p:r:e:w:",
#endif
                        long_options,
                        &option_index);

                /* Detect the end of the options. */
                if (c == -1) break;

                switch (c) {
                    case 'i':
                        s.image_index = strtol(optarg, nullptr, 10);
                        break;
                    case 'b':
                        s.batch_size = strtol(optarg, nullptr, 10);
                        break;
                    case 'v':
                        s.verbose = strtol(optarg, nullptr, 10);
                        break;
                    case 'm':
                        s.model_file = optarg;
                        break;
#ifdef USE_DATA_READER
                    case 'd':
                        s.data_dir = optarg;
                        break;
#endif
                    case 'c':
                        s.loop_count = strtol(optarg, nullptr, 10);
                        break;
                    case 'p':
                        s.trace_profiling = strtol(optarg, nullptr, 10);
                        break;
                    case 'r':
                        s.number_of_results = strtol(optarg, nullptr, 10);
                        break;
                    case 'e':
                        s.max_trace_profiling_buffer_entries = strtol(optarg, nullptr, 10);
                        break;
                    case 'w':
                        s.warmup_runs = strtol(optarg, nullptr, 10);
                        break;
                    case 'h':
                    case '?':
                        /* getopt_long already printed an error message. */
                        display_usage();
                        exit(-1);
                    default:
                        exit(-1);
                }
            }

            RunInference(&s);

            return 0;
        }
        //
    }  // namespace tflite_application
}  // namespace tflite

int main(int argc, char** argv) {

    LOG_INFO << "████████╗███████╗██╗     ██╗████████╗███████╗" << std::endl;
    LOG_INFO << "╚══██╔══╝██╔════╝██║     ██║╚══██╔══╝██╔════╝" << std::endl;
    LOG_INFO << "   ██║   █████╗  ██║     ██║   ██║   █████╗" << std::endl;
    LOG_INFO << "   ██║   ██╔══╝  ██║     ██║   ██║   ██╔══╝" << std::endl;
    LOG_INFO << "   ██║   ██║     ███████╗██║   ██║   ███████╗" << std::endl;
    LOG_INFO << "   ╚═╝   ╚═╝     ╚══════╝╚═╝   ╚═╝   ╚══════╝" << std::endl;
    LOG_INFO << "              SLD Group - Columbia University" << std::endl;
    LOG_INFO << "" << std::endl;
    LOG_INFO << "Application name: " << tflite::tflite_application::APPLICATION_NAME << std::endl;
    LOG_INFO << "" << std::endl;
#ifndef USE_DATA_READER
    LOG_INFO << "WARNING: Data from header files." << std::endl;
#endif
    return tflite::tflite_application::Main(argc, argv);
}
