---
runAt: 2026-04-17.14-04-35
title: 追加特殊计费板
author:
---
实时监控板下边1/4追加一个特殊监控板，查看路径：
1. 网页打开：https://code.ppchat.vip/（添加时直接使用 emoji 名称：PP Coding）
2. 请求：GET  https://his.ppchat.vip/api/token-logs?token_key=xxxx&page=1&page_size=10
3. 响应信息参考：.r2mo/task/resource/resource.json 中

主要捕捉：
- token名称 🔑 和套餐类型 ⚙️：token_info -> "name": "silentbalanceyh-xsvip",
- 当天增加额度 ➕："today_added_quota": 7485
- 当天使用额度 🔥："today_used_quota": 3510
- 当天剩余额度 🎯："remain_quota_display": 3975
- 已使用次数 📈："today_usage_count": 234
- OPUS使用次数 💎："today_opus_usage": 0

其中：
- 统计多少条是通过增删改来实现的（就在下边表格处处理增删改）
- 统一链接下可以统计不同的 token
- 后期扩展可以追加不同的链接，现阶段只要是 https://code.ppchat.vip/ 这个地址则直接将上述流程做固定，未来有新的再重新开发
- Token列不用显示完全，多余部分 ...
- 账号和类型都来自于 name 属性。
- 记得在表格头部标题处要追加对应的 emoji 信息。
