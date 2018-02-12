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
#include <thread>
#include "curses_mw_ui.hpp"

using std::string;
using std::cout;
using std::endl;
using std::ofstream;
using std::thread;

Curses_mw_ui::Curses_mw_ui():
	its_x(3), its_y(3), its_ch(0),
	its_cfg_file_name(""), its_error_msg(""),
	its_midi_input_name(""), its_midi_output_name(""),
	its_status_line(17), its_error_line(18)
{
	its_midi_name = string("MWII Display");
	its_midi_in = new RtMidiIn();
	its_midi_out = new RtMidiOut();
	its_error_flag.store(false);
	its_synth_info = new Synth_info(0x3e,0x0e,0x7f,0x05,0x15,40,2);
	its_mw_miner = new Curses_mw_miner(its_midi_out,its_synth_info);
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
			its_midi_in->openPort(port_number,its_midi_name);
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
				its_midi_in->openPort(port_number,its_midi_name);
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
			its_midi_out->openPort(port_number,its_midi_name);
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
				its_midi_out->openPort(port_number,its_midi_name);
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
	
	// Print a line of dashes
	for (int i = 1;i<79;i++)
	{
		mvwprintw(its_win,cur_line,i,"-");
	}
	cur_line++;

	mvwprintw(its_win,cur_line,5,"The following commands are available:");
	cur_line++;
	mvwprintw(its_win,cur_line,3,"Cursor UP - move up one line in the data display");
	cur_line++;
	mvwprintw(its_win,cur_line,3,"Cursor DOWN - move down one line in the data display");
	cur_line++;
	mvwprintw(its_win,cur_line,3,"SPACE - toggle contnuous display mode");
	cur_line++;
	mvwprintw(its_win,cur_line,3,"d - toggle direct data / display on demand mode");
	cur_line++;
	mvwprintw(its_win,cur_line,3,"i - set new MIDI input port");
	cur_line++;
	mvwprintw(its_win,cur_line,3,"o - set new MIDI output port");
	cur_line++;
	mvwprintw(its_win,cur_line,3,"r - redraw screen");
	cur_line++;
	mvwprintw(its_win,cur_line,3,"v - change device ID");
	cur_line++;
	mvwprintw(its_win,cur_line,3,"w - write configuration to config file");
	cur_line++;
	mvwprintw(its_win,cur_line,3,"q - quit %s",PACKAGE_NAME);
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
	wmove(its_win,its_status_line,2);
	wrefresh(its_win);
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
			case 81:
			case 113: // Q or q
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
			case 81:
			case 113: // q or Q
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
	cfg_out << "device_id = " << (unsigned short int)its_synth_info->get_dev_id();
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
			case 32: // SPACE bar
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
			case 68:
			case 100: // d or D
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
			case 81:
			case 113: // q or Q
			{
				its_mw_miner->set_quit(true);
				break;
			}
			case 87:
			case 119: // w or W
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
			case 73:
			case 105: // i or I
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
			case 79:
			case 111: // o or O
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
			case 86:
			case 118: // v or V
			{
				change_dev_id();
				print_main_screen();
				its_mw_miner->focus();
				break;
			}
			case 82:
			case 114: // r or R
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

void mw_midi_callback(double delta_time, std::vector<unsigned char>* message, void* user_data)
{
	Curses_mw_miner *my_miner = (Curses_mw_miner *)user_data;
	my_miner->accept_msg(delta_time,message);
}
