/*
 Copyright (C) Mellanox Technologies, Ltd. 2001-2020. ALL RIGHTS RESERVED.

 This software product is a proprietary product of Mellanox Technologies, Ltd.
 (the "Company") and all right, title, and interest in and to the software
 product, including all associated intellectual property rights, are and shall
 remain exclusively with the Company.  All rights in or to the software product
 are licensed, not sold.  All rights not licensed are reserved.

 This software product is governed by the End User License Agreement provided
 with the software product.
*/
#pragma once
// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//
#include <string>
#include <errno.h>
#include <windows.h>
#include <winnt.h>

#pragma warning(push)
#pragma warning(disable : 4200)
#include <mlx5dv_win.h>
#pragma warning(pop)

#include "dcmd/dcmd.h"
#include "log.h"
