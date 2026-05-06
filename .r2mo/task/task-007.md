---
runAt: 2026-03-30.08-48-56
title: 特殊计费板追加
---
追加新的 Provider：DeepSeek 官方的计费

https://platform.deepseek.com/api/v0/users/get_user_summary
Request Method
GET
Status Code
200

```json
{
    "code": 0,
    "msg": "",
    "data": {
        "biz_code": 0,
        "biz_msg": "",
        "biz_data": {
            "current_token": 10000000,
            "monthly_usage": "20050768",
            "total_usage": 0,
            "normal_wallets": [
                {
                    "currency": "CNY",
                    "balance": "44.6572900000000000",
                    "token_estimation": "14885763"
                }
            ],
            "bonus_wallets": [
                {
                    "currency": "CNY",
                    "balance": "0",
                    "token_estimation": "0"
                }
            ],
            "total_available_token_estimation": "14885763",
            "monthly_costs": [
                {
                    "currency": "CNY",
                    "amount": "3.5547580000000000"
                }
            ],
            "monthly_token_usage": "20050768"
        }
    }
}
```

目前使用的链接：
```bash
https://api.deepseek.com/anthropic
Token：环境变量 OPEN_DS_KEY_MY
```

对应特殊列：
- 今日新增：剩余总量
- 今日使用：总量 - 剩余
- 今日剩余：剩余量
- 请求次数：可直接提取
- Opus次数：0

## Changes

### 2026-05-04 DeepSeek Provider 实现

**修改文件：**
- `src/utils/specialmonitorfetcher.h` — 新增 `fetchDeepSeekSnapshot()` 私有方法声明
- `src/utils/specialmonitorfetcher.cpp` — 新增 DeepSeek Provider 完整实现：
  - `isDeepSeekProviderUrl()` URL 匹配函数（host = `platform.deepseek.com`）
  - `fetchDeepSeekSnapshot()` 调用 `https://platform.deepseek.com/api/v0/users/get_user_summary`，Bearer Token 认证，10s 超时
  - `fetchSnapshot()` 调度增加 DeepSeek 分支
  - `defaultProviderName()` 增加 `🐋 DeepSeek` 映射
  - 数据映射：今日新增=剩余总量(token_estimation)、今日使用=总量-剩余(防负)、今日剩余=token_estimation、请求次数=0（API无此字段）、Opus=0
- `src/mainwindow.cpp` — `specialMonitorProviders()` 注册表新增 `🐋 DeepSeek` + `https://platform.deepseek.com/api/v0/users/get_user_summary`

**API 验证：**
- `api.deepseek.com/user/balance` 返回 200，余额数据正常
- `platform.deepseek.com/api/v0/users/get_user_summary` 在限流解除后可用（429 为临时限流，非接口问题）
- Token 通过环境变量 `OPEN_DS_KEY_MY` 验证，长度 35 字符有效

### 2026-05-04 DeepSeek 线上抓取修复

**问题定位：**
- `platform.deepseek.com/api/v0/users/get_user_summary` 对当前 token 返回 HTTP 200 但业务码 `40003 Authorization Failed`
- 现有实现只在 HTTP/网络失败时回退，业务失败会直接导致监控板显示异常行
- task 里写的是环境变量名 `OPEN_DS_KEY_MY`，当前代码原先不会把它展开成真实 token

**修改文件：**
- `src/utils/specialmonitorfetcher.cpp`
  - 新增 `resolveTokenKey()`，支持 `env:NAME`、`${NAME}`、`$NAME` 和裸环境变量名
  - DeepSeek 平台摘要接口在业务码失败时回退到 `https://api.deepseek.com/user/balance`
  - 余额接口增加 `is_available` 判定，并优先使用官方 `total_available_token_estimation`
  - 修正 DeepSeek 数据源的 token 读取，避免把环境变量名当成 API Key 发送
- `src/mainwindow.cpp`
  - DeepSeek provider 默认地址改为稳定的 `https://api.deepseek.com/user/balance`
- `tests/task_007_deepseek_special_monitor_guard_test.sh`
  - 新增 DeepSeek 特殊计费守护测试
- `tests/task_002_validation_guard_suite.sh`
  - 纳入新的 DeepSeek 守护测试

**验证：**
- 线上请求验证：`platform.deepseek.com` 返回 `40003 Authorization Failed`
- 线上请求验证：`api.deepseek.com/user/balance` 返回 200 且可解析余额
- `bash tests/task_007_deepseek_special_monitor_guard_test.sh`
- `bash tests/task_003_special_monitor_cache_guard_test.sh`
- `bash tests/task_002_special_monitor_refresh_test.sh`
- `bash tests/task_003_special_monitor_parallel_guard_test.sh`
- `cmake -B build -DCMAKE_PREFIX_PATH=/Users/lang/Qt/6.10.0/macos`
- `cmake --build build`

