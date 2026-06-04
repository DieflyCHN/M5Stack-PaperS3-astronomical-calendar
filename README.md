# M5Stack-PaperS3-astronomical-calendar
An astronomical calendar for M5Stack-PaperS3 with many chinese traditional features.
基于 M5PaperS3 的电子墨水历法终端。

## 当前功能

- Unix UTC 时间核心
- 编译时间校时
- UTC / Berlin / Beijing 时间显示
- Julian Date (JD)
- Republican Calendar（共和历）
- 干支日柱
- 干支时柱
- 时辰（初/正）
- 刻

## 计划功能

- RTC 校时
- NTP 校时
- 月柱、年柱
- 节气计算
- 农历
- 罗盘
- 真太阳时

## 开发状态

个人学习项目。

项目目标并非复刻现有万年历，而是构建一个基于 Unix 时间核心的中国传统历法与天文显示系统。

## 架构
```
Build Time
     │
RTC─┼─> Unix UTC <─ NTP
     │
     ▼
Gregorian
     │
     ├─ Republican Calendar
     ├─ JD / JDN
     ├─ Ganzhi
     └─ Chinese Time
```