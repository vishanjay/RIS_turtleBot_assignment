#include "ros/ros.h"
#include "geometry_msgs/Twist.h"
#include "stdlib.h"
#include "sensor_msgs/LaserScan.h"
#include "geometry_msgs/Pose.h"
#include <tf/transform_datatypes.h>
#include "nav_msgs/Odometry.h"
#include <cmath>

// global constants
const int FRONT_EDGE_INDEX = 319;
const int LEFT_EDGE_INDEX = 639;

const double WALL_FRONT_SAFETY_DIST= 1;
const double WALL_FOLLOWING_DIST_MIN = 0.2;
const double WALL_FOLLOWING_DIST_MAX = 0.4;
const double WALL_FOLLOWING_LASER_DIST_MIN = 0.4;
const double WALL_FOLLOWING_LASER_DIST_MAX = 1.0;


// global variables
double front_edge = -1.0;
double left_edge = -1.0;
double closest_edge = -1.0;

double current_x = 0;
double current_y = 0;


// function prototypes
void goStraight(geometry_msgs::Twist msg, ros::Rate loop_rate, ros::Publisher twist_pub, double goal_x = 0.5);
void turnRight(geometry_msgs::Twist msg, ros::Rate loop_rate, ros::Publisher twist_pub);
void turnLeft(geometry_msgs::Twist msg, ros::Rate loop_rate, ros::Publisher twist_pub);
void stopRobot(geometry_msgs::Twist msg, ros::Rate loop_rate, ros::Publisher twist_pub);
void correctCourse(geometry_msgs::Twist msg, ros::Rate loop_rate, ros::Publisher twist_pub);
std::map<char, bool> detectWalls();


// callback functions
void scanCallback(const sensor_msgs::LaserScan::ConstPtr& scan)
{
  int closest_edge_index = FRONT_EDGE_INDEX;
  for (int i = FRONT_EDGE_INDEX; i <= LEFT_EDGE_INDEX; i++) {
    if (scan->ranges[i] < scan->ranges[closest_edge_index]) {
      closest_edge_index = i;
    }
  }

  front_edge = scan->ranges[FRONT_EDGE_INDEX];
  left_edge = scan->ranges[LEFT_EDGE_INDEX];
  closest_edge = scan->ranges[closest_edge_index];

  ROS_INFO("closest_edge : %f", closest_edge);
}

void ComPoseCallback(const nav_msgs::Odometry::ConstPtr& msg)
{

  current_x = msg->pose.pose.position.x;
  current_y = msg->pose.pose.position.y;
  // yaw = tf::getYaw(msg->pose.pose.orientation);
}


int main(int argc, char **argv)
{
  ros::init(argc, argv, "talker");
  ros::NodeHandle n;
  ros::Publisher twist_pub = n.advertise<geometry_msgs::Twist>("/mobile_base/commands/velocity", 100);

  ros::Subscriber scanSub = n.subscribe<sensor_msgs::LaserScan>("/scan", 10, scanCallback);
  ros::Subscriber ComPose_sub = n.subscribe<nav_msgs::Odometry>("/odom", 10, ComPoseCallback);

  geometry_msgs::Twist msg;

  ros::Rate loop_rate(100);

  while (ros::ok())
  {
    ros::spinOnce();

    if (left_edge != -1) {
      std::map<char, bool> wallMap = detectWalls();

      if (!wallMap.at('f')) {
        goStraight(msg ,loop_rate, twist_pub);
      }

      else if (!wallMap.at('l')) {
        turnLeft(msg ,loop_rate, twist_pub);
      }
      else {
        turnRight(msg ,loop_rate, twist_pub);
      }

      // TODO : 2. Include bumper controllers <-- collision detection

      // ISSUE : Correcting course happens too often. Reduce it by increasing the max range or by a timer.
      correctCourse(msg ,loop_rate, twist_pub);
    }

    loop_rate.sleep();
  }

  return 0;
}

// COMPLETE!
void goStraight(geometry_msgs::Twist msg, ros::Rate loop_rate, ros::Publisher twist_pub, double goal_x) {
  // double goal_x = 1.0;
  double current_pos_x = 0.0;
  double linear_speed = 0.2;
  ros::WallTime t0 = ros::WallTime::now();

  do {
    // ROS_INFO("current_pos_x : %f", current_pos_x);
    msg.linear.x = linear_speed;
    msg.angular.z = 0;
    twist_pub.publish(msg);
    current_pos_x = linear_speed * (ros::WallTime::now().toSec() - t0.toSec());
  } while (current_pos_x < goal_x);

  stopRobot(msg ,loop_rate, twist_pub);
}

