# DV ROS2

This package contains ROS2 wrappers for iniVation cameras. The package is based on the [dv-ros](https://gitlab.com/inivation/dv/dv-ros) repository and has been adapted to work with ROS2. The package is still under development and some features are not yet implemented. This project is not affiliated with iniVation AG.

<p align="center">
    <img src="images/event_camera_images.gif" width=1000>
</p>

<!-- ## Installation -->


<!-- First, clone the repository.
    
```
git clone https://github.com/Telios/dv-ros2.git
```

For ease of use, build the packages with docker:
```
cd docker && docker compose up
xhost +local:docker
docker compose up
```
Otherwise, follow the instructions below. -->

### Dependencies
Install dependencies using the following commands:

```bash
sudo add-apt-repository ppa:inivation-ppa/inivation
sudo apt-get update
sudo apt-get install dv-processing cmake boost-inivation libopencv-dev libeigen3-dev libcaer-dev libfmt-dev liblz4-dev libzstd-dev libssl-dev
sudo apt-get install dv-runtime-dev
```

###  Installation
Tested on Ubuntu 22.04 with ROS2 Humble.

- Prepare workspace  
```bash
mkdir ~/event_ws
cd ~/event_ws
mkdir src
cd src
```
- clone repo
```bash
git clone https://github.com/Murdism/dv-ros2.git
```
Source ROS2:
```bash
source /opt/ros/humble/setup.bash
```
- Build and source:

```bash
colcon build
source install/setup.bash
```


### Run

- To only capture and publish events at /events topic use the following command:  
```bash
ros2 run dv_ros2_capture dv_ros2_capture_node 
```
- To capture, publish and vizualize events use the following command:
```bash
ros2 launch dv_ros2_visualization all.launch.py
```
This should open a GUI with the 4 different camera streams.
 

### Record Events
- To record ros2 bag of events use the following command:
```bash
ros2 bag record -o <output_directory> <topic_names>
```
```bash
 ros2 bag record -o events_rosbag /events
```

#### Run from Rosbag (Replay Events)
Replay recorded events:
```bash
ros2 bag play /path/to/rosbag 
```
To visualize results launch viz_rosbag

```bash
ros2 launch dv_ros2_visualization viz_rosbag.py 
```

### Repository structure

The repository contains multiple projects:

- `dv_ros2_msgs`: Basic msg and srv definitions for the ROS2 interface.
- `dv_ros2_messaging`: C++ headers required to use dv-processing in ROS2.
- `dv_ros2_capture`: Camera driver node (supports live camera data streaming and aedat4 file playback)
- `dv_ros2_accumulation`: Event stream to frame/edge accumulation
- `dv_ros2_aedat4`: Convert aedat4 files to rosbags 
- `dv_ros2_runtime_modules`: DV runtime modules for integration with ROS
- `dv_ros2_visualization`: Simple visualization of events
- `dv_ros2_tracker`: Lucas-Kanade feature trackers for event and image streams
