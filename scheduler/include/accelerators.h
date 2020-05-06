/*
 * Copyright 2020 IBM
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef H_ACCELERATORS_H
#define H_ACCELERATORS_H

extern void execute_cpu_fft_accelerator(task_metadata_block_t* task_metadata_block); // float* data);
extern void execute_hwr_fft_accelerator(int n, float* data); // Execute FFT on hardware FFT accelerator "n"

#endif
