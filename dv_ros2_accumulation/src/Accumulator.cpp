#include "dv_ros2_accumulation/Accumulator.h"

namespace dv_ros2_accumulation
{
    Accumulator::Accumulator(const std::string &t_node_name)
    : Node(t_node_name), m_node{this}
    {
        RCLCPP_INFO(m_node->get_logger(), "Constructor is initialized");
        parameterInitilization();

        if (!readParameters())
        {
            RCLCPP_ERROR(m_node->get_logger(), "Failed to read parameters");
            rclcpp::shutdown();
            std::exit(EXIT_FAILURE);
        }

        parameterPrinter();

        m_events_subscriber = m_node->create_subscription<dv_ros2_msgs::msg::EventArray>("events", 10, std::bind(&Accumulator::eventCallback, this, std::placeholders::_1));
        m_frame_publisher = m_node->create_publisher<sensor_msgs::msg::Image>("image", 10);
        m_slicer = std::make_unique<dv::EventStreamSlicer>();

        RCLCPP_INFO(m_node->get_logger(), "Successfully launched.");
    }

    void Accumulator::start()
    {
        m_accumulation_thread = std::thread(&Accumulator::accumulate, this);
        RCLCPP_INFO(m_node->get_logger(), "Accumulation started");
    }

    void Accumulator::stop()
    {
        RCLCPP_INFO(m_node->get_logger(), "Stopping the accumulation node...");
        m_spin_thread = false;
        m_accumulation_thread.join();
    }

    bool Accumulator::isRunning() const
    {
        return m_spin_thread.load(std::memory_order_relaxed);
    }

    void Accumulator::eventCallback(dv_ros2_msgs::msg::EventArray::SharedPtr events)
    {
        if (m_accumulator == nullptr)
        {
            m_accumulator = std::make_unique<dv::Accumulator>(cv::Size(events->width, events->height));
            updateConfiguration();
        }
        auto store = dv_ros2_msgs::toEventStore(*events);

        try
        {
            m_slicer->accept(store);
        }
        catch (std::out_of_range &e)
        {
            RCLCPP_WARN_STREAM(m_node->get_logger(), "Event out of range: " << e.what());
        }

    }

    void Accumulator::slicerCallback(const dv::EventStore &events)
    {
        m_event_queue.push(events);
    }

    void Accumulator::updateConfiguration()
    {
        m_accumulator->setEventContribution(m_params.event_contribution);
        m_accumulator->setDecayParam(m_params.decay_param);
        m_accumulator->setMinPotential(m_params.min_potential);
        m_accumulator->setMaxPotential(m_params.max_potential);
        m_accumulator->setNeutralPotential(m_params.neutral_potential);
        m_accumulator->setRectifyPolarity(m_params.rectify_polarity);
        m_accumulator->setSynchronousDecay(m_params.synchronous_decay);
        m_accumulator->setDecayFunction(static_cast<dv::Accumulator::Decay>(m_params.decay_function));

        switch (m_params.slice_method)
        {
            case static_cast<int>(SliceMethod::TIME):
            {
                m_slicer->doEveryTimeInterval(dv::Duration(m_params.accumulation_time * 1000LL), std::bind(&Accumulator::slicerCallback, this, std::placeholders::_1));
                break;
            }
            case static_cast<int>(SliceMethod::NUMBER):
            {
                m_slicer->doEveryNumberOfEvents(m_params.accumulation_number, std::bind(&Accumulator::slicerCallback, this, std::placeholders::_1));
                break;
            }
            default:
            {
                throw dv::exceptions::InvalidArgument<int>("Unknown slicing method id", m_params.slice_method);
            }
        }
    }

    void Accumulator::accumulate()
    {
        RCLCPP_INFO(m_node->get_logger(), "Starting accumulation.");
        
        while (m_spin_thread)
        {
            if (m_accumulator != nullptr)
            {
                m_event_queue.consume_all([&](const dv::EventStore &events)
                {
                    m_accumulator->accept(events);
                    dv::Frame frame = m_accumulator->generateFrame();
                    sensor_msgs::msg::Image msg = dv_ros2_msgs::toRosImageMessage(frame.image);
                    msg.header.stamp = dv_ros2_msgs::toRosTime(frame.timestamp);
                    m_frame_publisher->publish(msg);
                });
            }
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }
    }



   
    Accumulator::~Accumulator()
    {
        RCLCPP_INFO(m_node->get_logger(), "Destructor is activated. ");
        stop();
        rclcpp::shutdown();
    }

