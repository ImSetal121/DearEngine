//
// Created by ImSetal on 2026/4/14.
//

#include "Physics.h"
#include "PhysicsContactListener.h"

namespace DE {

    void Physics::InitWorld() {
        temp_allocator_ = std::make_unique<JPH::TempAllocatorImpl>(10 * 1024 * 1024);
        job_system_ = std::make_unique<JPH::JobSystemThreadPool>(
            JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers,
            std::thread::hardware_concurrency() - 1);
        physics_system_ = std::make_unique<JPH::PhysicsSystem>();
        physics_system_->Init(cMaxBodies, cNumBodyMutexes, cMaxBodyPairs, cMaxContactConstraints,
            broad_phase_layer_interface_,
            object_vs_broadphase_layer_filter_,
            object_vs_object_layer_filter_);
        contact_listener_ = std::make_unique<PhysicsContactListener>();
        physics_system_->SetContactListener(contact_listener_.get());
    }

    void Physics::StepWorld(float delta_time) {
        accumulator_ += delta_time;
        while (accumulator_ >= fixed_step_) {
            physics_system_->Update(fixed_step_, 2, temp_allocator_.get(), job_system_.get());
            accumulator_ -= fixed_step_;
        }
    }

    void Physics::ResetWorld() {
        contact_listener_.reset();
        physics_system_.reset();
        job_system_.reset();
        temp_allocator_.reset();
    }

    Physics::Physics() = default;
    Physics::~Physics() = default;

}