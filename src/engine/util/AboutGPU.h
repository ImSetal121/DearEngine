//
// Created by ImSetal on 2026/2/10.
//

#ifndef LEARNCPP_ABOUTGPU_H
#define LEARNCPP_ABOUTGPU_H

#include <string>

#include "SDL3/SDL_gpu.h"
#include "SDL3/SDL_storage.h"

SDL_GPUShader* LoadShader(SDL_GPUDevice* device, std::string filename, SDL_Storage* storage, SDL_GPUShaderStage stage);

SDL_GPUGraphicsPipeline* CreatePipeline(SDL_GPUDevice* device, SDL_GPUShader* vs, SDL_GPUShader* fs);

#endif //LEARNCPP_ABOUTGPU_H
