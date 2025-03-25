# obci_brainflow_lsl.py
"""
Author: Marcin Lesniak @marles77 https://github.com/marles77/openbci-brainflow-lsl
Open BCI + BrainFlow + LSL
Use BrainFlow to read data from Open BCI board and send it as LSL streams.
This program is based on a script originally created by Richard Waltman  @retiutut
(OpenBCI): https://github.com/OpenBCI/OpenBCI_GUI/tree/master/Networking-Test-Kit/LSL

Install dependencies with:
pip install --upgrade numpy brainflow pylsl pyserial PyYAML

This has only been tested with Cyton + Dongle and Cyton + Daisy + Dongle, for now.

Use:
python obci_brainflow_lsl.py --set settings8.yml
"""

import yaml
import os
import sys
import time
import numpy as np
import pandas as pd
import threading
import subprocess
import brainflow
from brainflow.board_shim import BoardShim, BrainFlowInputParams
from pylsl import StreamInfo, StreamOutlet, local_clock

if sys.platform.lower() == "win32":
    os.system("color")

# ==== constants ====

CRED = "\033[91m"
CGREEN = "\33[32m"
CYELLOW = "\33[33m"
CEND = "\033[0m"
CBOARD1 = "\033[96m"  # Cyan for board 1
CBOARD2 = "\033[94m"  # Blue for board 2
# default openbci channel commands (to avoid reflushing the board)
OBCI_COMMANDS = (
    "x1060110X",
    "x2060110X",
    "x3060110X",
    "x4060110X",  # channels 1-4   /Cyton
    "x5060110X",
    "x6060110X",
    "x7060110X",
    "x8060110X",  # channels 5-8   /Cyton
    "xQ060110X",
    "xW060110X",
    "xE060110X",
    "xR060110X",  # channels 9-12  /Daisy
    "xT060110X",
    "xY060110X",
    "xU060110X",
    "xI060110X",
)  # channels 13-16 /Daisy
BOARD_N_CHANNELS = {0: 8, 0: 8, 5: 8, 2: 16, 6: 16}  # Cyton or Cyton + Daisy
ALLOWED_DATA_TYPES = ["EXG", "AUX"]
REQUIRED_ARGS = ("board_id", "name", "data_type", "channel_names", "uid", "max_time")
REQUIRED_ARDUINO = ("name", "type", "channel_count", "channel_format", "source_id")
MARKER = {
    "1": 11,
    "2": 22,
}  # mapping of external markers; can add more but avoid '0' and '7'

# ==== auxiliary functions ====


def channel_select(board, board_id, data_type):
    # keys in switcher must correspond to ALLOWED_DATA_TYPES constant
    switcher = {
        "EXG": board.get_exg_channels(board_id),
        "AUX": board.get_analog_channels(board_id),
    }
    return switcher.get(data_type, "error")


def read_settings(file_name):
    """Reads args and board commands from settings file"""

    with open(file_name) as file:
        data = yaml.safe_load(file)

    if data:
        print(CGREEN + "Settings read successfully" + CEND)
        return data
    else:
        print(CRED + "Failed to read settings form file" + CEND)
        return None


def user_choice(prompt, boards=None, thread_initiated=False):
    """Awaits user's choice (yes or quit).
    This function can be used to give the user some control"""
    global bridge_process

    while True:
        try:
            user_res = input(CYELLOW + prompt + CEND)
            if (
                (user_res == "y")
                and (not thread_initiated)
                and (not stop_event.is_set())
            ):
                break
            if user_res == "q":
                # Set the stop event immediately to prevent double-quit requirement
                stop_event.set()  # message for threads to stop
                print("Stopping streams...")
                time.sleep(1)

                if boards:
                    try:
                        for board in boards:
                            board.stop_stream()
                    except brainflow.board_shim.BrainFlowError:
                        print(CRED + "Board is not streaming" + CEND)
                    for board in boards:
                        board.release_session()

                # Terminate the LSL-to-OSC bridge if it's running
                if bridge_process is not None:
                    try:
                        print(CYELLOW + "Shutting down LSL-to-OSC bridge..." + CEND)
                        # Use more force to terminate on Windows
                        if sys.platform.lower() == "win32":
                            import signal

                            try:
                                bridge_process.send_signal(signal.CTRL_C_EVENT)
                                time.sleep(0.5)  # Give it a moment to handle CTRL_C
                                if bridge_process.poll() is None:  # Still running
                                    bridge_process.terminate()
                                    time.sleep(0.5)  # Give it a moment to terminate
                                    if bridge_process.poll() is None:  # Still running
                                        bridge_process.kill()  # Force kill as last resort
                            except Exception:
                                bridge_process.kill()  # Force kill if the above fails
                        else:
                            bridge_process.terminate()

                        bridge_process.wait(
                            timeout=2
                        )  # Reduced timeout for faster exit
                        print(
                            CGREEN + "LSL-to-OSC bridge shut down successfully." + CEND
                        )
                    except Exception as e:
                        print(CRED + f"Error shutting down bridge: {e}" + CEND)
                        # Force kill as a last resort
                        try:
                            bridge_process.kill()
                        except:
                            pass

                print("The end")
                sys.exit(0)  # Exit immediately to prevent hanging
            else:
                continue
        except KeyboardInterrupt:
            # Handle Ctrl+C the same as 'q'
            print("\nDetected Ctrl+C, exiting...")
            stop_event.set()
            sys.exit(0)


