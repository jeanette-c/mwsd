/* main.cpp - main program of the mwsd (MicroWave Synthesizer Display)
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

#include "config.h"
#include <boost/program_options.hpp>
namespace po = boost::program_options;
#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;
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
	string res_dir; // path to resource folder
	char *home_path = getenv("HOME");
	res_dir = string(home_path) + string("/.mwsd");
	cfg_file_name = string(home_path) + string("/.mwsd.cfg");
	Curses_mw_ui my_ui(res_dir); // The main UI object

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
			("config_file,c", po::value<string>()->value_name("filename"), "Use a different configuration file")
		;
		po::options_description config_desc("Configuration options");
		config_desc.add_options()
			("input_port,i", po::value<string>()->value_name("port_name"), "Set the MIDI input port")
			("output_port,o", po::value<string>()->value_name("port_name"), "Set the MIDI output port")
			("io_ports,p", po::value<string>()->value_name("port_name"), "Set input and output MIDI ports.")
			("device_id,d", po::value<unsigned short int>()->value_name("ID"), "Set the device ID")
			("resource_folder,r", po::value<string>()->value_name("path"), "Set a different resource folder")
		;
		po::options_description commandline_desc;
		commandline_desc.add(info_desc).add(config_desc);
		po::variables_map vm;
		store(po::parse_command_line(argc,argv,commandline_desc), vm);
		notify(vm);

		if (vm.count("resource_folder"))
		{
			res_dir = vm["resource_folder"].as<string>();
			my_ui.set_res_dir(res_dir);
		}

		if (vm.count("config_file"))
		{
			cfg_file_name = vm["config_file"].as<string>();
			fs::path cfg_file_cmd_path(cfg_file_name);
			if (fs::exists(cfg_file_cmd_path))
			{
				if (!fs::is_regular_file(cfg_file_cmd_path))
				{
					cout << "ERROR:\nThe specified configuration file " << cfg_file_name << " exists, but is not a regular file.\n";
					return 1;
				}
			}
			else // the path doesn't exist at all
			{
				cout << "ERROR:\nThe specified configuration file " << cfg_file_name << " does not exist.\n";
				return 1;
			}
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
			cout << "Copyright (c) 2018-2020 by Jeanette C.\n";
			cout << "Released under the GPL version 3.\n";
			cout << commandline_desc << endl;
			return 0;
		}

		if (vm.count("version"))
		{
			cout << PACKAGE_STRING << endl;
			cout << "\t(Released 04.2020)\n";
			cout << "Copyright (c) 2018-2020 by Jeanette C.\n";
			cout << "This is free software released under the terms of the GPL version 3.\n";
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
			my_ui.set_dev_id(static_cast<unsigned char>(vm["device_id"].as<unsigned short int>()));
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

	// Set resource and config file name Create resource dir, if it doesn't exist.
	my_ui.set_res_dir(res_dir);
	my_ui.set_cfg_file_name(cfg_file_name);
	fs::path res_path(res_dir);
	if (!fs::exists(res_path))
	{
		if (!fs::create_directory(res_path))
		{
			cout << "ERROR:\nCould not create resource folder " << res_dir << "\n";
			return 1;
		}
	}
	else // the resource path exists
	{
		if (!fs::is_directory(res_path))
		{
			cout << "ERROR:\nThe name of the resource folder " << res_dir << " exists,\nbut is no directory.\n";
			return 1;
		}
		else
		{
		}
	}

	// Now check for subfolders of resource directory
	bool ret = my_ui.check_res_dir();
	if (ret == false)
	{
		cout << "ERROR:\n" << my_ui.get_error_msg() << endl;
		return 1;
	}

	// Initialise the UI to continue with the interactive stuff
	my_ui.init_ui();

	// Interactively query missing information
	if ((has_midi_in == false) && (has_midi_out == false))
	{
		ret = my_ui.probe_synth();
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

	// If config file doesn't exist, write it now
	fs::path cfg_file_path(cfg_file_name);
	if (!fs::exists(cfg_file_path))
	{
		my_ui.write_cfg();
	}

	my_ui.run();

	my_ui.shut_ui();
	cout << "\33c";
	if (my_ui.get_error() == true)
	{
		cout << "ERROR:\n" << my_ui.get_error_msg() << endl;
		return 1;
	}
	return 0;
}
