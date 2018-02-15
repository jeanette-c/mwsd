/* main.cpp - main program of the mwsd (MicroWave Synthesizer Display)
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
#include <boost/program_options.hpp>
namespace po = boost::program_options;
#include <iterator>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <string>
#include "synth_info.hpp"
#include "curses_mw_miner.hpp"
#include "curses_mw_ui.hpp"

using std::cout;
using std::endl;
using std::ifstream;
using std::string;
using std::exception;
using std::getenv;

int main(int argc, char *argv[])
{
	// Set up config filename
	string cfg_file_name;
	char *home_path = getenv("HOME");
	cfg_file_name = string(home_path) + string("/.mwsd.cfg");
	Curses_mw_ui my_ui; // The main UI object

	// Parse options on commandline and from config file
	// These flags are set, once values have been determined
	bool has_midi_in = false;
	bool has_midi_out = false;
	bool has_dev_id = false;
	try
	{
		po::options_description info_desc("Information options");
		info_desc.add_options()
			("help,h", "Show this help")
			("version,v","Show version information")
			("list_ports,l", "List available MIDI input and output ports")
			("config_file,c", po::value<string>(&cfg_file_name)->value_name("filename"), "Use a different configuration file")
		;
		po::options_description config_desc("Configuration options");
		config_desc.add_options()
			("input_port,i", po::value<string>()->value_name("port_name"), "Set the MIDI input port")
			("output_port,o", po::value<string>()->value_name("port_name"), "Set the MIDI output port")
			("io_ports,p", po::value<string>()->value_name("port_name"), "Set input and output MIDI ports.")
			("device_id,d", po::value<unsigned short int>()->value_name("ID"), "Set the device ID")
		;
		po::options_description commandline_desc;
		commandline_desc.add(info_desc).add(config_desc);
		po::variables_map vm;
		store(po::parse_command_line(argc,argv,commandline_desc), vm);
		notify(vm);

		if (vm.count("config_file"))
		{
			cfg_file_name = vm["config_file"].as<string>();
		}

		if (vm.count("io_ports"))
		{
			if (vm.count("input_port"))
			{
				cout << "ERROR:\nYou can't use the io_ports and input_port options together.\n";
				return 1;
			}
			if (vm.count("output_port"))
			{
				cout << "ERROR:\nYou can't use the io_ports and output_port options together.\n";
				return 1;
			}
		}

		ifstream cfg_file(cfg_file_name.c_str());
		if (cfg_file)
		{
			store(po::parse_config_file(cfg_file,config_desc), vm);
			notify(vm);
			cfg_file.close();
		}

		// print help
		if (vm.count("help"))
		{
			cout << PACKAGE_STRING << endl;
			cout << "Copyright (c) 2018 by Jeanette C.\n";
			cout << "Released under the GPL version 3.\n";
			cout << commandline_desc << endl;
			return 0;
		}

		if (vm.count("version"))
		{
			cout << PACKAGE_STRING << endl;
			cout << "Released 02.2018\n";
			return 0;
		}

		if (vm.count("list_ports"))
		{
			my_ui.list_ports();
			return 0;
		}

		if (vm.count("input_port"))
		{
			has_midi_in = my_ui.set_midi_input(vm["input_port"].as<string>());
		}

		if (vm.count("output_port"))
		{
			has_midi_out = my_ui.set_midi_output(vm["output_port"].as<string>());
			if (has_midi_out == true)
			{
			}
		}

		if (vm.count("io_ports"))
		{
			has_midi_in = my_ui.set_midi_input(vm["io_ports"].as<string>());
			has_midi_out = my_ui.set_midi_output(vm["io_ports"].as<string>());
		}

		if (vm.count("device_id"))
		{
			my_ui.set_dev_id(vm["device_id"].as<unsigned short int>());
			has_dev_id = true;
		}
	}
	catch(exception& e)
	{
		cout << "ERROR:\n" << e.what() << endl;
		return 1;
	}
	catch(...)
	{
		cout << "ERROR:\nCaught unknown exception.\n";
		return 1;
	}

	my_ui.set_cfg_file_name(cfg_file_name);

	my_ui.init_ui();

	// Interactively query missing information
	if ((has_midi_in == false) && (has_midi_out == false))
	{
		bool ret = my_ui.probe_synth();
		if (ret == false)
		{
			if (my_ui.get_error() == true)
			{
				my_ui.shut_ui();
				cout << "ERROR:\nAn error occured during automatic synth detection.\n";
				cout << my_ui.get_error_msg() << endl;
				return 1;
			}
		}
		else // Synth probe successful
		{
			has_midi_in = true;
			has_midi_out = true;
			has_dev_id = true;
		}
	}
	if (has_midi_in == false)
	{
		has_midi_in = my_ui.change_port('i');
		if (has_midi_in == false)
		{
			my_ui.shut_ui();
			cout << "ERROR:\n";
			cout << "Couldn't set MIDI input port.\n";
			if (my_ui.get_error() == true)
			{
				cout << my_ui.get_error_msg() << endl;
			}
			return 1;
		}
	}

	if (has_midi_out == false)
	{
		has_midi_out = my_ui.change_port('o');
		if (has_midi_out == false)
		{
			my_ui.shut_ui();
			cout << "ERROR:\n";
			cout << "Couldn't set MIDI output port.\n";
			if (my_ui.get_error() == true)
			{
				cout << my_ui.get_error_msg() << endl;
			}
			return 1;
		}
	}

	if (has_dev_id == false)
	{
		my_ui.change_dev_id();
	}

	my_ui.run();

	my_ui.shut_ui();
	if (my_ui.get_error() == true)
	{
		cout << "ERROR:\n" << my_ui.get_error_msg() << endl;
		return 1;
	}
	return 0;
}
