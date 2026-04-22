# Task-002 Validation

`task-002` 的严格验证已拆成 4 组，便于单独执行或在 CI 中复用。

## Suites

- `tests/task_002_validation_guard_suite.sh`
  适合做快速源码守护回归，覆盖 Codex / Claude / OpenCode / 远端监控 / 字体下限。
- `tests/task_002_validation_build_suite.sh`
  只负责 `cmake -B build` 和 `cmake --build build -j4`。
- `tests/task_002_validation_local_suite.sh`
  覆盖本地源码约束、`task-002` 变更记录、以及 `~/.local/share/opencode/opencode.db` 的真实数据校验。
- `tests/task_002_validation_remote_suite.sh`
  覆盖远端扫描源码约束，以及 `ssh lang@mxt.webos.cn` 的真实探测校验。

## Full Run

完整 20 用例总入口：

```bash
bash tests/task_002_strict_validation_test.sh
```

分组执行：

```bash
bash tests/task_002_validation_guard_suite.sh
bash tests/task_002_validation_build_suite.sh
bash tests/task_002_validation_local_suite.sh
bash tests/task_002_validation_remote_suite.sh
```

## Notes

- 远端分组依赖免密 SSH：
  - `ssh lang@mxt.webos.cn`
- 远端分组依赖 `/tmp/remote_opencode_probe.py` 已存在。
  - 该文件在本轮任务里已经由本地源码中的远端 probe 提取生成。
- 若仅跑 CI，建议只跑 `guard` 与 `build` 两组；`remote` 更适合本地或受控环境。
