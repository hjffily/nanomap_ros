#include "pose_manager.h"

void PoseManager::AddPose(NanoMapPose const& pose) {
	poses.push_front(pose);
}

void PoseManager::DeleteMemoryBeforeTime(NanoMapTime const& delete_time) {
	while (poses.size() >= 0) {
		NanoMapPose i = poses.back(); 
		if ( (i.time.sec <= delete_time.sec) && (i.time.nsec < delete_time.nsec) ) {
			poses.pop_back();
		}
		else {
			break;
		}
	}
}

NanoMapTime PoseManager::GetMostRecentPoseTime() const {
	return poses.front().time;
}

NanoMapTime PoseManager::GetOldestPoseTime() const {
	return poses.back().time;
}

bool PoseManager::CanInterpolatePoseAtTime(NanoMapTime const& query_time) const {
	if (poses.size() == 0) {
		return false;
	} 

	NanoMapTime oldest_time = poses.back().time;
	NanoMapTime newest_time = poses.front().time;

	std::cout << "query time " << query_time.sec << "." << query_time.nsec << std::endl;
	std::cout << "oldest_time " << oldest_time.sec << "." << oldest_time.nsec << std::endl;
	std::cout << "newest_time " << newest_time.sec << "." << newest_time.nsec << std::endl;


	if ( (query_time.sec <= oldest_time.sec) && (query_time.nsec < oldest_time.nsec) ) {
		std::cout << "returning false 1 in can interpolate" << std::endl;
		return false;
	}
	if ( (query_time.sec >= newest_time.sec) && (query_time.nsec > newest_time.nsec) ) {
		std::cout << "returning false 2 in can interpolate" << std::endl;
		return false;
	}

	return true;
}

bool PoseManager::CanInterpolatePosesForTwoTimes(NanoMapTime const& time_from, NanoMapTime const& time_to) const {
	return (CanInterpolatePoseAtTime(time_from) && CanInterpolatePoseAtTime(time_to) );	
}

NanoMapPose PoseManager::GetPoseAtTime(NanoMapTime const& query_time) {
	std::cout << "Inside GetPoseAtTime" << std::endl;

	// iterate through pose times and find bookends
	size_t oldest_pose_index = poses.size()-1;

	std::cout << "oldest_pose_index " << oldest_pose_index << std::endl;

    NanoMapPose pose_before = poses[oldest_pose_index];
    NanoMapPose pose_after;

    std::cout << "starting search" << std::endl;
    for (int i = oldest_pose_index - 1; i >= 0; i--) {
      std::cout << "i is " << i << std::endl;
      pose_after = poses[i];
      if ((pose_after.time.sec > query_time.sec) && (pose_after.time.nsec > query_time.nsec)) {
        break;
      }
      pose_before = pose_after;
    }

    std::cout << "found bookends " << std::endl;

    // find pose interpolation parameter
    double t_1  = (query_time.sec  - pose_before.time.sec) * 1.0 + (query_time.nsec - pose_before.time.nsec)/ 1.0e9;
    double t_2  = (pose_after.time.sec  - query_time.sec)  * 1.0 + (pose_after.time.nsec - query_time.nsec) / 1.0e9;

    double t_parameter = t_1 / (t_1 + t_2);

	// interpolate
	return InterpolateBetweenPoses(pose_before, pose_after, t_parameter);
}

NanoMapPose PoseManager::InterpolateBetweenPoses(NanoMapPose const& pose_before, NanoMapPose const& pose_after, double t_parameter) {
	std::cout << "Inside InterpolateBetweenPoses" << std::endl;
	std::cout << "t_parameter " << t_parameter << std::endl;

	// position interpolation
	Vector3 interpolated_vector = pose_before.position + (pose_after.position - pose_before.position)*t_parameter;

	// quaternion interpolation
	Quat interpolated_quat;
	interpolated_quat = pose_before.quaternion.slerp(t_parameter, pose_after.quaternion);

	NanoMapPose pose;
	pose.position = interpolated_vector;
	pose.quaternion = interpolated_quat;
	return pose;
}

Matrix4 PoseManager::GetRelativeTransformFromTo(NanoMapTime const& time_from, NanoMapTime const& time_to) {
	std::cout << "Inside GetRelativeTransformFromTo" << std::endl;
	std::cout << "time_from" << time_from.sec << "." << time_from.nsec << std::endl;
	std::cout << "time_to" << time_to.sec << "." << time_to.nsec << std::endl;
	NanoMapPose pose_from = GetPoseAtTime(time_from);
	NanoMapPose pose_to   = GetPoseAtTime(time_to);
	return FindTransform(pose_from, pose_to);
}

Matrix4 PoseManager::FindTransform(NanoMapPose const& new_pose, NanoMapPose const& previous_pose) {
	return FindTransform(previous_pose)*InvertTransform(FindTransform(new_pose));
}

Matrix4 PoseManager::FindTransform(NanoMapPose const& pose) {
  Matrix4 transform = Eigen::Matrix4d::Identity();
  transform.block<3,3>(0,0) = pose.quaternion.toRotationMatrix();
  transform.block<3,1>(0,3) = pose.position;
  return transform;
}

Matrix4 PoseManager::InvertTransform(Matrix4 const& transform) {
  Matrix3 R = transform.block<3,3>(0,0);
  Vector3 t = transform.block<3,1>(0,3);
  Matrix4 inverted_transform = Eigen::Matrix4d::Identity();
  inverted_transform.block<3,3>(0,0) = R.transpose();
  inverted_transform.block<3,1>(0,3) = -1.0 * R.transpose() * t;
  return inverted_transform;
}