def default_chan_commands(board_id, chan_commands=None):
    """Creates a dictionary of default commands for specified board"""

    if not chan_commands:
        n_channels = BOARD_N_CHANNELS[board_id]
        chan_commands = {
            "chan" + str(num + 1): OBCI_COMMANDS[num] for num in range(0, n_channels)
        }
        return chan_commands


def manage_settings_data(data):
    """Reads and manages settings data"""

    args = data.get("args", None)
    chan_commands = data.get("commands", None)
    if args:
        # manage missing required args

        missing_args = [p for p in REQUIRED_ARGS if args.get(p) == None]
        if missing_args:
            print(
                CRED + "Missing args:" + CEND,
                "\n",
                ", ".join(missing_args),
                "\nThe end",
                sep="",
            )
            sys.exit()
        if args["board_id"] not in BOARD_N_CHANNELS:
            print(CRED + "Unsupported board" + CEND, "\nThe end", sep="")
            sys.exit()
        # allowed data types check
        if not all(type in ALLOWED_DATA_TYPES for type in args["data_type"]):
            print(
                CRED
                + f"Not allowed data type/s in settings. Allowed data types: "
                + f"{', '.join(ALLOWED_DATA_TYPES)}"
                + CEND
                + "\nThe end"
            )
            sys.exit()
        # yaml does not support empty strings, so None values have to be converted
        args["ip_address"] = args["ip_address"] if args.get("ip_address") else ""
        args["ip_port"] = args["ip_port"] if args.get("ip_port") else ""
        args["streamer_params"] = (
            args["streamer_params"] if args.get("streamer_params") else ""
        )
        args["serial_port"] = args["serial_port"] if args.get("serial_port") else ""
        args["master_id"] = args["master_id"] if args.get("master_id") else "2"
    else:
        print(CRED + "No args" + CEND + "\nThe end")
        sys.exit()

    if not chan_commands:
        # setting default chan commands for Cyton (+ Daisy); alternatively command 'd' can be sent to board
        print(CYELLOW + "No commands. Using default." + CEND, "\nThe end", sep="")
        chan_commands = default_chan_commands(args["board_id"])

    return args, chan_commands


def collect_cont(board, args, srate, outlet, fw_delay):
    """Collects continuous data with brainflow and sends it via LSL"""

    chans = {}
    sent_samples = {}
    data = {}
    mychunk = {}
    for type in args["data_type"]:
        chans[type] = channel_select(
            board=board, board_id=args["board_id"], data_type=type
        )
        sent_samples[type] = 0

    print(CGREEN + "Now sending data from board..." + CEND)
    start_time = local_clock()

    while not stop_event.is_set():
        # continuous data collection
        elapsed_time = local_clock() - start_time
        data_from_board = board.get_board_data()

        for type in args["data_type"]:
            required_samples = int(srate[type] * elapsed_time) - sent_samples[type]
            if required_samples > 0:
                data[type] = data_from_board[chans[type]]
                mychunk[type] = []
                for i in range(len(data[type][0])):
                    mychunk[type].append(data[type][:, i].tolist())
                stamp = local_clock() - fw_delay[type]
                outlet[type].push_chunk(mychunk[type], stamp)
                sent_samples[type] += required_samples

        if elapsed_time > args["max_time"]:
            stop_event.set()  # message for threads to stop
            print(
                CRED
                + "\nTime limit reached! Data collection has been stopped."
                + CEND
                + CYELLOW
                + "\nPress 'q' + ENTER to exit\n--> "
                + CEND,
                end="",
            )

        time.sleep(1)

    # print("Stopped collecting data")