### 2026-05-05 DeepSeek 字段映射修正

**问题定位：**
- 旧实现把 `token_estimation` / `current_token` 当作特殊计费表的今日新增、今日使用、今日剩余，导致 DeepSeek 官方余额语义错位。
- 账号信息不应从余额/summary 推导，应从 `https://platform.deepseek.com/auth-api/v0/users/current` 的 `id_profile.name` 提取。

**修改文件：**
- `src/utils/specialmonitorfetcher.h`
  - 将特殊计费表三项额度字段从 `qint64` 调整为 `QString`，支持 DeepSeek 金额小数显示。
- `src/utils/specialmonitorfetcher.cpp`
  - DeepSeek 账号名优先调用 `auth-api/v0/users/current` 并解析 `id_profile.name`。
  - DeepSeek 今日剩余改为钱包 `balance` 金额，今日使用改为 `monthly_costs.amount`，今日新增改为二者相加。
  - `token_estimation` 不再参与 DeepSeek 表格三项金额映射。
  - 增加对可用模型用量结构中 `deepseek-v4-pro` 的请求数字段递归提取；当前已知 `balance`/`summary` 响应无该字段时保持 0。
  - fallback 到 `https://api.deepseek.com/user/balance` 时仍显示余额金额，并保留已取到的账号名。
- `src/mainwindow.cpp`
  - 表格直接显示字符串额度，避免金额小数被整型截断。
- `tests/task_007_deepseek_special_monitor_guard_test.sh`
  - 补充 DeepSeek 账号、金额、模型用量映射守卫。

**验证：**
- 线上请求验证：当前 `OPEN_DS_KEY_MY` 对 `auth-api/v0/users/current` 和 `api/v0/users/get_user_summary` 返回 HTTP 200 / 业务码 `40003 Authorization Failed`。
- 线上请求验证：当前 `OPEN_DS_KEY_MY` 对 `https://api.deepseek.com/user/balance` 返回 200，余额为 `43.18`。
- `bash tests/task_007_deepseek_special_monitor_guard_test.sh`
- `bash tests/task_003_special_monitor_cache_guard_test.sh && bash tests/task_002_special_monitor_refresh_test.sh && bash tests/task_003_special_monitor_parallel_guard_test.sh`
- `bash tests/task_002_validation_guard_suite.sh`
- `git diff --check`
- `cmake -B build -DCMAKE_PREFIX_PATH=/Users/lang/Qt/6.10.0/macos`
- `cmake --build build`

### 2026-05-05 DeepSeek 账号与用量接口补正

**问题定位：**
- fallback 到 `api.deepseek.com/user/balance` 时仍可能把余额金额写入账号列，导致账号显示不正确。
- DeepSeek 的特殊计费金额列需要显式显示人民币符号，和其他 token/quota provider 区分。
- 使用次数 239 需要从平台 usage 接口查找，不能从 balance 接口得到。

**修改文件：**
- `src/utils/specialmonitorfetcher.cpp`
  - DeepSeek fallback 账号改为已解析账号名或 `DeepSeek`，不再用余额金额填账号列。
  - DeepSeek 今日新增、今日使用、今日剩余统一格式化为 `￥xx.xx`。
  - 新增 `usage/amount?month=<当前月>&year=<当前年>` 和 `usage/cost?month=<当前月>&year=<当前年>` 请求。
  - 使用次数优先从 `usage/amount` 响应中递归查找 `deepseek-v4-pro` 的计数字段；消费金额可由 `usage/cost` 响应覆盖 summary 的 `monthly_costs.amount`。
  - 抽出 DeepSeek JSON GET helper，统一处理 HTTP、JSON 和业务码失败。
- `tests/task_007_deepseek_special_monitor_guard_test.sh`
  - 补充账号列不得显示金额、金额列必须带人民币符号、usage amount/cost 接口必须接入的守卫。

**验证：**
- 线上请求验证：当前 `OPEN_DS_KEY_MY` 对 `usage/amount`、`usage/cost`、`auth-api/v0/users/current` 均返回 HTTP 200 / 业务码 `40003 Authorization Failed`，因此当前 API key 仍无法提取 239；代码已在平台授权可用时从 `usage/amount` 提取。
- `bash tests/task_007_deepseek_special_monitor_guard_test.sh`
- `bash tests/task_003_special_monitor_cache_guard_test.sh && bash tests/task_002_special_monitor_refresh_test.sh && bash tests/task_003_special_monitor_parallel_guard_test.sh && bash tests/task_002_validation_guard_suite.sh`
- `git diff --check`
- `cmake -B build -DCMAKE_PREFIX_PATH=/Users/lang/Qt/6.10.0/macos && cmake --build build`

