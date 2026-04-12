// Jolt 物理库 (https://github.com/jrouwe/JoltPhysics)
// SPDX-FileCopyrightText: 2025 Jorrit Rouwe
// SPDX-License-Identifier: CC0-1.0
// 此文件属于公共领域。可作为使用 Jolt Physics 构建自己应用程序的起点示例，随意复制粘贴，无需署名。

// Jolt 头文件不包含 Jolt.h。在包含任何其他 Jolt 头文件之前，请始终先包含 Jolt.h。
// 可以将 Jolt.h 放入预编译头文件以加速编译。
#include <Jolt/Jolt.h>

// Jolt 相关头文件
#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyActivationListener.h>

// STL 头文件
#include <iostream>
#include <cstdarg>
#include <thread>

// 禁用 Jolt 触发的常见警告，可以使用 JPH_SUPPRESS_WARNING_PUSH / JPH_SUPPRESS_WARNING_POP 保存和恢复警告状态
JPH_SUPPRESS_WARNINGS

// 所有 Jolt 符号都在 JPH 命名空间中
using namespace JPH;

// 若希望代码在单精度或双精度下均能编译，使用 0.0_r 获取 Real 值，
// 该值会根据 JPH_DOUBLE_PRECISION 是否设置而编译为 double 或 float。
using namespace JPH::literals;

// 本示例中也使用了 STL 类
using namespace std;

// 追踪回调，将其连接到你自己的追踪函数（如果有）
static void TraceImpl(const char *inFMT, ...)
{
	// 格式化消息
	va_list list;
	va_start(list, inFMT);
	char buffer[1024];
	vsnprintf(buffer, sizeof(buffer), inFMT, list);
	va_end(list);

	// 输出到终端
	cout << buffer << endl;
}

#ifdef JPH_ENABLE_ASSERTS

// 断言回调，将其连接到你自己的断言处理函数（如果有）
static bool AssertFailedImpl(const char *inExpression, const char *inMessage, const char *inFile, uint inLine)
{
	// 输出到终端
	cout << inFile << ":" << inLine << ": (" << inExpression << ") " << (inMessage != nullptr? inMessage : "") << endl;

	// 触发断点
	return true;
};

#endif // JPH_ENABLE_ASSERTS

// 对象所属的层，决定其可以与哪些其他对象发生碰撞。
// 通常至少需要 1 个层用于运动刚体，1 个层用于静态刚体，
// 也可以根据需要设置更多层，例如为高精度碰撞单独设置一个层
//（仅用于碰撞检测，不参与物理模拟）。
namespace Layers
{
	static constexpr ObjectLayer NON_MOVING = 0;
	static constexpr ObjectLayer MOVING = 1;
	static constexpr ObjectLayer NUM_LAYERS = 2;
};

/// 决定两个对象层之间是否可以发生碰撞的类
class ObjectLayerPairFilterImpl : public ObjectLayerPairFilter
{
public:
	virtual bool					ShouldCollide(ObjectLayer inObject1, ObjectLayer inObject2) const override
	{
		switch (inObject1)
		{
		case Layers::NON_MOVING:
			return inObject2 == Layers::MOVING; // 静止物体只与运动物体碰撞
		case Layers::MOVING:
			return true; // 运动物体与所有物体碰撞
		default:
			JPH_ASSERT(false);
			return false;
		}
	}
};

// 每个宽相层在宽相中各自生成一棵独立的包围体树。
// 至少需要为静止对象和运动对象分别设置一个层，以避免每帧更新充满静态对象的树。
// 对象层与宽相层之间可以一一映射（如本例），但若对象层过多则会创建大量宽相树，效率较低。
// 若需要精细调整宽相层，可定义 JPH_TRACK_BROADPHASE_STATS 并查看终端输出的统计信息。
namespace BroadPhaseLayers
{
	static constexpr BroadPhaseLayer NON_MOVING(0);
	static constexpr BroadPhaseLayer MOVING(1);
	static constexpr uint NUM_LAYERS(2);
};

// BroadPhaseLayerInterface 的实现
// 定义对象层到宽相层的映射关系。
class BPLayerInterfaceImpl final : public BroadPhaseLayerInterface
{
public:
									BPLayerInterfaceImpl()
	{
		// 创建从对象层到宽相层的映射表
		mObjectToBroadPhase[Layers::NON_MOVING] = BroadPhaseLayers::NON_MOVING;
		mObjectToBroadPhase[Layers::MOVING] = BroadPhaseLayers::MOVING;
	}