# Add a global variable to store the bridge process
bridge_process = None


def kill_existing_bridges():
    """Find and kill any existing bridge processes."""
    try:
        print(CYELLOW + "Checking for existing bridge processes..." + CEND)
        import psutil

        bridge_killed = False
        for proc in psutil.process_iter(["pid", "name", "cmdline"]):
            try:
                cmdline = proc.info["cmdline"]
                if cmdline and len(cmdline) > 1 and "bridge.py" in cmdline[-1]:
                    print(
                        CYELLOW
                        + f"Found existing bridge process (PID: {proc.info['pid']}), terminating..."
                        + CEND
                    )
                    psutil.Process(proc.info["pid"]).terminate()
                    bridge_killed = True
            except (psutil.NoSuchProcess, psutil.AccessDenied, psutil.ZombieProcess):
                pass

        if bridge_killed:
            # Give processes time to terminate
            time.sleep(2)
            print(CGREEN + "Existing bridge processes terminated." + CEND)
        else:
            print(CGREEN + "No existing bridge processes found." + CEND)

        return bridge_killed
    except ImportError:
        print(
            CYELLOW + "psutil not installed, cannot check for existing bridges." + CEND
        )
        return False
    except Exception as e:
        print(CRED + f"Error checking for existing bridges: {e}" + CEND)
        return False


def start_bridge():
    """Start the LSL-to-OSC bridge in a separate process."""
    global bridge_process
    try:
        # Kill any existing bridge processes first
        kill_existing_bridges()

        bridge_path = os.path.join(
            os.path.dirname(os.path.dirname(os.path.abspath(__file__))),
            "lsl-to-osc-bridge",
            "src",
            "bridge.py",
        )

        # Check if bridge.py exists at the expected path
        if not os.path.exists(bridge_path):
            print(CRED + f"Bridge script not found at {bridge_path}" + CEND)
            return False

        print(CGREEN + "Starting LSL-to-OSC bridge..." + CEND)
        # Run the bridge script in a separate process
        bridge_process = subprocess.Popen([sys.executable, bridge_path])

        # Give the bridge a moment to start and verify it's running
        time.sleep(2)
        if bridge_process.poll() is not None:  # Process has terminated
            print(CRED + "Bridge process terminated unexpectedly." + CEND)
            return False

        print(
            CGREEN
            + f"Bridge started successfully with PID: {bridge_process.pid}"
            + CEND
        )
        return True
    except Exception as e:
        print(CRED + f"Error starting bridge: {e}" + CEND)
        return False


# ==== main function ====


