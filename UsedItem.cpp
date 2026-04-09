#include "UsedItem.h"
#include "Plugin.h"

CUsedItem::CUsedItem()
{
}

const wchar_t* CUsedItem::GetItemName() const
{
    return L"NewAPI已用";
}

const wchar_t* CUsedItem::GetItemId() const
{
    return L"NewApiUsed01";
}

const wchar_t* CUsedItem::GetItemLableText() const
{
    return L"";
}

const wchar_t* CUsedItem::GetItemValueText() const
{
    auto& plugin = CNewApiPlugin::Instance();
    std::lock_guard<std::mutex> lock(plugin.m_data_mutex);
    return plugin.m_used_text.c_str();
}

const wchar_t* CUsedItem::GetItemValueSampleText() const
{
    return L"$999.99";
}
