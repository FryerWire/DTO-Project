import pandas as pd
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D
import numpy as np
import os
import math
from matplotlib.lines import Line2D

# --- CONFIGURATION ---
# 1. Setup File Path
file_path = r'C:\Users\maxwe\OneDrive\Desktop\GitHub Repos\DTO-Project\Logs\Keybind_Log.csv'
filename = os.path.basename(file_path)

# Time Marker Settings
TICK_INTERVAL = 5.0   # Drop a time marker every 5 seconds

def format_time(seconds):
    """Converts seconds to M:SS format"""
    minutes = int(seconds // 60)
    secs = int(seconds % 60)
    return f"{minutes}:{secs:02d}"

# Incremental transformation mapping
step_increment = 1.0  
rot_increment = np.radians(2.0) 

def get_rotation_matrix(axis, theta):
    """Calculates standard rotation matrix for X, Y, or Z axis."""
    axis = axis.lower()
    if axis == 'x': # Roll
        return np.array([[1, 0, 0],
                         [0, np.cos(theta), -np.sin(theta)],
                         [0, np.sin(theta), np.cos(theta)]])
    elif axis == 'y': # Pitch
        return np.array([[np.cos(theta), 0, np.sin(theta)],
                         [0, 1, 0],
                         [-np.sin(theta), 0, np.cos(theta)]])
    elif axis == 'z': # Yaw
        return np.array([[np.cos(theta), -np.sin(theta), 0],
                         [np.sin(theta), np.cos(theta), 0],
                         [0, 0, 1]])
    else:
        return np.eye(3)

def rotation_matrix_to_euler(R):
    """Extracts Roll, Pitch, Yaw (in degrees) from a rotation matrix"""
    sy = math.sqrt(R[0,0] * R[0,0] +  R[1,0] * R[1,0])
    singular = sy < 1e-6

    if not singular:
        x = math.atan2(R[2,1] , R[2,2]) # Roll
        y = math.atan2(-R[2,0], sy)     # Pitch
        z = math.atan2(R[1,0], R[0,0])  # Yaw
    else:
        x = math.atan2(-R[1,2], R[1,1])
        y = math.atan2(-R[2,0], sy)
        z = 0

    return np.array([math.degrees(x), math.degrees(y), math.degrees(z)])

# Base movement vectors (before rotation applied)
base_translation = {
    '--': [step_increment, 0, 0],   # Idle moves forward along local X
    '+X': [step_increment, 0, 0], '-X': [-step_increment, 0, 0],
    '+Y': [0, step_increment, 0], '-Y': [0, -step_increment, 0],
    '+Z': [0, 0, step_increment], '-Z': [0, 0, -step_increment]
}

rotational_moves = {
    '+R': ('x', rot_increment),  '-R': ('x', -rot_increment),  # Roll
    '+P': ('y', rot_increment),  '-P': ('y', -rot_increment),  # Pitch
    '+Y_rot': ('z', rot_increment), '-Y_rot': ('z', -rot_increment) # Yaw
}

dof_colors = {
    'X': '#FF4500', 'Y': '#32CD32', 'Z': '#1E90FF', 
    'ROLL': '#FFD700', 'PITCH': '#BA55D3', 'YAW': '#4682B4',
    'IDLE': '#A9A9A9'
}

# --- MAIN LOOP ---
try:
    # 2. Load Data
    df = pd.read_csv(file_path)
    time_col = 'Time(s)' 
    interval = 30 
    
    max_time = df[time_col].max()
    num_chunks = int(np.ceil(max_time / interval))
    
    # ==========================================
    # PART 1: CHUNKED 3D TRAJECTORY GRAPHS
    # ==========================================
    for i in range(num_chunks):
        start_t = i * interval
        end_t = (i + 1) * interval
        chunk_df = df[(df[time_col] >= start_t) & (df[time_col] < end_t)].copy()
        
        if chunk_df.empty: continue

        # Determine dynamic title end time
        actual_end_time = chunk_df[time_col].max()
        display_end_t = actual_end_time if actual_end_time < end_t - 0.1 and actual_end_time == max_time else end_t

        # --- 3. CREATE INDEPENDENT PLOT ---
        fig = plt.figure(figsize=(10, 8)) 
        ax = fig.add_subplot(111, projection='3d')
        
        current_pos = np.array([0.0, 0.0, 0.0])
        current_rot_matrix = np.eye(3) 
        
        directions = chunk_df.iloc[:, 3].values 
        timestamps = chunk_df[time_col].values
        
        points = [current_pos]
        
        # Track the next multiple of our interval for the markers
        next_tick_time = start_t + TICK_INTERVAL

        # --- 4. STEP-BY-STEP CALCULATION ---
        for idx, move in enumerate(directions):
            curr_time = timestamps[idx]
            
            # Check if this is the absolute last point of the ENTIRE dataset
            is_absolute_last_point = (i == num_chunks - 1) and (idx == len(directions) - 1)
            
            # 4a. Update Rotational Orientation
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
                # 4b. Standard Translation
                base_move_vec = np.array(base_translation.get(move, base_translation['--']), dtype=float)
                actual_move_vec = np.dot(current_rot_matrix, base_move_vec)
                next_pos = current_pos + actual_move_vec
                
                if 'X' in move: color = dof_colors['X']
                elif 'Y' in move: color = dof_colors['Y']
                elif 'Z' in move: color = dof_colors['Z']
                else: color = dof_colors['IDLE']

            marker_drawn = False
            
            # 4c. Draw Standard 5s Time Markers
            if curr_time >= next_tick_time:
                ax.scatter(current_pos[0], current_pos[1], current_pos[2], color='black', s=15, zorder=5)
                ax.text(current_pos[0], current_pos[1], current_pos[2], f" {int(next_tick_time)}s", 
                        color='black', fontsize=10, fontweight='bold', zorder=10)
                next_tick_time += TICK_INTERVAL
                marker_drawn = True

            # 4d. Draw Final End-of-Path Marker (ONLY on the very last graph of the dataset)
            if is_absolute_last_point and not marker_drawn:
                ax.scatter(current_pos[0], current_pos[1], current_pos[2], color='black', s=15, zorder=5)
                ax.text(current_pos[0], current_pos[1], current_pos[2], f" {curr_time:.1f}s", 
                        color='black', fontsize=10, fontweight='bold', zorder=10)

            # Plot segment
            ax.plot([current_pos[0], next_pos[0]], 
                    [current_pos[1], next_pos[1]], 
                    [current_pos[2], next_pos[2]], 
                    color=color, linewidth=2)
            
            current_pos = next_pos
            points.append(current_pos)

        # --- 5. PER-GRAPH AUTOSCALING FIX ---
        pts = np.array(points)
        max_range = np.array([pts[:,0].max()-pts[:,0].min(), 
                              pts[:,1].max()-pts[:,1].min(), 
                              pts[:,2].max()-pts[:,2].min()]).max() / 2.0
        
        # Prevent zero-range crashing
        if max_range == 0: max_range = 1.0 

        mid_x = (pts[:,0].max()+pts[:,0].min()) * 0.5
        mid_y = (pts[:,1].max()+pts[:,1].min()) * 0.5
        mid_z = (pts[:,2].max()+pts[:,2].min()) * 0.5
        
        buf = max_range * 0.1 
        ax.set_xlim(mid_x - max_range - buf, mid_x + max_range + buf)
        ax.set_ylim(mid_y - max_range - buf, mid_y + max_range + buf)
        ax.set_zlim(mid_z - max_range - buf, mid_z + max_range + buf)
        ax.set_box_aspect((1, 1, 1)) 

        # --- 6. FORMATTING ---
        ax.view_init(elev=20, azim=-50) 
        ax.set_title(f"Time: {format_time(start_t)} - {format_time(display_end_t)}", fontsize=14, pad=20)
        ax.set_xlabel('X (Roll)', fontsize=10)
        ax.set_ylabel('Y (Pitch)', fontsize=10)
        ax.set_zlabel('Z (Yaw)', fontsize=10)

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
        ax.legend(handles=legend_elements, loc='upper left', bbox_to_anchor=(0.0, 1.1), fontsize=9)

        plt.tight_layout()
        plt.show() 

    # ==========================================
    # PART 2: FULL DATA TELEMETRY CALCULATIONS
    # ==========================================
    
    # Clean dataframe to ensure stable gradients (remove true duplicate time rows if any)
    telemetry_df = df.drop_duplicates(subset=[time_col]).sort_values(by=time_col).reset_index(drop=True)
    
    full_times = telemetry_df[time_col].values
    full_directions = telemetry_df['Direction'].values
    
    global_pos = np.array([0.0, 0.0, 0.0])
    global_rot_matrix = np.eye(3) 
    
    positions_list = []
    attitudes_list = []

    for move in full_directions:
        if move in rotational_moves:
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

    # Calculate Velocity and Acceleration using central differences
    vel_x = np.gradient(pos_x, full_times)
    vel_y = np.gradient(pos_y, full_times)
    vel_z = np.gradient(pos_z, full_times)
    
    acc_x = np.gradient(vel_x, full_times)
    acc_y = np.gradient(vel_y, full_times)
    acc_z = np.gradient(vel_z, full_times)

    # ==========================================
    # PART 3: GENERATE FULL-TIME TELEMETRY GRAPHS
    # ==========================================
    
    # Graph A: Absolute Position
    fig_pos = plt.figure(figsize=(10, 5))
    plt.plot(full_times, pos_x, label='X Position')
    plt.plot(full_times, pos_y, label='Y Position')
    plt.plot(full_times, pos_z, label='Z Position')
    plt.title("Absolute Position")
    plt.xlabel("time (s)")
    plt.ylabel("Units")
    plt.grid(True)
    plt.legend()
    plt.tight_layout()
    plt.show()

    # Graph B: Attitude (Euler Angles)
    fig_att = plt.figure(figsize=(10, 5))
    plt.plot(full_times, roll_arr, label='Roll')
    plt.plot(full_times, pitch_arr, label='Pitch')
    plt.plot(full_times, yaw_arr, label='Yaw')
    plt.title("Attitude (Euler Angles)")
    plt.xlabel("time (s)")
    plt.ylabel("Degrees")
    plt.grid(True)
    plt.legend()
    plt.tight_layout()
    plt.show()

    # Graph C: Linear Velocity
    fig_vel = plt.figure(figsize=(10, 5))
    plt.plot(full_times, vel_x, label='Velocity X')
    plt.plot(full_times, vel_y, label='Velocity Y')
    plt.plot(full_times, vel_z, label='Velocity Z')
    plt.title("Linear Velocity")
    plt.xlabel("time (s)")
    plt.ylabel("Units / s")
    plt.grid(True)
    plt.legend()
    plt.tight_layout()
    plt.show()

    # Graph D: Linear Acceleration
    fig_acc = plt.figure(figsize=(10, 5))
    plt.plot(full_times, acc_x, label='Accel X')
    plt.plot(full_times, acc_y, label='Accel Y')
    plt.plot(full_times, acc_z, label='Accel Z')
    plt.title("Linear Acceleration")
    plt.xlabel("time (s)")
    plt.ylabel("Units / s²")
    plt.grid(True)
    plt.legend()
    plt.tight_layout()
    plt.show()

except Exception as e:
    print(f"Error processing {filename}: {e}")