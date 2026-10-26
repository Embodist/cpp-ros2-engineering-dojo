from launch import LaunchDescription
from launch_ros.actions import Node


def generate_launch_description():
    return LaunchDescription([
        Node(package='robot_dojo_pipeline', executable='sensor_source', output='screen',
             parameters=[{'frequency_hz': 10.0}]),
        Node(package='robot_dojo_pipeline', executable='sensor_processor', output='screen'),
        Node(package='robot_dojo_pipeline', executable='sensor_monitor', output='screen'),
    ])
