#pragma once
#include "PluginInterface.h"
#include <string>

class CBalanceItem : public IPluginItem
{
public:
    CBalanceItem();

    const wchar_t* GetItemName() const override;
    const wchar_t* GetItemId() const override;
    const wchar_t* GetItemLableText() const override;
    const wchar_t* GetItemValueText() const override;
    const wchar_t* GetItemValueSampleText() const override;
};