	virtual uint					GetNumBroadPhaseLayers() const override
	{
		return BroadPhaseLayers::NUM_LAYERS;
	}

	virtual BroadPhaseLayer			GetBroadPhaseLayer(ObjectLayer inLayer) const override
	{
		JPH_ASSERT(inLayer < Layers::NUM_LAYERS);
		return mObjectToBroadPhase[inLayer];
	}

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
	virtual const char *			GetBroadPhaseLayerName(BroadPhaseLayer inLayer) const override
	{
		switch ((BroadPhaseLayer::Type)inLayer)
		{
		case (BroadPhaseLayer::Type)BroadPhaseLayers::NON_MOVING:	return "NON_MOVING";
		case (BroadPhaseLayer::Type)BroadPhaseLayers::MOVING:		return "MOVING";
		default:													JPH_ASSERT(false); return "INVALID";
		}
	}
#endif // JPH_EXTERNAL_PROFILE || JPH_PROFILE_ENABLED

private:
	BroadPhaseLayer					mObjectToBroadPhase[Layers::NUM_LAYERS];
};

/// 决定对象层是否可以与宽相层发生碰撞的类
class ObjectVsBroadPhaseLayerFilterImpl : public ObjectVsBroadPhaseLayerFilter
{
public:
	virtual bool				ShouldCollide(ObjectLayer inLayer1, BroadPhaseLayer inLayer2) const override
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

// 接触监听器示例
class MyContactListener : public ContactListener
{
public:
	// 参见：ContactListener
	virtual ValidateResult	OnContactValidate(const Body &inBody1, const Body &inBody2, RVec3Arg inBaseOffset, const CollideShapeResult &inCollisionResult) override
	{
		cout << "接触验证回调" << endl;

		// 允许在接触创建前将其忽略（使用层来阻止对象碰撞的开销更小！）
		return ValidateResult::AcceptAllContactsForThisBodyPair;
	}

	virtual void			OnContactAdded(const Body &inBody1, const Body &inBody2, const ContactManifold &inManifold, ContactSettings &ioSettings) override
	{
		cout << "新增了一个接触" << endl;
	}

	virtual void			OnContactPersisted(const Body &inBody1, const Body &inBody2, const ContactManifold &inManifold, ContactSettings &ioSettings) override
	{
		cout << "一个接触持续存在" << endl;
	}

	virtual void			OnContactRemoved(const SubShapeIDPair &inSubShapePair) override
	{
		cout << "移除了一个接触" << endl;
	}
};

// 激活监听器示例
class MyBodyActivationListener : public BodyActivationListener
{
public:
	virtual void		OnBodyActivated(const BodyID &inBodyID, uint64 inBodyUserData) override
	{
		cout << "一个刚体被激活" << endl;
	}

