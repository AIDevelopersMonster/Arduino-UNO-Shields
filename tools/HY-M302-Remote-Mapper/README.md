# HY-M302 Remote Mapper — CLI + GUI

Host-side wizard for learning the button codes of an NEC remote and generating a reusable C++ key map for the HY_M302 library / KonSol-HY.

The tool targets Arduino UNO + HY-M302 with the onboard IR receiver on D6.

## What it does

The mapper automates the bench workflow:

1. uses the bundled UNO mapper firmware from the tool folder;
2. downloads the repository copy only if that bundled firmware is missing locally;
3. compiles it with \`arduino-cli\`;
4. uploads it to the selected Arduino UNO;
5. opens the serial port at 115200 baud;
6. asks for remote buttons one by one;
7. ignores NEC repeat frames and records full frames;
8. rejects duplicate address/command assignments;
9. saves a reusable remote profile.

Default learning sequence:

\`\`\`text
0 1 2 3 4 5 6 7 8 9
OK
HOME
RETURN
MENU
UP
DOWN
LEFT
RIGHT
POWER
\`\`\`

Current independently verified reference for the iDroid / Orange Pi remote:

\`\`\`text
OK:
Protocol = NEC
Address  = 0x04
Command  = 0x5C
Raw      = 0xA35CFB04
\`\`\`

The wizard still learns OK from the physical remote instead of hard-coding this value.

## Requirements

- Python 3
- \`arduino-cli\` in PATH
- Arduino AVR core
- pyserial

Install:

\`\`\`powershell
python -m pip install -r .\tools\HY-M302-Remote-Mapper\requirements.txt
\`\`\`

Close Arduino Serial Monitor and \`arduino-cli monitor\` before using the mapper because the COM port can have only one owner.

## GUI

From repository root:

\`\`\`powershell
.\tools\HY-M302-Remote-Mapper\run_gui.ps1
\`\`\`

or:

\`\`\`powershell
python .\tools\HY-M302-Remote-Mapper\remote_mapper.py gui
\`\`\`

Workflow:

1. select COM port;
2. set the profile name;
3. click **1. Flash mapper**;
4. click **2. Start learning**;
5. for each requested key, click **Capture current key** first, then press that button once on the remote;
6. use **Skip** if a button should not be part of the profile;
7. click **Save files**.

## CLI

List ports:

\`\`\`powershell
python .\tools\HY-M302-Remote-Mapper\remote_mapper.py ports
\`\`\`

Compile and upload mapper firmware:

\`\`\`powershell
python .\tools\HY-M302-Remote-Mapper\remote_mapper.py flash --port COM4
\`\`\`

Learn with already uploaded mapper:

\`\`\`powershell
python .\tools\HY-M302-Remote-Mapper\remote_mapper.py learn --port COM4 --name iDroid-OrangePi
\`\`\`

Compile/upload and immediately start learning:

\`\`\`powershell
python .\tools\HY-M302-Remote-Mapper\remote_mapper.py learn --port COM4 --name iDroid-OrangePi --flash
\`\`\`

For every requested button the CLI prints the received code and asks:

\`\`\`text
Accept? [Y]es / [R]etry / [S]kip:
\`\`\`

## Generated files

Profiles are stored under:

\`\`\`text
profiles/HY-M302-Remotes/<profile-name>/
\`\`\`

Generated files:

\`\`\`text
remote_map.json
HY_M302_RemoteMap.h
HY_M302_RemoteMap.cpp
\`\`\`

The JSON file is the reproducible source profile. The C++ files expose symbolic keys such as:

\`\`\`cpp
KEY_0
KEY_1
KEY_OK
KEY_HOME
KEY_RETURN
KEY_UP
KEY_DOWN
KEY_LEFT
KEY_RIGHT
KEY_POWER
\`\`\`

and a compact decoder:

\`\`\`cpp
Key decode(uint16_t address, uint8_t command);
\`\`\`

This is the bridge from raw NEC codes to the HY-M302 test menu and later KonSol events.

## Mapper firmware protocol

Full frames are machine-readable:

\`\`\`text
FRAME RAW=0xA35CFB04 ADDR=0x4 CMD=0x5C
\`\`\`

NEC repeats are reported separately:

\`\`\`text
REPEAT ADDR=0x4 CMD=0x5C
\`\`\`

The host tool ignores repeat lines while learning keys.

## Scope

This first version learns NEC remotes because NEC is physically verified on the tested HY-M302 and iDroid remote. The generated symbolic-key layer is intentionally independent of raw protocol details so other remote protocols can be added later.


## Bundled firmware

The host tool is self-contained with its UNO mapper sketch:

```text
tools/HY-M302-Remote-Mapper/
  remote_mapper.py
  run_gui.cmd
  generate_from_json.cmd
  firmware/
    RemoteMapper/
      RemoteMapper.ino
```

The GUI/CLI compiles this bundled sketch directly. The library example
`libraries/HY_M302/examples/06_Remote_Mapper/` remains useful as an Arduino
example, but the host mapper no longer depends on that example being present.

When **Start learning (auto prepare)** is pressed, the tool first checks whether
the UNO already runs the mapper firmware. If not, it compiles and uploads the
bundled `RemoteMapper.ino` automatically.
