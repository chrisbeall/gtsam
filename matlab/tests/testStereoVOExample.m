%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% GTSAM Copyright 2010, Georgia Tech Research Corporation,
% Atlanta, Georgia 30332-0415
% All Rights Reserved
% Authors: Frank Dellaert, et al. (see THANKS for the full author list)
%
% See LICENSE for the license information
%
% @brief Basic VO Example with 3 landmarks and two cameras
% @author Chris Beall
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

%% Assumptions
%  - For simplicity this example is in the camera's coordinate frame
%  - X: right, Y: down, Z: forward
%  - Pose x1 is at the origin, Pose 2 is 1 meter forward (along Z-axis)
%  - x1 is fixed with a constraint, x2 is initialized with noisy values
%  - No noise on measurements

%% Create keys for variables
x1 = symbol('x',1); x2 = symbol('x',2); 
l1 = symbol('l',1); l2 = symbol('l',2); l3 = symbol('l',3);

%% Create graph container and add factors to it
graph = visualSLAMGraph;

%% add a constraint on the starting pose
first_pose = gtsamPose3();
graph.addPoseConstraint(x1, first_pose);

%% Create realistic calibration and measurement noise model
% format: fx fy skew cx cy baseline
K = gtsamCal3_S2Stereo(1000, 1000, 0, 320, 240, 0.2);
stereo_model = gtsamnoiseModelDiagonal.Sigmas([1.0; 1.0; 1.0]);

%% Add measurements
% pose 1
graph.addStereoMeasurement(gtsamStereoPoint2(520, 480, 440), stereo_model, x1, l1, K);
graph.addStereoMeasurement(gtsamStereoPoint2(120,  80, 440), stereo_model, x1, l2, K);
graph.addStereoMeasurement(gtsamStereoPoint2(320, 280, 140), stereo_model, x1, l3, K);

%pose 2
graph.addStereoMeasurement(gtsamStereoPoint2(570, 520, 490), stereo_model, x2, l1, K);
graph.addStereoMeasurement(gtsamStereoPoint2( 70,  20, 490), stereo_model, x2, l2, K);
graph.addStereoMeasurement(gtsamStereoPoint2(320, 270, 115), stereo_model, x2, l3, K);


%% Create initial estimate for camera poses and landmarks
initialEstimate = visualSLAMValues;
initialEstimate.insertPose(x1, first_pose);
% noisy estimate for pose 2
initialEstimate.insertPose(x2, gtsamPose3(gtsamRot3(), gtsamPoint3(0.1,-.1,1.1)));
expected_l1 = gtsamPoint3( 1,  1, 5);
initialEstimate.insertPoint(l1, expected_l1);
initialEstimate.insertPoint(l2, gtsamPoint3(-1,  1, 5));
initialEstimate.insertPoint(l3, gtsamPoint3( 0,-.5, 5));

%% optimize
result = graph.optimize(initialEstimate,0);

%% check equality for the first pose and point
pose_x1 = result.pose(x1);
CHECK('pose_x1.equals(first_pose,1e-4)',pose_x1.equals(first_pose,1e-4));

point_l1 = result.point(l1);
CHECK('point_1.equals(expected_l1,1e-4)',point_l1.equals(expected_l1,1e-4));