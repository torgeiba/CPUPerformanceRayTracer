# CPUPerformanceRayTracer

![output image](https://github.com/torgeiba/CPUPerformanceRayTracer/blob/master/CPUPerformanceRayTracer/output_image.bmp)

## Description

Path tracer using multithreading and SIMD to accelerate rendering.\
Based on the blog series and shader implementation by Alan Wolfe (demofox / @atrix256):\
https://blog.demofox.org/2020/05/25/casual-shadertoy-path-tracing-1-basic-camera-diffuse-emissive/  \
https://www.shadertoy.com/view/ttfyzN  \

## Requirements

* Windows 10
* CPU supporting AVX and AVX2
* (I also used Visual studio 2019 Community for this)

## How to configure

Edit the file global_preprocessor flags to configure.\
From there you can toggle between windowed mode and offline rendering mode,
set resolution, number of threads to use, and how to split rendering into tiles.\
The resolution width must be divisible by the number of tiles in the horizontal direction times 8, since the rendering is 8 pixels wide due to SIMD.\
The resolution height must be divisible by the number of tiles in the vertical direction.\

