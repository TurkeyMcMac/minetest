/*
Minetest
Copyright (C) 2022 TurkeyMcMac, Jude Melton-Houghton <jwmhjwmh@gmail.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation; either version 2.1 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#pragma once

#include "cpp_api/s_base.h"
#include "irrlichttypes.h"

class Inventory;

class ScriptApiCSM : public virtual ScriptApiBase
{
public:
	void on_mods_loaded();

	void on_shutdown();

	void on_client_ready();

	void on_camera_ready();

	void on_minimap_ready();

	bool on_sending_message(const std::string &message);

	bool on_receiving_message(const std::string &message);

	bool on_hp_modification(u16 hp);

	bool on_inventory_open(const Inventory *inventory);

	// Called on environment step
	void environment_Step(float dtime);
};
