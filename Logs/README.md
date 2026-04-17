# Overview
The DTO-Project/Logs folder is responsible for holding project-level log files, including the running changelog that tracks all version updates and feature changes across the codebase.

# File Tree
```
DTO-Project/
└── Logs/
    ├── Activity_Log.csv
    ├── Keybind_Log.csv
    ├── changelogs.md
    └── README.md
```

## File Tree Description
- **Activity_Log.csv** Timestamped STATUS and ERROR code log from the most recent controller session; mirrors the format written by DTOController.cpp and DTOManualTesting.cpp.
- **Keybind_Log.csv** Timestamped key event log from the most recent controller session; records type, direction, and key name per event.
- **changelogs.md** Running version history for the project, documenting new features, bug fixes, and structural changes per release.
- **README.md** General overview of project folder.
