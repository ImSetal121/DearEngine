// =============================================================================
// Jolt Physics HelloWorld — 详细中文注释版
// 原始来源：https://github.com/jrouwe/JoltPhysics
// 本文件在原示例基础上加入了面向初学者的讲解性注释。
// =============================================================================

// ── 第一步：包含 Jolt 的"总头文件" ──────────────────────────────────────────
// Jolt 要求 Jolt.h 永远排在最前面，因为它定义了全局宏、类型别名和编译选项，
// 后续所有头文件都依赖这些定义。把它想成搭积木前必须先铺好底板。
#include <Jolt/Jolt.h>

// ── 第二步：按需引入功能模块 ──────────────────────────────────────────────
// RegisterTypes.h ── 把所有内置形状、约束等"注册"到工厂，类似游戏里注册道具表
#include <Jolt/RegisterTypes.h>
// Factory.h ── 工厂类：根据名字/哈希创建对象实例，主要为序列化/反序列化服务
#include <Jolt/Core/Factory.h>
// TempAllocator.h ── 临时分配器：物理模拟每帧产生的短命内存就放这里
#include <Jolt/Core/TempAllocator.h>
// JobSystemThreadPool.h ── 线程池：把物理计算拆成多个任务并行跑，充分利用多核 CPU
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Physics/PhysicsSettings.h>    // 全局物理常量（最大 Job 数等）
#include <Jolt/Physics/PhysicsSystem.h>      // 核心：物理世界本身
#include <Jolt/Physics/Collision/Shape/BoxShape.h>    // 盒体碰撞形状
#include <Jolt/Physics/Collision/Shape/SphereShape.h> // 球体碰撞形状
#include <Jolt/Physics/Body/BodyCreationSettings.h>   // 刚体创建参数包
#include <Jolt/Physics/Body/BodyActivationListener.h> // 刚体休眠/激活事件监听

#include <iostream>
#include <cstdarg>
#include <thread>

// Jolt 内部可能触发一些平台警告（如对齐、符号转换），这里统一压制，
// 避免日志被无关警告污染。
JPH_SUPPRESS_WARNINGS

// Jolt 把所有符号放在 JPH 命名空间，避免与其他库冲突。
using namespace JPH;

// JPH::literals 提供 0.0_r 这种字面量后缀。
// 「Real」是一个编译期类型别名：若启用 JPH_DOUBLE_PRECISION 就是 double，否则是 float。
// 这样同一份代码可以在两种精度下编译，无需手写 0.0f / 0.0 的条件分支。
using namespace JPH::literals;

using namespace std;

// =============================================================================
// 【调试工具】追踪（Trace）与断言（Assert）回调
// =============================================================================

// Jolt 内部遇到需要打日志的情况会调用全局函数指针 Trace（定义在 Jolt/Core/Core.h）。
// 这里我们把它接到 cout，你在真实项目里可以接到自己的 Logger。
// 必须在调用任何 Jolt 函数之前把 Trace 指针赋值好，否则内部日志无处可去。
static void TraceImpl(const char *inFMT, ...)
{
    // va_list 是 C 语言可变参数的标准用法，类似 printf 的处理方式。
    va_list list;
    va_start(list, inFMT);
    char buffer[1024];
    vsnprintf(buffer, sizeof(buffer), inFMT, list);
    va_end(list);

    cout << buffer << endl;
}

// 只有在 Debug/Development 构建中（JPH_ENABLE_ASSERTS 宏存在）才需要断言回调。
// Release 构建通常关闭断言以换取性能，所以用条件编译包裹。
#ifdef JPH_ENABLE_ASSERTS

// 当 Jolt 内部的 JPH_ASSERT() 失败时，会调用全局函数指针 AssertFailed。
// 返回 true 表示"请触发调试中断（breakpoint）"，让调试器在此处暂停。
// 返回 false 则跳过中断，继续执行（通常用于非交互式环境）。
static bool AssertFailedImpl(const char *inExpression, const char *inMessage, const char *inFile, uint inLine)
{
    cout << inFile << ":" << inLine << ": (" << inExpression << ") "
         << (inMessage != nullptr ? inMessage : "") << endl;

    return true; // 触发调试断点
}

