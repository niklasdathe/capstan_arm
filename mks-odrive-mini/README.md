# README
## Firmware version 0.5.6 upgrade
The axes use a "Makerbase MKS XDRIVE 56 V" as their motor driver. One for each axis. The problem is that it ships with an old version of the Firmware (0.5.1). The following things have been done to be able to upgrade to 0.5.6, which is the last open source release. 
## Configuration
odrv0.config.enable_brake_resistor=true #connect brake resitor to terminal block
odrv0.axis0.motor.config.pole_pairs=39 #eagle power 8308 has 39 magnets
odrv0.axis0.motor.config.torque_constant=8.27/90 #set to 8.27 / (motor KV)
odrv0.axis0.motor.config.motor_type #TODO gimbal or high current
odrv0.axis0.encoder.config.cpr=16384 #TODO load and motor encoder - load: 16384cpr(spreadsheet), motor: 4096cpr?


# Original README:
![ODrive Logo](https://static1.squarespace.com/static/58aff26de4fcb53b5efd2f02/t/59bf2a7959cc6872bd68be7e/1505700483663/Odrive+logo+plus+text+black.png?format=1000w)

This project is all about accurately driving brushless motors, for cheap. The aim is to make it possible to use inexpensive brushless motors in high performance robotics projects, like [this](https://www.youtube.com/watch?v=WT4E5nb3KtY).

| Branch | Build Status |
|--------|--------------|
| master | [![Build Status](https://travis-ci.org/madcowswe/ODrive.png?branch=master)](https://travis-ci.org/madcowswe/ODrive) |
| devel  | [![Build Status](https://travis-ci.org/madcowswe/ODrive.png?branch=devel)](https://travis-ci.org/madcowswe/ODrive) |

[![pip install odrive (nightly)](https://github.com/madcowswe/ODrive/workflows/pip%20install%20odrive%20(nightly)/badge.svg)](https://github.com/madcowswe/ODrive/actions?query=workflow%3A%22pip+install+odrive+%28nightly%29%22)

Please refer to the [Developer Guide](https://docs.odriverobotics.com/developer-guide) to get started with ODrive firmware development.


### Repository Structure
 * **Firmware**: ODrive firmware
 * **tools**: Python library & tools
 * **docs**: Documentation

### Other Resources

 * [Main Website](https://www.odriverobotics.com/)
 * [User Guide](https://docs.odriverobotics.com/)
 * [Forum](https://discourse.odriverobotics.com/)
 * [Chat](https://discourse.odriverobotics.com/t/come-chat-with-us/281)