	virtual void		OnBodyDeactivated(const BodyID &inBodyID, uint64 inBodyUserData) override
	{
		cout << "一个刚体进入休眠" << endl;
	}
};

// 程序入口
int main(int argc, char** argv)
{
	// 注册内存分配钩子。本示例直接使用 malloc / free，但你可以根据需要重写（参见 Memory.h）。
	// 此操作必须在调用任何其他 Jolt 函数之前完成。
	RegisterDefaultAllocator();

	// 安装追踪和断言回调
	Trace = TraceImpl;
	JPH_IF_ENABLE_ASSERTS(AssertFailed = AssertFailedImpl;)

	// 创建工厂，该类负责根据名称或哈希创建类的实例，主要用于已保存数据的反序列化。
	// 本示例中不直接使用，但仍然必须创建。
	Factory::sInstance = new Factory();

	// 向工厂注册所有物理类型，并向 CollisionDispatch 注册它们的碰撞处理器。
	// 若有自定义形状类型，需要在调用此函数之前向 CollisionDispatch 注册其处理器。
	// 若实现了自定义默认材质（PhysicsMaterial::sDefault），请在调用此函数之前完成初始化，
	// 否则此函数会自动创建一个默认材质。
	RegisterTypes();

	// 需要一个临时分配器，用于物理更新期间的临时内存分配。
	// 这里预先分配 10 MB 以避免在物理更新期间进行动态内存分配。
	// 顺便说一句，对于本示例而言 10 MB 远超所需，但这是一个典型的参考值。
	// 若不想预先分配，也可以使用 TempAllocatorMalloc 回退到 malloc / free。
	TempAllocatorImpl temp_allocator(10 * 1024 * 1024);

	// 需要一个任务系统来在多个线程上执行物理任务。
	// 通常你会自己实现 JobSystem 接口，让 Jolt Physics 运行在你自己的任务调度器之上。
	// JobSystemThreadPool 是一个示例实现。
	JobSystemThreadPool job_system(cMaxPhysicsJobs, cMaxPhysicsBarriers, thread::hardware_concurrency() - 1);

	// 物理系统中可添加的刚体最大数量。超出此数量将报错。
	// 注意：此值较小，因为这是一个简单测试。实际项目中建议使用 65536 左右的值。
	const uint cMaxBodies = 1024;

	// 用于保护刚体免受并发访问的互斥锁数量。设为 0 使用默认设置。
	const uint cNumBodyMutexes = 0;

	// 同一时刻可排队的刚体对最大数量（宽相基于包围盒检测重叠刚体对，并将其插入窄相队列）。
	// 若此缓冲区过小，队列将被填满，宽相任务将开始执行窄相工作，效率略有降低。
	// 注意：此值较小，因为这是一个简单测试。实际项目中建议使用 65536 左右的值。
	const uint cMaxBodyPairs = 1024;

	// 接触约束缓冲区的最大容量。若检测到的接触（刚体间碰撞）数量超过此值，
	// 多余的接触将被忽略，刚体将开始相互穿透或穿过世界。
	// 注意：此值较小，因为这是一个简单测试。实际项目中建议使用 10240 左右的值。
	const uint cMaxContactConstraints = 1024;

	// 创建从对象层到宽相层的映射表
	// 注意：这是一个接口，PhysicsSystem 会持有其引用，因此该实例需保持存活！
	// 也可以参考 BroadPhaseLayerInterfaceTable 或 BroadPhaseLayerInterfaceMask，接口更简洁。
	BPLayerInterfaceImpl broad_phase_layer_interface;

	// 创建过滤对象层与宽相层碰撞关系的类
	// 注意：这是一个接口，PhysicsSystem 会持有其引用，因此该实例需保持存活！
	// 也可以参考 ObjectVsBroadPhaseLayerFilterTable 或 ObjectVsBroadPhaseLayerFilterMask，接口更简洁。
	ObjectVsBroadPhaseLayerFilterImpl object_vs_broadphase_layer_filter;

	// 创建过滤对象层与对象层碰撞关系的类
	// 注意：这是一个接口，PhysicsSystem 会持有其引用，因此该实例需保持存活！
	// 也可以参考 ObjectLayerPairFilterTable 或 ObjectLayerPairFilterMask，接口更简洁。
	ObjectLayerPairFilterImpl object_vs_object_layer_filter;

	// 现在可以创建实际的物理系统了。
	PhysicsSystem physics_system;
	physics_system.Init(cMaxBodies, cNumBodyMutexes, cMaxBodyPairs, cMaxContactConstraints, broad_phase_layer_interface, object_vs_broadphase_layer_filter, object_vs_object_layer_filter);

	// 刚体激活监听器在刚体激活和进入休眠时收到通知。
	// 注意：这是从任务线程调用的，因此此处的操作必须是线程安全的。
	// 注册监听器完全是可选的。
	MyBodyActivationListener body_activation_listener;
	physics_system.SetBodyActivationListener(&body_activation_listener);

	// 接触监听器在刚体（即将）碰撞以及分离时收到通知。
	// 注意：这是从任务线程调用的，因此此处的操作必须是线程安全的。
	// 注册监听器完全是可选的。
	MyContactListener contact_listener;
	physics_system.SetContactListener(&contact_listener);

	// 与物理系统中刚体交互的主要方式是通过刚体接口。
	// 该接口有加锁版和非加锁版，这里使用加锁版（即使我们不打算从多个线程访问刚体）。
	BodyInterface &body_interface = physics_system.GetBodyInterface();

	// 接下来创建一个刚体作为地板，使用一个大型盒体。
	// 创建碰撞体积（形状）的设置。
	// 注意：对于简单形状（如盒体），也可以直接构造 BoxShape。
	BoxShapeSettings floor_shape_settings(Vec3(100.0f, 1.0f, 100.0f));
	floor_shape_settings.SetEmbedded(); // 栈上的引用计数对象（基类 RefTarget）应标记为此状态，以防引用计数归零时被释放。

	// 创建形状
	ShapeSettings::ShapeResult floor_shape_result = floor_shape_settings.Create();
	ShapeRefC floor_shape = floor_shape_result.Get(); // 此处不期望出错，但你可以通过 HasError() / GetError() 检查 floor_shape_result

	// 创建刚体本身的设置。注意此处也可以设置其他属性，如弹性系数 / 摩擦力。
	BodyCreationSettings floor_settings(floor_shape, RVec3(0.0_r, -1.0_r, 0.0_r), Quat::sIdentity(), EMotionType::Static, Layers::NON_MOVING);

	// 创建实际的刚体
	Body *floor = body_interface.CreateBody(floor_settings); // 注意：若刚体数量耗尽，此处可能返回 nullptr

	// 将其添加到世界中
	body_interface.AddBody(floor->GetID(), EActivation::DontActivate);

	// 现在创建一个动态刚体，让它在地板上弹跳。
	// 注意：这里使用了创建并添加刚体到世界的简写版本。
	BodyCreationSettings sphere_settings(new SphereShape(0.5f), RVec3(0.0_r, 2.0_r, 0.0_r), Quat::sIdentity(), EMotionType::Dynamic, Layers::MOVING);
	BodyID sphere_id = body_interface.CreateAndAddBody(sphere_settings, EActivation::Activate);

	// 现在可以与动态刚体交互，这里给它一个初速度。
	//（注意：若使用 CreateBody，可以在添加到物理系统之前直接在刚体上设置速度）
	body_interface.SetLinearVelocity(sphere_id, Vec3(0.0f, -5.0f, 0.0f));

	// 以离散时间步长模拟物理世界。60 Hz 是更新物理系统的推荐频率。
	const float cDeltaTime = 1.0f / 60.0f;

	// 可选步骤：在开始物理模拟之前可以优化宽相，这能提升碰撞检测性能
	//（本示例中意义不大，因为只有 2 个刚体）。
	// 切勿每帧调用此函数，也不要在流式加载新关卡区域时调用，因为这是一个耗时操作。
	// 应当批量插入新对象，而非逐一插入，以保持宽相的效率。
	physics_system.OptimizeBroadPhase();

	// 现在可以开始模拟刚体了，持续模拟直到其进入休眠
	uint step = 0;
	while (body_interface.IsActive(sphere_id))
	{
		// 进入下一步
		++step;

		// 输出当前球体的位置和速度
		RVec3 position = body_interface.GetCenterOfMassPosition(sphere_id);
		Vec3 velocity = body_interface.GetLinearVelocity(sphere_id);
		cout << "步骤 " << step << ": 位置 = (" << position.GetX() << ", " << position.GetY() << ", " << position.GetZ() << "), 速度 = (" << velocity.GetX() << ", " << velocity.GetY() << ", " << velocity.GetZ() << ")" << endl;

		// 若时间步长超过 1/60 秒，需要执行多次碰撞步骤以保持模拟稳定性。
		// 每 1/60 秒执行 1 次碰撞步骤（向上取整）。
		const int cCollisionSteps = 1;

		// 推进世界模拟
		physics_system.Update(cDeltaTime, cCollisionSteps, &temp_allocator, &job_system);
	}

	// 从物理系统中移除球体。注意：球体本身保留所有状态，可以随时重新添加。
	body_interface.RemoveBody(sphere_id);

	// 销毁球体。此后球体 ID 将不再有效。
	body_interface.DestroyBody(sphere_id);

	// 移除并销毁地板
	body_interface.RemoveBody(floor->GetID());
	body_interface.DestroyBody(floor->GetID());

	// 向工厂注销所有类型并清理默认材质
	UnregisterTypes();

	// 销毁工厂
	delete Factory::sInstance;
	Factory::sInstance = nullptr;

	return 0;
}