    inline void Accumulator::parameterInitilization() const
    {

        m_node->declare_parameter("accumulation_time", m_params.accumulation_time);
        m_node->declare_parameter("accumulation_number", m_params.accumulation_number);
        m_node->declare_parameter("synchronous_decay", m_params.synchronous_decay);
        m_node->declare_parameter("min_potential", m_params.min_potential);
        m_node->declare_parameter("max_potential", m_params.max_potential);
        m_node->declare_parameter("neutral_potential", m_params.neutral_potential);
        m_node->declare_parameter("event_contribution", m_params.event_contribution);
        m_node->declare_parameter("rectify_polarity", m_params.rectify_polarity);
        m_node->declare_parameter("decay_param", m_params.decay_param);
        m_node->declare_parameter("slice_method", m_params.slice_method);
        m_node->declare_parameter("decay_function", m_params.decay_function);
    }

    inline void Accumulator::parameterPrinter() const
    {
        RCLCPP_INFO(m_node->get_logger(), "-------- Parameters --------");
        RCLCPP_INFO(m_node->get_logger(), "accumulation_time: %d", m_params.accumulation_time);
        RCLCPP_INFO(m_node->get_logger(), "accumulation_number: %d", m_params.accumulation_number);
        RCLCPP_INFO(m_node->get_logger(), "synchronous_decay: %s", m_params.synchronous_decay ? "true" : "false");
        RCLCPP_INFO(m_node->get_logger(), "min_potential: %f", m_params.min_potential);
        RCLCPP_INFO(m_node->get_logger(), "max_potential: %f", m_params.max_potential);
        RCLCPP_INFO(m_node->get_logger(), "neutral_potential: %f", m_params.neutral_potential);
        RCLCPP_INFO(m_node->get_logger(), "event_contribution: %f", m_params.event_contribution);
        RCLCPP_INFO(m_node->get_logger(), "rectify_polarity: %s", m_params.rectify_polarity ? "true" : "false");
        RCLCPP_INFO(m_node->get_logger(), "decay_param: %f", m_params.decay_param);
        RCLCPP_INFO(m_node->get_logger(), "slice_method: %d", m_params.slice_method);
        RCLCPP_INFO(m_node->get_logger(), "decay_function: %d", m_params.decay_function);
    }

    inline bool Accumulator::readParameters()
    {
        if (!m_node->get_parameter("accumulation_time", m_params.accumulation_time))
        {
            RCLCPP_ERROR(m_node->get_logger(), "Failed to read parameter accumulation_time");
            return false;
        }
        if (!m_node->get_parameter("accumulation_number", m_params.accumulation_number))
        {
            RCLCPP_ERROR(m_node->get_logger(), "Failed to read parameter accumulation_number");
            return false;
        }
        if (!m_node->get_parameter("synchronous_decay", m_params.synchronous_decay))
        {
            RCLCPP_ERROR(m_node->get_logger(), "Failed to read parameter synchronous_decay");
            return false;
        }
        if (!m_node->get_parameter("min_potential", m_params.min_potential))
        {
            RCLCPP_ERROR(m_node->get_logger(), "Failed to read parameter min_potential");
            return false;
        }
        if (!m_node->get_parameter("max_potential", m_params.max_potential))
        {
            RCLCPP_ERROR(m_node->get_logger(), "Failed to read parameter max_potential");
            return false;
        }
        if (!m_node->get_parameter("neutral_potential", m_params.neutral_potential))
        {
            RCLCPP_ERROR(m_node->get_logger(), "Failed to read parameter neutral_potential");
            return false;
        }
        if (!m_node->get_parameter("event_contribution", m_params.event_contribution))
        {
            RCLCPP_ERROR(m_node->get_logger(), "Failed to read parameter event_contribution");
            return false;
        }
        if (!m_node->get_parameter("rectify_polarity", m_params.rectify_polarity))
        {
            RCLCPP_ERROR(m_node->get_logger(), "Failed to read parameter rectify_polarity");
            return false;
        }
        if (!m_node->get_parameter("decay_param", m_params.decay_param))
        {
            RCLCPP_ERROR(m_node->get_logger(), "Failed to read parameter decay_param");
            return false;
        }
        if (!m_node->get_parameter("slice_method", m_params.slice_method))
        {
            RCLCPP_ERROR(m_node->get_logger(), "Failed to read parameter slice_method");
            return false;
        }
        if (!m_node->get_parameter("decay_function", m_params.decay_function))
        {
            RCLCPP_ERROR(m_node->get_logger(), "Failed to read parameter decay_function");
            return false;
        }
        return true;
    }

}  // namespace dv_ros2_accumulation