// COMPLETE!
void turnRight(geometry_msgs::Twist msg, ros::Rate loop_rate, ros::Publisher twist_pub) {
  double angular_speed = (M_PI)/36.0;
  msg.linear.x = 0;
  msg.angular.z =  -angular_speed;
  long double wanted_angle = M_PI/2;
  long double turn_angle = 0.0;
  ros::WallTime t0 = ros::WallTime::now();

  do {
    twist_pub.publish(msg);
    ros::WallTime t1 = ros::WallTime::now();
    turn_angle = wanted_angle - (t1.toSec() - t0.toSec())*angular_speed;
    loop_rate.sleep();

    // ROS_INFO("dt %f", t1.toSec() - t0.toSec());
    // ROS_INFO("turn_angle %Lf", turn_angle * 180.0 / M_PI);
  } while(turn_angle > 0);

  msg.angular.z =  0;
  twist_pub.publish(msg);
}

// COMPLETE!
void turnLeft(geometry_msgs::Twist msg, ros::Rate loop_rate, ros::Publisher twist_pub) {
  double angular_speed = (M_PI)/36.0;
  msg.linear.x = 0;
  msg.angular.z = angular_speed;
  long double wanted_angle = M_PI/2;
  long double turn_angle = 0.0;
  ros::WallTime t0 = ros::WallTime::now();

  do {
    twist_pub.publish(msg);
    ros::WallTime t1 = ros::WallTime::now();
    turn_angle = wanted_angle - (t1.toSec() - t0.toSec())*angular_speed;
    loop_rate.sleep();

    // ROS_INFO("dt %f", t1.toSec() - t0.toSec());
    // ROS_INFO("turn_angle %Lf", turn_angle * 180.0 / M_PI);
  } while(turn_angle > 0);

  msg.angular.z =  0;
  twist_pub.publish(msg);
}

// COMPLETE!
void stopRobot(geometry_msgs::Twist msg, ros::Rate loop_rate, ros::Publisher twist_pub) {
  for (int i = 0; i < 50; i++) {
    ROS_INFO_STREAM("Robot Stop");
    msg.linear.x = 0;
    msg.angular.z =  0;
    twist_pub.publish(msg);
    loop_rate.sleep();
  }
}

// COMPLETE!
void correctCourse(geometry_msgs::Twist msg, ros::Rate loop_rate, ros::Publisher twist_pub) {
  // get distance from the left wall (orientation consider later)
  // if (distance < safe_distance_min)
  //  turnRight()
  //  goStraight(safe_distance_min - distance + 0.2)
  //  turnLeft()
  //  stopRobot()
  // else if (distance > safe_distance_max || NaN) {
  //  turnLeft()
  //  goStraight(distance - safe_distance_max + 0.2)
  //  turnRight()
  //  stopRobot()

  if (left_edge < WALL_FOLLOWING_LASER_DIST_MIN) {
    turnRight(msg ,loop_rate, twist_pub);
    double correction = (WALL_FOLLOWING_DIST_MIN - (left_edge * 0.5)); // 0.5 = Sin 30 deg
    // ROS_INFO("correction : %f", correction);
    goStraight(msg ,loop_rate, twist_pub, correction);
    turnLeft(msg ,loop_rate, twist_pub);
    stopRobot(msg ,loop_rate, twist_pub);
  }
  else if (left_edge > WALL_FOLLOWING_LASER_DIST_MAX || std::isnan(left_edge)) {
    turnLeft(msg ,loop_rate, twist_pub);
    double correction = ((left_edge * 0.5) - WALL_FOLLOWING_DIST_MAX); // 0.5 = Sin 30 deg
    // ROS_INFO("correction : %f", correction);
    goStraight(msg ,loop_rate, twist_pub, correction);
    turnRight(msg ,loop_rate, twist_pub);
    stopRobot(msg ,loop_rate, twist_pub);
  }
}

// COMPLETE!
std::map<char, bool> detectWalls() {
  std::map<char, bool> wallMap;

  ROS_INFO("wallFront %f", front_edge);
  ROS_INFO("wallLeft %f", left_edge);
  // ROS_INFO("wallRight %f", right_edge);

  bool wallFront  = (closest_edge < WALL_FRONT_SAFETY_DIST) ? true : false; // front_edge changed to closest_edge
  bool wallLeft   = (left_edge < WALL_FOLLOWING_LASER_DIST_MAX) ? true : false;
  // bool wallRight  = (right_edge < WALL_FOLLOWING_LASER_DIST_MAX) ? true : false;

  ROS_INFO("wallFront %d", wallFront);
  ROS_INFO("wallLeft %d", wallLeft);
  // ROS_INFO("wallRight %d", wallRight);

  wallMap.insert(std::pair<char, bool>('f', wallFront));
  wallMap.insert(std::pair<char, bool>('l', wallLeft));
  // wallMap.insert(std::pair<char, bool>('r', wallRight));

  return wallMap;
}
