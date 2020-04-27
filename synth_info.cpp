/* synth_info.cpp - implementation of the Synth_info class, a storage class,
 * which holds synthesizer specific properties and values, in preparation
 * for further synthesizers being added to this tool.
 * Copyright (C) 2018-2020 Jeanette C. <jeanette@juliencoder.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include "synth_info.hpp"

using std::vector;
using std::string;
using std::unordered_map;
using std::out_of_range;

Synth_info::Synth_info(unsigned char man_id, unsigned char equip_id, \
	unsigned char dev_id, unsigned char disp_req_cmd, \
	unsigned char disp_dump_cmd, \
	unsigned int disp_cols, unsigned int disp_rows):
		its_man_id(man_id), its_equip_id(equip_id), its_dev_id(dev_id), \
		its_disp_req_cmd(disp_req_cmd), its_disp_dump_cmd(disp_dump_cmd), \
		its_disp_cols(disp_cols), its_disp_rows(disp_rows)
{
	its_disp_req.reserve(7);
	its_disp_req.push_back(0xf0);
	its_disp_req.push_back(its_man_id);
	its_disp_req.push_back(its_equip_id);
	its_disp_req.push_back(its_dev_id);
	its_disp_req.push_back(its_disp_req_cmd);
	its_disp_req.push_back(0x00);
	its_disp_req.push_back(0xf7);
	
		// Set up list/map of all dump commands and additional data
	its_dump_cmds.reserve(10);
	its_dump_bank.reserve(10);
	its_dump_patch.reserve(10);
	its_dump_name_start.reserve(2);
	its_dump_name_chars.reserve(2);
	its_dump_cmds.emplace(0x10,string("sound"));
	its_dump_bank.emplace(0x10,5);
	its_dump_patch.emplace(0x10,6);
	its_dump_name_start.emplace(0x10,247);
	its_dump_name_chars.emplace(0x10,16);
	its_dump_cmds.emplace(0x11,string("multi"));
	its_dump_bank.emplace(0x11,5);
	its_dump_patch.emplace(0x11,6);
	its_dump_name_start.emplace(0x11,23);
	its_dump_name_chars.emplace(0x11,16);
	its_dump_cmds.emplace(0x12,string("wave"));
	its_dump_bank.emplace(0x12,5);
	its_dump_patch.emplace(0x12,6);
	its_dump_cmds.emplace(0x13,string("wave control table"));
	its_dump_bank.emplace(0x13,5);
	its_dump_patch.emplace(0x13,6);
	its_dump_cmds.emplace(0x14,string("global parameter"));
	its_dump_cmds.emplace(0x15,string("display"));
	its_dump_cmds.emplace(0x26,string("remote"));
	its_dump_cmds.emplace(0x17,string("mode"));
}

void Synth_info::set_dev_id(unsigned char dev_id)
{
	its_dev_id = dev_id;
	its_disp_req[3] = dev_id;
}

// Format a display dump into a string vector
void Synth_info::prepare_disp(vector<unsigned char>* syx_msg, vector<string>* disp)
{
	// JBS: in future properly parse and prepare the display
	if (syx_msg->at(4) == its_disp_dump_cmd)
	{
		string tmp_msg;
		tmp_msg.reserve(syx_msg->size());
		for (unsigned long int i = 5;i<(syx_msg->size() - 1);i++)
		{
			if (syx_msg->at(i) >= 32)
			{
				tmp_msg.push_back(static_cast<char>(syx_msg->at(i)));
			}
		}
		disp->clear();
		for (unsigned long int i = 0;i<its_disp_rows;i++)
		{
			disp->push_back(tmp_msg.substr(40*i,40));
		}
	}
}

string Synth_info::get_dump_name(unsigned char cmd)
{
	string cmd_name("");
	try
	{
		cmd_name = its_dump_cmds.at(cmd);
	}
	catch (out_of_range& e)
	{
	}
	return cmd_name;
}

unsigned int Synth_info::get_dump_bank(unsigned char cmd)
{
	unsigned int bank_no = 0;
	try
	{
		bank_no = its_dump_bank.at(cmd);
	}
	catch(out_of_range& e)
	{
	}
	return bank_no;
}

unsigned int Synth_info::get_dump_patch(unsigned char cmd)
{
	unsigned int patch_no = 0;
	try
	{
		patch_no = its_dump_patch.at(cmd);
	}
	catch (out_of_range& e)
	{
	}
	return patch_no;
}

unsigned int Synth_info::get_dump_name_start(unsigned char cmd)
{
	unsigned int name_start = 0;
	try
	{
		name_start = its_dump_name_start.at(cmd);
	}
	catch (out_of_range& e)
	{
	}
	return name_start;
}

unsigned int Synth_info::get_dump_name_chars(unsigned char cmd)
{
	unsigned int name_chars;
	try
	{
		name_chars = its_dump_name_chars.at(cmd);
	}
	catch (out_of_range& e)
	{
	}
	return name_chars;
}

vector<string> Synth_info::get_dump_names() const
{
	vector<string> the_names;
	the_names.reserve(its_dump_cmds.size());
	for (auto cmd_name: its_dump_cmds)
	{
		the_names.push_back(cmd_name.second);
	}
	return the_names;
}
