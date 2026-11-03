# HotSend

Ubuntu 原生主机侧热传工具：把本地文件差量写入 Blocks 板端 FAT 路径（网络 TCP 主路，串口备份）。

- **产品名**：HotSend  
- **本仓路径**：`~/HotSend`（独立 Git 库，**不**在 Blocks 树内）  
- **文档**：[`Docs/`](Docs/)  
- **源码**：[`Code/`](Code/)（本刀仅占位；实现按 PR 拆分合入）  
- **产物**：[`Build/`](Build/)（不入库二进制）  
- **协议对表**（姊妹仓 Blocks）：板端 `Kernel/HAL/Common/Ksend/`；现网对照 `Tools/ksend.py`（HotSend 绿之前保留）

Blocks 活 ★ 仍是板侧网络热更等；HotSend 文档先行，实现刀点名再开。

入门读：[`Docs/00-概述.md`](Docs/00-概述.md) → [`Docs/03-PR拆分.md`](Docs/03-PR拆分.md)。