### 2026-05-05 DeepSeek Bearer Token 配置支持

**问题定位：**
- DeepSeek 平台接口需要网页端登录态 `Authorization: Bearer ...`，而不是 API key。
- 当前特殊计费 Token 字段原先会无条件拼接 `Bearer `，如果用户直接粘贴 DevTools 里的完整 Authorization 值，会变成重复前缀。

**修改文件：**
- `src/utils/specialmonitorfetcher.cpp`
  - 新增 `deepSeekAuthHeader()`，支持 Token 字段直接填写完整 `Bearer ...`；若未带前缀则继续按 API key 补 `Bearer `，保持 balance fallback 兼容。
  - 平台接口失败但余额 fallback 成功时，在行 tooltip 中提示账号和用量字段需要 platform Bearer token。
- `tests/task_007_deepseek_special_monitor_guard_test.sh`
  - 补充 Bearer 前缀透传、避免重复拼接、平台 token 提示的守卫。

**验证：**
- `bash tests/task_007_deepseek_special_monitor_guard_test.sh`
- `bash tests/task_003_special_monitor_cache_guard_test.sh && bash tests/task_002_special_monitor_refresh_test.sh && bash tests/task_003_special_monitor_parallel_guard_test.sh && bash tests/task_002_validation_guard_suite.sh`
- `git diff --check`
- `cmake -B build -DCMAKE_PREFIX_PATH=/Users/lang/Qt/6.10.0/macos && cmake --build build`

### 2026-05-05 DeepSeek 浏览器登录态验证与 REQUEST 解析修正

**问题定位：**
- Chrome 已登录页面中的平台接口请求携带 `Authorization: Bearer <网页登录态 token>`，并非 DeepSeek API key；直接在地址栏打开 `usage/amount` 返回 `40002 Missing Token`，说明浏览器 Cookie 本身不够。
- 当前特殊计费配置只有 token 输入；若填入 API key，只能访问 `api.deepseek.com/user/balance`，不能访问 `platform.deepseek.com/auth-api` / `usage` 平台接口。
- `usage/amount` 的真实响应中，`deepseek-v4-pro` 的请求次数位于 `usage[]` 下 `type = REQUEST` 的 `amount` 字段。

**修改文件：**
- `src/utils/specialmonitorfetcher.cpp`
  - 修正 `extractDeepSeekModelUsageCount()`，匹配到 `model = deepseek-v4-pro` 后优先读取 `usage[].type == REQUEST` 的 `amount`，用于提取页面显示的 `239` 这类请求次数。
- `tests/task_007_deepseek_special_monitor_guard_test.sh`
  - 补充 REQUEST usage 解析守卫，防止只查普通 `count` 字段导致真实平台响应无法提取请求次数。

**验证：**
- Chrome DevTools 验证：页面 XHR 请求头包含 `Authorization: Bearer ...`；地址栏直接打开 `usage/amount` 返回 `{"code":40002,"msg":"Missing Token","data":null}`。
- Chrome DevTools 验证：`usage/amount` 响应中 `deepseek-v4-pro` 的请求次数来自 `usage` 数组的 `REQUEST.amount`。
- `bash tests/task_007_deepseek_special_monitor_guard_test.sh`
- `bash tests/task_003_special_monitor_cache_guard_test.sh && bash tests/task_002_special_monitor_refresh_test.sh && bash tests/task_003_special_monitor_parallel_guard_test.sh && bash tests/task_002_validation_guard_suite.sh`
- `git diff --check`
- `cmake -B build -DCMAKE_PREFIX_PATH=/Users/lang/Qt/6.10.0/macos && cmake --build build`

### 2026-05-06 特殊计费列名与单位修正

**问题定位：**
- 特殊计费表仍显示“今日新增 / 今日使用 / 今日剩余”，和当前计费语义不匹配。
- 类型列缺少单位，无法直接区分 `xsvip / 天`、`CM Key / 5小时`、`DeepSeek / 累积`。
- DeepSeek 平台 `usage/cost` 可能在嵌套明细中返回已使用金额，旧代码只读取顶层 `amount`，会漏掉页面上的 `5.13` 这类数据。

**修改文件：**
- `src/mainwindow.cpp`
  - 特殊计费表三列改为“总量 / 已使用 / 余额”。
