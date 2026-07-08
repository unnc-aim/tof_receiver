import os

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch_ros.actions import Node


def generate_launch_description():
    config = os.path.join(
        get_package_share_directory('tof_receiver'), 'config', 'serial_driver.yaml')

    tof_receiver_node = Node(
        package='tof_receiver',
        executable='tof_receiver_node',
        namespace='',
        output='screen',
        emulate_tty=True,
        parameters=[config],
    )

    return LaunchDescription([tof_receiver_node])
