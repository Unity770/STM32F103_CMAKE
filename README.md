# 一、构建说明
## 环境配置
- 下载CMAKE并将路径添加到系统环境变量
- 下载arm_gcc与ninja并添加到CmakePreset的environment；
- 下载openocd并将路径添加到task.json
## 编译指令
- 配置（仅生成构建系统）：
```powershell
cmake --preset Debug
```
- 构建（编译并链接）：
```powershell
cmake --preset Debug --build 
```
- 清理（推荐）：使用 CMake 的 clean 目标：
```powershell
cmake --build --preset Debug --target clean
```
## 自动化任务
  已配置vscode的自动化任务：
  - build\clean\flash