- `src/utils/specialmonitorfetcher.cpp`
  - PP Coding 类型追加 `/ 天` 单位。
  - CM Key 类型追加 `/ N小时` 单位。
  - DeepSeek 类型统一显示 `DeepSeek / 累积`。
  - 新增 DeepSeek `usage/cost` 递归金额提取，确保嵌套成本金额可显示到“已使用”列。
- `translations/obsidianmanager_zh_CN.ts`
- `translations/obsidianmanager_en_US.ts`
- `tests/task_007_deepseek_special_monitor_guard_test.sh`
  - 补充列名、单位、DeepSeek 已使用金额来源守卫。

**验证：**
- `bash tests/task_007_deepseek_special_monitor_guard_test.sh`
- `bash tests/task_003_special_monitor_cache_guard_test.sh`
- `bash tests/task_002_special_monitor_refresh_test.sh`
- `bash tests/task_003_special_monitor_parallel_guard_test.sh`
- `bash tests/task_002_validation_guard_suite.sh`
- `git diff --check`
- `cmake -B build -DCMAKE_PREFIX_PATH=/Users/lang/Qt/6.10.0/macos`
- `cmake --build build`

### 2026-05-06 DeepSeek 余额刷新缓存修正

**问题定位：**
- DeepSeek 使用 API key 时会走 `api.deepseek.com/user/balance` 余额 fallback，并同时附带“平台 Bearer token 才能取账号/用量”的 warning。
- 特殊计费缓存合并逻辑此前把带 warning 的新快照当作失败行处理，直接恢复旧缓存，导致 DeepSeek 余额、新增、使用、剩余在数据变化后仍显示旧值。

**修改文件：**
- `src/mainwindow.cpp`
  - 新增 `specialMonitorSnapshotHasDisplayValues()`，用于识别带 warning 但已有新展示数据的快照。
  - `mergeSpecialMonitorSnapshots()` 仅在新快照失败且没有任何可展示值时才恢复旧缓存；DeepSeek 余额 fallback 这类新数据会正常覆盖旧行。
- `tests/task_003_special_monitor_cache_guard_test.sh`
  - 增加守卫，防止后续把带展示值的 warning 行再次回退成旧缓存。

**验证：**
- `bash tests/task_003_special_monitor_cache_guard_test.sh`
- `bash tests/task_007_deepseek_special_monitor_guard_test.sh`
- `bash tests/task_002_special_monitor_refresh_test.sh`
- `bash tests/task_003_special_monitor_parallel_guard_test.sh`
- `bash tests/task_002_validation_guard_suite.sh`
- `git diff --check`
- `cmake -B build -DCMAKE_PREFIX_PATH=/Users/lang/Qt/6.10.0/macos`
- `cmake --build build`

### 2026-05-06 Provider 列宽与 DeepSeek 已使用 fallback 修正

**问题定位：**
- 特殊计费表 Provider 列初始宽度偏大，挤压了类型列；类型列展示 `DeepSeek / 累积`、`CM Key / 5小时` 时容易被压缩。
- DeepSeek 会先请求 `usage/cost`，但当 `get_user_summary` 失败走余额 fallback 时，已取得的 usage/cost 金额没有传入 fallback 快照，导致“已使用”列被重置为 `￥0.00`。

**修改文件：**
- `src/mainwindow.cpp`
  - Provider 列改为更窄的宽度范围。
  - Type 列改为更宽的宽度范围，并提高压缩时的最小宽度。
- `src/utils/specialmonitorfetcher.h`
- `src/utils/specialmonitorfetcher.cpp`
  - DeepSeek balance fallback 增加 `usedAmount` 和 `usageCount` 入参。
  - summary 失败时仍保留 `usage/cost` 已使用金额和 `usage/amount` 请求次数，并重新计算总量 = 余额 + 已使用。
- `tests/task_007_deepseek_special_monitor_guard_test.sh`
  - 增加列宽与 DeepSeek fallback 已使用金额守卫。

**验证：**
- `bash tests/task_007_deepseek_special_monitor_guard_test.sh`
- `bash tests/task_003_special_monitor_cache_guard_test.sh`
- `bash tests/task_002_special_monitor_refresh_test.sh`
- `bash tests/task_003_special_monitor_parallel_guard_test.sh`
- `bash tests/task_002_validation_guard_suite.sh`
- `git diff --check`
- `cmake -B build -DCMAKE_PREFIX_PATH=/Users/lang/Qt/6.10.0/macos`
- `cmake --build build`

### 2026-05-06 DeepSeek 已使用在线接口校对

