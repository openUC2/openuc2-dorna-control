# Dorna AI Robot Interface

**FROM HERE: https://github.com/aicell-lab/reef-imaging/tree/main/reef_imaging/control/dorna-control**

This document provides a brief overview of the interface for the Dorna AI Robot.  
Here you will learn how to initialize the robot and how to execute the simplest commands.

## Installation

1. make sure that Python 3.7 or higher is installed  
2. install all required dependencies, for example
   ```bash
   pip3 install dorna
3. connect the robot to your network or USB port according to the official instructions.


# First steps

- Set the IP address or serial port of the robot.
- Perform simple movements and read status data (position, speed, etc.).

# Example code

```py
# This example shows a trivial usage of the Dorna interface.
# Please replace 'YOUR_BOT_IP_OR_PORT' with the actual address of your robot.

import dorna

def connect_and_move():
    # Create a Dorna object
    robot = dorna.Dorna()
    
    # Connect to the robot
    robot.connect('YOUR_BOT_IP_OR_PORT')
    
    # Send a simple command
    # Move the robot joints or coordinate system according to your setup
    robot.play({“command”: “jmove”, “j0”: 10, “j1”: 0, “speed”: 100})
    
    # Disconnect after usage
    robot.disconnect()

if __name__ == “__main__”:
    connect_and_move()
```

# Installation

```py
cd dorna2
pip install -e .
```