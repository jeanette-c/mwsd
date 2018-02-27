/* curses_mw_miner.hpp - definition of the class
 * Curses_mw_miner the data miner class that talks to the synthesizer and
 * interpretes its MIDI/SysEx responses.
 * This class uses the ncurses library for its display output, in future
 * the screen UI part should be separated from the underlying functionality.
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

#ifndef _MWSD_CURSES_MW_MINER_HPP_
#define _MWSD_CURSES_MW_MINER_HPP_

#include <atomic>
#include <string>
#include <vector>
#include <ncurses.h>
#include <RtMidi.h>
#include "synth_info.hpp" // contains Synth_info data class

/* Curses_mw_miner - the main work class
 * receive data
 * request display dump
 * print and interpret received data
*/

class Curses_mw_miner {
	public:
		Curses_mw_miner() = delete;
		Curses_mw_miner(RtMidiOut *midi_out, Synth_info *synth_info);
		~Curses_mw_miner();

			// Basic access methods
		void set_thru(bool thru_flag);
		void set_quit(bool quit_flag);
		void set_disp(bool disp_flag);
		void set_paused(bool paused);
		bool get_thru() const { return its_thru_flag.load(); }
		bool get_quit() const { return its_quit_flag.load(); }
		bool get_disp() const { return its_disp_flag.load(); }
		bool get_error() const { return its_error_flag.load(); }
		bool get_paused() const { return its_paused.load(); }
		unsigned short int get_unanswered() const { return its_unanswered.load(); }
		std::string get_error_msg() const { return its_error_msg; }

			// Utility methods
		void init_win();
		void shut_win();
		void run(); // mainloop for the thread
			// Callback function for RtMidiIn
		void accept_msg(double delta_time, std::vector<unsigned char> *message);
		void focus(); // just move the cursor into the data window
		void process_cmd(int ch); // process user input from main thread
		void print_msg(); // wrapper function for printing data
		std::string get_last_type() const; // return dump type of last msg or empty
		bool write_last_dump(std::string filename); // Write last dump to file
		std::string get_suggested_dump_filename() const; // from the MIDI message
	private:
			// Private methods
		void print_thru(); // print direct data
		void print_disp(); // print display contents

			// Internal state flags
		std::atomic_bool its_thru_flag; // direct data / display
		std::atomic_bool its_disp_flag; // toggle continuous display mode
		std::atomic_bool its_quit_flag; // set to true to quit the program
		std::atomic_bool its_new_flag; // set to true, when new data is there
		std::atomic_bool its_error_flag; // set to true upon error
		std::atomic_bool its_paused; // to be set, when action should be paused

			// Other internal variables
		std::atomic_ushort its_unanswered; // count of unanswered commands, reset by
			// an answered command
		int its_x; // x position on the data window
		int its_y; // y position on the data window
		std::vector<unsigned char> its_old_midi_msg; // previous different MIDI
			// message, which is not a display dump
		std::vector<unsigned char> its_old_disp_msg; // previous different display dump
		RtMidiOut *its_midi_out; // MIDI output port to send display request
		std::vector<std::string> its_disp; // processed display contents
		Synth_info *its_synth_info;
		std::string its_error_msg; // error message string
		WINDOW *window; // data window
};

#endif // #ifndef _MWSD_CURSES_MW_MINER_HPP_
