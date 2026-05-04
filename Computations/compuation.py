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
- STATUS-300: Init Started: first log_event() succeeded; setup phase starting
- STATUS-301: Config Loaded: all constants and lookup structures defined
- STATUS-302: File Validated: CSV file path exists and accessible
- STATUS-303: CSV Loaded: pd.read_csv() parsed successfully; row count logged
- STATUS-304: 3D Chunk Phase Started: trajectory segments built; per-chunk loop starting
- STATUS-305: Processing Chunk N: chunk iteration N; filtering segments
- STATUS-306: Chunk Skipped: no segments overlap chunk window; continuing
- STATUS-307: 3D Chunk Saved: PNG written to Images/3D_Chunk_NN.png
- STATUS-308: 3D Figure Initialized: Axes3D ready for plotting
- STATUS-309: Telemetry Phase Started: Type-T rows deduplicated and sorted
- STATUS-310: Trajectory Integrated: positions_arr and attitudes_arr complete
- STATUS-311: Derivatives Computed: velocity and acceleration arrays calculated
- STATUS-312: 2D Plot Phase Started: four sequential 2D plots beginning
- STATUS-313: Position Plot Saved: Images/Position.png written
- STATUS-314: Attitude Plot Saved: Images/Attitude.png written
- STATUS-315: Velocity Plot Saved: Images/Velocity.png written
- STATUS-316: Acceleration Plot Saved: Images/Acceleration.png written
- STATUS-317: Script Complete: all PNG files written; execution finished
- STATUS-318: Segment List Built: all segments collected; total count logged
- STATUS-319: Chunk Segments Prepared: clipping complete; segment count logged
- STATUS-320: Tick Label Placed: time string label placed on 3D plot
- STATUS-321: Tick Labels Complete: all labels written for chunk
- STATUS-322: 3D Axis Limits Applied: uniform cubic axis space set
- STATUS-323: max_range Defaulted: max_range was 0; overridden to 1.0
- STATUS-324: Rotation Move Processed: rotation matrix updated
- STATUS-325: Translation Move Processed: position updated from body frame
- STATUS-326: Gimbal Lock Detected: sy < 1e-6; fallback Euler computation used
- STATUS-327: 3D Panes White: axis panes set to white background
- STATUS-328: 3D View Initialized: camera perspective set
- ERROR-300: File Access Failed: file path does not exist
- ERROR-301: Column Missing: Time(s) or Direction column not found
- ERROR-302: 3D Processing Error: exception during chunk generation; error logged
- ERROR-303: Telemetry Error: exception during integration or derivative calc
- ERROR-304: Unexpected Error: non-categorized exception; script ends
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

from mpl_toolkits.mplot3d import Axes3D  # noqa: F401



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

file_path = r'C:\Users\maxwe\OneDrive\Desktop\DTO-Project\Controller\Logs\Keybind_Logs.csv'
filename = os.path.basename(file_path)
images_dir = r'C:\Users\maxwe\OneDrive\Desktop\DTO-Project\Images'

TICK_INTERVAL = 5.0 



