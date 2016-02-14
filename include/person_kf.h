#ifndef __PERSON_KF_H__
#define __PERSON_KF_H__

#include <Eigen/Core>
#include <Eigen/Dense>
#include <ros/ros.h>
//#include <Eigen/LU>

#define PERSONKF_POS_VAR 0.1
#define PERSONKF_VEL_NOISE_VAR 0.2  

struct PersonKF
{
	// State vector: [x (m), y (m), vx (m/s), vy (m/s)]
	Eigen::MatrixXd x;
	Eigen::MatrixXd P;
	
	// Person ID
	int id;
	
	// Last time update
	ros::Time tStamp;
	
	// Updated flag
	bool updated;
	
	// Default constructor
	PersonKF(void) : x(4,1), P(4,4) 
	{
		x.setZero(4, 1);
		P.setIdentity(4, 4);
		id = -1;
		tStamp = ros::Time::now();
		updated = false;
	}
	
	PersonKF(const PersonKF &data) : x(4,1), P(4,4)
	{
		x = data.x;
		P = data.P;
		id = data.id;
		tStamp = data.tStamp;
		updated = data.updated;
	}
	
	PersonKF &operator=(const PersonKF &data)
	{
		x = data.x;
		P = data.P;
		id = data.id;
		tStamp = data.tStamp;
		updated = data.updated;
		return *this;
	}
	
	// Filter initialization, position in m and velocity on m/s 
	void init(int _id, double _x, double _y, double _vx, double _vy)
	{
		// Get person ID
		id = _id;
		
		// Setup state vector
		x.setZero(4, 1);
		x(0,0) = _x;
		x(1,0) = _y;
		x(2,0) = _vx;
		x(3,0) = _vy;
		
		// Setup cov matrix
		P.setIdentity(4, 4);
		P(0,0) = PERSONKF_POS_VAR;
		P(1,1) = PERSONKF_POS_VAR;
		P(2,2) = 1.0*1.0;
		P(3,3) = 1.0*1.0;
		
		// Update time stamp
		tStamp = ros::Time::now();
		updated = false;
	}
	
	// State prediction, time in seconds 
	void predict(double _dt)
	{
		// State vector prediction
		x(0,0) += x(2,0)*_dt;
		x(1,0) += x(3,0)*_dt;
		
		// Convariance matrix prediction
		Eigen::Matrix<double, 4, 4> F;
		F.setIdentity(4, 4);
		F(0,2) = _dt;
		F(1,3) = _dt;
		Eigen::Matrix<double, 4, 4> Q;
		Q.setZero(4, 4);
		Q(2,2) = PERSONKF_VEL_NOISE_VAR*_dt*_dt;
		Q(3,3) = PERSONKF_VEL_NOISE_VAR*_dt*_dt;
		P = F*P*F.transpose() + Q;
		
		updated = false;
	}
	
	// State update
	void update(double _x, double _y, ros::Time _t)
	{
		// Update time stamp
		tStamp = _t;
		
		// Compute update jacobian
		Eigen::Matrix<double, 2, 4> H;
		H.setZero(2, 4);
		H(0,0) = 1.0;
		H(1,1) = 1.0;
		
		// Compute update noise matrix
		Eigen::Matrix<double, 2, 2> R;
		R.setZero(2, 2);
		R(0,0) = PERSONKF_POS_VAR;
		R(1,1) = PERSONKF_POS_VAR;
		
		// Calculate innovation matrix
		Eigen::Matrix<double, 2, 2> S;
		S = H*P*H.transpose() + R;
		
		// Calculate kalman gain
		Eigen::Matrix<double, 4, 2> K;
		K = P*H.transpose()*S.inverse();
		
		// Calculate innovation vector
		Eigen::Matrix<double, 2, 1> y;
		y(0,0) = _x - x(0,0);
		y(1,0) = _y - x(1,0);
		
		// Calculate new state vector
		x = x + K*y;
		
		// Calculate new cov matrix
		Eigen::Matrix<double, 4, 4> I;
		I.setIdentity(4, 4);
		P = (I - K*H)*P;
		
		updated = true;
	}
};

#endif