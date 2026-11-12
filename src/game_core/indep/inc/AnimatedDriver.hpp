#pragma once
// TASK-0099 E-0099-W28. Exact original header ../indep/inc/AnimatedDriver.hpp
// is referenced in captured original constructors. Memory view is port-side.
#include "game_core/port/driving/object_view.hpp"
#include <string>
namespace hp2::debug {
enum DriverAnimationType {DRIVER_ANIMATION_TYPE_NONE=0,DRIVER_ANIMATION_TYPE_STEERING=1,
 DRIVER_ANIMATION_TYPE_IDLE=2,DRIVER_ANIMATION_TYPE_DRIVING_BACKWARDS=3,
 DRIVER_ANIMATION_TYPE_LOOKING_BACKWARDS_LEFT=4,DRIVER_ANIMATION_TYPE_LOOKING_BACKWARDS_RIGHT=5,
 DRIVER_ANIMATION_TYPE_COLLISION_LIGHT=6,DRIVER_ANIMATION_TYPE_COLLISION_FRONT=7,
 DRIVER_ANIMATION_TYPE_COLLISION_RIGHT=8,DRIVER_ANIMATION_TYPE_COLLISION_LEFT=9,
 DRIVER_ANIMATION_TYPE_COLLISION_BACK=10,DRIVER_ANIMATION_TYPE_GEAR_CHANGE=11,
 DRIVER_ANIMATION_TYPE_PRERACEG02=12,DRIVER_ANIMATION_TYPE_PRERACEG14=13,DRIVER_ANIMATION_NUM_TYPES=14};
class AnimationPlayer:public ::port::driving::ObjectView {
public:using ObjectView::ObjectView;
 int GetFrameNumber() const; //001A68E8.
 void ReleasePausedFrame(); //001A6830.
 void ReleaseAnimation(); //001A6890.
 void CancelAnimation(); //001A7DD0, E-0099-W28.
 void BeginAnimation(DriverAnimationType,int begin_time); //001A67C8.
};
int IsThisAnimationBlackballed(::port::driving::Memory&,DriverAnimationType,int car_type,std::uint32_t character_info); //001A66A8.
class AnimatedDriver:public ::port::driving::ObjectView {
public:using ObjectView::ObjectView;
 void Sub_001A6CF0(std::uint32_t car,const std::string& car_base_name);
 void BeginCollision(DriverAnimationType,int priority); //001A7538, E-0099-W28.
 void DoSnapshot(std::uint32_t snapshot); //Debug001A6EA0, E-0099-W74.
 void CancelAllAnimations(); //001A7240.
 void BeginGearChange(); //001A72C0.
 void BeginDrivingBackwards(); //001A72C8.
 void EndDrivingBackwards(); //001A72E8.
 void HandleDrivingBackwards(); //001A7310.
 void BeginLookingBackwards(); //001A7350; E-0112-C01.
 void EndLookingBackwards(); //001A7378; E-0112-C01.
 void HandleLookingBackwards(); //001A73C8; E-0112-C01.
 std::uint32_t FindAnimation(DriverAnimationType) const; //001A7700.
 void ReleaseAnimation(DriverAnimationType); //001A7750.
 int QueueAnimation(DriverAnimationType); //001A7608.
 void InitializeArtwork(); //001A6F18.
 void SetupSteeringFrame(); //001A6B90.
 void Sub_001A6DE8(int); // Original pre-simulation destruction.
};
}
