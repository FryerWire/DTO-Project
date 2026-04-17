# Overview
The DTO-Project/Computations folder is responsible for providing all post-session data analysis, including 3D trajectory visualization and 2D telemetry plots derived from controller log output.

# File Tree
```
DTO-Project/
└── Computations/
    ├── compuation.py
    └── README.md
```

## File Tree Description
- **compuation.py** Reads Keybind_Log.csv output from DTOController or DTOManualTesting, reconstructs a 6-DOF trajectory via sequential rotation matrices and body-frame translations, generates segmented 3D trajectory chunk PNGs, and produces four 2D telemetry plots (position, attitude, velocity, acceleration) saved to the Images/ directory.
- **README.md** General overview of project folder.
