# 基础工具
sudo apt update
sudo apt install -y software-properties-common curl gnupg2 lsb-release

# 启用 universe 源
sudo add-apt-repository universe -y

# 添加 ROS 2 GPG key
sudo curl -sSL https://raw.githubusercontent.com/ros/rosdistro/master/ros.key \
  -o /usr/share/keyrings/ros-archive-keyring.gpg

# 添加 ROS 2 软件源
echo "deb [arch=$(dpkg --print-architecture) signed-by=/usr/share/keyrings/ros-archive-keyring.gpg] \
http://packages.ros.org/ros2/ubuntu $(. /etc/os-release && echo $UBUNTU_CODENAME) main" | \
sudo tee /etc/apt/sources.list.d/ros2.list > /dev/null

# 安装 ROS 2 Humble（完整版）
sudo apt update
sudo apt install -y ros-humble-desktop

# 开发常用工具（建议）
sudo apt install -y python3-colcon-common-extensions python3-rosdep python3-vcstool

# rosdep 初始化（首次机器需要）
sudo rosdep init
rosdep update

# 配置环境变量
echo "source /opt/ros/humble/setup.bash" >> ~/.bashrc
source ~/.bashrc

# in vm clone code base repo
# SSH 方式
git clone git@github.com:kk-Syuer/UAV_UGV_monitoring.git 

# 或 HTTPS 方式
git clone https://github.com/kk-Syuer/UAV_UGV_monitoring.git 


# use vs code to open code folder and in terminal run:
cd ~/UAV_UGV_netmonitoring
rm -rf build install log
colcon build --symlink-install
source install/setup.bash
# change yaml file for different protocols 
ros2 launch system_bringup experiment.launch.py config:=system_bringup/config/runs/ugv_role_priority.yaml
