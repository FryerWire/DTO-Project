
"""
Computation Script for DTO Project

[STATUS CODES]
STATUS-00 : Script Initialization Started
STATUS-01 : Configuration and mapping variables loaded
STATUS-02 : File path validation successful (File exists)
STATUS-03 : CSV Data loaded successfully into Pandas DataFrame
STATUS-04 : 3D Trajectory chunk generation phase started
STATUS-05 : Starting processing for specific 3D chunk
STATUS-06 : 3D Plot successfully generated and opened (Waiting for user close)
STATUS-07 : 3D Plot closed by user
STATUS-08 : Full telemetry data processing phase started
STATUS-09 : Global trajectory integration complete
STATUS-10 : Telemetry derivatives (Velocity/Acceleration) calculated
STATUS-11 : Telemetry 2D plots generation phase started
STATUS-12 : Absolute Position plot opened (Waiting for user close)
STATUS-13 : Attitude plot opened (Waiting for user close)
STATUS-14 : Velocity plot opened (Waiting for user close)
STATUS-15 : Acceleration plot opened (Waiting for user close)
STATUS-16 : Script execution completed successfully

[ERROR CODES]
ERROR-00  : File access failed (File not found or inaccessible)
ERROR-01  : Data parsing failed (Missing expected columns in CSV)
ERROR-02  : Mathematical or integration calculation error
ERROR-03  : Unexpected general execution error
"""



# Libraries =========================================================================================================================================
import os
import math
import numpy as np
import pandas as pd
from datetime import datetime
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



log_event("STATUS-00", "Script Initialization Started.")

file_path = r'C:\Users\maxwe\OneDrive\Desktop\GitHub Repos\DTO-Project\Logs\Keybind_Log.csv'
filename = os.path.basename(file_path)

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

log_event("STATUS-01", "Configuration and mapping variables loaded.")



