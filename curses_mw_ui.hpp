/* curses_mw_ui.hpp - definition of the Curses_mw_ui class. This class
 * handles all screens and all functionality that isn't directly concerned
 * with the MIDI/SysEx communication.
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

#ifndef _MWSD_CURSES_MW_UI_HPP_
#define _MWSD_CURSES_MW_UI_HPP_

#include <ncurses.h>
#include <string>
#include <atomic>
#include <RtMidi.h>
#include "synth_info.hpp"
#include "curses_mw_miner.hpp"

class Curses_mw_ui
{
	public:
			// Constructor and destructor
		Curses_mw_ui(std::string res_dir);
		~Curses_mw_ui();

			// Access methods
		void set_dev_id(unsigned char id) { its_synth_info->set_dev_id(id); }
		bool set_midi_input(int port_number);
		bool set_midi_input(std::string port_name);
		bool set_midi_output(int port_number);
		bool set_midi_output(std::string port_name);
		void set_cfg_file_name(std::string name) { its_cfg_file_name = name; }
		void set_res_dir(std::string res_dir) { its_res_dir = res_dir; }
		std::string get_error_msg() const { return its_error_msg; }
		bool get_error() const { return its_error_flag.load(); }
			// local part of port discovery RtMidi callback
		void discover_port(std::vector<unsigned char> *message);
			// Local part of dev ID discovery RtMidi callback
		void discover_id(std::vector<unsigned char> *message);
		std::string trim(char*) const;
		bool check_res_dir(); // Check that all subpaths exist

			// UI screen functions
		void print_main_screen(); // just print the main screen again
		void print_help(); // print help screen
		bool change_port(char port_designation); // Change MIDI I or O port
		void change_dev_id(); // Change device ID
		bool probe_synth(); // probe for the synth (MWII/XT for now)
		bool save_dump(); // Save last MIDI message, if it's a dump
		bool write_cfg(); // Write configuration to file
		void init_ui(); // Set up curses UI
		void shut_ui(); // Shut down curses UI
		bool run(); // main event UI loop
		void list_ports(); // print a list of MIDI I/O ports to stdout
	private:
		bool its_use_res_dir; // use the directory if true
		std::string its_res_dir; // Path to the resources folder
		std::string its_cfg_file_name; // name of the attached config file
		std::string its_midi_name; // Port name for MIDI I/O ports
		std::string its_midi_input_name; // name of connected input port
		std::string its_midi_output_name; // name of connected output port
		std::string its_error_msg; // string containing error message
		std::atomic_bool its_error_flag; // set upon error
		WINDOW *its_win; // main window
		int its_x; // current x position on the window
		int its_y; // current y position on the window
		int its_error_line; // where to print errors
		int its_status_line; // where to print status information
		int its_ch; // character input by user
		RtMidiIn *its_midi_in; // MIDI input port
		RtMidiOut *its_midi_out; // MIDI output port
		std::atomic_bool its_discovery_flag; // used for port/dev_id probing
		unsigned char its_suggested_dev_id; // used for synth probing
		Synth_info *its_synth_info; // data class holding synth specific info
		Curses_mw_miner *its_mw_miner;
};

// Callback function to be passed to RtMidiIn, user_data the Mw_miner
void mw_midi_callback(double deltatime, std::vector<unsigned char>* message, void * user_data);
// RtMidi callback function for synth probing, user_data is the
// Curses_mw_ui object
void mw_port_discovery_callback(double deltatime,std::vector<unsigned char> *message,void *user_data);
void mw_dev_id_discovery_callback(double deltatime,std::vector<unsigned char> *message, void *user_data);

#endif // #ifndef _MWSD_CURSES_MW_UI_HPP_