#endif // JPH_ENABLE_ASSERTS

// =============================================================================
// 【碰撞层系统】对象层（Object Layer）与宽相层（Broad Phase Layer）
// =============================================================================
//
// 碰撞检测分两阶段：
//   宽相（Broad Phase）── 粗筛，用轴对齐包围盒（AABB）快速排除不可能碰撞的对象对。
//                         好比超市收银台先看购物车里大约有多少东西，快速估算。
//   窄相（Narrow Phase）── 精算，对通过宽相的候选对真正算碰撞体积交叠。
//                         好比收银员逐件商品扫码精确计价。
//
// 「对象层」是业务语义层面的分组（静止的地形 vs 运动的角色），
// 「宽相层」是宽相内部数据结构的分组（每层各维护一棵 BVH 树）。
// 两者分开设计，使你可以用少量宽相层来优化数据结构，同时用多对象层表达丰富的碰撞规则。

namespace Layers
{
    // NON_MOVING：地板、墙壁等静止对象。它们不移动，所以宽相树不需要每帧重建。
    static constexpr ObjectLayer NON_MOVING = 0;

    // MOVING：有物理运动的对象（受重力、碰撞等影响）。
    static constexpr ObjectLayer MOVING     = 1;

    // NUM_LAYERS 是一个"哨兵常量"，值等于最后一个层的索引 + 1，即层的总数。
    // 用途一：定义数组大小，如 mObjectToBroadPhase[Layers::NUM_LAYERS]，自动跟随层数扩容。
    // 用途二：边界检查，如 JPH_ASSERT(inLayer < Layers::NUM_LAYERS)，防止越界访问。
    // 如果将来新增一个层 TRIGGER = 2，只需把 NUM_LAYERS 改为 3，其他地方自动适配。
    static constexpr ObjectLayer NUM_LAYERS = 2;
}

// 这个类回答一个问题：「对象层 A 和对象层 B 之间，要不要做碰撞检测？」
// 不让两类对象互相碰撞的最高效方法就是在这里返回 false，
// 这样连宽相都不会为它们生成候选对，省去后续所有计算。
class ObjectLayerPairFilterImpl : public ObjectLayerPairFilter
{
public:
    virtual bool ShouldCollide(ObjectLayer inObject1, ObjectLayer inObject2) const override
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

// 宽相层的命名空间，和对象层一一对应（简单项目可以这样做）。
// 若对象层很多（几十上百个），不要每个都对应一个宽相层，
// 因为每个宽相层都会维护一棵 BVH 树，树多了反而慢。
namespace BroadPhaseLayers
{
    static constexpr BroadPhaseLayer NON_MOVING(0);
    static constexpr BroadPhaseLayer MOVING(1);
    static constexpr uint NUM_LAYERS(2); // 同上，宽相层总数，用于数组定义和越界检查
}

// 这个类回答：「对象层 X 映射到哪个宽相层？」
// 宽相用这个映射决定把一个对象放进哪棵 BVH 树里存储。
// PhysicsSystem 会持有指向它的指针，所以它的生命周期必须比 PhysicsSystem 长。
class BPLayerInterfaceImpl final : public BroadPhaseLayerInterface
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
    virtual BroadPhaseLayer GetBroadPhaseLayer(ObjectLayer inLayer) const override
    {
        JPH_ASSERT(inLayer < Layers::NUM_LAYERS);
        return mObjectToBroadPhase[inLayer];
    }

