/* synth_info.cpp - definition of the Synth_info class, a storage class
 * holding synthesizer specific properties and values.
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

#ifndef MWSD_SYNTH_INFO_HPP
#define MWSD_SYNTH_INFO_HPP

#include <vector>
#include <string>
#include <unordered_map> // for collection of dump/request commands

/* Synth_info - a data storage class holding basic information about a synth
 * manufacturer ID, equipment ID, device ID (if supported),
 * display request command byte, display dump command byte,
 * a complete display request command SysEx string
 * width and height of the synth's display
*/

class Synth_info
{
	public:
		Synth_info() = delete;
		Synth_info(unsigned char man_id, unsigned char equip_id, \
			unsigned char dev_id, unsigned char disp_req_cmd, \
			unsigned char disp_dump_cmd, unsigned int disp_cols, \
			unsigned int disp_rows);
		~Synth_info() {}

			// Access methods
		unsigned char get_man_id() const { return its_man_id; }
		unsigned char get_equip_id() const { return its_equip_id; }
		unsigned char get_dev_id() const { return its_dev_id; }
		unsigned char get_disp_req_cmd() const { return its_disp_req_cmd; }
		unsigned char get_disp_dump_cmd() const { return its_disp_dump_cmd; }
		unsigned int get_disp_cols() const { return its_disp_cols; }
		unsigned int get_disp_rows() const { return its_disp_rows; }
		const std::vector<unsigned char>& get_disp_req() const { return its_disp_req; }
			// Return name of cmd or empty string
		std::string get_dump_name(unsigned char cmd);
		unsigned int get_dump_bank(unsigned char cmd);
		unsigned int get_dump_patch(unsigned char cmd);
		unsigned int get_dump_name_start(unsigned char cmd);
		unsigned int get_dump_name_chars(unsigned char cmd);
		std::vector<std::string> get_dump_names() const;
		void set_dev_id(unsigned char dev_id); // set dev_id and adapt disp_req vector
		void prepare_disp(std::vector<unsigned char>* syx_msg, \
			std::vector<std::string>* disp); // put formatted
			// display content into disp
	private:
		unsigned char its_man_id;
		unsigned char its_equip_id;
		unsigned char its_dev_id;
		unsigned char its_disp_req_cmd; // dispaly request command byte
		unsigned char its_disp_dump_cmd; // display dump command byte
		unsigned int its_disp_cols;
		unsigned int its_disp_rows;
		std::vector<unsigned char> its_disp_req; // full display request SysEx
		std::unordered_map<unsigned char,std::string> its_dump_cmds; // dump commands
			// bank number of dump, if it has one
		std::unordered_map<unsigned char,unsigned int> its_dump_bank;
			// Patch number of dump, if it has one
		std::unordered_map<unsigned char,unsigned int> its_dump_patch;
			// Begin of data name, if it has one
		std::unordered_map<unsigned char,unsigned int> its_dump_name_start;
			// Number of name characters, if appropriate
		std::unordered_map<unsigned char,unsigned int> its_dump_name_chars;
};

#endif // #ifndef SYNTH_INFO_HPP