# Main Execution Block ==============================================================================================================================
try:
    # Validate File Path ----------------------------------------------------------------------------------------------------------------------------
    if not os.path.exists(file_path):
        log_event("ERROR-00", f"File access failed. Path does not exist: {file_path}")
        raise FileNotFoundError(f"Cannot find {file_path}")
    
    log_event("STATUS-02", "File path validation successful.")

    # Load CSV Data ---------------------------------------------------------------------------------------------------------------------------------
    try:
        df = pd.read_csv(file_path)
        time_col = 'Time(s)'
        if (time_col not in df.columns) or ('Direction' not in df.columns):
            raise KeyError(f"Missing required columns ('{time_col}' or 'Direction').")
        log_event("STATUS-03", f"CSV Data loaded successfully. Rows: {len(df)}")
    except Exception as e:
        log_event("ERROR-01", f"Data parsing failed: {e}")
        raise e

    interval = 30 
    max_time = df[time_col].max()
    num_chunks = int(np.ceil(max_time / interval))
        
    
    # Generate 3D Trajectory Chunks =================================================================================================================
    log_event("STATUS-04", f"3D Trajectory chunk generation started ({num_chunks} chunks total).")

    # Process each chunk of data to create 3D trajectory plots --------------------------------------------------------------------------------------
    for i in range(num_chunks):
        log_event("STATUS-05", f"Starting processing for 3D chunk {i+1}/{num_chunks}.")
        start_t = i * interval
        end_t = (i + 1) * interval
        chunk_df = df[(df[time_col] >= start_t) & (df[time_col] < end_t)].copy()
        
        # If the chunk is empty, skip to the next iteration -----------------------------------------------------------------------------------------
        if chunk_df.empty: 
            log_event("STATUS-05", f"Chunk {i+1} is empty, skipping.")
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
        
        points = [current_pos]
        next_tick_time = start_t + TICK_INTERVAL

        # Process each movement in the chunk to calculate the trajectory and plot it ----------------------------------------------------------------
        try:
            for idx, move in enumerate(directions):
                curr_time = timestamps[idx]
                is_absolute_last_point = (i == num_chunks - 1) and (idx == len(directions) - 1)
                
                # Determine if the move is a rotation or translation and calculate the next position accordingly ------------------------------------
                if move in rotational_moves:
                    axis_char, theta = rotational_moves[move]
                    new_rot = get_rotation_matrix(axis_char, theta)
                    current_rot_matrix = np.dot(new_rot, current_rot_matrix)
                    
                    if 'R' in move: color = dof_colors['ROLL']
                    elif 'P' in move: color = dof_colors['PITCH']
                    elif 'Y_rot' in move: color = dof_colors['YAW']
                    else: color = dof_colors['IDLE']
                    
                    path_vec = np.dot(current_rot_matrix, np.array([step_increment * 0.5, 0, 0]))
                    next_pos = current_pos + path_vec
                else:
                    base_move_vec = np.array(base_translation.get(move, base_translation['--']), dtype=float)
                    actual_move_vec = np.dot(current_rot_matrix, base_move_vec)
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
            log_event("ERROR-02", f"Mathematical/Integration error during 3D chunk processing: {e}")
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

        log_event("STATUS-06", f"3D Plot for chunk {i+1} successfully opened. Waiting for user close...")
        plt.show() 
        log_event("STATUS-07", f"3D Plot for chunk {i+1} closed by user.")
    
    
    # Process the entire dataset to calculate global position, attitude, velocity, and acceleration -------------------------------------------------
    log_event("STATUS-08", "Full telemetry data processing phase started.")
    try:
        telemetry_df = df.drop_duplicates(subset=[time_col]).sort_values(by=time_col).reset_index(drop=True)
        full_times = telemetry_df[time_col].values
        full_directions = telemetry_df['Direction'].values
        
        global_pos = np.array([0.0, 0.0, 0.0])
        global_rot_matrix = np.eye(3) 
        
        positions_list = []
        attitudes_list = []

        # Process each movement in the full dataset to calculate the global trajectory and attitude -------------------------------------------------
        for move in full_directions:
            if (move in rotational_moves):
                axis_char, theta = rotational_moves[move]
                new_rot = get_rotation_matrix(axis_char, theta)
                global_rot_matrix = np.dot(new_rot, global_rot_matrix)
                
                path_vec = np.dot(global_rot_matrix, np.array([step_increment * 0.5, 0, 0]))
                global_pos = global_pos + path_vec
            else:
                base_move_vec = np.array(base_translation.get(move, base_translation['--']), dtype=float)
                actual_move_vec = np.dot(global_rot_matrix, base_move_vec)
                global_pos = global_pos + actual_move_vec
                
            positions_list.append(global_pos.copy())
            attitudes_list.append(rotation_matrix_to_euler(global_rot_matrix))

        positions_arr = np.array(positions_list)
        attitudes_arr = np.array(attitudes_list)
        
        pos_x, pos_y, pos_z = positions_arr[:, 0], positions_arr[:, 1], positions_arr[:, 2]
        roll_arr, pitch_arr, yaw_arr = attitudes_arr[:, 0], attitudes_arr[:, 1], attitudes_arr[:, 2]
        log_event("STATUS-09", "Global trajectory integration complete.")

        vel_x = np.gradient(pos_x, full_times)
        vel_y = np.gradient(pos_y, full_times)
        vel_z = np.gradient(pos_z, full_times)
        
        acc_x = np.gradient(vel_x, full_times)
        acc_y = np.gradient(vel_y, full_times)
        acc_z = np.gradient(vel_z, full_times)
        log_event("STATUS-10", "Telemetry derivatives (Velocity/Acceleration) calculated.")
    except Exception as e:
        log_event("ERROR-02", f"Mathematical/Integration error during telemetry calculation: {e}")
        raise e

    
    # Generate 2D plots for Absolute Position, Attitude, Velocity, and Acceleration over time -------------------------------------------------------
    log_event("STATUS-11", "Telemetry 2D plots generation phase started.")
    
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
    log_event("STATUS-12", "Absolute Position plot opened. Waiting for user close...")
    plt.show()

    
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
    log_event("STATUS-13", "Attitude plot opened. Waiting for user close...")
    plt.show()

    
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
    log_event("STATUS-14", "Velocity plot opened. Waiting for user close...")
    plt.show()

    
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
    log_event("STATUS-15", "Acceleration plot opened. Waiting for user close...")
    plt.show()

    log_event("STATUS-16", "Script execution completed successfully.")

except FileNotFoundError:
    pass # Already logged in the check above
except KeyError:
    pass # Already logged in the check above
except Exception as e:
    log_event("ERROR-03", f"Unexpected general execution error: {e}")
