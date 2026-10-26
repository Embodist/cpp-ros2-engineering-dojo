#include <rclcpp/rclcpp.hpp>
#include <std_msgs/msg/float64_multi_array.hpp>
#include <memory>

class SensorProcessor : public rclcpp::Node {
public:
    SensorProcessor() : Node("sensor_processor") {
        publisher_ = create_publisher<std_msgs::msg::Float64MultiArray>("sensor/processed", 10);
        subscriber_ = create_subscription<std_msgs::msg::Float64MultiArray>("sensor/raw",10,
            [this](std_msgs::msg::Float64MultiArray::ConstSharedPtr msg) {
                std_msgs::msg::Float64MultiArray out;
                // TODO: Stage2 C01 数据滤波、错误检测、处理延迟及回调组实验。
                out.data = msg->data; // 临时 pass-through；不代表任务完成。
                publisher_->publish(out);
            });
    }
private:
    rclcpp::Publisher<std_msgs::msg::Float64MultiArray>::SharedPtr publisher_;
    rclcpp::Subscription<std_msgs::msg::Float64MultiArray>::SharedPtr subscriber_;
};
int main(int argc,char** argv) { rclcpp::init(argc,argv); rclcpp::spin(std::make_shared<SensorProcessor>()); rclcpp::shutdown(); return 0; }
