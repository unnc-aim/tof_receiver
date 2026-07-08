//
// Created by txy on 7/8/26.
//

#ifndef TOF_RECEIVER_HPP
#define TOF_RECEIVER_HPP

#include <rclcpp/publisher.hpp>
#include <rclcpp/rclcpp.hpp>
#include <serial_driver/serial_driver.hpp>
#include <tof_msgs/msg/to_f_distances.hpp>

#include <memory>
#include <string>
#include <thread>

namespace rm_serial_driver
{
    class RMSerialDriver : public rclcpp::Node
    {
    public:
        explicit RMSerialDriver(const rclcpp::NodeOptions& options);

        ~RMSerialDriver() override;

    private:
        void getParams();

        void receiveData();

        void reopenPort();

        // Serial port
        std::unique_ptr<IoContext> owned_ctx_;
        std::string device_name_;
        std::unique_ptr<drivers::serial_driver::SerialPortConfig> device_config_;
        std::unique_ptr<drivers::serial_driver::SerialDriver> serial_driver_;

        // For debug usage
        rclcpp::Publisher<tof_msgs::msg::ToFDistances>::SharedPtr distance_pub_;
        tof_msgs::msg::ToFDistances distances_;
        uint32_t last_recv_time_;
        std::thread receive_thread_;
    };
} // namespace rm_serial_driver


#endif //TOF_RECEIVER_HPP
