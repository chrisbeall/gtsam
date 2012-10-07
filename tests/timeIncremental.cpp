/* ----------------------------------------------------------------------------

 * GTSAM Copyright 2010, Georgia Tech Research Corporation,
 * Atlanta, Georgia 30332-0415
 * All Rights Reserved
 * Authors: Frank Dellaert, et al. (see THANKS for the full author list)

 * See LICENSE for the license information
 * -------------------------------------------------------------------------- */

/**
 * @file    timeIncremental.cpp
 * @brief   Overall timing tests for incremental solving
 * @author  Richard Roberts
 */

#include <gtsam/base/timing.h>
#include <gtsam/slam/dataset.h>
#include <gtsam/geometry/Pose2.h>
#include <gtsam/slam/PriorFactor.h>
#include <gtsam/slam/BetweenFactor.h>
#include <gtsam/nonlinear/Symbol.h>
#include <gtsam/nonlinear/ISAM2.h>
#include <gtsam/nonlinear/Marginals.h>

using namespace std;
using namespace gtsam;
using namespace gtsam::symbol_shorthand;

typedef Pose2 Pose;

double chi2_red(const gtsam::NonlinearFactorGraph& graph, const gtsam::Values& config) {
  // Compute degrees of freedom (observations - variables)
  // In ocaml, +1 was added to the observations to account for the prior, but
  // the factor graph already includes a factor for the prior/equality constraint.
  //  double dof = graph.size() - config.size();
  int graph_dim = 0;
  BOOST_FOREACH(const boost::shared_ptr<gtsam::NonlinearFactor>& nlf, graph) {
    graph_dim += nlf->dim();
  }
  double dof = graph_dim - config.dim(); // kaess: changed to dim
  return 2. * graph.error(config) / dof; // kaess: added factor 2, graph.error returns half of actual error
}

int main(int argc, char *argv[]) {

  cout << "Loading data..." << endl;

  gttic_(Find_datafile);
  string datasetFile = findExampleDataFile("w10000-odom");
  std::pair<NonlinearFactorGraph::shared_ptr, Values::shared_ptr> data =
    load2D(datasetFile);
  gttoc_(Find_datafile);

  NonlinearFactorGraph measurements = *data.first;
  Values initial = *data.second;

  cout << "Playing forward time steps..." << endl;

  ISAM2 isam2;

  size_t nextMeasurement = 0;
  for(size_t step=1; nextMeasurement < measurements.size(); ++step) {

    Values newVariables;
    NonlinearFactorGraph newFactors;

    // Collect measurements and new variables for the current step
    gttic_(Collect_measurements);
    if(step == 1) {
      //      cout << "Initializing " << 0 << endl;
      newVariables.insert(0, Pose());
      // Add prior
      newFactors.add(PriorFactor<Pose>(0, Pose(), noiseModel::Unit::Create(Pose::Dim())));
    }
    while(nextMeasurement < measurements.size()) {

      NonlinearFactor::shared_ptr measurementf = measurements[nextMeasurement];

      if(BetweenFactor<Pose>::shared_ptr measurement =
        boost::dynamic_pointer_cast<BetweenFactor<Pose> >(measurementf))
      {
        // Stop collecting measurements that are for future steps
        if(measurement->key1() > step || measurement->key2() > step)
          break;

        // Require that one of the nodes is the current one
        if(measurement->key1() != step && measurement->key2() != step)
          throw runtime_error("Problem in data file, out-of-sequence measurements");

        // Add a new factor
        newFactors.push_back(measurement);

        // Initialize the new variable
        if(measurement->key1() == step && measurement->key2() == step-1) {
          if(step == 1)
            newVariables.insert(step, measurement->measured().inverse());
          else {
            Pose prevPose = isam2.calculateEstimate<Pose>(step-1);
            newVariables.insert(step, prevPose * measurement->measured().inverse());
          }
          //        cout << "Initializing " << step << endl;
        } else if(measurement->key2() == step && measurement->key1() == step-1) {
          if(step == 1)
            newVariables.insert(step, measurement->measured());
          else {
            Pose prevPose = isam2.calculateEstimate<Pose>(step-1);
            newVariables.insert(step, prevPose * measurement->measured());
          }
          //        cout << "Initializing " << step << endl;
        }
      } else {
        throw std::runtime_error("Unknown factor type read from data file");
      }
      ++ nextMeasurement;
    }
    gttoc_(Collect_measurements);

    // Update iSAM2
    gttic_(Update_ISAM2);
    isam2.update(newFactors, newVariables);
    gttoc_(Update_ISAM2);

    if(step % 100 == 0) {
      gttic_(chi2);
      Values estimate(isam2.calculateEstimate());
      double chi2 = chi2_red(isam2.getFactorsUnsafe(), estimate);
      cout << "chi2 = " << chi2 << endl;
      gttoc_(chi2);
    }

    tictoc_finishedIteration_();

    if(step % 1000 == 0) {
      cout << "Step " << step << endl;
      tictoc_print_();
    }
  }

  // Compute marginals
  Marginals marginals(isam2.getFactorsUnsafe(), isam2.calculateEstimate());
  int i=0;
  BOOST_FOREACH(Key key, initial.keys()) {
    gttic_(marginalInformation);
    Matrix info = marginals.marginalInformation(key);
    gttoc_(marginalInformation);
    tictoc_finishedIteration_();
    if(i % 1000 == 0)
      tictoc_print_();
    ++i;
  }

  return 0;
}