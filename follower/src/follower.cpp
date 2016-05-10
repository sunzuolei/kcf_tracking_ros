#include<ros/ros.h>
#include "geometry_msgs/Twist.h"
#include <algorithm>
#include <math.h>
#include <cstring>
#include <std_msgs/String.h>
#include <kobuki_msgs/BumperEvent.h>
#include"turtlebot_cf_tracking/BoundingBox.h"



/*视频显示框的中心点*/
#define VIDEO_CENTER_X 160
#define VIDEO_CENTER_Y 120
/*中心点横纵坐标允许的误差偏移*/
#define ERROR_OFFSET_X 10
#define ERROR_OFFSET_Y 2

/*turtleBot运动线速度角速度默认值*/
#define CONTROL_SPEED 0.1
#define CONTROL_TURN 0.1
/*线速度控制和与深度的比例*/
#define CONTROL_SPEED_RATIO 0.006
/*角速度控制和偏移距离的比例*/
#define CONTROL_TURN_RATIO 0.2
/*线速度和角速度最大值*/
//#define CONTROL_SPEED_MAX 0.4
#define CONTROL_SPEED_MAX 0.3
#define CONTROL_TURN_MAX 0.5
/*消息计数的最大值*/
#define COUNT_MAX 1000000
void transform_callback(const turtlebot_cf_tracking::BoundingBox& msg);
void bumperEventCB(const kobuki_msgs::BumperEventConstPtr & msg);

struct bounding_box_info{
int centerX;
int centerY;
int area;
}BB_info;//boundingBox的信息

//double control_speed = 0;//turtlebot线速度控制
//double control_turn = 0;//turtlebot角速度控制

//int depthmini = 1600;
//int depthmax = 1700;
//int depth_value = 0;

//int count = 0;//消息发布计数
/*为bumper反馈的状态设置标志位*/
bool bumper_left_pressed_ = false;
bool bumper_center_pressed_ = false;
bool bumper_right_pressed_ = false;
bool change_direction_ = false;
bool enable_obs_avoid_ = false;

