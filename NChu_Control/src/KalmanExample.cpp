#include "KalmanExample.hpp"
#include "ArduinoEigen.h"

//以下は拡張カルマンフィルタ

void KalmanFilter::update(Eigen::VectorXf y, Eigen::VectorXf gyro, Eigen::VectorXf acc, Eigen::VectorXf mag, float dt) {
	x += f(x,gyro);
  	F = Jf(x,gyro);
  	P = F * P * F.transpose() + B * Q * B.transpose();
  	H = Jh(x);
  	Eigen::MatrixXf G = P * H.transpose() * (H * P * H.transpose() + R).inverse();
  	x += G * (y - h(x));
  	P -= G * H * P;
	}

Eigen::VectorXf KalmanFilter::get_x(Eigen::VectorXf euler){
	float phi, theta, psi;
	float q0, q1, q2, q3;
	phi = euler(0);
	theta = euler(1);
	psi = euler(2);

	q0 = cos(phi/2.0f)*cos(theta/2.0f)*cos(psi/2.0f)+sin(phi/2.0f)*sin(theta/2.0f)*sin(psi/2.0f);
	q1 = sin(phi/2.0f)*cos(theta/2.0f)*cos(psi/2.0f)-cos(phi/2.0f)*sin(theta/2.0f)*sin(psi/2.0f);
	q2 = cos(phi/2.0f)*sin(theta/2.0f)*cos(psi/2.0f)+sin(phi/2.0f)*cos(theta/2.0f)*sin(psi/2.0f);
	q3 = cos(phi/2.0f)*cos(theta/2.0f)*sin(psi/2.0f)-sin(phi/2.0f)*sin(theta/2.0f)*cos(psi/2.0f);

	x << q0,q1,q2,q3;
	return x;
}

Eigen::VectorXf KalmanFilter::f(Eigen::VectorXf x, Eigen::VectorXf gyro){
	Eigen::Matrix4f a;
	Eigen::MatrixXf omega(4,1);
	omega << 0,gyro(0),gyro(1),gyro(2);
	a << x(3),-x(2),x(1),x(0),
	     x(2),x(3),-x(0),x(1),
		 -x(1),x(0),x(3),x(2),
		 -x(0),-x(1),-x(2),x(3);

	Eigen::VectorXf xdot;
	xdot = 1.0/2*a*omega;//xdotがクォータニオンの微分
	return xdot;
}

Eigen::VectorXf KalmanFilter::h(Eigen::VectorXf x){
	float q0,q1,q2,q3;
	q0 = x(0);
	q1 = x(1);
	q2 = x(2);
	q3 = x(3);

	Eigen::MatrixXf R;
	// 対角成分
	R(0, 0) = q0*q0 + q1*q1 - q2*q2 - q3*q3;
	R(1, 1) = q0*q0 - q1*q1 + q2*q2 - q3*q3;
	R(2, 2) = q0*q0 - q1*q1 - q2*q2 + q3*q3;
	// 対角成分以外
	R(0, 1) = 2.0f*(q1*q2 - q0*q3);
	R(1, 0) = 2.0f*(q1*q2 + q0*q3);
	R(0, 2) = 2.0f*(q1*q3 + q0*q2);
	R(2, 0) = 2.0f*(q1*q3 - q0*q2);
	R(2, 1) = 2.0f*(q2*q3 + q0*q1);
	R(1, 2) = 2.0f*(q2*q3 - q0*q1);
	Eigen::MatrixXf R_total;
	R_total << R,0,
	           0,R;
	Eigen::VectorXf earth;
	earth << 0, 0, g, mag_calib(0), mag_calib(1), mag_calib(2);
	return R_total*earth;
	//返り値は観測量で、[ax,ay,az,mx,my,mz]
}

Eigen::MatrixXf KalmanFilter::Jf(Eigen::VectorXf x, Eigen::VectorXf gyro){
	Eigen::Matrix4f F; //Fはf(x)のJacobian
	F << gyro(3),gyro(2),-gyro(1),0,
	     -gyro(2),gyro(3),0,gyro(1),
		 gyro(1),0,gyro(3),gyro(2),
		 0,-gyro(1),-gyro(2),gyro(3);

	return F;
}

Eigen::MatrixXf KalmanFilter::Jh(Eigen::VectorXf x){
	float q0,q1,q2,q3,mx,my,mz;
	q0 = x(0);
	q1 = x(1);
	q2 = x(2);
	q3 = x(3);
	mx = mag_calib(0);
	my = mag_calib(1);
	mz = mag_calib(2);

	Eigen::MatrixXf H; //Hはh(x)のJacobian
	F << 2*g*q2, 2*g*q3, 2*g*q0, 2*g*q1,
	     -2*g*q1, -2*g*q0, 2*g*q3, 2*g*q2,
		 2*g*q0, -2*g*q1, -2*g*q2, 2*g*q3,
		 2*mx*q0-2*my*q3+2*mz*q2, 2*mx*q1+2*my*q2+2*mz*q3, -2*mx*q2+2*my*q1+2*mz*q0, -2*mx*q3-2*my*q0+2*mz*q1,
		 2*mx*q3+2*my*q0-2*mz*q1, 2*mx*q2-2*my*q1-2*mz*q0, 2*mx*q1+2*my*q2+2*mz*q3, 2*mx*q0-2*my*q3+2*mz*q2,
		 -2*mx*q2+2*my*q1+2*mz*q0, 2*mx*q3+2*my*q0-2*mz*q1, -2*mx*q0+2*my*q3-2*mz*q2, 2*mx*q1+2*my*q2+2*mz*q3;

	return H;
}
 //線形kf
// //値を代入する関数
// void KalmanFilter::setmatrix(Eigen::MatrixXf A_in, Eigen::MatrixXf B_in, Eigen::MatrixXf C_in,
// Eigen::MatrixXf Q_in, Eigen::MatrixXf R_in, Eigen::MatrixXf P_in, Eigen::VectorXf x_in){
// 	//nは状態変数の数、mは観測変数の次元、rは
//     A = A_in;
//  	B = B_in;
//  	C = C_in;
//  	Q = Q_in;
//  	R = R_in;
//  	x = x_in;
//  	P = P_in;
// }

// void KalmanFilter::update(Eigen::VectorXf y){
//     x = A*x;
//  	P = A*P*A.transpose() + B*Q*B.transpose();
//  	Eigen::MatrixXf G = P*C.transpose() * (C*P*C.transpose() + R).inverse();
//  	x += G*(y - C*x);
//  	P = P - G*C*P;
// }
