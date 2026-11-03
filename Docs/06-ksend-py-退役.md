# 06 · 与 `Tools/ksend.py` 退役条件

HotSend **协议同形**，目标是日常主机侧逐步取代 Blocks 仓内 `Tools/ksend.py`。  
**不**在板侧另起 OTA；py 在退役前仍是字节级金样。

## 可宣布「日常改用 HotSend」的门槛（须全部满足）

| # | 条件 | 说明 |
| --- | --- | --- |
| 1 | TCP 差量/全量绿 | NUC（或同形板）`ksend serve` → `hotsend --host …` → `KSEND OK` |
| 2 | 串口备份路径绿 | `hotsend --port /dev/ttyUSB0 …` → `KSEND OK`（壳 `Blocks>`，非 occupy） |
| 3 | `--info` | 板回 `KSEND INFO`；带 `--file` 时可判 match / diff |
| 4 | `--dry-run` | READY+CRC 表+差量表后 abort；板上文件未改 |
| 5 | 节流可选 | 默认不睡；`--pace-ms > 0`（及每 16 块加长歇）对表 py；`--steady-throttle`≡pace 500 |
| 6 | 文档指向 | Blocks 侧手测/脚本说明改为推荐 `~/HotSend`（本条由 Blocks 仓另改） |

## 仍保留 py 的理由（未退役前）

- 新协议字段 / 板 FAIL 文案变更时，先对表 py 再改 HotSend。  
- CI 或他机尚未装 Meson/GTK 时的应急。  
- `--selftest` 等 HotSend 未收录项（若日后补齐，再从本表删例外）。

## 退役动作（满足门槛后，属 Blocks 仓）

1. `Tools/ksend.py` 文首注明：日常请用 HotSend；本文件仅金样/应急。  
2. 课堂/手测步骤改贴 `hotsend` / `hotsend-gui` 命令。  
3. **不**立刻删除 py，直到连续若干周无回归依赖。

HotSend 仓本身 **不**删除或 vendoring py。