ros::Publisher pub;//声明一个全局的pub对象
int main(int argc, char **argv)
{
ros::init(argc, argv, "follower");
ros::NodeHandle m;
pub = m.advertise<geometry_msgs::Twist>("turtlebot_KCF/cmd_vel", 1000);
ros::Subscriber bumper_event_sub = m.subscribe("/mobile_base/events/bumper", 1000, bumperEventCB);
ros::Subscriber tld_msg_sub = m.subscribe("BoundingBox", 1000, transform_callback);
ros::spin();
return 0;
}
/************************************************************************
*ROS_openTLD反馈的跟踪目标大小和位置信息与turtlebot的速度控制的转换；
*如果发生碰撞，那么首先处理碰撞事件，碰撞信息与turtlebot控制信息的转换
***********************************************************************/
void transform_callback(const turtlebot_cf_tracking::BoundingBox& msg)
{
ROS_INFO("transform_callback start! ");
ROS_INFO("bool enable_obs_avoid_ in function transform_callback(): %d", enable_obs_avoid_);
ros::Rate loop_rate(10);//loop_rate(10)可以控制while(ros:ok()的循环频率
if (!enable_obs_avoid_)
{
    ROS_INFO("tracking control program start!");
    int x = msg.x;
    int y = msg.y;
    int width = msg.width;
    int height = msg.height;
    ROS_INFO("x,y,width,height:%d %d %d %d",x,y,width,height);
  //  ROS_INFO("turtlebot tracking control message count: %d",count);

    BB_info.centerX = x+width/2;
    BB_info.centerY = y-height/2;//由于turtleBot不能在三维平面移动，centerY在这里实际上没有起作用
    BB_info.area = width * height;   
  //  ROS_INFO("transformat successs!") ;
    /*发布turtleBot控制消息*/
    geometry_msgs::Twist twist;
    twist.linear.x = msg.controlSpeed; twist.linear.y = 0; twist.linear.z = 0;
    twist.angular.x = 0; twist.angular.y = 0; twist.angular.z = msg.controlTurn;
    pub.publish(twist);
    //ros::spinOnce();
  //  loop_rate.sleep();
    ROS_INFO("Publish turtlebot control message sucess!");
  //  }
}
/*如果enable_obs_avoid被置位，那么启动避障模块obs_avoid*/
else
{
ROS_INFO("Obstacle avoiding program start!");
geometry_msgs::Twist cmd2;
int cmd_count = 0;
if(bumper_left_pressed_)
{
ROS_INFO("Bumper_left_ is pressing!");
while(ros::ok()&&change_direction_)
{
cmd_count ++;
ROS_INFO("I am changing!");
cmd2.angular.z = -0.4;
cmd2.linear.x = -0.2;
pub.publish(cmd2);
loop_rate.sleep();
if (cmd_count > 15)
{
change_direction_ = false;
cmd_count = 0;
}
//break ;
}
}
else if(bumper_center_pressed_)
{
while(ros::ok()&&change_direction_)
{
cmd_count ++;
ROS_INFO("I am changing!");
cmd2.angular.z = -0.5;
cmd2.linear.x = -0.2 ;
pub.publish(cmd2);
loop_rate.sleep();
if (cmd_count > 20)
{
change_direction_ = false;
cmd_count = 0;
}
//break;
}
}
else if(bumper_right_pressed_)
{
while(ros::ok()&&change_direction_)
{
cmd_count ++;
ROS_INFO("I am changing!");
cmd2.angular.z = 0.4;
cmd2.linear.x = -0.2;
pub.publish(cmd2);
loop_rate.sleep();
if (cmd_count > 15)
{
change_direction_ = false;
cmd_count = 0;
}
//break;
}
}
enable_obs_avoid_ = false;//避障程序结束之后，把enable_obs_avoid之后重新置为false.
}
//if(count == COUNT_MAX) count = 1;//count大于一定值，可能会报溢出，每发布100000条消息，把count重置1
}
/************************************************************************************
碰撞事件的回调函数，当碰撞发生时，检测左侧碰撞，中心碰撞， 还是右侧碰撞，
分别置不同的flag位：bumper_left_pressed_，bumper_center_pressed_，bumper_right_pressed_
*************************************************************************************/
void bumperEventCB(const kobuki_msgs::BumperEventConstPtr & msg)
{
ROS_INFO("bumperEventCB() start!");
ROS_INFO("bool enable_obstacle_avoid_ in function bumperEventCB(): %d", enable_obs_avoid_);
if (msg->state == kobuki_msgs::BumperEvent::PRESSED)
{
enable_obs_avoid_ = true;//如果检测到碰撞，把避障使能位置为true
switch (msg->bumper)
{
case kobuki_msgs::BumperEvent::LEFT:
if (!bumper_left_pressed_)
{
bumper_left_pressed_ = true;
ROS_INFO("bumper_left_pressed_,bumperEventCB") ;
change_direction_ = true;
}
break;
case kobuki_msgs::BumperEvent::CENTER:
if (!bumper_center_pressed_)
{
bumper_center_pressed_ = true;
ROS_INFO("bumper_center_pressed_,bumperEventCB") ;
change_direction_ = true;
}
break;
case kobuki_msgs::BumperEvent::RIGHT:
if (!bumper_right_pressed_)
{
bumper_right_pressed_ = true;
change_direction_ = true;
ROS_INFO("bumper_right_pressed_") ;
}
break;
}
}
else
{
switch (msg->bumper)
{
case kobuki_msgs::BumperEvent::LEFT:
bumper_left_pressed_ = false;
break;
case kobuki_msgs::BumperEvent::CENTER:
bumper_center_pressed_ = false;
break;
case kobuki_msgs::BumperEvent::RIGHT:
bumper_right_pressed_ = false;
break;
}
}
ROS_INFO("bumperEventCB end!");
}

