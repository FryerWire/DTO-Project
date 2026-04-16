"""
computations.py
DTO Computation Script - Trajectory Visualization and Telemetry Analysis

Features:
- Loads and validates a Keybind_Log.csv file produced by either DTOController.cpp or DTOManualTesting.cpp, verifying path existence and required column presence before any processing begins.
- Parses the Direction column of the log to reconstruct a six-degree-of-freedom trajectory by applying sequential rotation matrices and body-frame translation vectors, accumulating position and attitude state across every logged movement event.
- Generates segmented 3D trajectory plots in configurable time-interval chunks (default 30 seconds), each displayed interactively with per-axis color coding, time-tick markers at a fixed interval (default 5 seconds), and a synchronized legend; waits for user close before proceeding to the next chunk.
- Applies a two-phase movement model: translation events move the vehicle along a body-frame axis vector rotated into world frame by the current cumulative rotation matrix; rotation events update the cumulative rotation matrix via pre-multiplication of a freshly computed single-axis rotation matrix.
- Computes Euler angles (Roll, Pitch, Yaw in degrees) from the cumulative rotation matrix at each timestep using an atan2-based decomposition with a numerical singularity guard for near-zero sy values (gimbal lock fallback).
- Processes the full deduplicated and time-sorted dataset after chunk visualization to compute global absolute position arrays and attitude arrays for use in 2D time-series plots.
- Derives linear velocity (m/s per unit) and linear acceleration (m/s^2 per unit) from the global position arrays using numpy.gradient for central differencing with the actual timestamp array as the spacing argument.
- Generates four sequential interactive 2D Matplotlib plots covering Absolute Position (X/Y/Z vs time), Attitude (Roll/Pitch/Yaw vs time), Linear Velocity (Vx/Vy/Vz vs time), and Linear Acceleration (Ax/Ay/Az vs time), each waiting for user close before the next is displayed.
- Uses a structured try/except hierarchy with specific exception types (FileNotFoundError, KeyError, generic Exception) to catch and log distinct ERROR codes for file access failure, column parsing failure, mathematical errors, and unexpected general errors.
- Logs all script lifecycle events with wall-clock timestamps (HH:MM:SS.mmm) and standardized STATUS/ERROR codes to stdout via a unified log_event() function.

Functions:
- log_event(code, message): Formats the current wall-clock time as HH:MM:SS.mmm and prints a structured log line containing the timestamp, STATUS or ERROR code, and descriptive message to stdout.
- format_time(seconds): Converts a floating-point time in seconds to a "M:SS" minute-seconds string for use as chunk plot titles and axis tick labels.
- get_rotation_matrix(axis, theta): Returns a 3x3 numpy rotation matrix for a rotation of theta radians about the specified axis ('x' for Roll, 'y' for Pitch, 'z' for Yaw); returns the identity matrix for unrecognized axis strings.
- rotation_matrix_to_euler(R): Extracts Roll, Pitch, and Yaw Euler angles in degrees from a 3x3 rotation matrix using atan2 decomposition; applies a gimbal lock fallback when the sy component falls below 1e-6.

Codes:
- STATUS-000: Program Initialization Started
- STATUS-001: Session Started
- STATUS-002: Session Ended
- STATUS-003: Shutdown Successful
- STATUS-004: Log Directory Verified or Already Exists
- STATUS-005: Log Directory Created Successfully
- STATUS-006: Activity Log Opened and Header Written
- STATUS-007: Keybind Log Opened and Header Written
- STATUS-008: All Log Files Initialized with CSV Headers
- STATUS-009: Startup Successful: All Log Files Ready
- STATUS-010: Mode Changed
- STATUS-011: Key Registered
- STATUS-012: Startup Sequence Initiated
- STATUS-013: Startup Sequence Completed: All GPIO Activated
- STATUS-014: Rack Connector Test Started
- STATUS-015: Rack Connector Test Passed
- STATUS-016: GPIO Pin Activated (ON)
- STATUS-017: GPIO Pin Deactivated (OFF)
- STATUS-018: Partial GPIO Activation: Connection Issues Detected
- STATUS-019: UI Menu Refreshed
- STATUS-020: Operational Mode Activated
- STATUS-300: Script Initialization Started: log_event System Online
- STATUS-301: Configuration and Mapping Variables Loaded: step_increment and rot_increment Set
- STATUS-302: File Path Validation Successful: os.path.exists Confirmed
- STATUS-303: CSV Data Loaded into Pandas DataFrame: Row Count Verified
- STATUS-304: 3D Trajectory Chunk Generation Phase Started: Total Chunk Count Calculated
- STATUS-305: Processing Started for Specific 3D Chunk: Time Window Defined
- STATUS-306: 3D Chunk Skipped: Filtered DataFrame for Time Window is Empty
- STATUS-307: 3D Trajectory Plot Rendered and Displayed to User
- STATUS-308: 3D Trajectory Plot Closed by User: Proceeding to Next Chunk or Phase
- STATUS-309: Full Telemetry Data Processing Phase Started: Deduplication and Sort Applied
- STATUS-310: Global Trajectory Integration Complete: All Position and Attitude Arrays Built
- STATUS-311: Velocity and Acceleration Derivative Arrays Calculated via numpy.gradient
- STATUS-312: 2D Telemetry Plot Generation Phase Started
- STATUS-313: Absolute Position Plot (X/Y/Z vs Time) Displayed to User
- STATUS-314: Attitude Plot (Roll/Pitch/Yaw vs Time) Displayed to User
- STATUS-315: Linear Velocity Plot (Vx/Vy/Vz vs Time) Displayed to User
- STATUS-316: Linear Acceleration Plot (Ax/Ay/Az vs Time) Displayed to User
- STATUS-317: Script Execution Completed Successfully: All Plots Closed
- STATUS-318: Single-Axis Rotation Matrix Computed for Current Move
- STATUS-319: Euler Angles Extracted from Rotation Matrix: Roll/Pitch/Yaw in Degrees
- STATUS-320: Time-Tick Marker Placed on 3D Plot at Scheduled Interval
- STATUS-321: Final Absolute Last-Point Marker Placed on 3D Plot
- STATUS-322: 3D Plot Axis Limits Computed from Point Cloud Bounding Box
- STATUS-323: max_range Defaulted to 1.0: All Points Collapsed to Single Location
- STATUS-324: Rotation Move Processed: Current Rotation Matrix Updated by Pre-Multiplication
- STATUS-325: Translation Move Processed: World-Frame Displacement Applied to Global Position
- STATUS-326: Unknown Direction Code Encountered in 3D Chunk: Defaulted to base_translation '--'
- STATUS-327: Unknown Direction Code Encountered in Full Telemetry Pass: Defaulted to Idle Step
- STATUS-328: Gimbal Lock Singularity Detected: Euler Fallback Applied for Roll and Pitch
- STATUS-329: 3D Plot Pane Colors Set to White for All Three Axes
- STATUS-330: 3D Plot View Angle Initialized: elev=20 azim=-50
- ERROR-000: Startup Failure: Log Path Inaccessible or Cannot Be Created
- ERROR-001: Activity Log Write Failure: File Inaccessible
- ERROR-002: Keybind Log Write Failure: File Inaccessible
- ERROR-003: Log Directory Creation Failed: Check Permissions
- ERROR-004: Incorrect Keybind: No Mapping Found for Key
- ERROR-005: Startup Sequence Aborted by User via ESC
- ERROR-006: Rack Connector Test Failed: GPIO Connection Issue on Pin
- ERROR-007: GPIO Pin Activation Failed
- ERROR-300: File Access Failed: Specified Path Does Not Exist
- ERROR-301: Data Parsing Failed: Required Column Missing from CSV (Time(s) or Direction)
- ERROR-302: Mathematical or Integration Error During 3D Chunk Processing
- ERROR-303: Mathematical or Integration Error During Full Telemetry Calculation
- ERROR-304: Unexpected General Execution Error: Caught by Outer Exception Handler
"""



