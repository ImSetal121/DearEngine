# 子模块使用说明

## 固定到指定版本

```bash
cd submodule/SDL
git fetch --tags
git checkout <标签或 commit>   # 例如: SDL3.0.0
cd ../..
git add submodule/SDL
git commit -m "Pin SDL to <版本>"
```

## 更新到最新版本

```bash
cd submodule/SDL
git fetch --tags
git checkout main   # 或你要跟的分支/标签
cd ../..
git add submodule/SDL
git commit -m "Update SDL submodule"
```

## 克隆时拉取子模块

```bash
git clone --recurse-submodules <仓库地址>
```

已有仓库未拉过子模块时：

```bash
git submodule update --init
```
