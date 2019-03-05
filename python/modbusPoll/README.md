## IMPORTANT NOTE:

- NOTE:  When doing maintenance or configuring, run 'modbus-stop.py'.

- NOTE:  When done with maintenance or configuring, run 'modbus-start.py'.

## Installation instructions:

Install:
- Python 3 (https://www.python.org/downloads)  ... Version should be Python 3.X.X (installed on first machine was 3.4.2
- Pip Installer (http://pypi.python.org/pypi/pip ... used to install minimalmodbus)
- MinimalModbus (see https://pypi.python.org/pypi/MinimalModbus/0.6  generally... pip install -U minimalmodbus )

### Configure the script:
- Run: python modbus-config.py
- Go through the options, setting up parameters as required
    - If you are unsure of what channels the sensors are on, go through as best you can and set up 1 channel, then:
        - Once through the configurataion, run: python modbus-poll.py
        - Assuming there are no errors, Available channel numbers will appear in the brackets []... example... [1,2]
        - Reconfigure when you know the channels

### Check the script:
- run python modbus-live.py
    - This will show you a live readout of values.  It does not save them anywhere.  It is also very raw

### Setting up task scheduler:
- Go to the start menu and type "Task Scheduler" into the search field.  Open Task Scheduler
- Create a Basic Task...
    - The task will be daily... 	start at 0:00 (or whatever time you like) and recur every one day
    - It will start a program
    - For the program, navigate to the "ModBus" directory and select modbus-run.py
- With the basic task finished, find it in the Active Tasks section (double click)
- To the right, click "Properties"... (properties for the ModBus Task you created).
- You should see a screen with the task properties.
    - Check the radio button that says "Run whether user is logged on or not"
    - Check the box that says "Run with highest privileges"
    - Click the "Triggers" tab
    - Edit the Daily Trigger
        - Under Advanced Settings Click the box that says "Repeat Task Every: " and make it 1 minutes
        - Click that box that says "Stop task if it runs longer than " and make it 30 minutes
- "Ok" / password as necessary

Once setup is complete, navigate to the ModBus folder and double click "modbus-start.py"