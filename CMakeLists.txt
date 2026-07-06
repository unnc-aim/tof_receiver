cmake_minimum_required(VERSION 3.8)
project(tof_receiver)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake REQUIRED)
find_package(rosidl_default_generators REQUIRED)
find_package(std_msgs REQUIRED)

# 生成自定义消息 tof_receiver/TOF
rosidl_generate_interfaces(${PROJECT_NAME}
  "msg/TOF.msg"
  DEPENDENCIES std_msgs
)
ament_export_dependencies(rosidl_default_runtime)

# 安装 Python 节点可执行脚本
install(PROGRAMS
  tof_receiver_node.py
  DESTINATION lib/${PROJECT_NAME}
  RENAME tof_receiver_node
)

# 安装 launch 文件
install(DIRECTORY
  launch
  DESTINATION share/${PROJECT_NAME}
)

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  ament_lint_auto_find_test_dependencies()
endif()

ament_package()
