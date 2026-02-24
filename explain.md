# Figure Explanation Notes (timeseries)

本文档解释以下新旧图的**数据来源**、**坐标轴单位**、以及**图像含义**：

1. `death_events_timeseries_all/death_events_timeseries_all.*`
2. `energy_charged_timeseries_all/energy_charged_timeseries_all.*`
3. `charge_queue_queue_len_all/charge_queue_queue_len_all.*`

---

## 1) Cumulative death events over mission time

- 生成脚本：`tools/plot_death_events_timeseries.py`
- 数据来源：每个策略目录下的 `death_events.csv`
  - 路径示例：`experiment_data_collection/test_round1/<policy>/death_events.csv`
  - 关键字段：
    - `time`（事件发生时间戳）
    - `role`（0/member, 1/CH）
    - `run_id`

### Axes / Units
- X 轴：`Mission time (s)`，单位是秒（s）
  - 由脚本通过 `align_time_seconds()` 转成相对任务时间。
- Y 轴：`Cumulative death events (count)`，单位是次数（count）
  - 含义：到当前时间累计发生了多少次死亡事件。

### 为什么现在都从 0 开始？
脚本已经强制了基线点 `(t=0, y=0)`，并把真实事件时刻轻微右移一个极小量（`EPS=1e-6`）来避免“在 t=0 同时出现 0 和 1 导致非零起点”的绘图伪影。

### 图里的色块是什么意思？
每条线是该策略的均值曲线（跨 run 聚合）；周围半透明色块是 bootstrap 置信区间（约 95% CI），表示跨 run 波动范围。

---

## 2) Cumulative charged energy over mission time

- 生成脚本：`tools/plot_energy_charged_timeseries.py`
- 数据来源：每个策略目录下的 `charge_events.csv`
  - 路径示例：`experiment_data_collection/test_round1/<policy>/charge_events.csv`
  - 关键字段：
    - `energy_recovered`（单次充电恢复量）
    - `charge_end_time`（优先）或 `terminal_time`（后备）
    - `charge_completed`（若存在，仅统计 completed）
    - `role`, `run_id`

### Axes / Units
- X 轴：`Mission time (s)`，单位秒（s）
- Y 轴：`Cumulative charged energy (battery percentage-points, %pt)`
  - 单位是电量百分比点（%pt），来自 `energy_recovered` 字段。
  - 例如一次从 30% 充到 100%，恢复量约 70 %pt。

### 为什么现在都从 0 开始？
同 death 曲线，脚本加入了 `(0,0)` 基线并将事件时间轻微右移，保证视觉上所有策略从 0 起步。

### 色块含义
同上：半透明区域是均值周围的 bootstrap 95% 置信区间。

---

## 3) Charge queue dynamics: queue_len (all)

- 生成脚本：`tools/plot_charge_queue_timeseries.py`
- 数据来源：每个策略目录下的 `charge_queue_timeseries.csv`
  - 关键字段（其一）：`queue_length_ugv` 或 `queue_length`

### Axes / Units
- X 轴：`Mission time (s)`，单位秒（s）
- Y 轴：`Queue length (vehicles)`，单位是车辆数量（vehicles）

### 为什么会出现小数（fraction）？
这通常不是“单次观测值是小数”，而是**跨 run 求均值后的结果**：
- 脚本会先按 `run_id,time` 聚合，再对 run 求平均；
- 所以同一时刻不同 run 队列长度（整数）平均后会变成小数。

也就是说，0.3 并不代表“0.3 辆车”，而是“该时刻跨 run 的平均队列长度约 0.3”。

---

## How to regenerate

```bash
python3 -m tools.run_all_figures --data_root experiment_data_collection/test_round1 --out_dir analysis/test_round1
```

如果只重画某一张：

```bash
python3 -m tools.plot_death_events_timeseries --data_root experiment_data_collection/test_round1 --out_dir analysis/test_round1 --role ALL
python3 -m tools.plot_energy_charged_timeseries --data_root experiment_data_collection/test_round1 --out_dir analysis/test_round1 --role ALL
python3 -m tools.plot_charge_queue_timeseries --data_root experiment_data_collection/test_round1 --out_dir analysis/test_round1 --metric queue_len --role_scope all
```
