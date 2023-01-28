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

#include "irr_v3d.h"
#include "mapnode.h"
#include "modchannels.h"
#include "threading/ipc_channel.h"
#include "util/basic_macros.h"
#include "util/string.h"
#include <vector>
#if defined(_WIN32)
#include <windows.h>
#else
#include <unistd.h>
#endif

class Client;
class Inventory;
struct ItemDefinition;
struct ItemStack;
struct PointedThing;

class CSMController
{
public:
	CSMController(Client *client);
	~CSMController();

	DISABLE_CLASS_COPY(CSMController)

	bool start();
	void stop();

#if defined(_WIN32)
	bool isStarted() const { return m_script_handle != INVALID_HANDLE_VALUE; }
#else
	bool isStarted() const { return m_script_pid != 0; }
#endif

	void runLoadMods();
	void runShutdown();
	void runClientReady();
	void runCameraReady();
	void runMinimapReady();
	bool runSendingMessage(const std::string &message);
	bool runReceivingMessage(const std::string &message);
	bool runDamageTaken(u16 damage);
	bool runHPModification(u16 hp);
	void runDeath();
	void runModchannelMessage(const std::string &channel, const std::string &sender,
			const std::string &message);
	void runModchannelSignal(const std::string &channel, ModChannelSignal signal);
	bool runFormspecInput(const std::string &formname, const StringMap &fields);
	bool runInventoryOpen(const Inventory *inventory);
	bool runItemUse(const ItemStack &selected_item, const PointedThing &pointed);
	void runPlacenode(const PointedThing &pointed, const ItemDefinition &def);
	bool runPunchnode(v3s16 pos, MapNode n);
	bool runDignode(v3s16 pos, MapNode n);
	void runStep(float dtime);

private:
	template<typename T>
	bool exchange(const T &send) noexcept;

	template<typename T>
	bool deserializeMsg(T &recv) noexcept;

	void listen(bool succeeded);

	bool getDoneBool();

	Client *const m_client;
	int m_timeout = 1000;
	std::vector<char> m_send_buf;
#if defined(_WIN32)
	HANDLE m_script_handle = INVALID_HANDLE_VALUE;
	HANDLE m_ipc_shm;
	HANDLE m_ipc_sem_a;
	HANDLE m_ipc_sem_b;
#else
	pid_t m_script_pid = 0;
#endif
	IPCChannelShared *m_ipc_shared = nullptr;
	IPCChannelEnd m_ipc;
};