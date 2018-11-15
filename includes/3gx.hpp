#pragma once
#include "types.hpp"

#define _3GX_MAGIC (0x3130303024584733) /* "3GX$0001" */

struct _3gx_Infos
{
    u32             authorLen{0};
    u32             authorMsg{0};
    u32             titleLen{0};
    u32             titleMsg{0};
    u32             summaryLen{0};
    u32             summaryMsg{0};
    u32             descriptionLen{0};
    u32             descriptionMsg{0};
} PACKED;

struct _3gx_Targets
{
    u32             count{0};
    u32             titles{0};
} PACKED;

struct _3gx_Header
{
    u64             magic{_3GX_MAGIC};
    u32             version{0};
    u32             codeSize{0};
    u32             code{0};
    _3gx_Infos      infos{};
    _3gx_Targets    targets{};
} PACKED;
