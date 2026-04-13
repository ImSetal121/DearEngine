//
// Created by ImSetal on 2026/4/13.
//

#ifndef DEARENGINE_DEPHYSICS_H
#define DEARENGINE_DEPHYSICS_H
#include "../DObject.h"

#include <Jolt/Jolt.h>
#include <Jolt/Core/Memory.h>

#include "Jolt/RegisterTypes.h"
#include "Jolt/Core/Factory.h"
#include "Jolt/Core/JobSystemThreadPool.h"
#include "Jolt/Core/TempAllocator.h"
#include <Jolt/Physics/PhysicsSystem.h>
#include "Jolt/Physics/Collision/BroadPhase/BroadPhaseLayer.h"

namespace JPH {
    class PhysicsSystem;
}

namespace Layers {
    // NON_MOVING：地板、墙壁等静止对象。它们不移动，所以宽相树不需要每帧重建。
    static constexpr JPH::ObjectLayer NON_MOVING = 0;
    // MOVING：有物理运动的对象（受重力、碰撞等影响）。
    static constexpr JPH::ObjectLayer MOVING     = 1;
    // NUM_LAYERS 是一个"哨兵常量"，值等于最后一个层的索引 + 1，即层的总数。
    static constexpr JPH::ObjectLayer NUM_LAYERS = 2;
}

namespace BroadPhaseLayers
{
    static constexpr JPH::BroadPhaseLayer NON_MOVING(0);
    static constexpr JPH::BroadPhaseLayer MOVING(1);
    static constexpr uint NUM_LAYERS(2); // 同上，宽相层总数，用于数组定义和越界检查
}

namespace DE {
    static void TraceImpl(const char *inFMT, ...)
    {
        // va_list 是 C 语言可变参数的标准用法，类似 printf 的处理方式。
        va_list list;
        va_start(list, inFMT);
        char buffer[1024];
        vsnprintf(buffer, sizeof(buffer), inFMT, list);
        va_end(list);

        std::cout << buffer << std::endl;
    }

    static bool AssertFailedImpl(const char *inExpression, const char *inMessage, const char *inFile, uint inLine)
    {
        std::cout << inFile << ":" << inLine << ": (" << inExpression << ") "
             << (inMessage != nullptr ? inMessage : "") << std::endl;

        return true; // 触发调试断点
    }

    class BPLayerInterfaceImpl final : public JPH::BroadPhaseLayerInterface
    {
    public:
        BPLayerInterfaceImpl()
        {
            // 在构造函数里建立映射表：对象层索引 → 宽相层值
            mObjectToBroadPhase[Layers::NON_MOVING] = BroadPhaseLayers::NON_MOVING;
            mObjectToBroadPhase[Layers::MOVING]     = BroadPhaseLayers::MOVING;
        }

        virtual uint GetNumBroadPhaseLayers() const override
        {
            return BroadPhaseLayers::NUM_LAYERS;
        }

        // 给定一个对象层，返回对应的宽相层。这是宽相内部频繁调用的热路径。
        virtual JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer inLayer) const override
        {
            JPH_ASSERT(inLayer < Layers::NUM_LAYERS);
            return mObjectToBroadPhase[inLayer];
        }

        // 只在开启性能分析（Profiling）时需要：提供宽相层的可读名字，方便在报告里识别。
#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
        virtual const char *GetBroadPhaseLayerName(JPH::BroadPhaseLayer inLayer) const override
        {
            switch ((JPH::BroadPhaseLayer::Type)inLayer)
            {
                case (JPH::BroadPhaseLayer::Type)BroadPhaseLayers::NON_MOVING: return "NON_MOVING";
                case (JPH::BroadPhaseLayer::Type)BroadPhaseLayers::MOVING:     return "MOVING";
                default: JPH_ASSERT(false); return "INVALID";
            }
        }
#endif
    private:
        // 数组大小由 NUM_LAYERS 决定，新增层时不需要手动改这里。
        JPH::BroadPhaseLayer mObjectToBroadPhase[Layers::NUM_LAYERS];
    };

    class ObjectVsBroadPhaseLayerFilterImpl : public JPH::ObjectVsBroadPhaseLayerFilter
    {
    public:
        virtual bool ShouldCollide(JPH::ObjectLayer inLayer1, JPH::BroadPhaseLayer inLayer2) const override
        {
            switch (inLayer1)
            {
                case Layers::NON_MOVING:
                    return inLayer2 == BroadPhaseLayers::MOVING;
                case Layers::MOVING:
                    return true;
                default:
                    JPH_ASSERT(false);
                    return false;
            }
        }
    };

    class ObjectLayerPairFilterImpl : public JPH::ObjectLayerPairFilter
    {
    public:
        virtual bool ShouldCollide(JPH::ObjectLayer inObject1, JPH::ObjectLayer inObject2) const override
        {
            switch (inObject1)
            {
                case Layers::NON_MOVING:
                    // 静止对象（地板/墙）只需要和运动对象碰撞；
                    // 两块地板之间永远不会互相穿透，所以不需要检测。
                    return inObject2 == Layers::MOVING;

                case Layers::MOVING:
                    // 运动对象需要和所有对象碰撞：既可能撞地板，也可能撞其他运动对象。
                    return true;

                default:
                    JPH_ASSERT(false);
                    return false;
            }
        }
    };

    class Physics : public DObject{
    public:
        const uint cMaxBodies = 1024;
        const uint cNumBodyMutexes = 0;
        const uint cMaxBodyPairs = 1024;
        const uint cMaxContactConstraints = 1024;

        static Physics& Instance() {
            static Physics instance;
            return instance;
        }

        void Init() {
            JPH::RegisterDefaultAllocator();
            JPH::Trace = TraceImpl;
            JPH_IF_ENABLE_ASSERTS(JPH::AssertFailed = AssertFailedImpl;)
            JPH::Factory::sInstance = new JPH::Factory();
            JPH::RegisterTypes();
        }

        void InitWorld() {
            temp_allocator_ = std::make_unique<JPH::TempAllocatorImpl>(10 * 1024 * 1024);
            job_system_ = std::make_unique<JPH::JobSystemThreadPool>(JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers,
                                                                     std::thread::hardware_concurrency() - 1);
            physics_system_ = std::make_unique<JPH::PhysicsSystem>();
            physics_system_->Init(cMaxBodies, cNumBodyMutexes, cMaxBodyPairs, cMaxContactConstraints,
                broad_phase_layer_interface_,
                object_vs_broadphase_layer_filter_,
                object_vs_object_layer_filter_);
        }

        void ResetWorld() {
            physics_system_.reset();
            job_system_.reset();
            temp_allocator_.reset();
        }

        JPH::PhysicsSystem* GetPhysicsSystem() {
            return physics_system_.get();
        }
    private:
        Physics() = default;

        std::unique_ptr<JPH::TempAllocatorImpl> temp_allocator_;
        std::unique_ptr<JPH::JobSystemThreadPool> job_system_;
        std::unique_ptr<JPH::PhysicsSystem> physics_system_;
        BPLayerInterfaceImpl broad_phase_layer_interface_;                    // 对象层 → 宽相层 映射
        ObjectVsBroadPhaseLayerFilterImpl object_vs_broadphase_layer_filter_; // 对象层 vs 宽相层 碰撞过滤
        ObjectLayerPairFilterImpl object_vs_object_layer_filter_;             // 对象层 vs 对象层 碰撞过滤
    };

}

#endif //DEARENGINE_DEPHYSICS_H
