#include <rclcpp/rclcpp.hpp>
#include <std_msgs/msg/float64_multi_array.hpp>
#include <memory>
class SensorMonitor : public rclcpp::Node {
public:
    SensorMonitor() : Node("sensor_monitor") {
        subscriber_ = create_subscription<std_msgs::msg::Float64MultiArray>("sensor/processed",10,
            [this](std_msgs::msg::Float64MultiArray::ConstSharedPtr msg) {
                ++seen_;
                // TODO: 消息时序、丢弃率、监控指标；不要高频打印完整 payload。
                if (seen_ % 100 == 0) RCLCPP_INFO(get_logger(),"seen=%zu length=%zu",seen_,msg->data.size());
            });
    }
private:
    rclcpp::Subscription<std_msgs::msg::Float64MultiArray>::SharedPtr subscriber_;
    std::size_t seen_ = 0;
};
int main(int argc,char** argv) { rclcpp::init(argc,argv); rclcpp::spin(std::make_shared<SensorMonitor>()); rclcpp::shutdown(); return 0; }
