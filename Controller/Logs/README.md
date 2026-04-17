# Overview
The DTO-Project/Controller/Logs folder is responsible for holding the most recent session log files written by DTOController.cpp or DTOManualTesting.cpp at runtime.

# File Tree
```
DTO-Project/
└── Controller/
    └── Logs/
        ├── Activity_Logs.csv
        ├── Keybind_Logs.csv
        └── README.md
```

## File Tree Description
- **Activity_Logs.csv** Timestamped log of all STATUS and ERROR code entries emitted during a controller session; columns are Time(s), Code, and Description.
- **Keybind_Logs.csv** Timestamped log of all key events recorded during Operational Mode; columns are Time(s), Type, Direction, and Key.
- **README.md** General overview of project folder.
