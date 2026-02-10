//
// Created by ImSetal on 2026/2/10.
//

#include "AboutGPU.h"

#include <vector>

SDL_GPUShader* LoadShader(SDL_GPUDevice* device, std::string filename, SDL_Storage* storage, SDL_GPUShaderStage stage) {
    const char* backend = SDL_GetGPUDeviceDriver(device);
    bool use_metal = backend && SDL_strcasecmp(backend, "Metal") == 0;
    char* entrypoint;
    SDL_GPUShaderFormat format;
    if (use_metal) {
        filename+=".msl";
        entrypoint = "main0";
        format = SDL_GPU_SHADERFORMAT_MSL;
    }else {
        filename+=".spv";
        entrypoint = "main";
        format = SDL_GPU_SHADERFORMAT_SPIRV;
    }
    Uint64 file_size;
    if (!SDL_GetStorageFileSize(storage, filename.c_str(), &file_size)) {
        printf("无法获取文件大小。FileName: %s\n", filename.c_str());
        return nullptr;
    }
    std::vector<Uint8> data(file_size);
    if (!SDL_ReadStorageFile(storage, filename.c_str(), data.data(), file_size)) {
        printf("无法读取文件。FileName: %s\n", filename.c_str());
        return nullptr;
    }
    SDL_GPUShaderCreateInfo ci = {};
    ci.code = data.data();
    ci.code_size = data.size();
    ci.entrypoint = entrypoint;
    ci.format = format;
    ci.num_samplers = 0;
    ci.num_storage_buffers = 0;
    ci.num_storage_textures = 0;
    ci.num_uniform_buffers = 0;
    ci.stage = stage;
    ci.props = 0;
    return SDL_CreateGPUShader(device, &ci);
}

SDL_GPUGraphicsPipeline* CreatePipeline(SDL_GPUDevice* device, SDL_GPUShader* vs, SDL_GPUShader* fs) {
    SDL_GPUGraphicsPipelineCreateInfo ci = {};
    ci.vertex_input_state.num_vertex_attributes = 0;
    ci.vertex_input_state.num_vertex_buffers = 0;
    ci.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
    ci.vertex_shader = vs;
    ci.fragment_shader = fs;
    ci.rasterizer_state.cull_mode = SDL_GPU_CULLMODE_NONE;
    ci.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_FILL;
    ci.multisample_state.enable_mask = false;
    ci.multisample_state.sample_count = SDL_GPU_SAMPLECOUNT_1;
    ci.target_info.num_color_targets = 1;
    ci.target_info.has_depth_stencil_target = false;
    SDL_GPUColorTargetDescription desc = {};
    desc.blend_state.alpha_blend_op = SDL_GPU_BLENDOP_ADD;
    desc.blend_state.color_blend_op = SDL_GPU_BLENDOP_ADD;
    desc.blend_state.color_write_mask = SDL_GPU_COLORCOMPONENT_A | SDL_GPU_COLORCOMPONENT_R | SDL_GPU_COLORCOMPONENT_G | SDL_GPU_COLORCOMPONENT_B;
    desc.blend_state.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
    desc.blend_state.src_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
    desc.blend_state.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ZERO;
    desc.blend_state.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ZERO;
    desc.blend_state.enable_blend = true;
    desc.blend_state.enable_color_write_mask = false;
    desc.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
    ci.target_info.color_target_descriptions = &desc;
    return SDL_CreateGPUGraphicsPipeline(device, &ci);
}