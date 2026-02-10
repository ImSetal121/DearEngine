//
// Created by ImSetal on 2026/2/10.
//

#ifndef LEARNCPP_ABOUTGPU_H
#define LEARNCPP_ABOUTGPU_H

#include "SDL3/SDL_gpu.h"
#include "SDL3/SDL_storage.h"

SDL_GPUShader* LoadSceneShader(SDL_GPUDevice* device, const char* filename, SDL_Storage* storage, SDL_GPUShaderStage stage, SDL_GPUShaderFormat format);

SDL_GPUGraphicsPipeline* CreateSceneTrianglePipeline(SDL_GPUDevice* device, SDL_GPUShader* vs, SDL_GPUShader* fs);

#endif //LEARNCPP_ABOUTGPU_H