    // 只在开启性能分析（Profiling）时需要：提供宽相层的可读名字，方便在报告里识别。
#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
    virtual const char *GetBroadPhaseLayerName(BroadPhaseLayer inLayer) const override
    {
        switch ((BroadPhaseLayer::Type)inLayer)
        {
        case (BroadPhaseLayer::Type)BroadPhaseLayers::NON_MOVING: return "NON_MOVING";
        case (BroadPhaseLayer::Type)BroadPhaseLayers::MOVING:     return "MOVING";
        default: JPH_ASSERT(false); return "INVALID";
        }
    }
#endif

private:
    // 数组大小由 NUM_LAYERS 决定，新增层时不需要手动改这里。
    BroadPhaseLayer mObjectToBroadPhase[Layers::NUM_LAYERS];
};

// 这个类回答：「对象层 A 是否需要去查询宽相层 B 的 BVH 树？」
// 它是宽相阶段的第一道过滤器，在比较 AABB 之前就可以整棵树地跳过，极为高效。
// 逻辑与 ObjectLayerPairFilter 对称：静止对象只查 MOVING 树，运动对象查所有树。
class ObjectVsBroadPhaseLayerFilterImpl : public ObjectVsBroadPhaseLayerFilter
{
public:
    virtual bool ShouldCollide(ObjectLayer inLayer1, BroadPhaseLayer inLayer2) const override
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

// =============================================================================
// 【事件监听】接触监听器（Contact Listener）
// =============================================================================
//
// 每当两个刚体发生碰撞（接触）或分离时，物理系统会回调这里的函数。
// 注意：这些回调是从工作线程调用的，如果你在回调里访问共享数据，
//       必须加锁或使用原子操作保证线程安全。
class MyContactListener : public ContactListener
{
public:
    // OnContactValidate：宽相检测到两个对象可能碰撞后，在真正计算接触之前调用。
    // 返回 AcceptAllContactsForThisBodyPair 表示"允许碰撞"；
    // 也可以返回拒绝，用于实现"单向穿透平台"等特殊逻辑。
    // 注意：用对象层过滤比在这里拒绝效率高得多，因为层过滤发生在更早阶段。
    virtual ValidateResult OnContactValidate(const Body &inBody1, const Body &inBody2,
                                             RVec3Arg inBaseOffset,
                                             const CollideShapeResult &inCollisionResult) override
    {
        cout << "接触验证回调" << endl;
        return ValidateResult::AcceptAllContactsForThisBodyPair;
    }

    // OnContactAdded：两个刚体刚刚开始接触（本帧新产生的接触点）。
    // ContactManifold 包含接触点坐标、法线、穿透深度等详细信息。
    // ioSettings 可以修改本次接触的弹性/摩擦参数。
    virtual void OnContactAdded(const Body &inBody1, const Body &inBody2,
                                const ContactManifold &inManifold,
                                ContactSettings &ioSettings) override
    {
        cout << "新增了一个接触" << endl;
    }

    // OnContactPersisted：两个刚体持续接触（上帧也有、本帧还在）。
    // 每帧都会为仍然存在的接触调用一次，可以用来持续施加摩擦音效等。
    virtual void OnContactPersisted(const Body &inBody1, const Body &inBody2,
                                    const ContactManifold &inManifold,
                                    ContactSettings &ioSettings) override
    {
        cout << "一个接触持续存在" << endl;
    }

    // OnContactRemoved：两个刚体分离，接触消失。
    // 参数是子形状 ID 对（SubShapeIDPair），因为一个复杂刚体可以有多个子形状。
    virtual void OnContactRemoved(const SubShapeIDPair &inSubShapePair) override
    {
        cout << "移除了一个接触" << endl;
    }
};

// =============================================================================
// 【事件监听】刚体激活监听器（Body Activation Listener）
// =============================================================================
//
// 物理引擎为了节省计算，会让静止的刚体进入"休眠（Sleep）"状态，停止模拟。
// 当附近有碰撞或施加力时，休眠的刚体会被重新"激活（Activate）"。
// 这两个回调让你知道刚体状态的变化，比如可以用来触发音效或粒子效果。
//
// 同样地，这些回调从工作线程调用，需要注意线程安全。
class MyBodyActivationListener : public BodyActivationListener
{
public:
    virtual void OnBodyActivated(const BodyID &inBodyID, uint64 inBodyUserData) override
    {
        cout << "一个刚体被激活" << endl;
    }