def main(argv):
    """Takes args and initiates streaming"""

    if argv:
        # manage settings read form a yaml file
        if argv[0] == "--set":
            file_settings_OBCI_1 = argv[1]
            data_OBCI_1 = read_settings(file_settings_OBCI_1)
            if not data_OBCI_1:
                print("The end")
                sys.exit()
            args_OBCI_1, chan_commands_OBCI_1 = manage_settings_data(data_OBCI_1)

            file_settings_OBCI_2 = argv[2]
            data_OBCI_2 = read_settings(file_settings_OBCI_2)
            if not data_OBCI_2:
                print("The end")
                sys.exit()
            args_OBCI_2, chan_commands_OBCI_2 = manage_settings_data(data_OBCI_2)

    else:
        print(CRED + "Use --set *.yml to load settings" + CEND, "\nThe end", sep="")
        sys.exit()

    ser = None

    user_choice("Initiate? 'y' -> yes, 'q' -> quit\n--> ")

    BoardShim.enable_dev_board_logger()

    # brainflow initialization
    params_OBCI_1 = BrainFlowInputParams()
    params_OBCI_1.serial_port = args_OBCI_1["serial_port"]
    params_OBCI_1.ip_address = args_OBCI_1["ip_address"]
    board_OBCI_1 = BoardShim(args_OBCI_1["board_id"], params_OBCI_1)

    params_OBCI_2 = BrainFlowInputParams()
    params_OBCI_2.serial_port = args_OBCI_2["serial_port"]
    params_OBCI_2.ip_address = args_OBCI_2["ip_address"]
    board_OBCI_2 = BoardShim(args_OBCI_2["board_id"], params_OBCI_2)

    # LSL internal outlet (stream form board) configuration and initialization
    channel_names_OBCI_1 = {}
    n_channels_OBCI_1 = {}
    srate_OBCI_1 = {}
    info_OBCI_1 = {}
    outlet_int_OBCI_1 = {}
    fw_delay_OBCI_1 = {}
    for type in args_OBCI_1["data_type"]:
        channel_names_OBCI_1[type] = args_OBCI_1["channel_names"][type].split(",")
        n_channels_OBCI_1[type] = len(channel_names_OBCI_1[type])
        srate_OBCI_1[type] = board_OBCI_1.get_sampling_rate(args_OBCI_1["board_id"])
        # Create more descriptive stream names with participant identifiers
        name_OBCI_1 = f"userA_{args_OBCI_1['name']}_{type}"
        uid = args_OBCI_1["uid"] + "_" + type + "_" + args_OBCI_1["serial_port"]
        info_OBCI_1[type] = StreamInfo(
            name_OBCI_1,
            type,
            n_channels_OBCI_1[type],
            srate_OBCI_1[type],
            "double64",
            uid,
        )
        # add channel labels
        chans_OBCI_1 = info_OBCI_1[type].desc().append_child("channels")
        for label in channel_names_OBCI_1[type]:
            chan_OBCI_1 = chans_OBCI_1.append_child("channel")
            chan_OBCI_1.append_child_value("label", label)
        outlet_int_OBCI_1[type] = StreamOutlet(info_OBCI_1[type])
        fw_delay_OBCI_1[type] = args_OBCI_1["delay"]

    # prepare session; exit if board is not ready
    try:
        board_OBCI_1.prepare_session()
    except brainflow.board_shim.BrainFlowError as e:
        print(CRED + f"Error: {e}" + CEND)

        print("The end")
        time.sleep(1)
        sys.exit()

    # LSL internal outlet (stream form board) configuration and initialization
    channel_names_OBCI_2 = {}
    n_channels_OBCI_2 = {}
    srate_OBCI_2 = {}
    info_OBCI_2 = {}
    outlet_int_OBCI_2 = {}
    fw_delay_OBCI_2 = {}
    for type in args_OBCI_2["data_type"]:
        channel_names_OBCI_2[type] = args_OBCI_2["channel_names"][type].split(",")
        n_channels_OBCI_2[type] = len(channel_names_OBCI_2[type])
        srate_OBCI_2[type] = board_OBCI_2.get_sampling_rate(args_OBCI_2["board_id"])
        # Create more descriptive stream names with participant identifiers
        name_OBCI_2 = f"userB_{args_OBCI_2['name']}_{type}"
        uid = args_OBCI_2["uid"] + "_" + type + "_" + args_OBCI_2["serial_port"]
        info_OBCI_2[type] = StreamInfo(
            name_OBCI_2,
            type,
            n_channels_OBCI_2[type],
            srate_OBCI_2[type],
            "double64",
            uid,
        )
        # add channel labels
        chans_OBCI_2 = info_OBCI_2[type].desc().append_child("channels")
        for label in channel_names_OBCI_2[type]:
            chan_OBCI_2 = chans_OBCI_2.append_child("channel")
            chan_OBCI_2.append_child_value("label", label)
        outlet_int_OBCI_2[type] = StreamOutlet(info_OBCI_2[type])
        fw_delay_OBCI_2[type] = args_OBCI_2["delay"]

    # prepare session; exit if board is not ready
    try:
        board_OBCI_2.prepare_session()
    except brainflow.board_shim.BrainFlowError as e:
        print(CRED + f"Error: {e}" + CEND)

        print("The end")
        time.sleep(1)
        sys.exit()

    user_choice(
        "Send commands to board? 'y' -> yes, 'q' -> quit\n--> ",
        boards=[board_OBCI_1, board_OBCI_2],
    )

    # iterate over channel commands, send one and wait for a response from board
    # to restore default channel settings 'd' can be sent
    print("Configuring Board 1:")
    # print(chan_commands_OBCI_1)
    for chan, command in chan_commands_OBCI_1.items():
        res_string = board_OBCI_1.config_board(command)
        time.sleep(0.1)
        if res_string.find("Success") != -1:
            res = CGREEN + res_string + CEND
        else:
            res = CRED + res_string + CEND
        print(f"Response from {chan}: {res}")

    print("Configuring Board 2:")
    print(chan_commands_OBCI_2)
    for chan, command in chan_commands_OBCI_2.items():
        res_string = board_OBCI_2.config_board(command)
        time.sleep(0.1)
        if res_string.find("Success") != -1:
            res = CGREEN + res_string + CEND
        else:
            res = CRED + res_string + CEND
        print(f"Response from {chan}: {res}")

    # show stream configuration and wait until user accepts or quits
    print(50 * "-")
    for type in args_OBCI_1["data_type"]:
        print(
            f"{type}:\nNumber of channels: {n_channels_OBCI_1[type]}\nSampling rate: {srate_OBCI_1[type]}\n"
            f"Time limit: {args_OBCI_1['max_time'] // 60} min. {args_OBCI_1['max_time'] % 60} sec.\n"
        )

    for type in args_OBCI_2["data_type"]:
        print(
            f"{type}:\nNumber of channels: {n_channels_OBCI_2[type]}\nSampling rate: {srate_OBCI_2[type]}\n"
            f"Time limit: {args_OBCI_2['max_time'] // 60} min. {args_OBCI_2['max_time'] % 60} sec.\n"
        )

    # Ask user if they want to start the LSL-to-OSC bridge BEFORE starting the stream
    bridge_prompt = "Do you want to start the LSL-to-OSC bridge after streaming begins? 'y' -> yes, 'n' -> no\n--> "

    # Use direct input instead of user_choice to avoid conflicts
    start_bridge_after = False
    user_res = input(CYELLOW + bridge_prompt + CEND)
    if user_res.lower() == "y":
        start_bridge_after = True
        print(CGREEN + "Bridge will start after streaming begins." + CEND)
    else:
        print(CYELLOW + "Bridge will not be started." + CEND)

    # Now ask to start the streaming
    user_choice(
        "Start streaming? 'y' -> yes, 'q' -> quit\n--> ",
        boards=[board_OBCI_1, board_OBCI_2],
    )

    # board starts streaming
    board_OBCI_1.start_stream(45000, args_OBCI_1["streamer_params"])

    board_OBCI_2.start_stream(45000, args_OBCI_2["streamer_params"])
    time.sleep(1)

    # Start the data collection threads
    thread_cont_OBCI_1 = threading.Thread(
        target=collect_cont,
        args=[
            board_OBCI_1,
            args_OBCI_1,
            srate_OBCI_1,
            outlet_int_OBCI_1,
            fw_delay_OBCI_1,
        ],
    )
    thread_cont_OBCI_1.daemon = (
        True  # Mark thread as daemon so it exits with main program
    )
    thread_cont_OBCI_1.start()

    thread_cont_OBCI_2 = threading.Thread(
        target=collect_cont,
        args=[
            board_OBCI_2,
            args_OBCI_2,
            srate_OBCI_2,
            outlet_int_OBCI_2,
            fw_delay_OBCI_2,
        ],
    )
    thread_cont_OBCI_2.daemon = (
        True  # Mark thread as daemon so it exits with main program
    )
    thread_cont_OBCI_2.start()

    # Add a short sleep to ensure streams are fully started
    time.sleep(2)

    # Only start the bridge if explicitly requested earlier
    if start_bridge_after and not stop_event.is_set():
        bridge_started = start_bridge()
        if bridge_started:
            print(CGREEN + "LSL-to-OSC bridge started successfully." + CEND)
        else:
            print(CRED + "Failed to start LSL-to-OSC bridge." + CEND)

    # Wait for stop message from user while running data collecting threads
    if not stop_event.is_set():
        user_choice(
            "To stop streaming and quit press 'q' + ENTER\n--> ",
            boards=[board_OBCI_1, board_OBCI_2],
            thread_initiated=True,
        )

    print(f"{CBOARD1}Processing data from User A...{CEND}")
    print(f"{CBOARD2}Processing data from User B...{CEND}")


# ==== start the program ====
if __name__ == "__main__":
    stop_event = threading.Event()
    main(sys.argv[1:])
