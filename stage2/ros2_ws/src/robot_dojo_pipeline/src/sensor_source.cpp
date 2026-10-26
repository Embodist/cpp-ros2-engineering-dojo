#include <rclcpp/rclcpp.hpp>
#include <std_msgs/msg/float64_multi_array.hpp>
#include <chrono>
#include <memory>
using namespace std::chrono_literals;

class SensorSource : public rclcpp::Node {
public:
    SensorSource() : Node("sensor_source") {
        declare_parameter<double>("frequency_hz", 10.0);
        // TODO: 配置 QoS / 设置定时周期；这里只提供最小编译骨架。
        publisher_ = create_publisher<std_msgs::msg::Float64MultiArray>("sensor/raw", 10);
        timer_ = create_wall_timer(100ms,[this] {
            std_msgs::msg::Float64MultiArray m;
            // TODO: 真实传感器模型、序号/时间戳与频率参数；不用 vector 的 index 越界。
            m.data = {0.0};
            publisher_->publish(m);
        });
    }
private:
    rclcpp::Publisher<std_msgs::msg::Float64MultiArray>::SharedPtr publisher_;
    rclcpp::TimerBase::SharedPtr timer_;
};
int main(int argc,char** argv) { rclcpp::init(argc,argv); rclcpp::spin(std::make_shared<SensorSource>()); rclcpp::shutdown(); return 0; }
