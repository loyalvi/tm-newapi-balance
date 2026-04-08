#include "Plugin.h"
#include <Windows.h>
#include <winhttp.h>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cstdio>

#pragma comment(lib, "winhttp.lib")

CNewApiPlugin CNewApiPlugin::m_instance;

CNewApiPlugin::CNewApiPlugin()
{
    m_balance_text = L"--";
}

CNewApiPlugin& CNewApiPlugin::Instance()
{
    return m_instance;
}

IPluginItem* CNewApiPlugin::GetItem(int index)
{
    if (index == 0)
        return &m_balance_item;
    return nullptr;
}

void CNewApiPlugin::DataRequired()
{
    DWORD now = GetTickCount();
    DWORD interval_ms = static_cast<DWORD>(m_config.poll_interval_sec) * 1000;

    // 首次调用或到达轮询间隔时才发起 HTTP 请求
    if (m_last_fetch_tick == 0 || (now - m_last_fetch_tick) >= interval_ms)
    {
        m_last_fetch_tick = now;
        FetchBalance();
    }
}

void CNewApiPlugin::FetchBalance()
{
    if (m_config.base_url.empty() || m_config.access_token.empty())
    {
        std::lock_guard<std::mutex> lock(m_data_mutex);
        m_balance_text = L"未配置";
        return;
    }

    std::wstring url = m_config.base_url + L"/api/user/self";
    std::string response = HttpGet(url, m_config.access_token);

    if (response.empty())
    {
        std::lock_guard<std::mutex> lock(m_data_mutex);
        m_balance_text = L"请求失败";
        return;
    }

    if (m_config.debug_log)
        WriteDebugLog(response);

    // 检查 API 是否返回 success:true
    if (!ExtractJsonBool(response, "success"))
    {
        std::lock_guard<std::mutex> lock(m_data_mutex);
        m_balance_text = L"认证失败";
        return;
    }

    // 解析 JSON 中的 quota、used_quota、username、display_name
    // New API 返回: {"success":true,"message":"","data":{...,"quota":12345,...}}
    // 先尝试从 "data" 对象中获取
    double quota = ExtractJsonNumber(response, "quota");
    double used_quota = ExtractJsonNumber(response, "used_quota");
    std::wstring username = ExtractJsonString(response, "username");
    std::wstring display_name = ExtractJsonString(response, "display_name");

    double balance = quota / 500000.0;
    double used = used_quota / 500000.0;

    wchar_t buf[64];
    swprintf_s(buf, L"$%.2f", balance);

    wchar_t tip[256];
    swprintf_s(tip, L"NewAPI 余额\n用户: %s\n余额: $%.2f\n已用: $%.2f",
        display_name.empty() ? username.c_str() : display_name.c_str(),
        balance, used);

    std::lock_guard<std::mutex> lock(m_data_mutex);
    m_balance_text = buf;
    m_tooltip_text = tip;
}

const wchar_t* CNewApiPlugin::GetInfo(PluginInfoIndex index)
{
    switch (index)
    {
    case TMI_NAME:
        return L"tm-newapi-balance";
    case TMI_DESCRIPTION:
        return L"显示 New API 账户余额";
    case TMI_AUTHOR:
        return L"TrafficMonitor Plugin";
    case TMI_COPYRIGHT:
        return L"Copyright (C) 2024";
    case TMI_VERSION:
        return L"1.0";
    case TMI_URL:
        return L"https://github.com/QuantumNous/new-api";
    default:
        break;
    }
    return L"";
}

void CNewApiPlugin::OnExtenedInfo(ExtendedInfoIndex index, const wchar_t* data)
{
    if (index == EI_CONFIG_DIR && data)
    {
        m_config_dir = data;
        LoadConfig();
    }
}

void CNewApiPlugin::OnInitialize(ITrafficMonitor* pApp)
{
    m_app = pApp;
}

const wchar_t* CNewApiPlugin::GetTooltipInfo()
{
    std::lock_guard<std::mutex> lock(m_data_mutex);
    m_tooltip_buf = m_tooltip_text;
    return m_tooltip_buf.c_str();
}

// ============================================================
// 加载配置文件 (INI 格式)
// ============================================================
void CNewApiPlugin::LoadConfig()
{
    std::wstring config_path = m_config_dir + L"tm-newapi-balance.ini";

    wchar_t buf[1024];

    GetPrivateProfileStringW(L"General", L"BaseUrl", L"", buf, _countof(buf), config_path.c_str());
    m_config.base_url = buf;
    // 去除末尾的 '/'
    while (!m_config.base_url.empty() && m_config.base_url.back() == L'/')
        m_config.base_url.pop_back();

    GetPrivateProfileStringW(L"General", L"AccessToken", L"", buf, _countof(buf), config_path.c_str());
    m_config.access_token = buf;

    m_config.user_id = GetPrivateProfileIntW(L"General", L"UserID", 1, config_path.c_str());
    m_config.poll_interval_sec = GetPrivateProfileIntW(L"General", L"PollInterval", 60, config_path.c_str());
    m_config.debug_log = GetPrivateProfileIntW(L"General", L"DebugLog", 0, config_path.c_str()) != 0;

    if (m_config.poll_interval_sec < 5)
        m_config.poll_interval_sec = 5;
}

