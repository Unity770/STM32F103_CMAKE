# 构建说明（README_BUILD）

本文档说明如何在本工程使用 CMake presest / PowerShell 在 Windows 上构建、清理与生成固件文件。

## 1. 工具与预设（presets）
- 本工程使用 CMake Presets (`CMakePresets.json`) 来统一配置生成器（Ninja）、构建目录与交叉编译工具链路径。
- 推荐做法：把 Ninja 与 ARM 工具链（arm-none-eabi-*）加入系统 PATH，或在 `CMakePresets.json` 的 `environment` 字段中为 `default` preset 指定绝对路径（例如 `ARM_GCC_TOOLCHAIN`、`NINJA_TOOLCHAIN`）。
  - 注意：不要在 presets 的 environment 中使用类似 `${env:PATH}` 进行复杂宏拼接（某些 CMake 版本会报错），推荐写入显式路径或仅设置单独的环境变量名。
- Toolchain 文件：`cmake/gcc-arm-none-eabi.cmake`（由 preset 的 `toolchainFile` 指定），该文件负责设置 `CMAKE_C_COMPILER`、`CMAKE_OBJCOPY`、链接脚本等交叉编译配置。

## 2. 常用 PowerShell 命令（在项目根执行）
- 切换到项目根（示例）：
```powershell
Set-Location D:\Work\Project\STM32F103_Learn\STM32F103
```

- 配置（仅生成构建系统）：
```powershell
cmake --preset Debug
```
等同于：
```powershell
cmake -S . -B build/Debug -G "Ninja" -DCMAKE_TOOLCHAIN_FILE=cmake/gcc-arm-none-eabi.cmake -DCMAKE_BUILD_TYPE=Debug
```

- 构建（编译并链接）：
```powershell
cmake --build --preset Debug
```
该命令会在 `build/Debug` 下调用 Ninja 完成编译与链接，最终目标是 `STM32F103.elf`（在 `build/Debug/`）。

- 只构建某个目标（示例）：
```powershell
cmake --build --preset Debug --target <target_name>
```

- 清理（推荐）：使用 CMake 的 clean 目标：
```powershell
cmake --build --preset Debug --target clean
```
该命令会调用底层生成器（Ninja/Make/MSBuild）的 clean 功能，并删除由 CMake 知道的中间文件与 `ADDITIONAL_CLEAN_FILES` 指定的额外文件（如 `.map`）。

- 彻底删除构建目录（重建时使用）：
```powershell
Remove-Item -Recurse -Force .\build\Debug
```

## 3. 生成 bin / hex
- 本仓库已在 `CMakeLists.txt` 添加 post-build 命令，构建完成后会自动生成：
  - `build/Debug/STM32F103.elf`
  - `build/Debug/STM32F103.bin`（binary）
  - `build/Debug/STM32F103.hex`（Intel HEX）
- 如果你希望 `cmake --build --preset Debug --target clean` 同时删除 `.bin` 和 `.hex`，可以把它们加入 `ADDITIONAL_CLEAN_FILES`：
```cmake
set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES
    ADDITIONAL_CLEAN_FILES "${CMAKE_PROJECT_NAME}.map;${CMAKE_PROJECT_NAME}.bin;${CMAKE_PROJECT_NAME}.hex"
)
```

## 4. 验证工具链是否生效
- 在 configure 输出中查找 `C` 编译器路径或检查 `build/Debug/CMakeCache.txt` 中的变量：
  - `CMAKE_TOOLCHAIN_FILE`
  - `CMAKE_C_COMPILER`
  - `CMAKE_OBJCOPY`
- 在 PowerShell 中查看工具链版本：
```powershell
D:\programs\ARM_GCC\...\bin\arm-none-eabi-gcc.exe --version
```

## 5. VS Code + CMake Tools 建议（可选）
- 推荐安装扩展：CMake Tools、Cortex-Debug（调试）、CMake（语法）。
- CMake Tools 会识别 `CMakePresets.json` 并提供选择 configure/build preset 的 UI（无需手写 tasks.json）。
- 你可以添加 `.vscode/launch.json` 与 `.vscode/tasks.json` 实现一键构建与烧写/调试工作流（仓库可以额外保存示例配置）。

## 6. 常见问题与快速排查
- 如果 `cmake --preset Debug` 报“找不到编译器”，请确认工具链路径在系统 PATH 或在 `CMakePresets.json` 中用绝对路径注入。
- 如果 presets 报错“Invalid macro expansion”，检查 `CMakePresets.json` 中是否使用了不被允许的宏（例如 `${env:PATH}` 的直接拼接）；把需要的路径改为显式字符串或让用户系统 PATH 已包含对应路径。
- 链接器错误通常与链接脚本（`.ld`）或缺失库有关，检查 `CMAKE_EXE_LINKER_FLAGS` 与 `TOOLCHAIN_LINK_LIBRARIES`。

---

如需，我可以：
- 把 `.bin/.hex` 自动加入 `ADDITIONAL_CLEAN_FILES`（使 clean 一并删除）；
- 帮你创建 `.vscode/launch.json`（OpenOCD/Cortex-Debug 示例）和 `tasks.json`（build/flash 一键化）；
- 或现在远程为你执行一次完整的构建+烧写（视你的工具链与硬件配置）。
