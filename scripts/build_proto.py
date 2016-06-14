""" Python script to stage firmware files for building"""
import os
import sys
import subprocess
import glob
import datetime
import shutil
import fnmatch

#Script constants
required_programs = ["protoc", "ino"]
build_dir = "mktemp/"
firmware_source_dir = "../OpenROV"
plugin_source_dir = "../cockpit/"
nanopb_dir = "../libs/nanopb/"
nanopb_compiler = os.path.join(nanopb_dir, "generator/protoc-gen-nanopb")
master_proto_filename = "Master.proto"

arduino_firmware_prefix = "ArduinoPlugin_"
#Colors for debug output
class ConsoleColors:
    HEADER = '\033[95m'
    OKBLUE = '\033[94m'
    OKGREEN = '\033[92m'
    WARNING = '\033[93m'
    FAIL = '\033[91m'
    ENDC = '\033[0m'
    BOLD = '\033[1m'
    UNDERLINE = '\033[4m'

#Status for debug
class DebugStatus:
    INFO = ConsoleColors.OKGREEN + "[INFO]"
    FATAL_ERROR = ConsoleColors.FAIL + "[FATAL_ERROR]"
    WARNING = ConsoleColors.WARNING + "[WARNING]"



#Helper functions
def get_current_time():
    """
        Returns the current system time

        @rtype: string
        @return: The current time, formatted
    """
    return str(datetime.datetime.now().strftime("%H:%M:%S"))

def print_status(type, message):
    """
        Prints a log message to the console
    """
    debug_status = type + " " + get_current_time() + " " + message + ConsoleColors.ENDC
    print(debug_status)

def is_program_installed(program):
    """
        Checks to see if program is installed on the linux system

        @type program: string
        @param program: The program name
        @rtype: bool
        @:return: true if the program is installed, false if not
    """
    try:
        output = subprocess.check_output(["which", program])
    except subprocess.CalledProcessError:
            #Subprocess was unable to find the program
            print_status(DebugStatus.FATAL_ERROR, ("%s not found. Install %s to continue!" % (program, program)))
            return False

    #Else, we found the program. Return true
    print_status(DebugStatus.INFO,("Found %s" % program))
    return True

def check_for_required_programs():
    """
        Iterates through the required program list and checks to see if they are installed

        @:return: Aborts the script if a required program is not found
    """
    for program in required_programs:
        if(is_program_installed(program) is False):
            sys.exit("Aborting")

def initalize_build_directory():
    """
        Creates a directory for the build
        Calls ino to initalize the directory
        Deletes boilerplate ino code

        @:return: Aborts the script if the build directory cannot be intialized
    """
    #First, check if directory exists, if not, create it
    if not os.path.exists(build_dir):
        print_status(DebugStatus.INFO, "Creating build directory")
        os.makedirs(build_dir)
    else:
        #If the directory does exist, we blast it
        print_status(DebugStatus.WARNING, "Temporary build directory already exists. Deleting.")
        shutil.rmtree(build_dir)
        print_status(DebugStatus.INFO, "Creating build directory")
        os.makedirs(build_dir)


    #Then, call ino to intialize the directory
    init_process = subprocess.Popen(["ino", "init"], cwd=build_dir)
    print_status(DebugStatus.INFO, "Intializing build directory")
    #Wait for this process to finish to continue
    init_process.communicate()

    #Finally, delete all boildplate ino code
    for parent_dir, dir_names, file_names in os.walk(build_dir):
        for file in file_names:
            if file.endswith(".ino"):
                os.remove(os.path.join(parent_dir,file))

def stage_source_files():
    """
        Copies the firmware source files to the build directory to build

        @:return:
    """
    print_status(DebugStatus.INFO, "Staging firmware source files")
    for firmware_source_file in os.listdir(firmware_source_dir):
        full_filepath = os.path.join(firmware_source_dir, firmware_source_file)
        if os.path.isfile(full_filepath):
            shutil.copy(full_filepath, build_dir + "src")
    print_status(DebugStatus.INFO, "Firmware source files sucessfully staged")


def get_arduino_plugin_name(arduino_plugin_path):
    """
        Strips the full path and the extension for a given arduino plugin

        @type arduino_plugin_path: string
        @param arduino_plugin_path: The fullpath to the arduino plugin directory
        @rtype: string
        @:return: the basename of the arduino plugin
    """
    #First, strip the extension
    #And only keep the basename
    basename = os.path.basename(os.path.splitext(arduino_plugin_path)[0])

    #And remove the prefix
    basename = basename.replace(arduino_firmware_prefix, "")
    return basename