# Libraries =========================================================================================================================================
import os
import math
import numpy as np
import pandas as pd
from datetime import datetime
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
from matplotlib.lines import Line2D
from mpl_toolkits.mplot3d import Axes3D



def log_event(code, message):
    """
    log_event: Logs a message with a timestamp and status/error code.
    
    Parameters:
    - code (str)    : A status or error code (e.g., "STATUS-01", "ERROR-02").
    - message (str) : A descriptive message about the event being logged.
    """
    
    timestamp = datetime.now().strftime("%H:%M:%S.%f")[:-3]
    
    print(f"[{timestamp}] [{code}] {message}")



log_event("STATUS-300", "Script Initialization Started.")

file_path = r'C:\Users\maxwe\OneDrive\Desktop\GitHub Repos\DTO-Project\Logs\Keybind_Log.csv'
filename = os.path.basename(file_path)
images_dir = r'C:\Users\maxwe\OneDrive\Desktop\GitHub Repos\DTO-Project\Images'

TICK_INTERVAL = 5.0 



def format_time(seconds):
    """
    format_time: Converts a time in seconds to a "minutes:seconds" string format.
    
    Parameters:
    - seconds (float): Time in seconds to be formatted.
    
    Returns:
    - (str): A string representing the time in "M:SS" format.
    """
    
    minutes = int(seconds // 60)
    secs = int(seconds % 60)
    
    return f"{minutes}:{secs:02d}"



step_increment = 1.0  
rot_increment = np.radians(2.0) 



def get_rotation_matrix(axis, theta):
    """
    get_rotation_matrix: Generates a 3D rotation matrix for a given axis and angle.
    
    Parameters:
    - axis (str)    : The axis of rotation ('x', 'y', or 'z').
    - theta (float) : The rotation angle in radians.
    
    Returns:
    - (numpy.ndarray): A 3x3 rotation matrix.
    """
    
    axis = axis.lower()
    if (axis == 'x'): # Roll (X-Axis)
        return np.array([[1, 0, 0],
                         [0, np.cos(theta), -np.sin(theta)],
                         [0, np.sin(theta), np.cos(theta)]])
    elif (axis == 'y'): # Pitch (Y-Axis)
        return np.array([[np.cos(theta), 0, np.sin(theta)],
                         [0, 1, 0],
                         [-np.sin(theta), 0, np.cos(theta)]])
    elif (axis == 'z'): # Yaw (Z-Axis)
        return np.array([[np.cos(theta), -np.sin(theta), 0],
                         [np.sin(theta), np.cos(theta), 0],
                         [0, 0, 1]])
    else:
        return np.eye(3)



def rotation_matrix_to_euler(R):
    """
    rotation_matrix_to_euler: Converts a rotation matrix to Euler angles (Roll, Pitch, Yaw).
    
    Parameters:
    - R (numpy.ndarray): A 3x3 rotation matrix.
    
    Returns:
    - (numpy.ndarray): An array of Euler angles in the order [Roll, Pitch, Yaw].
    """
    
    sy = math.sqrt(R[0,0] * R[0,0] +  R[1,0] * R[1,0])
    singular = sy < 1e-6
    if (not singular):
        x = math.atan2(R[2,1] , R[2,2])  # Roll (X-Axis)
        y = math.atan2(-R[2,0], sy)      # Pitch (Y-Axis)
        z = math.atan2(R[1,0], R[0,0])   # Yaw (Z-Axis)
    else:
        x = math.atan2(-R[1,2], R[1,1])  # Roll (X-Axis) in singular case
        y = math.atan2(-R[2,0], sy)      # Pitch (Y-Axis) in singular case
        z = 0                            # Yaw (Z-Axis) is set to zero in singular case
        
    return np.array([math.degrees(x), math.degrees(y), math.degrees(z)])



base_translation = {
    '--': [step_increment, 0, 0],
    '+X': [step_increment, 0, 0], '-X': [-step_increment, 0, 0],
    '+Y': [0, step_increment, 0], '-Y': [0, -step_increment, 0],
    '+Z': [0, 0, step_increment], '-Z': [0, 0, -step_increment]
}

rotational_moves = {
    '+R': ('x', rot_increment),  '-R': ('x', -rot_increment),
    '+P': ('y', rot_increment),  '-P': ('y', -rot_increment),
    '+Y_rot': ('z', rot_increment), '-Y_rot': ('z', -rot_increment)
}

dof_colors = {
    'X': '#FF0000', 'Y': '#00FF00', 'Z': '#0000FF', 
    'ROLL': '#FFFF00', 'PITCH': '#800080', 'YAW': '#FFA500',
    'IDLE': '#808080'
}

log_event("STATUS-301", "Configuration and mapping variables loaded.")



# Main Execution Block ==============================================================================================================================
try:
    # Validate File Path ----------------------------------------------------------------------------------------------------------------------------
    if not os.path.exists(file_path):
        log_event("ERROR-300", f"File access failed. Path does not exist: {file_path}")
        raise FileNotFoundError(f"Cannot find {file_path}")
    
    log_event("STATUS-302", "File path validation successful.")

    # Load CSV Data ---------------------------------------------------------------------------------------------------------------------------------
    try:
        df = pd.read_csv(file_path)
        time_col = 'Time(s)'
        if (time_col not in df.columns) or ('Direction' not in df.columns):
            raise KeyError(f"Missing required columns ('{time_col}' or 'Direction').")
        log_event("STATUS-303", f"CSV Data loaded successfully. Rows: {len(df)}")
    except Exception as e:
        log_event("ERROR-301", f"Data parsing failed: {e}")
        raise e

    # Sort by time and compute how long each event lasted (time until the next event)
    df = df.sort_values(by=time_col).reset_index(drop=True)
    df['_duration'] = (df[time_col].shift(-1) - df[time_col]).fillna(0.0).clip(lower=0.0)

    # Keep only movement (T) rows; N rows are stop markers and contribute no displacement
    if 'Type' in df.columns:
        move_df = df[df['Type'] == 'T'].copy()
    else:
        move_df = df.copy()

    os.makedirs(images_dir, exist_ok=True)

    interval = 30
    max_time = df[time_col].max()
    num_chunks = int(np.ceil(max_time / interval))
        
    
    # Generate 3D Trajectory Chunks =================================================================================================================
    log_event("STATUS-304", f"3D Trajectory chunk generation started ({num_chunks} chunks total).")

    # Process each chunk of data to create 3D trajectory plots --------------------------------------------------------------------------------------
    for i in range(num_chunks):
        log_event("STATUS-305", f"Starting processing for 3D chunk {i+1}/{num_chunks}.")
        start_t = i * interval
        end_t = (i + 1) * interval
        chunk_df = move_df[(move_df[time_col] >= start_t) & (move_df[time_col] < end_t)].copy()
        
        # If the chunk is empty, skip to the next iteration -----------------------------------------------------------------------------------------
        if chunk_df.empty: 
            log_event("STATUS-306", f"Chunk {i+1} is empty, skipping.")
            continue

        actual_end_time = chunk_df[time_col].max()
        display_end_t = actual_end_time if actual_end_time < end_t - 0.1 and actual_end_time == max_time else end_t

        fig = plt.figure(figsize=(10, 8)) 
        ax = fig.add_subplot(111, projection='3d')
        
        ax.xaxis.set_pane_color((1.0, 1.0, 1.0, 1.0))
        ax.yaxis.set_pane_color((1.0, 1.0, 1.0, 1.0))
        ax.zaxis.set_pane_color((1.0, 1.0, 1.0, 1.0))
        
        current_pos = np.array([0.0, 0.0, 0.0])
        current_rot_matrix = np.eye(3) 
        
        directions = chunk_df['Direction'].values
        timestamps = chunk_df[time_col].values
        durations_arr = chunk_df['_duration'].values
        
        points = [current_pos]
        next_tick_time = start_t + TICK_INTERVAL

        # Process each movement in the chunk to calculate the trajectory and plot it ----------------------------------------------------------------
        try:
            for idx, move in enumerate(directions):
                curr_time = timestamps[idx]
                is_absolute_last_point = (i == num_chunks - 1) and (idx == len(directions) - 1)
                
                # Determine if the move is a rotation or translation and calculate the next position accordingly ------------------------------------
                duration = durations_arr[idx]
                if move in rotational_moves:
                    axis_char, theta = rotational_moves[move]
                    new_rot = get_rotation_matrix(axis_char, theta * duration)
                    current_rot_matrix = np.dot(new_rot, current_rot_matrix)

                    if 'R' in move: color = dof_colors['ROLL']
                    elif 'P' in move: color = dof_colors['PITCH']
                    elif 'Y_rot' in move: color = dof_colors['YAW']
                    else: color = dof_colors['IDLE']

                    path_vec = np.dot(current_rot_matrix, np.array([step_increment * 0.5, 0, 0])) * duration
                    next_pos = current_pos + path_vec
                else:
                    base_move_vec = np.array(base_translation.get(move, base_translation['--']), dtype=float)
                    actual_move_vec = np.dot(current_rot_matrix, base_move_vec) * duration
                    next_pos = current_pos + actual_move_vec

                    if 'X' in move: color = dof_colors['X']
                    elif 'Y' in move: color = dof_colors['Y']
                    elif 'Z' in move: color = dof_colors['Z']
                    else: color = dof_colors['IDLE']

                # Check if we have reached or passed the next tick time to place a marker and label on the plot -------------------------------------
                marker_drawn = False
                if curr_time >= next_tick_time:
                    ax.scatter(current_pos[0], current_pos[1], current_pos[2], color='black', s=15, zorder=5)
                    ax.text(current_pos[0], current_pos[1], current_pos[2], f" {int(next_tick_time)}s", color='black', zorder=10)
                    next_tick_time += TICK_INTERVAL
                    marker_drawn = True

                if is_absolute_last_point and not marker_drawn:
                    ax.scatter(current_pos[0], current_pos[1], current_pos[2], color='black', s=15, zorder=5)
                    ax.text(current_pos[0], current_pos[1], current_pos[2], f" {curr_time:.1f}s", color='black', zorder=10)

                ax.plot([current_pos[0], next_pos[0]], 
                        [current_pos[1], next_pos[1]], 
                        [current_pos[2], next_pos[2]], color=color, linewidth=2)
                
                current_pos = next_pos
                points.append(current_pos)
        except Exception as e:
            log_event("ERROR-302", f"Mathematical/Integration error during 3D chunk processing: {e}")
            raise e

        pts = np.array(points)
        max_range = np.array([pts[:,0].max()-pts[:,0].min(), 
                              pts[:,1].max()-pts[:,1].min(), 
                              pts[:,2].max()-pts[:,2].min()]).max() / 2.0
        
        if max_range == 0: max_range = 1.0 

        mid_x = (pts[:,0].max()+pts[:,0].min()) * 0.5
        mid_y = (pts[:,1].max()+pts[:,1].min()) * 0.5
        mid_z = (pts[:,2].max()+pts[:,2].min()) * 0.5
        
        buf = max_range * 0.1 
        ax.set_xlim(mid_x - max_range - buf, mid_x + max_range + buf)
        ax.set_ylim(mid_y - max_range - buf, mid_y + max_range + buf)
        ax.set_zlim(mid_z - max_range - buf, mid_z + max_range + buf)
        ax.set_box_aspect((1, 1, 1)) 

        ax.view_init(elev=20, azim=-50) 
        ax.set_title(f"Time: {format_time(start_t)} - {format_time(display_end_t)}")
        ax.set_xlabel('X (Roll)')
        ax.set_ylabel('Y (Pitch)')
        ax.set_zlabel('Z (Yaw)')

        legend_elements = [
            Line2D([0], [0], color=dof_colors['X'], lw=3, label='X Translation'),
            Line2D([0], [0], color=dof_colors['Y'], lw=3, label='Y Translation'),
            Line2D([0], [0], color=dof_colors['Z'], lw=3, label='Z Translation'),
            Line2D([0], [0], color=dof_colors['ROLL'], lw=3, label='Roll (X Rot)'),
            Line2D([0], [0], color=dof_colors['PITCH'], lw=3, label='Pitch (Y Rot)'),
            Line2D([0], [0], color=dof_colors['YAW'], lw=3, label='Yaw (Z Rot)'),
            Line2D([0], [0], color=dof_colors['IDLE'], lw=3, label='Idle'),
            Line2D([0], [0], marker='o', color='w', markerfacecolor='black', markersize=6, label=f'Time Marker')
        ]
        ax.legend(handles=legend_elements, loc='upper left', bbox_to_anchor=(0.0, 1.1))
        plt.tight_layout()

        chunk_filename = os.path.join(images_dir, f"3D_Chunk_{i+1:02d}.png")
        plt.savefig(chunk_filename, dpi=150, bbox_inches='tight')
        plt.close(fig)
        log_event("STATUS-307", f"3D Plot for chunk {i+1} saved to: {chunk_filename}")
    
    
    # Process the entire dataset to calculate global position, attitude, velocity, and acceleration -------------------------------------------------
    log_event("STATUS-309", "Full telemetry data processing phase started.")
    try:
        telemetry_df = move_df.drop_duplicates(subset=[time_col]).sort_values(by=time_col).reset_index(drop=True)
        full_times = telemetry_df[time_col].values
        full_directions = telemetry_df['Direction'].values
        full_durations = telemetry_df['_duration'].values
        
        global_pos = np.array([0.0, 0.0, 0.0])
        global_rot_matrix = np.eye(3) 
        
        positions_list = []
        attitudes_list = []

        # Process each movement in the full dataset to calculate the global trajectory and attitude -------------------------------------------------
        for idx, move in enumerate(full_directions):
            duration = full_durations[idx]
            if (move in rotational_moves):
                axis_char, theta = rotational_moves[move]
                new_rot = get_rotation_matrix(axis_char, theta * duration)
                global_rot_matrix = np.dot(new_rot, global_rot_matrix)

                path_vec = np.dot(global_rot_matrix, np.array([step_increment * 0.5, 0, 0])) * duration
                global_pos = global_pos + path_vec
            else:
                base_move_vec = np.array(base_translation.get(move, base_translation['--']), dtype=float)
                actual_move_vec = np.dot(global_rot_matrix, base_move_vec) * duration
                global_pos = global_pos + actual_move_vec
                
            positions_list.append(global_pos.copy())
            attitudes_list.append(rotation_matrix_to_euler(global_rot_matrix))

        positions_arr = np.array(positions_list)
        attitudes_arr = np.array(attitudes_list)
        
        pos_x, pos_y, pos_z = positions_arr[:, 0], positions_arr[:, 1], positions_arr[:, 2]
        roll_arr, pitch_arr, yaw_arr = attitudes_arr[:, 0], attitudes_arr[:, 1], attitudes_arr[:, 2]
        log_event("STATUS-310", "Global trajectory integration complete.")

        vel_x = np.gradient(pos_x, full_times)
        vel_y = np.gradient(pos_y, full_times)
        vel_z = np.gradient(pos_z, full_times)
        
        acc_x = np.gradient(vel_x, full_times)
        acc_y = np.gradient(vel_y, full_times)
        acc_z = np.gradient(vel_z, full_times)
        log_event("STATUS-311", "Telemetry derivatives (Velocity/Acceleration) calculated.")
    except Exception as e:
        log_event("ERROR-303", f"Mathematical/Integration error during telemetry calculation: {e}")
        raise e

    
    # Generate 2D plots for Absolute Position, Attitude, Velocity, and Acceleration over time -------------------------------------------------------
    log_event("STATUS-312", "Telemetry 2D plots generation phase started.")
    
    # Absolute Position Graph -----------------------------------------------------------------------------------------------------------------------
    fig_pos = plt.figure(figsize = (10, 5))
    plt.plot(full_times, pos_x, label = 'X Position')
    plt.plot(full_times, pos_y, label = 'Y Position')
    plt.plot(full_times, pos_z, label = 'Z Position')
    plt.title("Absolute Position")
    plt.xlabel("Time (s)")
    plt.ylabel("Units")
    plt.grid(True)
    plt.legend()
    plt.tight_layout()
    plt.savefig(os.path.join(images_dir, "Position.png"), dpi=150, bbox_inches='tight')
    plt.close(fig_pos)
    log_event("STATUS-313", "Absolute Position plot saved.")

    
    # Attutude Graph --------------------------------------------------------------------------------------------------------------------------------
    fig_att = plt.figure(figsize = (10, 5))
    plt.plot(full_times, roll_arr, label = 'Roll')
    plt.plot(full_times, pitch_arr, label = 'Pitch')
    plt.plot(full_times, yaw_arr, label = 'Yaw')
    plt.title("Attitude (Euler Angles)")
    plt.xlabel("Time (s)")
    plt.ylabel("Degrees")
    plt.grid(True)
    plt.legend()
    plt.tight_layout()
    plt.savefig(os.path.join(images_dir, "Attitude.png"), dpi=150, bbox_inches='tight')
    plt.close(fig_att)
    log_event("STATUS-314", "Attitude plot saved.")

    
    # Linear Velocity Graph -------------------------------------------------------------------------------------------------------------------------
    fig_vel = plt.figure(figsize=(10, 5))
    plt.plot(full_times, vel_x, label = 'Velocity X')
    plt.plot(full_times, vel_y, label = 'Velocity Y')
    plt.plot(full_times, vel_z, label = 'Velocity Z')
    plt.title("Linear Velocity")
    plt.xlabel("Time (s)")
    plt.ylabel("Units / s")
    plt.grid(True)
    plt.legend()
    plt.tight_layout()
    plt.savefig(os.path.join(images_dir, "Velocity.png"), dpi=150, bbox_inches='tight')
    plt.close(fig_vel)
    log_event("STATUS-315", "Velocity plot saved.")

    
    # Linear Acceleration Graph ---------------------------------------------------------------------------------------------------------------------
    fig_acc = plt.figure(figsize = (10, 5))
    plt.plot(full_times, acc_x, label = 'Accel X')
    plt.plot(full_times, acc_y, label = 'Accel Y')
    plt.plot(full_times, acc_z, label = 'Accel Z')
    plt.title("Linear Acceleration")
    plt.xlabel("Time (s)")
    plt.ylabel("Units / s²")
    plt.grid(True)
    plt.legend()
    plt.tight_layout()
    plt.savefig(os.path.join(images_dir, "Acceleration.png"), dpi=150, bbox_inches='tight')
    plt.close(fig_acc)
    log_event("STATUS-316", "Acceleration plot saved.")

    log_event("STATUS-317", "Script execution completed successfully.")

except FileNotFoundError:
    pass # Already logged in the check above
except KeyError:
    pass # Already logged in the check above
except Exception as e:
    log_event("ERROR-304", f"Unexpected general execution error: {e}")