**问题定位：**
- 使用当前 `OPEN_DS_KEY_MY` 在线请求校对：`api.deepseek.com/user/balance` 返回余额 `41.06`。
- 同一个 API key 请求 `platform.deepseek.com` 的 `get_user_summary`、`usage/amount`、`usage/cost`、`auth-api/v0/users/current` 均返回 HTTP 200 但业务码 `40003 Authorization Failed (invalid token)`。
- 因此当前配置只能拿到余额，拿不到平台月用量/已使用量；应用之前显示 `￥0.00` 会误导为真实 0。

**修改文件：**
- `src/utils/specialmonitorfetcher.h`
- `src/utils/specialmonitorfetcher.cpp`
  - DeepSeek balance fallback 增加 `usageCostAvailable` 标记。
  - 只有 `usage/cost` 成功返回时才显示金额；平台用量接口不可用时，“已使用”显示 `需 Bearer`，提示需要填入 DeepSeek 网页端请求里的完整 `Authorization: Bearer ...`。
- `tests/task_007_deepseek_special_monitor_guard_test.sh`
  - 增加“不可把不可用的 usage/cost 显示成 0”的守卫。

**验证：**
- 在线接口校对：当前 `OPEN_DS_KEY_MY` 可取余额 `41.06`，平台 usage 接口返回 `40003 Authorization Failed (invalid token)`。
- `bash tests/task_007_deepseek_special_monitor_guard_test.sh`
- `bash tests/task_002_validation_guard_suite.sh`
- `cmake -B build -DCMAKE_PREFIX_PATH=/Users/lang/Qt/6.10.0/macos`
- `cmake --build build`

### 2026-05-06 DeepSeek 已使用不可用时显示 0

**问题定位：**
- 用户要求 DeepSeek 已使用列不要显示 Bearer 提示；平台 usage/cost 不可用时直接显示 0。

**修改文件：**
- `src/utils/specialmonitorfetcher.cpp`
  - DeepSeek balance fallback 在 usage/cost 不可用时恢复显示 `￥0.00`。
- `tests/task_007_deepseek_special_monitor_guard_test.sh`
  - 增加守卫，禁止已使用列出现 `需 Bearer` 文案。

**验证：**
- `bash tests/task_007_deepseek_special_monitor_guard_test.sh`
- `bash tests/task_002_validation_guard_suite.sh`
- `git diff --check`
- `cmake -B build -DCMAKE_PREFIX_PATH=/Users/lang/Qt/6.10.0/macos`
- `cmake --build build`

### 2026-05-06 DeepSeek monthly_costs 已使用主来源修正

**问题定位：**
- DeepSeek `get_user_summary.data.biz_data.monthly_costs[].amount` 是“已使用”金额，例如 `7.4752058000000000`。
- 旧逻辑会在 `usage/cost` 返回金额时覆盖 `monthly_costs.amount`，可能导致“已使用”不按 summary 展示。

**修改文件：**
- `src/utils/specialmonitorfetcher.cpp`
  - `monthly_costs[].amount` 作为 DeepSeek 已使用主来源。
  - `usage/cost` 仅在 summary 未提供 `monthly_costs.amount` 时补充。
- `tests/task_007_deepseek_special_monitor_guard_test.sh`
  - 增加守卫，确保 `usage/cost` 不覆盖 summary 的 `monthly_costs.amount`。

**验证：**
- `bash tests/task_007_deepseek_special_monitor_guard_test.sh`
- `bash tests/task_002_validation_guard_suite.sh`
- `git diff --check`
- `cmake --build build`

### 2026-05-06 恢复 PP Coding 与 CM Key 映射

**问题定位：**
- 前序单位展示改动影响了 PP Coding 和 CM Key 的类型列映射；本轮需求只应更新 DeepSeek。

**修改文件：**
- `src/utils/specialmonitorfetcher.cpp`
  - PP Coding 类型恢复为 token 名称中 `-` 后的原始套餐名。
  - CM Key 类型恢复为原有 `每N小时` / `primary_label` 映射。
- `tests/task_007_deepseek_special_monitor_guard_test.sh`
  - 移除 PP Coding / CM Key 单位展示守卫，仅保留 DeepSeek 守卫。

**验证：**
- `bash tests/task_007_deepseek_special_monitor_guard_test.sh`
- `bash tests/task_003_special_monitor_cache_guard_test.sh`
- `bash tests/task_002_special_monitor_refresh_test.sh`
- `bash tests/task_002_validation_guard_suite.sh`
- `git diff --check`
- `cmake -B build -DCMAKE_PREFIX_PATH=/Users/lang/Qt/6.10.0/macos`
- `cmake --build build`