// ============================================================
// WinHTTP GET 请求
// ============================================================
std::string CNewApiPlugin::HttpGet(const std::wstring& url, const std::wstring& token)
{
    std::string result;

    // 解析 URL
    URL_COMPONENTS urlComp{};
    urlComp.dwStructSize = sizeof(urlComp);

    wchar_t hostName[256]{};
    wchar_t urlPath[2048]{};
    urlComp.lpszHostName = hostName;
    urlComp.dwHostNameLength = _countof(hostName);
    urlComp.lpszUrlPath = urlPath;
    urlComp.dwUrlPathLength = _countof(urlPath);

    if (!WinHttpCrackUrl(url.c_str(), 0, 0, &urlComp))
        return result;

    HINTERNET hSession = WinHttpOpen(L"TrafficMonitor-NewAPI/1.0",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS,
        0);
    if (!hSession)
        return result;

    // 设置超时: 连接5秒, 收发10秒
    WinHttpSetTimeouts(hSession, 5000, 5000, 10000, 10000);

    HINTERNET hConnect = WinHttpConnect(hSession, hostName, urlComp.nPort, 0);
    if (!hConnect)
    {
        WinHttpCloseHandle(hSession);
        return result;
    }

    DWORD flags = (urlComp.nScheme == INTERNET_SCHEME_HTTPS) ? WINHTTP_FLAG_SECURE : 0;
    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"GET", urlPath,
        NULL, WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES,
        flags);
    if (!hRequest)
    {
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return result;
    }

    // 添加 Authorization 头
    std::wstring authHeader = L"Authorization: " + token;
    WinHttpAddRequestHeaders(hRequest, authHeader.c_str(), (DWORD)-1, WINHTTP_ADDREQ_FLAG_ADD);

    // 添加 New-Api-User 头（系统令牌调用 /api/user/self 时必须提供）
    std::wstring userIdHeader = L"New-Api-User: " + std::to_wstring(m_config.user_id);
    WinHttpAddRequestHeaders(hRequest, userIdHeader.c_str(), (DWORD)-1, WINHTTP_ADDREQ_FLAG_ADD);

    if (WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
        WINHTTP_NO_REQUEST_DATA, 0, 0, 0) &&
        WinHttpReceiveResponse(hRequest, NULL))
    {
        // 检查 HTTP 状态码，非 2xx 视为失败
        DWORD statusCode = 0;
        DWORD statusCodeSize = sizeof(statusCode);
        WinHttpQueryHeaders(hRequest,
            WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
            WINHTTP_HEADER_NAME_BY_INDEX,
            &statusCode, &statusCodeSize, WINHTTP_NO_HEADER_INDEX);

        if (statusCode >= 200 && statusCode < 300)
        {
            DWORD bytesAvailable = 0;
            while (WinHttpQueryDataAvailable(hRequest, &bytesAvailable) && bytesAvailable > 0)
            {
                std::string buf(bytesAvailable, '\0');
                DWORD bytesRead = 0;
                if (WinHttpReadData(hRequest, &buf[0], bytesAvailable, &bytesRead))
                {
                    buf.resize(bytesRead);
                    result += buf;
                }
            }
        }
    }

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);

    return result;
}

// ============================================================
// 简易 JSON 解析
// ============================================================
double CNewApiPlugin::ExtractJsonNumber(const std::string& json, const std::string& key)
{
    // 查找 "key": number 或 "key":number
    std::string pattern = "\"" + key + "\"";
    size_t pos = json.find(pattern);
    if (pos == std::string::npos)
        return 0.0;

    pos += pattern.length();

    // 跳过空白和冒号
    while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\t' || json[pos] == ':'))
        pos++;

    if (pos >= json.size())
        return 0.0;

    // 读取数字
    std::string numStr;
    while (pos < json.size() && (json[pos] == '-' || json[pos] == '.' || (json[pos] >= '0' && json[pos] <= '9')))
    {
        numStr += json[pos];
        pos++;
    }

    if (numStr.empty())
        return 0.0;

    return std::stod(numStr);
}

std::wstring CNewApiPlugin::ExtractJsonString(const std::string& json, const std::string& key)
{
    std::string pattern = "\"" + key + "\"";
    size_t pos = json.find(pattern);
    if (pos == std::string::npos)
        return L"";

    pos += pattern.length();

    // 跳过空白和冒号
    while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\t' || json[pos] == ':'))
        pos++;

    if (pos >= json.size() || json[pos] != '"')
        return L"";

    pos++; // 跳过开头引号
    std::string value;
    while (pos < json.size() && json[pos] != '"')
    {
        if (json[pos] == '\\' && pos + 1 < json.size())
        {
            pos++;
            value += json[pos];
        }
        else
        {
            value += json[pos];
        }
        pos++;
    }

    // UTF-8 转 wstring
    if (value.empty())
        return L"";

    int wlen = MultiByteToWideChar(CP_UTF8, 0, value.c_str(), (int)value.size(), NULL, 0);
    if (wlen <= 0)
        return L"";

    std::wstring wstr(wlen, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, value.c_str(), (int)value.size(), &wstr[0], wlen);
    return wstr;
}

// ============================================================
// 检查布尔字段（如 "success":true）
// ============================================================
bool CNewApiPlugin::ExtractJsonBool(const std::string& json, const std::string& key)
{
    std::string pattern = "\"" + key + "\"";
    size_t pos = json.find(pattern);
    if (pos == std::string::npos)
        return false;

    pos += pattern.length();

    // 跳过空白和冒号
    while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\t' || json[pos] == ':'))
        pos++;

    // 匹配 true
    return json.compare(pos, 4, "true") == 0;
}

// ============================================================
// 调试日志：将原始 HTTP 响应写入 plugins 目录下的 .log 文件
// ============================================================
void CNewApiPlugin::WriteDebugLog(const std::string& content)
{
    if (m_config_dir.empty())
        return;

    std::wstring log_path = m_config_dir + L"tm-newapi-balance.log";
    std::ofstream ofs(log_path, std::ios::app);
    if (!ofs.is_open())
        return;

    // 写入时间戳（tick 计数，足够用于区分请求）
    ofs << "[tick=" << GetTickCount() << "]\n" << content << "\n\n";
}
