/* synth_info.cpp - implementation of the Synth_info class, a storage class,
 * which holds synthesizer specific properties and values, in preparation
 * for further synthesizers being added to this tool.
 * Copyright (C) 2018 Jeanette C. <jeanette@juliencoder.de>
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
		for (int i = 5;i<(syx_msg->size() - 1);i++)
		{
			if (syx_msg->at(i) >= 32)
			{
				tmp_msg.push_back(syx_msg->at(i));
			}
		}
		disp->clear();
		for (int i = 0;i<its_disp_rows;i++)
		{
			disp->push_back(tmp_msg.substr(40*i,40));
		}
	}
}