def parse_time_value(t):
    """
    parse_time_value: Parses a time value that may be in 'SS:cc' (seconds:centiseconds)
    or standard decimal format, and returns a float in seconds.

    Parameters:
    - t: The raw time value from the CSV (str, int, or float).

    Returns:
    - (float): Time in seconds.
    """
    s = str(t).strip()
    if ':' in s:
        parts = s.split(':')
        return float(parts[0]) + float(parts[1]) / 100.0
    return float(s)



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
        df[time_col] = df[time_col].apply(parse_time_value)
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

    max_time = df[time_col].max()

    # Generate 3D Trajectory Chunks =============================================================================================================
    CHUNK_DURATION = 30.0

    log_event("STATUS-304", f"3D Trajectory chunk generation phase started. Total chunks: {math.ceil(max_time / CHUNK_DURATION)}")

    try:
        chunk_df   = move_df.sort_values(by=time_col).reset_index(drop=True)
        chunk_dirs = chunk_df['Direction'].values
        chunk_times = chunk_df[time_col].values
        chunk_durs  = chunk_df['_duration'].values

        # Build full trajectory as segments: (t, duration, from_pos, to_pos, color_key)
        segments = []
        curr_pos = np.array([0.0, 0.0, 0.0])
        curr_rot = np.eye(3)

        for idx, move in enumerate(chunk_dirs):
            t        = chunk_times[idx]
            duration = chunk_durs[idx]

            if move in rotational_moves:
                axis_char, theta = rotational_moves[move]
                new_rot  = get_rotation_matrix(axis_char, theta * duration)
                curr_rot = np.dot(new_rot, curr_rot)
                path_vec = np.dot(curr_rot, np.array([step_increment * 0.5, 0, 0])) * duration
                next_p   = curr_pos + path_vec
                if   'R'     in move: color_key = 'ROLL'
                elif 'P'     in move: color_key = 'PITCH'
                elif 'Y_rot' in move: color_key = 'YAW'
                else:                 color_key = 'IDLE'
            else:
                base_vec = np.array(base_translation.get(move, base_translation['--']), dtype=float)
                next_p   = curr_pos + np.dot(curr_rot, base_vec) * duration
                if   'X' in move: color_key = 'X'
                elif 'Y' in move: color_key = 'Y'
                elif 'Z' in move: color_key = 'Z'
                else:             color_key = 'IDLE'

            segments.append((t, duration, curr_pos.copy(), next_p.copy(), color_key))
            curr_pos = next_p

        log_event("STATUS-318", f"Full trajectory segment list built: {len(segments)} segments from {len(chunk_dirs)} movement rows.")
        total_chunks = math.ceil(max_time / CHUNK_DURATION)

        for chunk_idx in range(total_chunks):
            t_start   = chunk_idx * CHUNK_DURATION
            t_end     = t_start + CHUNK_DURATION
            chunk_num = chunk_idx + 1

            log_event("STATUS-305", f"Processing chunk {chunk_num}: {format_time(t_start)} - {format_time(t_end)}")

            # Include any segment that overlaps this chunk window, clipped to [t_start, t_end]
            chunk_segs = []
            for (t, dur, fp, tp, ck) in segments:
                seg_end = t + dur
                if seg_end <= t_start or t >= t_end:
                    continue
                clip_fp, clip_tp = fp.copy(), tp.copy()
                if t < t_start and dur > 0:
                    a = (t_start - t) / dur
                    clip_fp = fp + (tp - fp) * a
                if seg_end > t_end and dur > 0:
                    a = (t_end - t) / dur
                    clip_tp = fp + (tp - fp) * a
                chunk_segs.append((t, dur, clip_fp, clip_tp, ck))

            log_event("STATUS-319", f"Chunk {chunk_num}: {len(chunk_segs)} segments prepared in window [{format_time(t_start)}, {format_time(t_end)}].")

            if not chunk_segs:
                log_event("STATUS-306", f"Chunk {chunk_num} skipped: no data in time window.")
                continue

            fig = plt.figure(figsize=(10, 8))
            ax  = fig.add_subplot(111, projection='3d')
            log_event("STATUS-308", f"Chunk {chunk_num}: 3D figure initialized.")
            ax.set_facecolor('white')
            ax.xaxis.pane.set_facecolor('white')
            ax.yaxis.pane.set_facecolor('white')
            ax.zaxis.pane.set_facecolor('white')
            log_event("STATUS-327", f"Chunk {chunk_num}: 3D plot pane colors set to white.")

            # Group consecutive same-color segments using nan separators
            color_xs = {k: [] for k in dof_colors}
            color_ys = {k: [] for k in dof_colors}
            color_zs = {k: [] for k in dof_colors}
            all_chunk_pts = []

            prev_ck = None
            for (t, dur, fp, tp, ck) in chunk_segs:
                if prev_ck is not None and ck != prev_ck:
                    color_xs[prev_ck].append(np.nan)
                    color_ys[prev_ck].append(np.nan)
                    color_zs[prev_ck].append(np.nan)
                color_xs[ck].extend([fp[0], tp[0]])
                color_ys[ck].extend([fp[1], tp[1]])
                color_zs[ck].extend([fp[2], tp[2]])
                all_chunk_pts.extend([fp, tp])
                prev_ck = ck

            # Axis limits from chunk bounding box (computed first so max_range is available for offsets)
            pts_arr   = np.array(all_chunk_pts)
            max_range = max(
                pts_arr[:, 0].max() - pts_arr[:, 0].min(),
                pts_arr[:, 1].max() - pts_arr[:, 1].min(),
                pts_arr[:, 2].max() - pts_arr[:, 2].min()
            ) / 2.0
            if max_range == 0:
                max_range = 1.0
                log_event("STATUS-323", f"Chunk {chunk_num}: max_range defaulted to 1.0.")

            text_offset = max_range * 0.08

            dof_labels = {
                'X': 'X Translation', 'Y': 'Y Translation', 'Z': 'Z Translation',
                'ROLL': 'Roll (X Rot)', 'PITCH': 'Pitch (Y Rot)', 'YAW': 'Yaw (Z Rot)', 'IDLE': 'Idle'
            }
            plotted_keys = set()
            for ck in dof_colors:
                xs = [v for v in color_xs[ck] if not (isinstance(v, float) and math.isnan(v))]
                if not xs:
                    continue
                label = dof_labels[ck] if ck not in plotted_keys else None
                plotted_keys.add(ck)
                ax.plot(color_xs[ck], color_ys[ck], color_zs[ck],
                        color=dof_colors[ck], linewidth=2, label=label)

            # Interpolate position at tick_t using the global segments list so the
            # label placement matches the actual trajectory regardless of chunk boundaries.
            def interp_pos_at(t_query):
                best = None
                for seg in segments:
                    if seg[0] <= t_query:
                        best = seg
                    else:
                        break
                if best is None:
                    return None
                t_seg, dur_seg, fp_seg, tp_seg, _ = best
                alpha = max(0.0, min((t_query - t_seg) / dur_seg, 1.0)) if dur_seg > 0 else 0.0
                return fp_seg + (tp_seg - fp_seg) * alpha

            tick_t = math.ceil(t_start / TICK_INTERVAL) * TICK_INTERVAL
            while tick_t <= min(t_end, max_time):
                pos = interp_pos_at(tick_t)
                if pos is not None:
                    ax.text(pos[0] + text_offset, pos[1] + text_offset, pos[2] + text_offset,
                            f"{int(tick_t)}s", fontsize=8)
                    log_event("STATUS-320", f"Chunk {chunk_num}: tick label placed at {int(tick_t)}s.")
                tick_t += TICK_INTERVAL

            log_event("STATUS-321", f"Chunk {chunk_num}: tick labels placed.")
            mid_x = (pts_arr[:, 0].max() + pts_arr[:, 0].min()) * 0.5
            mid_y = (pts_arr[:, 1].max() + pts_arr[:, 1].min()) * 0.5
            mid_z = (pts_arr[:, 2].max() + pts_arr[:, 2].min()) * 0.5
            buf   = max_range * 0.1
            ax.set_xlim(mid_x - max_range - buf, mid_x + max_range + buf)
            ax.set_ylim(mid_y - max_range - buf, mid_y + max_range + buf)
            ax.set_zlim(mid_z - max_range - buf, mid_z + max_range + buf)
            log_event("STATUS-322", f"Chunk {chunk_num}: axis limits computed from bounding box.")

            ax.set_xlabel('X (Roll)')
            ax.set_ylabel('Y (Pitch)')
            ax.set_zlabel('Z (Yaw)')
            ax.set_title(f"3D Trajectory Chunk {chunk_num}: {format_time(t_start)} – {format_time(min(t_end, max_time))}")
            ax.view_init(elev=20, azim=-50)
            log_event("STATUS-328", f"Chunk {chunk_num}: view angle set to elev=20, azim=-50.")
            ax.legend(loc='upper left', fontsize=8)

            plt.tight_layout()
            chunk_filename = f"3D_Chunk_{chunk_num:02d}.png"
            plt.savefig(os.path.join(images_dir, chunk_filename), dpi=150, bbox_inches='tight')
            plt.close(fig)
            log_event("STATUS-307", f"3D Trajectory chunk {chunk_num} saved: {chunk_filename}")

    except Exception as e:
        log_event("ERROR-302", f"Error during 3D chunk generation: {e}")
        raise e
    
    
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