def stage_plugin_files():
    """
        Copies the plugin source files to the build directory to build

        TODO: This function is too long. Break into sub functions

        @:return:
    """
    arduino_plugins = []

    #Create a blank PluginConfig.h with header guards
    plugin_config_filename = build_dir + "src/" + "PluginConfig.h"
    plugin_header_guards = ["#ifndef __PLUGINSCONFIG_H_", "#define __PLUGINSCONFIG_H_", "#endif"]
    with open(plugin_config_filename, "a") as plugin_header_file:
        plugin_header_file.write("\n %s \n" % plugin_header_guards[0])
        plugin_header_file.write("%s \n" % plugin_header_guards[1])
    plugin_header_file.close()

    #Find the plugin files
    print_status(DebugStatus.INFO, "Staging plugin files")
    print_status(DebugStatus.INFO, "Searching %s for arduino plugin files" % plugin_source_dir)
    for parent_dir, dir_names, file_names in os.walk(plugin_source_dir):
        for file in file_names:
            if file.startswith(arduino_firmware_prefix):
                fullpath = os.path.join(parent_dir, file)
                arduino_plugins.append(fullpath)

    #Make the directories in the build folder and copy the source files over
    for plugin_header in arduino_plugins:
        arduino_basename = get_arduino_plugin_name(plugin_header)
        arduino_plugin_source_dir = build_dir + "src/" + arduino_basename

        print_status(DebugStatus.INFO, ("Creating directory for: %s in %s" % (arduino_basename, arduino_plugin_source_dir)))
        if not os.path.exists(arduino_plugin_source_dir):
            os.makedirs(arduino_plugin_source_dir)

        #Copy the source files over
        source_file_dir = os.path.dirname(plugin_header)
        source_files = os.listdir(source_file_dir)
        for source_file in source_files:
            print_status(DebugStatus.INFO, "Copying file: %s to %s" % (source_file, arduino_plugin_source_dir))
            shutil.copy(os.path.join(source_file_dir, source_file), arduino_plugin_source_dir)

        #Add the includes to the PluginConfig.h file
        with open(plugin_config_filename, "a") as plugin_header_file:
            include_line = "#include \"" + arduino_basename + "/" + arduino_firmware_prefix + arduino_basename + ".h\""
            plugin_header_file.write("%s \n" % include_line)
        plugin_header_file.close()

    #Add the last header file guard
    with open(plugin_config_filename, "a") as plugin_header_file:
        plugin_header_file.write("%s \n" % plugin_header_guards[2])
    plugin_header_file.close()

def add_protofile_to_master(protofile, dest_file):
    """
        Adds the contents to a proto file to the master for one compilation step

        @type protofile: string
        @param protofile: The fullpath to the protofile
    """
    with open(protofile) as protofile:
        with open(dest_file, "a") as master:
            master.write("\n")
            for line in protofile:
                if not "syntax" in line:
                    master.write(line)
        master.close()
    protofile.close()


def stage_proto_files():
    """
        Consolidates all of the protofiles into one proto file to then build using nanopb

        @:return:
    """
    #Copy the proto "library" to the build directory
    files_in_nanopb_dir = os.listdir(nanopb_dir)
    for file in files_in_nanopb_dir:
        if file.startswith("pb"):
            shutil.copy(os.path.join(nanopb_dir, file), build_dir+"src/")

    #Create a directory for the generated header and implmentation file
    generated_dir_path = build_dir + "generated/"
    os.mkdir(generated_dir_path)

    #Create the master proto file
    with open(generated_dir_path + master_proto_filename, "a") as master:
        master.write("syntax = \"proto2\";")
    master.close()

    #Search the proto file directory for proto files. Get a list.
    for parent_dir, directories, files in os.walk(plugin_source_dir):
        for file in files:
            if file.endswith(".proto"):
                print_status(DebugStatus.INFO, "Found proto file: %s " % file)
                #Write the contents of the file to the master proto file
                add_protofile_to_master(os.path.join(parent_dir, file), generated_dir_path + master_proto_filename)
    print_status(DebugStatus.INFO, "Done adding proto files to master")

def compile_proto_files():
    """
        Compiles the proto files using the nanopb compiler

        @:return:
    """
    print_status(DebugStatus.INFO, "Compiling Master.proto...")

    #Build vars
    #NOTE: Need to go up two levels to get to the nanopb compiler for this MVP script
    protoc_plugin_arg = "--plugin=protoc-gen-nanopb=" + "../../" + nanopb_compiler
    protoc_output_directives = "--nanopb_out=./"

    compile_process = subprocess.Popen(["protoc", protoc_plugin_arg, protoc_output_directives, master_proto_filename], cwd=build_dir + "generated/")

    #Wait for this process to finish to continue
    compile_process.communicate()
    print_status(DebugStatus.INFO, "Finished compiling proto file.")

    #Copy the generated implementation and header file out and into the src directory
    print_status(DebugStatus.INFO, "Copying the compiled proto files to be staged")
    generated_dir = build_dir + "generated/"
    for file in os.listdir(generated_dir):
        if not file.endswith(".proto"):
            shutil.copy(os.path.join(generated_dir, file), build_dir + "src/")

def build_firmware():
    """
        Builds the firmware for upload to the arduino using ino

        @:return:
    """
    print_status(DebugStatus.INFO, "Starting final build step")

    board = "mega2560"

    build_process = subprocess.Popen(["ino", "build", "-m" + board], cwd=build_dir)
    build_process.communicate()


def main(argv):
    """Main entry point of the script"""

    #Check if required programs are installed
    check_for_required_programs()

    #Initialize the firmware build directory
    initalize_build_directory()

    #Copy firmware to build directory
    stage_source_files()

    #Copy necessary plugin files to build directory
    stage_plugin_files()

    #Copy proto library over to build directory. Generate master protofile to then build
    stage_proto_files()

    #Compile the master proto file and move it to the source directory
    compile_proto_files()

    #And then build the whole project
    build_firmware()


if __name__ == '__main__':
    sys.exit(main(sys.argv))