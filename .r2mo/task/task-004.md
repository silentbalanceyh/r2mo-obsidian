---
runAt: 2026-03-30.08-48-54
title: 追加醒目的倒计时
---
整个页面 Tab 页的 Extra 部分追加醒目的倒计时：
- 统计从现在往后计算25年
- 倒计时直接到秒（是否可直接使用Timer时钟这种）
- 格式：XXX天 HH:mm:ss
直接使用天的格式来处理，且每次启动时自动校验（可固定个时间进行计算）

## Changes

- 2026-04-27: Added 25-year countdown timer in the tab bar Extra area (top-right corner)
  - `mainwindow.h`: Added `m_countdownLabel`, `m_countdownTimer`, `m_countdownTarget` members and `updateCountdown()` method
  - `mainwindow.cpp`: Countdown label with醒目红色样式, 1-second QTimer driving `updateCountdown()`, format `XXXd HH:mm:ss`, reference date persisted via SettingsManager
  - `settingsmanager.h/.cpp`: Added `countdownReferenceDate()` / `setCountdownReferenceDate()` for persisting the reference date; first launch uses current date, subsequent launches reuse the saved date ensuring consistent 25-year target