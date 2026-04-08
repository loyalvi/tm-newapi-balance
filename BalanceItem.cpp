#include "BalanceItem.h"
#include "Plugin.h"

CBalanceItem::CBalanceItem()
{
}

const wchar_t* CBalanceItem::GetItemName() const
{
    return L"NewAPI余额";
}

const wchar_t* CBalanceItem::GetItemId() const
{
    return L"NewApiBalance01";
}

const wchar_t* CBalanceItem::GetItemLableText() const
{
    return L"余额";
}

const wchar_t* CBalanceItem::GetItemValueText() const
{
    auto& plugin = CNewApiPlugin::Instance();
    std::lock_guard<std::mutex> lock(plugin.m_data_mutex);
    return plugin.m_balance_text.c_str();
}

const wchar_t* CBalanceItem::GetItemValueSampleText() const
{
    return L"$999.99";
}
