/*
 * Copyright (C) 2024-2026 David C. Manuelda (StormBytePP)
 *
 * This file is part of StormByte-Network.
 *
 * StormByte-Network is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License version 3
 * or later, as published by the Free Software Foundation.
 *
 * StormByte-Network is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with StormByte-Network. If not, see
 * <https://www.gnu.org/licenses/lgpl-3.0.html>.
 */

#pragma once

#include <StormByte/platform.h>

#ifdef WINDOWS
	#ifdef StormByte_Network_EXPORTS
		#define STORMBYTE_NETWORK_PUBLIC	__declspec(dllexport)
	#else
		#define STORMBYTE_NETWORK_PUBLIC	__declspec(dllimport)
	#endif
	#define STORMBYTE_NETWORK_PRIVATE
#else
	#define STORMBYTE_NETWORK_PUBLIC		__attribute__ ((visibility ("default")))
	#define STORMBYTE_NETWORK_PRIVATE		__attribute__ ((visibility ("hidden")))
#endif
