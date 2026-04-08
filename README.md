# tm-newapi-balance

[TrafficMonitor](https://github.com/zhongyang219/TrafficMonitor) 插件，用于在任务栏和主窗口中实时显示 [New API](https://github.com/QuantumNous/new-api) 账户余额。

---

## 功能特性

- 在 TrafficMonitor 任务栏/主窗口中显示 New API 账户余额（美元）
- 鼠标悬停时显示详细信息：用户名、余额、已用额度
- 可配置轮询间隔，避免频繁请求
- 支持 HTTP / HTTPS，使用 WinHTTP 原生 Windows API，无第三方依赖
- 通过 INI 配置文件灵活配置，无需重新编译

---

## 效果预览

任务栏显示示例：

```
余额  $12.34
```

鼠标悬停提示示例：

```
NewAPI 余额
用户: admin
余额: $12.34
已用: $5.66
```

---

## 项目结构

```
tm-newapi-balance/
├── PluginInterface.h       # TrafficMonitor 插件接口头文件（来自官方）
├── Plugin.h / Plugin.cpp   # 插件主类 CNewApiPlugin（继承 ITMPlugin）
├── BalanceItem.h / .cpp    # 显示项目类 CBalanceItem（继承 IPluginItem）
├── dllmain.cpp             # DLL 入口，导出 TMPluginGetInstance
├── CMakeLists.txt          # CMake 构建脚本
├── tm-newapi-balance.ini   # 配置文件模板
└── docs/
    └── newapi.md           # New API 相关文档
```

---

## 构建

### 前置要求

- Windows 10 / 11
- Visual Studio 2019+ 或 CMake 3.15+ + MSVC 工具链
- C++17

### 使用 CMake 构建

```bash
mkdir build && cd build
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release
```

生成的 DLL 位于 `build/Release/tm-newapi-balance.dll`（或 `build/bin/`，取决于 CMake 配置）。

### 使用 Visual Studio 构建

1. 用 Visual Studio 打开文件夹或导入 CMakeLists.txt
2. 选择 `Release / x64` 配置
3. 生成解决方案

---

## 安装

1. 找到 TrafficMonitor 主程序所在目录，确认其中有 `plugins` 子目录（若不存在请手动创建）。
2. 将编译好的 `tm-newapi-balance.dll` 复制到 `plugins` 目录中。
3. 将配置文件 `tm-newapi-balance.ini`（见下方说明）复制到 `plugins` 目录中。
4. 启动（或重启）TrafficMonitor，插件将自动加载。
5. 在 TrafficMonitor 右键菜单中选择 **其他功能 → 插件管理**，可查看插件是否已加载成功。

---

## 配置

配置文件路径：`<TrafficMonitor安装目录>/plugins/tm-newapi-balance.ini`

```ini
[General]
; New API 服务地址（不要以 / 结尾）
BaseUrl=https://your-newapi-domain.com

; 系统访问令牌
; 从「个人设置 → 账户管理 → 安全设置 → 系统访问令牌」获取
AccessToken=your-access-token-here

; 用户 ID（管理员默认为 1）
UserID=1

; 轮询间隔（秒），最小 5 秒
PollInterval=60
```

### 配置项说明

| 配置项 | 说明 | 默认值 |
|--------|------|--------|
| `BaseUrl` | New API 服务地址，不含末尾斜杠 | 无（必填） |
| `AccessToken` | 系统访问令牌，用于调用 `/api/user/self` 接口 | 无（必填） |
| `UserID` | 要查询的用户 ID，管理员默认为 1 | `1` |
| `PollInterval` | 余额刷新间隔（秒），最小值为 5 秒 | `60` |
| `DebugLog` | 设为 `1` 时将每次 HTTP 原始响应追加写入 `plugins/tm-newapi-balance.log`，排查完毕后改回 `0` | `0` |

> **注意**：`AccessToken` 请妥善保管，不要提交到公开代码仓库。

---

## 接口说明

本插件遵循 [TrafficMonitor 插件开发规范](https://github.com/zhongyang219/TrafficMonitor/wiki/%E6%8F%92%E4%BB%B6%E5%BC%80%E5%8F%91%E6%8C%87%E5%8D%97)，实现了以下接口：

### `CNewApiPlugin`（继承自 `ITMPlugin`）

| 方法 | 说明 |
|------|------|
| `GetItem(int index)` | 返回 `CBalanceItem` 显示项目对象 |
| `DataRequired()` | 按 `PollInterval` 间隔调用 New API 获取余额数据 |
| `GetInfo(PluginInfoIndex)` | 返回插件名称、描述、版本等元信息 |
| `OnExtenedInfo(...)` | 接收配置目录路径，触发加载 INI 配置文件 |
| `OnInitialize(...)` | 接收主程序接口指针 |
| `GetTooltipInfo()` | 返回鼠标悬停提示文本 |

### `CBalanceItem`（继承自 `IPluginItem`）

| 方法 | 返回值 |
|------|--------|
| `GetItemName()` | `"NewAPI余额"` |
| `GetItemId()` | `"NewApiBalance01"` |
| `GetItemLableText()` | `"余额"` |
| `GetItemValueText()` | 当前余额，如 `"$12.34"` |
| `GetItemValueSampleText()` | `"$999.99"` |

---

## API 接口

本插件请求以下 New API 接口：

```
GET {BaseUrl}/api/user/self
Authorization: {AccessToken}
```

响应示例（简化）：

```json
{
  "success": true,
  "data": {
    "username": "admin",
    "display_name": "管理员",
    "quota": 6170000,
    "used_quota": 2830000
  }
}
```

余额换算公式：`余额($) = quota / 500000`

---

## 常见问题

**Q: 任务栏显示"未配置"**  
A: 请检查 INI 文件中 `BaseUrl` 和 `AccessToken` 是否已正确填写。

**Q: 任务栏显示"请求失败"**  
A: 请检查：① `BaseUrl` 地址是否可访问；② `AccessToken` 是否有效；③ 网络是否正常；④ HTTP 状态码是否为 2xx（可开启 `DebugLog=1` 查看原始响应）。

**Q: 任务栏显示"认证失败"**  
A: API 返回了 `"success":false`，通常是 `AccessToken` 无效或权限不足，请重新从 New API「个人设置 → 账户管理 → 安全设置 → 系统访问令牌」获取令牌。

**Q: 余额长时间不刷新**  
A: 默认 60 秒轮询一次。可在 INI 中调小 `PollInterval`，但最小值为 5 秒。

**Q: 插件未在插件管理中出现**  
A: 请确认 DLL 已复制到 `plugins` 目录，且 TrafficMonitor 版本 ≥ 1.82（支持插件系统的最低版本）。

---

## 诊断与测试

### 方法一：启用调试日志

1. 编辑 `plugins/tm-newapi-balance.ini`，将 `DebugLog` 设为 `1`：
   ```ini
   DebugLog=1
   ```
2. 重启 TrafficMonitor，等待首次轮询（约数秒）。
3. 查看 `plugins/tm-newapi-balance.log`，其中记录了每次请求的原始 HTTP 响应体：
   - 若日志中出现 `"success":false`，说明令牌无效或权限不足。
   - 若日志为空，说明请求本身没有发出（检查 `BaseUrl` 格式）。
   - 确认 `"quota"` 字段值是否为预期值。
4. 排查完毕后将 `DebugLog` 改回 `0`。

### 方法二：用 PowerShell 手动验证接口

在 Windows PowerShell 中直接调用 New API 接口，确认令牌和地址是否正确：

```powershell
$baseUrl     = "https://your-newapi-domain.com"
$accessToken = "your-access-token-here"

$response = Invoke-RestMethod `
    -Uri "$baseUrl/api/user/self" `
    -Headers @{ Authorization = $accessToken } `
    -Method GET

$response | ConvertTo-Json -Depth 5
```

预期输出（成功时）：
```json
{
  "success": true,
  "data": {
    "username": "admin",
    "quota": 6170000,
    "used_quota": 2830000
  }
}
```

若 `success` 为 `false`，说明令牌不正确；若命令报错，说明 `BaseUrl` 不可达。

---

## 相关链接

- [TrafficMonitor](https://github.com/zhongyang219/TrafficMonitor) - 网络监控悬浮窗工具
- [TrafficMonitor 插件开发指南](https://github.com/zhongyang219/TrafficMonitor/wiki/%E6%8F%92%E4%BB%B6%E5%BC%80%E5%8F%91%E6%8C%87%E5%8D%97)
- [New API](https://github.com/QuantumNous/new-api) - OpenAI 接口管理平台

---

## License

MIT
