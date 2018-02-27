/* curses_mw_miner.cpp - implementation of the class
 * Curses_mw_miner the data miner class that actually talks to the synthesizer
 * and interpretes its MIDI/SysEx responses
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

#include <thread>
#include <chrono>
#include <fstream>
#include <iterator>
#include <boost/date_time.hpp>
#include "curses_mw_miner.hpp"

using std::string;
using std::to_string;
using std::vector;
using std::iterator;
using boost::posix_time::ptime;

// Constructor: initialise flags, set values from params and create window
Curses_mw_miner::Curses_mw_miner(RtMidiOut *midi_out, Synth_info *synth_info)
{
	its_thru_flag.store(true);
	its_disp_flag.store(false);
	its_quit_flag.store(false);
	its_new_flag.store(false);
	its_error_flag.store(false);
	its_paused.store(false);
	its_unanswered.store(0);
	its_x = 2;
	its_y = 3;
	its_old_midi_msg.reserve(16);
	its_old_disp_msg.reserve(88);
	its_midi_out = midi_out;
	its_synth_info = synth_info;
	its_disp.reserve(synth_info->get_disp_rows());
}

// Destructor: delete window and clear vectors
Curses_mw_miner::~Curses_mw_miner()
{
	its_disp.clear();
	its_old_disp_msg.clear();
	its_old_midi_msg.clear();
	its_midi_out = nullptr;
	its_synth_info = nullptr;
}

// set up window
void Curses_mw_miner::init_win()
{
	window = newwin(5,80,20,0);
	box(window,0,0);
	wmove(window,its_y,its_x);
	wrefresh(window);
}

void Curses_mw_miner::shut_win()
{
	wclear(window);
	wrefresh(window);
	delwin(window);
}

void Curses_mw_miner::set_thru(bool thru_flag)
{
	its_thru_flag.store(thru_flag);
	if (thru_flag == true)
	{
		wmove(window,1,1);
		wclrtoeol(window);
		wmove(window,2,1);
		wclrtoeol(window);
		box(window,0,0);
		its_y = 3;
	}
	else
	{
		its_y = 2;
	}
	its_x = 2;
	wmove(window,its_y,its_x);
	wrefresh(window);
}

void Curses_mw_miner::set_quit(bool quit_flag)
{
	its_quit_flag.store(quit_flag);
}

void Curses_mw_miner::set_disp(bool disp_flag)
{
	its_disp_flag.store(disp_flag);
	if ((disp_flag == true) || (its_thru_flag == false))
	{
		its_y = 2;
	}
	else
	{
		its_y = 3;
	}
	its_x = 2;
	wmove(window,its_y,its_x);
	wrefresh(window);
}

// Set paused, to pause all active MIDI sending
void Curses_mw_miner::set_paused(bool paused)
{
	if (its_paused != paused)
	{
		its_paused.store(paused);
		its_unanswered = 0;
	}
}

// The main loop for the mw_miner thread
void Curses_mw_miner::run()
{
	std::chrono::milliseconds sleep_time(100);
	vector<unsigned char> my_disp_req = its_synth_info->get_disp_req();
	init_win();
	while (its_quit_flag == false)
	{
		if (its_paused == false)
		{
			if (its_unanswered >10)
			{
				its_error_flag = true;
				its_error_msg = string("More than 10 unanswered requests from synthesizer.");
				set_quit(true);
			}
			else
			{
				if (its_disp_flag == true)
				{
					try
					{
						its_midi_out->sendMessage(&my_disp_req);
					}
					catch (RtMidiError& e)
					{
						its_error_flag.store(true);
						its_quit_flag.store(true);
						its_error_msg = e.getMessage();
					}
					its_unanswered++;
					std::this_thread::sleep_for(sleep_time);
				}
				else
				{
					if ((its_thru_flag == false) && (its_new_flag == true))
					{
						try
						{
							its_midi_out->sendMessage(&my_disp_req);
						}
						catch (RtMidiError& e)
						{
							its_error_flag.store(true);
							its_quit_flag.store(true);
							its_error_msg = e.getMessage();
						}
						its_unanswered++;
						std::this_thread::sleep_for(sleep_time);
					}
				}
			}
		}
	}
	shut_win();
}

// Callback function for the RtMidiIn port
void Curses_mw_miner::accept_msg(double delta_time, vector<unsigned char> *message)
{
	if (its_paused == false)
	{
		unsigned char cmd_byte; // command byte of the SysEx string
		bool same = true; // used to compare vectors by element
		int comp_size = 0; // how many vector elements to compare
		if (its_disp_flag == true)
		{
			if (message->size() >=5)
			{
				cmd_byte = message->at(4);
				if (its_synth_info->get_disp_dump_cmd() == cmd_byte)
				{
					if (its_unanswered >0)
					{
						its_unanswered = 0;
					}

					// Compare message to its_old_disp_msg
					comp_size = message->size();
					if (comp_size == its_old_disp_msg.size())
					{
						for (int i = 0;i<comp_size;i++)
						{
							if (message->at(i) != its_old_disp_msg[i])
							{
								same = false;
								break;
							}
						}
					}
					else // The new and old display dumps have different size, so not euqal
					{
						same = false;
					}
					if (same == false)
					{
						// save new display dump to its_old_disp_msg
						its_old_disp_msg.clear();
						for (auto byte: *message)
						{
							its_old_disp_msg.push_back(byte);
						}
						its_synth_info->prepare_disp(message,&its_disp);
						print_msg();
					}
				}
			}
		}
		else // disp_flag not true
		{
			if (message->size() >=5)
			{
				cmd_byte = message->at(4);
			}
			else
			{
				cmd_byte = 0;
			}
			if (cmd_byte != its_synth_info->get_disp_dump_cmd()) // no display dump
			{
				// Compare message to its_old_midi_msg
				same = true;
				comp_size = message->size();
				if (comp_size == its_old_midi_msg.size()) // different size, different msg
				{
					for (int i = 0;i<comp_size;i++)
					{
						if (message->at(i) != its_old_midi_msg[i])
						{
							same = false;
							break;
						}
					}
				}
				else // different size here, so not equal per se
				{
					same = false;
				}
				if (same == false)
				{
					// save message to its_old_midi_msg
					its_old_midi_msg.clear();
					for (auto byte: *message)
					{
						its_old_midi_msg.push_back(byte);
					}
					if (its_thru_flag == true)
					{
						print_msg();
					}
					else // Not in direct data mode, display on demand
					{
						its_new_flag = true;
					}
				}
			}
			else // message is a display dump
			{
				if (its_unanswered >0)
				{
					its_unanswered = 0;
				}
				if (its_thru_flag == false)
				{
					its_new_flag = false;
					// Compare message to its_old_disp_msg
					same = true;
					comp_size = message->size();
					if (comp_size == its_old_disp_msg.size())
					{
						for (int i = 0;i<comp_size;i++)
						{
							if (message->at(i) != its_old_disp_msg[i])
							{
								same = false;
								break;
							}
						}
					}
					else // different size, so they're not equal
					{
						same = false;
					}
					if (same == false)
					{
						// save message to its_old_disp_msg
						its_old_disp_msg.clear();
						for (auto byte: *message)
						{
							its_old_disp_msg.push_back(byte);
						}
						its_synth_info->prepare_disp(message,&its_disp);
						print_msg();
					}
				}
			}
		}
	}
	else // its_paused is true
	{
		// JBS only accept identity response
		;
	}
}

void Curses_mw_miner::print_disp()
{
	int i = 0; // line index
	for (auto line: its_disp)
	{
		wmove(window,(1+i),1);
		wclrtoeol(window);
		mvwprintw(window,(1+i),2,"%s",line.c_str());
		i++;
	}
	wmove(window,its_y,its_x);
	box(window,0,0);
	wrefresh(window);
}

void Curses_mw_miner::print_thru()
{
	// clear line and repaint box
	wmove(window,3,1);
	wclrtoeol(window);
	box(window,0,0);
	unsigned char cmd_byte;
	if (its_old_midi_msg.size() >= 5)
	{
		cmd_byte = its_old_midi_msg[4];
	}
	else
	{
		cmd_byte = 255;
	}
	if (its_old_midi_msg[0] == 0xf0) // it's SysEx
	{
		// Examine SysEx for type and gracefully handle long dumps
			  string cmd_name = its_synth_info->get_dump_name(cmd_byte);
		if (!cmd_name.empty())
		{
			mvwprintw(window,3,2,"%s dump",cmd_name.c_str());
		}
		else
		{
			int i = 0; // column to print the byte
			for (auto byte: its_old_midi_msg)
			{
				mvwprintw(window,3,(2 + (3*i)),"%02x",byte);
				i++;
			}
		}
	}
	else // The message is normal MIDI
	{
		switch(its_old_midi_msg[0])
		{
			case 176:
			{
				mvwprintw(window,3,2,"Controller %d: %d",its_old_midi_msg[1],its_old_midi_msg[2]);
				break;
			}
			case 192:
			{
				mvwprintw(window,3,2,"Program change: %d %d",its_old_midi_msg[1],its_old_midi_msg[2]);
				break;
			}
			default:
			{
				break;
			}
		}
	}
	wmove(window,its_y,its_x);
	wrefresh(window);
}

void Curses_mw_miner::print_msg()
{
	if ((its_disp_flag == true) || (its_thru_flag == false))
	{
		print_disp();
	}
	else if (its_thru_flag == true)
	{
		print_thru();
	}
}

void Curses_mw_miner::process_cmd(int ch)
{
	switch(ch)
	{
		case KEY_UP:
		{
			if (its_y >1)
			{
				its_y--;
				wmove(window,its_y,its_x);
			}
			break;
		}
		case KEY_DOWN:
		{
			if (its_y <3)
			{
				its_y++;
				wmove(window,its_y,its_x);
			}
			break;
		}
		default: // JBS: further data window commands to here
		{
			break;
		}
	}
	wrefresh(window);
}

// Bring the cursor to the data window, called after main window had action
void Curses_mw_miner::focus()
{
	wmove(window,its_y,its_x);
	wrefresh(window);
}

string Curses_mw_miner::get_last_type() const
{
	if (its_old_midi_msg[0] != 0xf0) // not SysEx
	{
		return string();
	}
	else
	{
		if (its_old_midi_msg.size() <5) // probably incomplete sysEx
		{
			return string();
		}
		else
		{
			return its_synth_info->get_dump_name(its_old_midi_msg[4]);
		}
	}
}

bool Curses_mw_miner::write_last_dump(string filename)
{
	bool return_value = true;
	std::ofstream fout(filename.c_str(), std::ios::out | std::ios::binary);
	if (!fout.bad())
	{
		fout.write((const char*)&its_old_midi_msg[0],its_old_midi_msg.size());
		fout.close();
	}
	else
	{
		return_value = false;
	}
	return return_value;
}

string Curses_mw_miner::get_suggested_dump_filename() const
{
	string filename;
	string msg_type = get_last_type();
	if (its_old_midi_msg.size() > 5)
	{
		unsigned char cmd = its_old_midi_msg.at(4);
			// Check the message type and prepare the name accordingly
		if (msg_type.compare("global") == 0)
		{
			filename = msg_type;
		}
		else if (msg_type.compare("remote") == 0)
		{
			filename = msg_type;
		}
		else if (msg_type.compare("wave") == 0)
		{
			if (its_old_midi_msg.size() == 137)
			{
				unsigned int bank_no = its_old_midi_msg.at(its_synth_info->get_dump_bank(cmd));
				unsigned int patch_no = its_old_midi_msg.at(its_synth_info->get_dump_patch(cmd));
				unsigned int wave_no = (128 * bank_no) + patch_no;
				if (wave_no <10)
				{
					filename = string("000");
				}
				else if (wave_no <100)
				{
					filename = string("00");
				}
				else if (wave_no <1000)
				{
					filename = string("0");
				}
				filename = filename + to_string(wave_no) + string("-") + msg_type;
			}
			else
			{
				filename = string("waves");
			}
		}
		else if (msg_type.compare("wave control table") == 0)
		{
			if (its_old_midi_msg.size() == 265)
			{
				unsigned int patch_no = its_old_midi_msg.at(its_synth_info->get_dump_patch(cmd));
				if (patch_no <10)
				{
					filename = string("00");
				}
				else if (patch_no <100)
				{
					filename = string("0");
				}
				filename = filename + to_string(patch_no) + string("-") + msg_type;
			}
			else
			{
				filename = string("wave control tables");
			}
		}
		else if (msg_type.compare("sound") == 0)
		{
			if (its_old_midi_msg.size() == 265)
			{
				unsigned int bank_no = its_old_midi_msg.at(its_synth_info->get_dump_bank(cmd));
				unsigned int patch_no = its_old_midi_msg.at(its_synth_info->get_dump_patch(cmd));
				unsigned int name_start = its_synth_info->get_dump_name_start(cmd);
				unsigned int name_chars = its_synth_info->get_dump_name_chars(cmd);
				string patch_name;
				if (bank_no <= 1) // This is a sound not an edit buffer dump
				{
					if (bank_no == 0)
					{
						filename = string("A");
					}
					else if (bank_no == 1)
					{
						filename = string("B");
					}
					if (patch_no <10)
					{
						filename = filename + string("00");
					}
					else if (patch_no <100)
					{
						filename = filename + string("0");
					}
					filename = filename + to_string(patch_no) + string("-");
				}
				string tmp_name;
				auto start_it = (its_old_midi_msg.begin() + name_start);
				auto end_it = (start_it + name_chars);
				tmp_name = string(start_it,end_it);
				size_t start_pos = tmp_name.find_first_not_of(' ');
				size_t end_pos = tmp_name.find_last_not_of(' ');
				patch_name = tmp_name.substr(start_pos,(end_pos - start_pos +1));
				filename = filename + patch_name;
			}
			else // A dump of all sounds
			{
				filename = string("sounds");
			}
		}
		else if (msg_type.compare("multi") == 0)
		{
			if (its_old_midi_msg.size() == 265)
			{
				unsigned int bank_no = its_old_midi_msg.at(its_synth_info->get_dump_bank(cmd));
				unsigned int patch_no = its_old_midi_msg.at(its_synth_info->get_dump_patch(cmd));
				if (bank_no == 0) // This is a multi, not an edit buffer
				{
					if (patch_no <10)
					{
						filename = string("00");
					}
					else if (patch_no <100)
					{
						filename = string("0");
					}
					filename = filename + to_string(patch_no) + string("-");
				}
				unsigned int name_start = its_synth_info->get_dump_name_start(cmd);
				unsigned int name_chars = its_synth_info->get_dump_name_chars(cmd);
				string tmp_name, patch_name;
				auto start_it = (its_old_midi_msg.begin() + name_start);
				auto end_it = (start_it + name_chars);
				tmp_name = string(start_it,end_it);
				size_t start_pos = tmp_name.find_first_not_of(' ');
				size_t end_pos = tmp_name.find_last_not_of(' ');
				patch_name = tmp_name.substr(start_pos,(end_pos - start_pos +1));
				filename = filename + patch_name;
			}
			else // It's all multis
			{
				filename = string("multis");
			}
		}
	}

	// Add the suffix of the current date and time
	ptime now = boost::posix_time::second_clock::local_time();
	string full_date = boost::posix_time::to_iso_string(now);
	string time_suffix = full_date.substr(0,4) + string("-") + full_date.substr(4,2) + string("-") + full_date.substr(6,2) + string("-") + full_date.substr(9,2) + string("-") + full_date.substr(11,2) + string("-") + full_date.substr(13,2);
	filename = filename + string("-") + time_suffix + string(".syx");
	return filename;
}
