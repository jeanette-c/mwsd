/* curses_mw_ui.cpp - implementatio of the Curses_mw_ui class.
 * This class handles the program's main event loop and all other functionality
 * that isn't directly involved with the MIDI/SysEx communication.
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

#include "config.h"
#include <iostream> // for cout int list_ports
#include <fstream>
#include <chrono>
#include <boost/filesystem.hpp>
#include <thread>
#include <cmath>
#include <cstring>
#include <cctype>
#include <form.h>
#include "curses_mw_ui.hpp"

using std::string;
using std::cout;
using std::endl;
using std::ofstream;
using std::thread;
using std::vector;
using std::ceil;
using std::strlen;
using std::isspace;
namespace fs = boost::filesystem;

Curses_mw_ui::Curses_mw_ui(string res_dir):
	its_x(3), its_y(3), its_ch(0), its_res_dir(res_dir),
	its_cfg_file_name(""), its_error_msg(""),
	its_midi_input_name("In"), its_midi_output_name("Out"),
	its_status_line(17), its_error_line(18), its_suggested_dev_id(0x7f),
		  its_use_res_dir(true)
{
	its_midi_name = string("MWII Display");
	its_midi_in = new RtMidiIn(RtMidi::Api::UNSPECIFIED,its_midi_name);
	its_midi_out = new RtMidiOut(RtMidi::Api::UNSPECIFIED,its_midi_name);
	its_error_flag.store(false);
	its_synth_info = new Synth_info(0x3e,0x0e,0x7f,0x05,0x15,40,2);
	its_mw_miner = new Curses_mw_miner(its_midi_out,its_synth_info);
	its_discovery_flag.store(false);
}

Curses_mw_ui::~Curses_mw_ui()
{
	if (its_midi_in->isPortOpen())
	{
		its_midi_in->closePort();
	}
	if (its_midi_out->isPortOpen())
	{
		its_midi_out->closePort();
	}
	delete its_midi_in;
	delete its_midi_out;
	if (its_mw_miner->get_quit() == false)
	{
		its_mw_miner->set_quit(true);
	}
	delete its_mw_miner;
	delete its_synth_info;
}

bool Curses_mw_ui::set_midi_input(int port_number)
{
	int port_count = its_midi_in->getPortCount();
	if (port_count == 0)
	{
		its_error_msg = string("There are no MIDI input ports available");
		its_error_flag.store(true);
		return false;
	}

	if (port_number >= port_count)
	{
		its_error_msg = string("There is no MIDI input port with that number.");
		return false;
	}
	else
	{
		if (its_midi_in->isPortOpen())
		{
			its_midi_in->closePort();
		}
		try
		{
			its_midi_in->openPort(port_number,string("In"));
		}
		catch (RtMidiError& e)
		{
			its_error_msg = e.getMessage();
			its_error_flag.store(true);
			return false;
		}
		its_midi_input_name = its_midi_in->getPortName(port_number);
	}
	return true;
}

bool Curses_mw_ui::set_midi_input(string port_name)
{
	int port_number = -1;
	int port_count = its_midi_in->getPortCount();
	string cur_port_name;
	if (port_count == 0)
	{
		its_error_msg = string("There are no MIDI input ports available.");
		its_error_flag.store(true);
		return false;
	}
	else
	{
		for (int i = 0;i<port_count;i++)
		{
			cur_port_name = its_midi_in->getPortName(i);
			if (cur_port_name == port_name)
			{
				port_number = i;
				break;
			}
		}
		if (port_number == -1) // no port found
		{
			its_error_msg = string("A port of this name does not exist.");
			return false;
		}
		else
		{
			if (its_midi_in->isPortOpen())
			{
				its_midi_in->closePort();
			}
			try
			{
				its_midi_in->openPort(port_number,string("In"));
			}
			catch (RtMidiError& e)
			{
				its_error_msg = e.getMessage();
				its_error_flag.store(true);
				return false;
			}
			its_midi_input_name = cur_port_name;
		}
	}
	return true;
}

bool Curses_mw_ui::set_midi_output(int port_number)
{
	int port_count = its_midi_out->getPortCount();
	if (port_count == 0)
	{
		its_error_msg = string("There are no MIDI output ports available");
		its_error_flag.store(true);
		return false;
	}

	if (port_number >= port_count)
	{
		its_error_msg = string("There is no MIDI output port with that number.");
		return false;
	}
	else
	{
		if (its_midi_out->isPortOpen())
		{
			its_midi_out->closePort();
		}
		try
		{
			its_midi_out->openPort(port_number,string("Out"));
		}
		catch (RtMidiError& e)
		{
			its_error_msg = e.getMessage();
			its_error_flag.store(true);
			return false;
		}
		its_midi_output_name = its_midi_out->getPortName(port_number);
	}
	return true;
}

bool Curses_mw_ui::set_midi_output(string port_name)
{
	int port_number = -1;
	int port_count = its_midi_out->getPortCount();
	string cur_port_name;
	if (port_count == 0)
	{
		its_error_msg = string("There are no MIDI output ports available.");
		its_error_flag.store(true);
		return false;
	}
	else
	{
		for (int i = 0;i<port_count;i++)
		{
			cur_port_name = its_midi_out->getPortName(i);
			if (cur_port_name == port_name)
			{
				port_number = i;
				break;
			}
		}
		if (port_number == -1) // no port found
		{
			its_error_msg = string("A port of this name does not exist.");
			return false;
		}
		else
		{
			if (its_midi_out->isPortOpen())
			{
				its_midi_out->closePort();
			}
			try
			{
				its_midi_out->openPort(port_number,string("Out"));
			}
			catch (RtMidiError& e)
			{
				its_error_msg = e.getMessage();
				its_error_flag.store(true);
				return false;
			}
			its_midi_output_name = cur_port_name;
		}
	}
	return true;
}

// Print the main screen/window
void Curses_mw_ui::print_main_screen()
{
	unsigned int cur_line = 1; // line number to print to
	wclear(its_win);
	box(its_win,0,0);
	mvwprintw(its_win,cur_line,5,"%s",PACKAGE_STRING);
	cur_line++;
	if (its_midi_input_name.empty())
	{
		mvwprintw(its_win,cur_line,3,"No MIDI input port selected.");
		cur_line++;
	}
	else
	{
		mvwprintw(its_win,cur_line,3,"MIDI input port: %s",its_midi_input_name.c_str());
		cur_line++;
	}

	if (its_midi_output_name.empty())
	{
		mvwprintw(its_win,cur_line,3,"No MIDI output port selected.");
		cur_line++;
	}
	else
	{
		mvwprintw(its_win,cur_line,3,"MIDI output port: %s",its_midi_output_name.c_str());
		cur_line++;
	}
	mvwprintw(its_win,cur_line,3,"Press 'Q' to quit or press 'H' for help on further commands");
	if (its_mw_miner->get_paused() == true)
	{
		mvwprintw(its_win,its_status_line,2,"[Paused]");
	}
	else
	{
		if (its_mw_miner->get_disp() == true)
		{
			mvwprintw(its_win,its_status_line,2,"[Continuous display mode]");
		}
		else
		{
			if (its_mw_miner->get_thru() == true)
			{
				mvwprintw(its_win,its_status_line,2,"[Direct MIDI mode]");
			}
			else
			{
				mvwprintw(its_win,its_status_line,2,"[Display on demand mode]");
			}
		}
	}
	if (its_use_res_dir == true)
	{
		mvwprintw(its_win,its_status_line,42,"[Using resource folder]");
	}
	else
	{
		mvwprintw(its_win,its_status_line,42,"[Not using resource folder]");
	}
	its_y = its_status_line;
	its_x = 2;
	wmove(its_win,its_y,its_x);
	wrefresh(its_win);
}

// Print help screen
void Curses_mw_ui::print_help()
{
	bool local_quit = false; // Set to true when leaving help screen

	// Prepare window and print header
	wclear(its_win);
	box(its_win,0,0);
	mvwprintw(its_win,1,5,"%s",PACKAGE_STRING);
	mvwprintw(its_win,2,3,"Press 'H' to leave the help screen, PGUP/PGDOWN to scroll");

	// Set up messages and paging system
	vector<string> content; // List of commands to print
	content.reserve(14);
	content.push_back(string("Cursor UP - Move one line up in the display window"));
	content.push_back(string("Cursor DOWN - Move one line down in the display ewindow"));
	content.push_back(string("SPACE - Toggle direct data/display on demand modes"));
	content.push_back(string("D - Turn continuous display mode on/off"));
	content.push_back(string("H - Turn help mode on/off"));
	content.push_back(string("Q - Quit the program"));
	content.push_back(string("I - Select a new MIDI input"));
	content.push_back(string("O - Select a new MIDI output"));
	content.push_back(string("P - Probe for a Micorwave II/Xt synthesizer"));
	content.push_back(string("R - Redraw the screen"));
	content.push_back(string("S - Save the last SysEx message, if it's a data dump"));
	content.push_back(string("U - Toggle use of resource folder"));
	content.push_back(string("V - Select a new device ID"));
	content.push_back(string("W - Write the configuration file"));
	int start_line = 3;
	int num_lines = (its_status_line - start_line); // Available screen lines
	int num_pages = ceil(content.size() / float(num_lines)); // number of pages
	unsigned int cur_page = 0; // number of current page
	unsigned int max_msg = 0; // maximum message index to print

	// print first page
	if (content.size() < (num_lines * (cur_page +1) -1))
	{
		max_msg = content.size();
	}
	else
	{
		max_msg = (num_lines * (cur_page + 1));
	}
	int cur_line = start_line;
	for (int i = (cur_page * num_lines);i<max_msg;i++, cur_line++)
	{
		mvwprintw(its_win,cur_line,2,"%s",content[i].c_str());
	}
	wmove(its_win,2,1);
	wrefresh(its_win);

	while ((its_mw_miner->get_quit()) != true && (local_quit == false))
	{
		its_ch = getch();
		switch(its_ch)
		{
			case KEY_PPAGE:
			{
				if (cur_page == 0)
				{
					beep();
				}
				else
				{
					cur_page--;
					if (content.size() < (num_lines * (cur_page +1) - 1))
					{
						max_msg = content.size();
					}
					else
					{
						max_msg = (num_lines * (cur_page + 1));
					}
					cur_line = start_line;
					wclear(its_win);
					box(its_win,0,0);
					mvwprintw(its_win,1,5,"%s",PACKAGE_STRING);
					mvwprintw(its_win,2,3,"Press 'H' to leave the help screen, PGUP/PGDOWN to scroll");
					for (int i = (cur_page * num_lines);i<max_msg;i++, cur_line++)
					{
						mvwprintw(its_win,cur_line,2,"%s",content[i].c_str());
					}
					wmove(its_win,2,1);
					wrefresh(its_win);
				}
				break;
			}
			case KEY_NPAGE:
			{
				if (cur_page == (num_pages -1))
				{
					beep();
				}
				else
				{
					cur_page++;
					if (content.size() < (num_lines * (cur_page +1) - 1))
					{
						max_msg = content.size();
					}
					else
					{
						max_msg = (num_lines * (cur_page + 1));
					}
					cur_line = start_line;
					wclear(its_win);
					box(its_win,0,0);
					mvwprintw(its_win,1,5,"%s",PACKAGE_STRING);
					mvwprintw(its_win,2,3,"Press 'H' to leave the help screen, PGUP/PGDOWN to scroll");
					for (int i = (cur_page * num_lines);i<max_msg;i++, cur_line++)
					{
						mvwprintw(its_win,cur_line,2,"%s",content[i].c_str());
					}
					wmove(its_win,2,1);
					wrefresh(its_win);
				}
				break;
			}
			case 'q':
			case 'Q':
			{
				its_mw_miner->set_quit(true);
				break;
			}
			case 'h':
			case 'H':
			case 27:
			{
				local_quit = true;
				break;
			}
			default:
			{
				if (its_ch != ERR)
				{
					beep();
				}
				break;
			}
		}
	}
}

bool Curses_mw_ui::change_port(char designation)
{
	bool search_quit = false; // set to true to leave screen
	RtMidi *cur_midi;
	int cur_port_number = -1;
	string cur_port_name;
	// prepare window look
	wclear(its_win);
	box(its_win,0,0);
	if (designation == 'i')
	{
		cur_midi = its_midi_in;
		mvwprintw(its_win,1,5,"Choose MIDI input port");
	}
	else if (designation == 'o')
	{
		cur_midi = its_midi_out;
		mvwprintw(its_win,1,5,"Choose MIDI output port");
	}
	else
	{
		its_error_msg = string("Wrong port designation");
		return false;
	}
	int port_count = cur_midi->getPortCount();
	if (port_count == 0)
	{
		if (designation == 'i')
		{
			its_error_msg = string("There are no MIDI input ports available.");
		}
		else
		{
			its_error_msg = string("There are no MIDI output ports available.");
		}
		its_error_flag.store(true);
		return false;
	}

	mvwprintw(its_win,2,3,"Use UP/DOWN cursor keys to select a port");
	mvwprintw(its_win,3,3,"Press ENTER to select a port or q to quit.");
	// Print a line of dashes
	for (int i = 1;i<79;i++)
	{
		mvwprintw(its_win,4,i,"-");
	}

	// Print the list of ports
	for (int i =0;i<port_count;i++)
	{
		cur_port_name = cur_midi->getPortName(i);
		mvwprintw(its_win,(5+i),3,"%d - %s",i,cur_port_name.c_str());
	}
	its_y = 5;
	its_x = 3;
	wmove(its_win,its_y,its_x);
	wrefresh(its_win);
	while (search_quit == false && its_mw_miner->get_quit() == false)
	{
		its_ch = getch();
		switch(its_ch)
		{
			case KEY_UP:
			{
				getyx(its_win,its_y,its_x);
				if (its_y >5)
				{
					its_y--;
					wmove(its_win,its_y,its_x);
					wrefresh(its_win);
				}
				else
				{
					beep();
				}
				break;
			}
			case KEY_DOWN:
			{
				getyx(its_win,its_y,its_x);
				if (its_y <(4 + port_count))
				{
					its_y++;
					wmove(its_win,its_y,its_x);
					wrefresh(its_win);
				}
				else
				{
					beep();
				}
				break;
			}
			case 10: // ENTER, KEY_ENTER didn't work on Linux
			{
				getyx(its_win,its_y,its_x);
				cur_port_number = its_y -5; // simple screen row arithmetic
				search_quit = true;
				break;
			}
			case 'q':
			case 'Q':
			case 27:
			{
				search_quit = true;
				break;
			}
			default:
			{
				if (its_ch != ERR)
				{
					beep();
				}
				break;
			}
		}
	}

	bool return_value = true; // derived from set_midi_input/output
	if (cur_port_number != -1)
	{
		if (designation == 'i')
		{
			return_value = set_midi_input(cur_port_number);
		}
		else
		{
			return_value = set_midi_output(cur_port_number);
		}
	}
	return return_value;
}

void Curses_mw_ui::change_dev_id()
{
	int cur_id = its_synth_info->get_dev_id();
	bool search_quit = false; // set to true to quit screen
	wclear(its_win);
	box(its_win,0,0);
	mvwprintw(its_win,1,5,"Choose a new device ID");
	mvwprintw(its_win,2,3,"Use UP?DOWN cursor keys to select the device ID.");
	mvwprintw(its_win,3,3,"Press ENTER to select or q to quit.");
	// Prnt a line of dashes
	for (int i = 1;i<79;i++)
	{
		mvwprintw(its_win,4,i,"-");
	}
	mvwprintw(its_win,5,3,"New device ID: ");
	its_y = 5;
	its_x = 18;
	
	// UI event loop
	while (search_quit == false && its_mw_miner->get_quit() == false)
	{
		wmove(its_win,its_y,its_x);
		wclrtoeol(its_win);
		box(its_win,0,0);
		if (cur_id == 127)
		{
			mvwprintw(its_win,its_y,its_x,"GLOBAL");
		}
		else
		{
			mvwprintw(its_win,its_y,its_x,"%d",cur_id);
		}
		wmove(its_win,its_y,its_x);
		wrefresh(its_win);
		its_ch = getch();
		switch(its_ch)
		{
			case KEY_DOWN:
			{
				if (cur_id >0)
				{
					cur_id--;
				}
				else
				{
					beep();
				}
				break;
			}
			case KEY_UP:
			{
				if (cur_id <127)
				{
					cur_id++;
				}
				else
				{
					beep();
				}
				break;
			}
			case 10: // ENTER, KEY_ENTER didn't work on Linux
			{
				its_synth_info->set_dev_id(cur_id);
				search_quit = true;
				break;
			}
			case 'q':
			case 'Q':
			case 27:
			{
				search_quit = true;
				break;
			}
			default:
			{
				if (its_ch != ERR)
				{
					beep();
				}
				break;
			}
		}
	}
}

bool Curses_mw_ui::write_cfg()
{
	if (its_cfg_file_name.empty())
	{
		its_error_msg = string("No configuration file set.");
		return false;
	}
	
	ofstream cfg_out(its_cfg_file_name.c_str());

	if (cfg_out.is_open() == false)
	{
		its_error_msg = string("Could not open configuration file.");
		its_error_flag.store(true);
		return false;
	}

	// Now write the configuration
	cfg_out << "# Configuration file for " << PACKAGE_STRING << endl;
	cfg_out << "input_port = " << its_midi_input_name << "\n";
	cfg_out << "output_port = " << its_midi_output_name << "\n";
	cfg_out << "device_id = " << (unsigned short int)its_synth_info->get_dev_id() << "\n";
	cfg_out << "resource_folder = " << its_res_dir;
	cfg_out.close();
	return true;
}

void Curses_mw_ui::list_ports()
{
	int port_count = its_midi_in->getPortCount();
	cout << PACKAGE_STRING << endl;
	cout << "Listing available MIDI ports.\n\n";
	cout << "MIDI input ports:\n";
	if (port_count >0)
	{
		for (int i = 0;i<port_count;i++)
		{
			cout << "\t" << its_midi_in->getPortName(i) << endl;
		}
	}
	else
	{
		cout << "\tNo MIDI input ports available.\n";
	}
	port_count = its_midi_out->getPortCount();
	cout << "MIDI output ports:\n";
	if (port_count >0)
	{
		for (int i = 0;i<port_count;i++)
		{
			cout << "\t" << its_midi_out->getPortName(i) << endl;
		}
	}
	else
	{
		cout << "\tNo MIDI output ports available.\n";
	}
}

// Initialise ncurses UI
void Curses_mw_ui::init_ui()
{
	initscr();
	clear();
	refresh();
	cbreak();
	noecho();
	keypad(stdscr,TRUE);
	nodelay(stdscr,TRUE);
	its_win = newwin(20,80,0,0);
	wrefresh(its_win);
}

// Stop ncurses UI
void Curses_mw_ui::shut_ui()
{
	delwin(its_win);
	clear();
	refresh();
	endwin();
}

// Trim leading and trailing whitespaces, copied from fields_magic.c, a simple
// example of the curses form library
string Curses_mw_ui::trim(char *str) const
{
	char *end;
	
	// Trim leading space
	while(isspace(*str))
	{
		str++;
	}
	if(*str == 0) // all spaces?
	{
		return string();
	}
	// trim trailing space
	end = str + strlen(str) - 1;
	while(end > str && isspace(*end))
	{
		end--;
	}
	// write new null terminator
	*(end+1) = '\0';
	return string(str);
}

// Save last MIDI message, if it's a data dump
bool Curses_mw_ui::save_dump()
{
	bool return_value = true;
	string msg_type = its_mw_miner->get_last_type();
	if (msg_type.empty())
	{
		return false;
	}
	else // It's some kind of dump
	{
		if ((msg_type.compare("display") == true) || (msg_type.compare("mode") == true) || (msg_type.compare("remote") == 0))
		{
			return false;
		}
		else // it's not a display/mode/remote dump, so save
		{
			bool local_quit = false; // set to true, when quitting
			string filename;
			filename = its_mw_miner->get_suggested_dump_filename();
			wclear(its_win);
			box(its_win,0,0);
			mvwprintw(its_win,1,5,"%s",PACKAGE_STRING);
			mvwprintw(its_win,2,2,"Enter filename, press return to confirm, or escape to quit without saving");
			FIELD *fields[5];
			FORM *form;
			fields[0] = new_field(1,9,1,0,0,0);
			fields[1] = new_field(1,1,1,10,0,0);
			fields[2] = new_field(1,64,1,11,0,0);
			fields[3] = new_field(1,1,1,75,0,0);
			fields[4] = NULL;
			set_field_buffer(fields[0],0,"Filename:");
			set_field_buffer(fields[1],0,"[");
			set_field_buffer(fields[2],0,filename.c_str());
			set_field_buffer(fields[3],0,"]");
			set_field_opts(fields[0],O_VISIBLE | O_PUBLIC | O_AUTOSKIP);
			set_field_opts(fields[1],O_VISIBLE | O_PUBLIC | O_AUTOSKIP);
			set_field_opts(fields[2],O_VISIBLE | O_PUBLIC | O_EDIT | O_ACTIVE);
			set_field_opts(fields[3],O_VISIBLE | O_PUBLIC | O_AUTOSKIP);
			form = new_form(fields);
			set_form_win(form,its_win);
			set_form_sub(form,derwin(its_win,2,78,3,1));
			post_form(form);
			form_driver(form,REQ_END_LINE);
			refresh();
			wrefresh(its_win);
			while ((local_quit == false) && (its_mw_miner->get_quit() == false))
			{
				its_ch = getch();
				switch(its_ch)
				{
					case KEY_LEFT:
					{
						form_driver(form, REQ_PREV_CHAR);
						break;
					}
					case KEY_RIGHT:
					{
						form_driver(form, REQ_NEXT_CHAR);
						break;
					}
					case KEY_BACKSPACE:
					case 127:
					{
						form_driver(form, REQ_DEL_PREV);
						break;
					}
					case KEY_DC:
					{
						form_driver(form, REQ_DEL_CHAR);
						break;
					}
					case KEY_HOME:
					case KEY_BEG:
					{
						form_driver(form,REQ_BEG_LINE);
						break;
					}
					case KEY_END:
					{
						form_driver(form,REQ_END_LINE);
						break;
					}
					case 27:
					{
						local_quit = true;
						break;
					}
					case 10: // Return
					{
						form_driver(form, REQ_PREV_FIELD);
						form_driver(form, REQ_NEXT_FIELD);
						filename = trim(field_buffer(fields[2],0));
						if (!filename.empty())
						{
							if (its_use_res_dir == true)
							{
								filename = its_res_dir + string("/") + its_mw_miner->get_last_type() + string("/") + filename;
							}
							return_value = its_mw_miner->write_last_dump(filename);
							if (return_value == false)
							{
								its_error_msg = string("Couldn't save ") + msg_type + string(" dump to ") + filename;
							}
						}
						local_quit = true;
						break;
					}
					default:
					{
						if (its_ch != ERR)
						{
							form_driver(form,its_ch);
						}
						break;
					}
				}
				wrefresh(its_win);
			}
			free_form(form);
			free_field(fields[0]);
			free_field(fields[1]);
			free_field(fields[2]);
			free_field(fields[3]);
			wclear(its_win);
		}
	}
	return return_value;
}

// Main UI event loop for the program
bool Curses_mw_ui::run()
{
	bool return_value = true; // return value of this function
	bool ret = true; // used to capture return values from other function calls

	// MIDI in will accept SysEx, but no clock and active sensing
	its_midi_in->ignoreTypes(false,true,true);
	// Set callback for MIDI input, so Curses_mw_miner is notified on new data
	its_midi_in->setCallback(&mw_midi_callback,(void *)its_mw_miner);
	print_main_screen();
	thread mw_miner_thread(&Curses_mw_miner::run,its_mw_miner);
	std::chrono::milliseconds sleep_time(5); // 5ms between each read

	while (its_mw_miner->get_quit() == false && its_error_flag == false)
	{
		its_ch = getch();
		switch(its_ch)
		{
			case ' ':
			{
				ret = its_mw_miner->get_thru();
				its_mw_miner->set_thru(!ret);
				if (its_mw_miner->get_disp() == false)
				{
					wmove(its_win,its_status_line,2);
					wclrtoeol(its_win);
					box(its_win,0,0);
					if (ret == true)
					{
						mvwprintw(its_win,its_status_line,2,"[Display on demand mode]");
					}
					else
					{
						mvwprintw(its_win,its_status_line,2,"[Direct MIDI mode]");
					}
					wrefresh(its_win);
					its_mw_miner->focus();
				}
				break;
			}
			case 'd':
			case 'D':
			{
				ret = its_mw_miner->get_disp();
				its_mw_miner->set_disp(!ret);
				if (ret == false)
				{
					wmove(its_win,its_status_line,2);
					wclrtoeol(its_win);
					box(its_win,0,0);
					mvwprintw(its_win,its_status_line,2,"[Continuous display mode]");
					wrefresh(its_win);
					its_mw_miner->focus();
				}
				break;
			}
			case KEY_UP:
			{
				its_mw_miner->process_cmd(its_ch);
				break;
			}
			case KEY_DOWN:
			{
				its_mw_miner->process_cmd(its_ch);
				break;
			}
			case 'q':
			case 'Q':
			{
				its_mw_miner->set_quit(true);
				break;
			}
			case 'h':
			case 'H':
			case KEY_F(1):
			{
				print_help();
				print_main_screen();
				break;
			}
			case 's':
			case 'S':
			{
				its_mw_miner->set_paused(true);
				bool ret = save_dump();
				its_mw_miner->set_paused(false);
				print_main_screen();
				if ((ret == false) && (its_error_msg.empty() == false))
				{
					mvwprintw(its_win,its_error_line,2,"%s",its_error_msg.c_str());
					its_error_msg.clear();
				}
				break;
			}
			case 'w':
			case 'W':
			{
				ret = write_cfg();
				if (ret == false && its_error_flag == false)
				{
					beep();
					wmove(its_win,its_error_line,2);
					wclrtoeol(its_win);
					box(its_win,0,0);
					mvwprintw(its_win,its_error_line,2,"%s",its_error_msg.c_str());
					wrefresh(its_win);
					its_mw_miner->focus();
					its_error_msg.clear();
				}
				if (its_error_flag == true)
				{
					its_mw_miner->set_quit(true);
				}
				break;
			}
			case 'i':
			case 'I':
			{
				its_mw_miner->set_paused(true);
				ret = change_port('i');
				print_main_screen();
				if (ret == false && its_error_flag == false)
				{
					wmove(its_win,its_error_line,2);
					wclrtoeol(its_win);
					box(its_win,0,0);
					mvwprintw(its_win,its_error_line,2,"%s",its_error_msg.c_str());
					wrefresh(its_win);
					its_error_msg.clear();
				}
				if (its_error_flag == true)
				{
					its_mw_miner->set_quit(true);
				}
				else
				{
					its_mw_miner->set_paused(false);
				}
				its_mw_miner->focus();
				break;
			}
			case 'o':
			case 'O':
			{
				its_mw_miner->set_paused(true);
				ret = change_port('o');
				print_main_screen();
				if (ret == false && its_error_flag == false)
				{
					wmove(its_win,its_error_line,2);
					wclrtoeol(its_win);
					box(its_win,0,0);
					mvwprintw(its_win,its_error_line,2,"%s",its_error_msg.c_str());
					wrefresh(its_win);
					its_error_msg.clear();
				}
				if (its_error_flag == true)
				{
					its_mw_miner->set_quit(true);
				}
				else
				{
					its_mw_miner->set_paused(false);
				}
				its_mw_miner->focus();
				break;
			}
			case 'u':
			case 'U':
			{
				its_use_res_dir = !its_use_res_dir;
				print_main_screen();
				break;
			}
			case 'v':
			case 'V':
			{
				change_dev_id();
				print_main_screen();
				its_mw_miner->focus();
				break;
			}
			case 'p':
			case 'P':
			{
				its_mw_miner->set_paused(true);
				bool ret = probe_synth();
				if (ret == false)
				{
					if (its_error_flag == true)
					{
						its_mw_miner->set_quit(true);
					}
					else
					{
						its_mw_miner->set_paused(false);
					}
				}
				else
				{
					its_mw_miner->set_paused(false);
				}
				print_main_screen();
				break;
			}
			case 'r':
			case 'R':
			{
				print_main_screen();
				its_mw_miner->focus();
				break;
			}
			default:
			{
				if (its_ch != ERR)
				{
					beep();
				}
				break;
			}
		}
		std::this_thread::sleep_for(sleep_time);
	}
	mw_miner_thread.join();


	// Close MIDI ports if necessary
	if (its_midi_in->isPortOpen())
	{
		its_midi_in->closePort();
	}
	if (its_midi_out->isPortOpen())
	{
		its_midi_out->closePort();
	}

	// destroy members and check for errors
	if (its_error_flag == true)
	{
	}
	if (its_mw_miner->get_error() == true)
	{
		its_error_msg += string("\n") + its_mw_miner->get_error_msg();
		its_error_flag = true;
	}
	if (its_error_flag == true)
	{
		return false;
	}
	return true;
}

// Local part of port discovery RtMidi callback
void Curses_mw_ui::discover_port(vector<unsigned char> *message)
{
	if (message->size() != 14) // 14 bytes for identity reply
	{
		return;
	}
	if (message->at(0) == 0xf0) // sysEx start
	{
		if (message->at(1) == 0x7e) // General MIDI
		{
			if (message->at(2) == 0x06) // Information related
			{
				if (message->at(3) == 02) // Identity response
				{
					if (message->at(4) == its_synth_info->get_man_id()) // man ID
					{
						if (message->at(5) == its_synth_info->get_equip_id()) // Equipment ID
						{
							its_discovery_flag.store(true);
						}
					}
				}
			}
		}
	}
}

// Local part of RtMidi dev ID discovery callback
void Curses_mw_ui::discover_id(vector<unsigned char> *message)
{
	if (message->size() != 14) // size of a correct identity response
	{
		return;
	}
	if (message->at(2) == 0x06) // MIDI information related
	{
		if (message->at(3) == 0x02) // identity response
		{
			its_suggested_dev_id = message->at(6); // where the MWII puts it
		}
	}
}

// Probe for a synthy (for now MWII/Xt only)
bool Curses_mw_ui::probe_synth()
{
		// Clear window and print comfort message
	wclear(its_win);
	box(its_win,0,0);
	mvwprintw(its_win,2,2,"Please wait, this may take a few seconds...");
	wmove(its_win,2,1);
	wrefresh(its_win);

		// general function variables
	bool return_value = true;
	std::chrono::milliseconds sleep_time(100); // wait for synth reply
	string probe_client_name("MWSD Synth Probe"); // RtMidi client name
	string tmp_name; // Used for probe port names
	bool search_quit = false; // Whether to quit the event loop or not
	unsigned int choice = 0; // The choice amongst the discovered synths
		// RtMidi I/O for port listing
	RtMidiIn *min = new RtMidiIn(RtMidi::Api::UNSPECIFIED,probe_client_name);
	RtMidiOut *mout = new RtMidiOut(RtMidi::Api::UNSPECIFIED,probe_client_name);
	unsigned int incount = min->getPortCount();
	unsigned int outcount = mout->getPortCount();
	if ((outcount == 0) || (incount == 0))
	{
		its_error_msg = string("There are no MIDI in- or output ports.");
		its_error_flag.store(true);
		its_error_msg = string("There are no MIDI in- or output ports");
		if (min->isPortOpen())
		{
			min->closePort();
		}
		if (mout->isPortOpen())
		{
			mout->closePort();
		}
		return false;
	}

		// Vectors holding all found ports
	vector<RtMidiIn *> vmin;
	vmin.reserve(incount);
	vector<RtMidiOut *> vmout;
	vmout.reserve(outcount);
		// vector holding I/O pairs for discovered synths
	vector<std::pair<unsigned int, unsigned int> > synth_ports;
	bool found = false; // set to true when the first synth is discovered
		// Vector holding identity request message
	vector<unsigned char> idreq { 0xf0, 0x7e, 0x7f, 0x06, 0x01, 0xf7 };
	its_discovery_flag.store(false); // will be set by discovery callbacks

	// Prepare the I/O listing and all vectors
	for (int i = 0;i<incount;i++)
	{
		vmin.push_back(new RtMidiIn(RtMidi::Api::UNSPECIFIED,probe_client_name));
		tmp_name = string("In-") + std::to_string(i);
		vmin[i]->openPort(i,tmp_name);
		vmin[i]->ignoreTypes(false,true,true);
	}
	for (int i = 0;i<outcount;i++)
	{
		vmout.push_back(new RtMidiOut(RtMidi::Api::UNSPECIFIED,probe_client_name));
		tmp_name = string("Out-") + std::to_string(i);
		vmout[i]->openPort(i,tmp_name);
	}

	// Discover synths and store I/O port_number pairs
	its_discovery_flag.store(false);
	for (int i = 0;i<outcount;i++)
	{
		for (int j = 0;j<incount;j++)
		{
			vmin[j]->setCallback(&mw_port_discovery_callback,this);
			if (vmout[i]->isPortOpen())
			{
				vmout[i]->sendMessage(&idreq);
				std::this_thread::sleep_for(sleep_time);
				if (its_discovery_flag == true) // a synth was discovered
				{
					synth_ports.push_back(std::pair<unsigned int, unsigned int> \
						(j,i));
					found = true;
					its_discovery_flag.store(false);
				}
			}
			vmin[j]->cancelCallback();
		}
	}
	its_discovery_flag.store(false);
	
		// show the choices, if any
	if (found == true)
	{
		wclear(its_win);
		box(its_win,0,0);
		mvwprintw(its_win,1,5,"%s",PACKAGE_NAME);
		mvwprintw(its_win,2,2,"Use UP/DOWN cursor keys to choose, ENTER to confirm or Q to quit.");
		if (synth_ports.size() == 1)
		{
			mvwprintw(its_win,3,2,"This synthesizer was found:");
		}
		else
		{
			mvwprintw(its_win,3,2,"These synthesizers were found:");
		}
		for (int i = 0;i<synth_ports.size();i++)
		{
			mvwprintw(its_win,(4 + (i*3)),2,"Synthesizer found on:");
			tmp_name = min->getPortName(synth_ports[i].first);
			mvwprintw(its_win,(5 + (i*3)),4,"In:  %s",tmp_name.c_str());
			tmp_name = mout->getPortName(synth_ports[i].second);
			mvwprintw(its_win,(6 + (i*3)),4,"Out: %s",tmp_name.c_str());
		}
		its_x = 2;
		its_y = 4;
		wmove(its_win,its_y,its_x);
		wrefresh(its_win);

		found = false; // reuse to mark that a synth was chosen
		while (search_quit == false && its_mw_miner->get_quit() == false)
		{
			its_ch = getch();
			switch(its_ch)
			{
				case KEY_UP:
				{
					if (choice >0)
					{
						choice--;
						its_y = 4 + (3 * choice);
						wmove(its_win,its_y,its_x);
						wrefresh(its_win);
					}
					else
					{
						beep();
					}
					break;
				}
				case KEY_DOWN:
				{
					if (choice < (synth_ports.size() -1))
					{
						choice++;
						its_y = 4 + (3 * choice);
						wmove(its_win,its_y,its_x);
						wrefresh(its_win);
					}
					else
					{
						beep();
					}
					break;
				}
				case 10: // KEY_ENTER didn't work on Linux
				{
					search_quit = true;
					return_value = true;
					found = true;
					break;
				}
				case 'q':
				case 'Q':
				{
					return_value = false;
					search_quit = true;
					break;
				}
				default:
				{
					if (its_ch != ERR)
					{
						beep();
					}
					break;
				}
			}
		}
		
		if (found == true) // synth chose, get dev ID now
		{
				// Number of input and output port
			int input_n = synth_ports[choice].first;
			int output_n = synth_ports[choice].second;
			vmin[input_n]->setCallback(&mw_dev_id_discovery_callback,this);
			vmout[output_n]->sendMessage(&idreq);
			std::this_thread::sleep_for(sleep_time);
			its_synth_info->set_dev_id(its_suggested_dev_id);
			its_midi_input_name = vmin[input_n]->getPortName(input_n);
			its_midi_output_name = vmout[output_n]->getPortName(output_n);
			return_value = set_midi_input(input_n);
			if (return_value == true)
			{
				return_value = set_midi_output(output_n);
				if (return_value == false)
				{
					its_error_flag.store(true);
				}
			}
			else
			{
				its_error_flag.store(true);
			}
		}
	}
	else
	{
		return_value = false;
		wclear(its_win);
		box(its_win,0,0);
		mvwprintw(its_win,1,5,"%s",PACKAGE_NAME);
		mvwprintw(its_win,2,2,"No synthesizers detected. You can try manually.");
		mvwprintw(its_win,3,2,"Press any key to return to main screen...");
		wmove(its_win,2,2);
		wrefresh(its_win);
		while (getch() == ERR)
		{
			;
		}
	}

		// Cleanup: close all ports, delete all new'ed objects
	if (min->isPortOpen())
	{
		min->closePort();
	}
	if (mout->isPortOpen())
	{
		mout->closePort();
	}
	for (auto port: vmin)
	{
		if (port->isPortOpen())
		{
			port->cancelCallback();
			port->closePort();
		}
		delete port;
	}

	for (auto port: vmout)
	{
		if (port->isPortOpen())
		{
			port->closePort();
		}
		delete port;
	}
	return return_value;
}

void mw_midi_callback(double delta_time, vector<unsigned char>* message, void* user_data)
{
	Curses_mw_miner *my_miner = (Curses_mw_miner *)user_data;
	my_miner->accept_msg(delta_time,message);
}

// RtMidi callback for port probing for synths
void mw_port_discovery_callback(double deltatime, vector<unsigned char> * message, void *user_data)
{
	Curses_mw_ui *my_ui = (Curses_mw_ui *)user_data;
	// Do the rest within the Curses_mw_ui object, because of local information
	// needed:
	my_ui->discover_port(message);
}

// RtMidi callback to discover the ID of a synth
void mw_dev_id_discovery_callback(double deltatime, vector<unsigned char> *message,void *user_data)
{
	Curses_mw_ui *my_ui = (Curses_mw_ui *)user_data;
	// Do the rest within Curses_mw_ui object, because of local information
	my_ui->discover_id(message);
}

bool Curses_mw_ui::check_res_dir()
{
	bool return_value = true;
	fs::path res_sub_path(its_res_dir);
	vector<string> the_names = its_synth_info->get_dump_names();
	string sub_dir;

	// Go through all possible dump types, sparing a few
	for (auto msg_type: the_names)
	{
		if ((msg_type.compare("display") != 0) && (msg_type.compare("mode") != 0) && (msg_type.compare("remote") != 0))
		{
			sub_dir = its_res_dir + string("/") + msg_type;
			res_sub_path = sub_dir;
			if (fs::exists(res_sub_path))
			{
				if (!fs::is_directory(res_sub_path))
				{
					return_value = false;
					its_error_msg = sub_dir + string("exists, but is no directory.");
					break;
				}
			}
			else
			{
				if (!fs::create_directory(res_sub_path))
				{
					return_value = false;
					its_error_msg = string("Can't create folder ") + sub_dir;
					break;
				}
			}
		}
	}
	return return_value;
}
