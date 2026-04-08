#pragma once
#include "PluginInterface.h"
#include "BalanceItem.h"
#include <string>
#include <mutex>
#include <atomic>

class CNewApiPlugin : public ITMPlugin
{
private:
    CNewApiPlugin();

public:
    static CNewApiPlugin& Instance();

    // ITMPlugin
    IPluginItem* GetItem(int index) override;
    void DataRequired() override;
    const wchar_t* GetInfo(PluginInfoIndex index) override;
    void OnExtenedInfo(ExtendedInfoIndex index, const wchar_t* data) override;
    void OnInitialize(ITrafficMonitor* pApp) override;
    const wchar_t* GetTooltipInfo() override;

    // 配置
    struct Config
    {
        std::wstring base_url;
        std::wstring access_token;
        int user_id = 1;
        int poll_interval_sec = 60;  // HTTP 请求间隔（秒）
    };

    const Config& GetConfig() const { return m_config; }

    // 数据
    std::wstring m_balance_text;     // 显示用的余额文本
    std::wstring m_tooltip_text;     // 鼠标提示文本
    std::mutex m_data_mutex;

private:
    void LoadConfig();
    void FetchBalance();
    std::string HttpGet(const std::wstring& url, const std::wstring& token);
    std::wstring ExtractJsonString(const std::string& json, const std::string& key);
    double ExtractJsonNumber(const std::string& json, const std::string& key);

    CBalanceItem m_balance_item;
    Config m_config;
    std::wstring m_config_dir;
    ITrafficMonitor* m_app = nullptr;

    DWORD m_last_fetch_tick = 0;
    std::wstring m_tooltip_buf;

    static CNewApiPlugin m_instance;
};