    virtual void OnBodyDeactivated(const BodyID &inBodyID, uint64 inBodyUserData) override
    {
        cout << "一个刚体进入休眠" << endl;
    }
};

// =============================================================================
// 【主函数】组装并运行物理模拟
// =============================================================================
int main(int argc, char** argv)
{
    // ── 1. 内存分配器 ────────────────────────────────────────────────────────
    // Jolt 所有内部的 new/delete 都通过全局函数指针完成，方便你替换为自定义分配器
    // （例如对齐分配、内存池、内存统计等）。
    // RegisterDefaultAllocator() 把这些指针接到标准的 malloc/free。
    // 这一行必须最先调用，因为后续所有 Jolt 对象的构造都会用到它。
    RegisterDefaultAllocator();

    // ── 2. 调试钩子 ──────────────────────────────────────────────────────────
    // 把我们自定义的函数赋给 Jolt 的全局函数指针。
    // JPH_IF_ENABLE_ASSERTS 是一个条件宏，在 Release 下展开为空，什么都不做。
    Trace = TraceImpl;
    JPH_IF_ENABLE_ASSERTS(AssertFailed = AssertFailedImpl;)

    // ── 3. 工厂（Factory）────────────────────────────────────────────────────
    // Factory 是一个全局单例，它维护一张"类型名 → 创建函数"的注册表。
    // 好比餐厅的菜单：先有菜单，才能根据菜名点菜。
    // 即使本示例不做序列化，也必须创建它，因为 RegisterTypes() 要往里写数据。
    Factory::sInstance = new Factory();

    // ── 4. 注册所有内置类型 ──────────────────────────────────────────────────
    // 把 BoxShape、SphereShape、各种约束等内置类型注册进工厂，
    // 同时把它们的碰撞处理函数注册进 CollisionDispatch（一张二维碰撞分发表）。
    // 如果你有自定义形状，需要在这行之前注册，否则 CollisionDispatch 不知道如何处理它们。
    RegisterTypes();

    // ── 5. 临时分配器（TempAllocator）────────────────────────────────────────
    // 物理更新每帧会产生大量短命的中间数据（碰撞结果、约束方程等），
    // 用完就扔，不会保留到下一帧。
    // 好比厨师做饭时用的案板空间：每道菜用完就清理，不需要永久存储。
    // TempAllocatorImpl 预先申请一块固定大小的内存，用简单的指针碰撞（bump pointer）
    // 来分配，速度极快，避免每帧调用大量 malloc。
    // 10 MB 是经验参考值；对于本示例的 2 个刚体来说绰绰有余。
    TempAllocatorImpl temp_allocator(10 * 1024 * 1024);

    // ── 6. 任务系统（Job System）─────────────────────────────────────────────
    // 现代物理引擎把碰撞检测、约束求解等工作拆成互相依赖的"Job"，
    // 通过任务系统并行运行在多个 CPU 核心上。
    // cMaxPhysicsJobs、cMaxPhysicsBarriers 是 Jolt 内置的推荐上限常量。
    // thread::hardware_concurrency() - 1 表示"用除主线程以外的所有核"，
    // 这样主线程仍然流畅，不会被物理计算完全抢占。
    JobSystemThreadPool job_system(cMaxPhysicsJobs, cMaxPhysicsBarriers,
                                   thread::hardware_concurrency() - 1);

    // ── 7. 物理系统容量参数 ───────────────────────────────────────────────────
    // 这些参数在初始化时一次性确定内存布局，之后不能动态扩容，所以要提前规划好上限。

    // 场景中刚体的最大数量。超出时 CreateBody 会返回 nullptr。
    // 生产项目建议 65536；这里用 1024 仅为演示。
    const uint cMaxBodies = 1024;

    // 保护刚体数据的互斥锁数量。0 表示使用 Jolt 的默认值（通常是 CPU 核心数的倍数）。
    // 多线程访问同一刚体时需要锁，锁的数量影响并发粒度。
    const uint cNumBodyMutexes = 0;

    // 宽相每帧产出的"候选碰撞对"的队列容量。
    // 好比工厂流水线上待检验的零件槽位数：槽位满了，上游就得等待。
    // 队列太小会导致宽相线程被迫串行执行窄相，降低并行效率。
    const uint cMaxBodyPairs = 1024;

    // 接触约束（Contact Constraint）缓冲区的容量。
    // 每对碰撞的刚体会产生若干接触约束（用于计算弹力和摩擦）。
    // 超出上限后多余的接触会被丢弃，刚体可能相互穿透。
    // 生产项目建议 10240；这里用 1024 仅为演示。
    const uint cMaxContactConstraints = 1024;

    // ── 8. 层接口对象 ────────────────────────────────────────────────────────
    // 下面三个对象实现了前面定义的碰撞过滤规则。
    // 重要：PhysicsSystem 内部只持有它们的指针（引用），
    //       所以这三个对象必须在 physics_system 销毁之前一直存活。
    //       这里把它们放在栈上，在 main 结束时和 physics_system 一起析构，没问题。

    BPLayerInterfaceImpl broad_phase_layer_interface;                    // 对象层 → 宽相层 映射
    ObjectVsBroadPhaseLayerFilterImpl object_vs_broadphase_layer_filter; // 对象层 vs 宽相层 碰撞过滤
    ObjectLayerPairFilterImpl object_vs_object_layer_filter;             // 对象层 vs 对象层 碰撞过滤

    // ── 9. 创建物理系统 ──────────────────────────────────────────────────────
    // PhysicsSystem 是整个物理世界的容器，包含所有刚体、约束和宽相数据结构。
    // Init() 根据上面的参数一次性分配好内部内存。
    PhysicsSystem physics_system;
    physics_system.Init(cMaxBodies, cNumBodyMutexes, cMaxBodyPairs, cMaxContactConstraints,
                        broad_phase_layer_interface,
                        object_vs_broadphase_layer_filter,
                        object_vs_object_layer_filter);

    // ── 10. 注册监听器（可选）────────────────────────────────────────────────
    // 监听器是纯观察者，不影响物理计算结果，只用于通知外部系统。
    // 如果不需要，直接跳过这两步即可。
    MyBodyActivationListener body_activation_listener;
    physics_system.SetBodyActivationListener(&body_activation_listener);

    MyContactListener contact_listener;
    physics_system.SetContactListener(&contact_listener);

    // ── 11. 获取刚体接口（BodyInterface）────────────────────────────────────
    // BodyInterface 是你与刚体交互的主要入口：创建、销毁、读写位置/速度等。
    // GetBodyInterface()（无 No 后缀）返回加锁版本，内部自动处理多线程竞争，
    // 适合从主线程使用。若确定只在单线程访问，可用 GetBodyInterfaceNoLock() 提升性能。
    BodyInterface &body_interface = physics_system.GetBodyInterface();

    // ── 12. 创建地板（静态刚体）──────────────────────────────────────────────
    // Jolt 创建刚体分三步：形状设置 → 形状实例 → 刚体设置 → 刚体实例。
    // 这种分离设计让同一个形状可以被多个刚体复用，节省内存。

    // 步骤 A：描述形状参数
    // BoxShapeSettings 接受一个"半尺寸"向量（half-extent）：
    // Vec3(100, 1, 100) 表示一个 200×2×200 的大平板。
    BoxShapeSettings floor_shape_settings(Vec3(100.0f, 1.0f, 100.0f));

    // SetEmbedded() 告诉引用计数系统"这个对象嵌入在栈上，不要在引用计数归零时 delete 它"。
    // 凡是在栈上构造的 RefTarget（引用计数基类）都必须调用此函数，否则会触发 double-free。
    floor_shape_settings.SetEmbedded();

    // 步骤 B：从设置创建形状实例（返回结果对象，可检查错误）
    ShapeSettings::ShapeResult floor_shape_result = floor_shape_settings.Create();
    // Get() 取出形状智能指针；如果 Create() 失败可以用 HasError() / GetError() 诊断。
    ShapeRefC floor_shape = floor_shape_result.Get();

    // 步骤 C：描述刚体参数
    // BodyCreationSettings 构造参数依次是：形状、位置、旋转、运动类型、对象层
    // RVec3(0, -1, 0) 把地板中心放在 Y=-1 处，使其上表面恰好在 Y=0（世界原点平面）。
    // Quat::sIdentity() 表示无旋转（单位四元数）。
    //
    // EMotionType 有三种选项：
    //
    //   Static    — 完全静止，不受任何力或冲量影响，物理引擎不为它积分运动方程。
    //               好比场景里钉死的墙和地板：其他物体会被它弹开，但它自己纹丝不动。
    //               CPU 开销最低，适合地形、建筑等永远不移动的物体。
    //
    //   Kinematic — 由程序直接驱动位置/速度，不响应力和碰撞冲量。
    //               好比游戏里的移动平台或电梯：它能把站在上面的角色推开，
    //               但你撞它一拳，它不会因为受力而改变轨迹——路径完全由代码控制。
    //               常用于需要精确运动控制、又要和动态物体交互的对象。
    //
    //   Dynamic   — 完全由物理引擎模拟：受重力、受碰撞冲量、有惯性和摩擦。
    //               好比真实世界里扔出去的球：你只需给初速度，之后引擎全权接管。
    //               CPU 开销最高，适合箱子、道具、布娃娃等需要真实物理表现的对象。
    BodyCreationSettings floor_settings(floor_shape,
                                        RVec3(0.0_r, -1.0_r, 0.0_r),
                                        Quat::sIdentity(),
                                        EMotionType::Static,
                                        Layers::NON_MOVING);

    // 步骤 D：创建刚体并添加到世界
    // CreateBody 仅创建刚体对象，不自动加入物理世界。
    // 若已达 cMaxBodies 上限，返回 nullptr，生产代码中务必检查。
    Body *floor = body_interface.CreateBody(floor_settings);

    // AddBody 把刚体"插入"物理世界，使其参与碰撞和宽相。
    //
    // EActivation 有两个选项：
    //
    //   Activate      — 立即激活，物理引擎从本帧开始模拟它的运动。
    //                   适合需要马上运动的动态对象，比如扔出去的球。
    //
    //   DontActivate  — 保持当前激活状态不变（新加入的对象默认是未激活的）。
    //                   适合静止对象（地板、墙壁），它们不需要每帧被模拟，
    //                   引擎只在其他对象碰到它时才会用到它的碰撞数据。
    //                   注意：此选项不会强制让一个已激活的对象进入休眠。
    body_interface.AddBody(floor->GetID(), EActivation::DontActivate);

    // ── 13. 创建球体（动态刚体）──────────────────────────────────────────────
    // 这里使用"创建并添加"的简写版 CreateAndAddBody，等价于 CreateBody + AddBody。
    // SphereShape(0.5f) 直接构造形状，半径 0.5m，直径 1m 的球。
    // RVec3(0, 2, 0) 初始位置在地板上方约 3m（地板上表面 Y=0，球心 Y=2，半径 0.5）。
    // EMotionType::Dynamic 表示完全受物理模拟控制：受重力、受碰撞力、有惯性。
    // EActivation::Activate 表示立即激活，否则球会静止在空中不下落。
    BodyCreationSettings sphere_settings(new SphereShape(0.5f),
                                         RVec3(0.0_r, 2.0_r, 0.0_r),
                                         Quat::sIdentity(),
                                         EMotionType::Dynamic,
                                         Layers::MOVING);
    BodyID sphere_id = body_interface.CreateAndAddBody(sphere_settings, EActivation::Activate);

    // 给球一个初始向下的速度（-5 m/s），模拟从上方扔下的场景。
    // 注意：若用 CreateBody 而非 CreateAndAddBody，可以在 AddBody 之前直接修改 Body* 的速度，
    //       这样更高效（不需要通过接口加锁）。
    body_interface.SetLinearVelocity(sphere_id, Vec3(0.0f, -5.0f, 0.0f));

    // ── 14. 模拟循环 ─────────────────────────────────────────────────────────
    // 物理模拟以固定时间步长（Fixed Timestep）推进，这是保证模拟稳定的标准做法。
    // 60 Hz（约 16.67 ms/帧）是 Jolt 推荐的更新频率，足以处理大多数游戏场景。
    // 若游戏帧率低于 60 Hz，需要每帧调用多次 Update，或使用次步骤（sub-steps）。
    const float cDeltaTime = 1.0f / 60.0f;

    // OptimizeBroadPhase 对宽相的 BVH 树做一次全局重建（SAH 算法），
    // 使树的结构最优，提升后续碰撞查询性能。
    // 适合在场景加载完成、批量插入对象之后调用一次。
    // 切勿每帧调用：重建代价很高，而且增量插入时宽相会自动维护树的质量。
    physics_system.OptimizeBroadPhase();

    // 持续模拟，直到球进入休眠（速度极小且静止在地板上）。
    // IsActive() 返回 false 意味着物理系统已自动将球设为休眠状态。
    uint step = 0;
    while (body_interface.IsActive(sphere_id))
    {
        ++step;

        // 读取球的质心位置和线速度，输出到终端。
        // GetCenterOfMassPosition 返回质心世界坐标（对于均匀球体，质心 = 几何中心）。
        RVec3 position = body_interface.GetCenterOfMassPosition(sphere_id);
        Vec3  velocity = body_interface.GetLinearVelocity(sphere_id);
        cout << "步骤 " << step
             << ": 位置 = (" << position.GetX() << ", " << position.GetY() << ", " << position.GetZ() << ")"
             << ", 速度 = (" << velocity.GetX() << ", " << velocity.GetY() << ", " << velocity.GetZ() << ")"
             << endl;

        // cCollisionSteps = 1 表示每次 Update 只做一个碰撞步骤。
        // 若 cDeltaTime 较大（如游戏卡顿），可以设为 2 或更多，
        // 相当于把一大步拆成多小步，防止高速物体"隧穿"（穿越薄墙而不触发碰撞）。
        const int cCollisionSteps = 1;

        // Update 是物理模拟的核心调用：
        //   1. 宽相更新：把移动的对象从旧位置移到新位置，维护 BVH 树
        //   2. 碰撞检测：宽相粗筛 → 窄相精算，产生接触点列表
        //   3. 约束求解：求解接触约束（弹力/摩擦）和关节约束，计算速度变化
        //   4. 位置积分：用新速度更新所有刚体的位置和旋转
        // temp_allocator 提供本次 Update 的临时内存；Update 结束后自动全部释放。
        // job_system 负责把上述子任务拆发到多个线程并行执行。
        physics_system.Update(cDeltaTime, cCollisionSteps, &temp_allocator, &job_system);
    }

    // ── 15. 清理 ─────────────────────────────────────────────────────────────
    // RemoveBody：把刚体从物理世界摘除（不再参与碰撞），但保留刚体数据。
    // DestroyBody：释放刚体占用的内存，之后 BodyID 永久失效，不能再使用。
    // 必须先 Remove 再 Destroy，顺序不能颠倒。
    body_interface.RemoveBody(sphere_id);
    body_interface.DestroyBody(sphere_id);

    body_interface.RemoveBody(floor->GetID());
    body_interface.DestroyBody(floor->GetID());

    // UnregisterTypes：注销所有内置类型，清理 CollisionDispatch 表和默认材质。
    // 必须在 delete Factory::sInstance 之前调用，因为注销过程会访问工厂。
    UnregisterTypes();

    // 删除工厂单例，释放注册表内存。
    // 置 nullptr 是良好习惯，防止悬空指针。
    delete Factory::sInstance;
    Factory::sInstance = nullptr;

    return 0;
}