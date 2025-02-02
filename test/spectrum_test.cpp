/**
 * Tests for the methods in the spectrum.h header file. Includes methods for
 * computing eigenvalues and eigenvectors.
 *
 * This file is provided as part of the STAG library and released under the GPL
 * license.
 */
// Standard C++ libraries
#define _USE_MATH_DEFINES
#include <stdexcept>
#include <cmath>

// Other libraries
#include <gtest/gtest.h>

// STAG modules
#include "graph.h"
#include "random.h"
#include "spectrum.h"
#include "graphio.h"

TEST(SpectrumTest, NormalisedLaplacianEigensystem) {
  // Create a small complete graph
  StagInt n = 10;
  stag::Graph testGraph = stag::complete_graph(n);

  // Compute the first few eigenvalues and eigenvectors - there should be no exceptions
  // thrown.
  StagInt k = 4;
  stag::EigenSystem eigensystem = stag::compute_eigensystem(
      &testGraph, stag::GraphMatrix::NormalisedLaplacian, k, stag::EigenSortRule::Smallest);

  // The eigenvalues should be equal to 0, and (n-1) copies of n/(n-1).
  Eigen::VectorXd eigenvalues = get<0>(eigensystem);

  EXPECT_NEAR(eigenvalues[0], 0, 0.000001);
  for (int i = 1; i < eigenvalues.size(); i++) {
      EXPECT_NEAR(eigenvalues[i], ((float) n) / (n-1), 0.000001);
  }
}

TEST(SpectrumTest, NormalisedLaplacianEigenvalues) {
  stag::Graph testGraph = stag::complete_graph(100);
  Eigen::VectorXd eigenvalues = stag::compute_eigenvalues(
      &testGraph, stag::GraphMatrix::NormalisedLaplacian, 5, stag::EigenSortRule::Smallest);
  EXPECT_NEAR(eigenvalues[0], 0, 0.000001);
}

TEST(SpectrumTest, LaplacianEigenvalues) {
  stag::Graph testGraph = stag::complete_graph(100);
  Eigen::VectorXd eigenvalues = stag::compute_eigenvalues(
      &testGraph, stag::GraphMatrix::Laplacian, 5, stag::EigenSortRule::Smallest);
  EXPECT_NEAR(eigenvalues[0], 0, 0.01);
}

TEST(SpectrumTest, CycleEigenvaluesSmallestMag) {
  StagInt n = 20;
  stag::Graph testGraph = stag::cycle_graph(n);
  Eigen::VectorXd eigenvalues = stag::compute_eigenvalues(
      &testGraph, stag::GraphMatrix::NormalisedLaplacian, 5, stag::EigenSortRule::Smallest);

  // The first eigenvalue should be 0
  EXPECT_NEAR(eigenvalues[0], 0, 0.000001);

  // For the cycle graph, eigenvalues other than 0 have multiplicity 2.
  StagReal second_eigenvalue = 1 - cos(2 * M_PI / (StagReal) n);
  EXPECT_NEAR(eigenvalues[1], second_eigenvalue, 0.000001);
  EXPECT_NEAR(eigenvalues[2], second_eigenvalue, 0.000001);

  StagReal third_eigenvalue = 1 - cos(4 * M_PI / (StagReal) n);
  EXPECT_NEAR(eigenvalues[3], third_eigenvalue, 0.000001);
  EXPECT_NEAR(eigenvalues[4], third_eigenvalue, 0.000001);
}

TEST(SpectrumTest, CycleEigenvaluesLargestMag) {
  StagInt n = 20;
  stag::Graph testGraph = stag::cycle_graph(n);
  Eigen::VectorXd eigenvalues = stag::compute_eigenvalues(
      &testGraph, stag::GraphMatrix::NormalisedLaplacian, 5, stag::EigenSortRule::Largest);

  // For the cycle graph, eigenvalues other than 0 and 2 have multiplicity 2.
  StagReal largest_eigenvalue = 1 - cos((2 * M_PI * 10) / n);
  EXPECT_NEAR(eigenvalues[0], largest_eigenvalue, 0.000001);

  StagReal second_eigenvalue = 1 - cos((2 * M_PI * 9) / n);
  EXPECT_NEAR(eigenvalues[1], second_eigenvalue, 0.000001);
  EXPECT_NEAR(eigenvalues[2], second_eigenvalue, 0.000001);

  StagReal third_eigenvalue = 1 - cos((2 * M_PI * 8) / n);
  EXPECT_NEAR(eigenvalues[3], third_eigenvalue, 0.000001);
  EXPECT_NEAR(eigenvalues[4], third_eigenvalue, 0.000001);
}

TEST(SpectrumTest, CycleEigenvaluesLapLargestMag) {
  StagInt n = 20;
  stag::Graph testGraph = stag::cycle_graph(n);
  Eigen::VectorXd eigenvalues = stag::compute_eigenvalues(
      &testGraph, stag::GraphMatrix::Laplacian, 5, stag::EigenSortRule::Largest);

  // For the cycle graph, eigenvalues other than 0 and 2 have multiplicity 2.
  StagReal largest_eigenvalue = 2 * (1 - cos((2 * M_PI * 10) / n));
  EXPECT_NEAR(eigenvalues[0], largest_eigenvalue, 0.000001);

  StagReal second_eigenvalue = 2 * (1 - cos((2 * M_PI * 9) / n));
  EXPECT_NEAR(eigenvalues[1], second_eigenvalue, 0.000001);
  EXPECT_NEAR(eigenvalues[2], second_eigenvalue, 0.000001);

  StagReal third_eigenvalue = 2 * (1 - cos((2 * M_PI * 8) / n));
  EXPECT_NEAR(eigenvalues[3], third_eigenvalue, 0.000001);
  EXPECT_NEAR(eigenvalues[4], third_eigenvalue, 0.000001);
}

TEST(SpectrumTest, CycleEigenvaluesLapSmallest) {
  StagInt n = 20;
  stag::Graph testGraph = stag::cycle_graph(n);
  Eigen::VectorXd eigenvalues = stag::compute_eigenvalues(
      &testGraph, stag::GraphMatrix::Laplacian, 5, stag::EigenSortRule::Smallest);

  // The first eigenvalue should be 0
  EXPECT_NEAR(eigenvalues[0], 0, 0.000001);

  // For the cycle graph, eigenvalues other than 0 have multiplicity 2.
  StagReal second_eigenvalue = 2 * (1 - cos(2 * M_PI / (StagReal) n));
  EXPECT_NEAR(eigenvalues[1], second_eigenvalue, 0.000001);
  EXPECT_NEAR(eigenvalues[2], second_eigenvalue, 0.000001);

  StagReal third_eigenvalue = 2 * (1 - cos(4 * M_PI / (StagReal) n));
  EXPECT_NEAR(eigenvalues[3], third_eigenvalue, 0.000001);
  EXPECT_NEAR(eigenvalues[4], third_eigenvalue, 0.000001);
}

TEST(SpectrumTest, CycleEigenvaluesAdjLargestMag) {
  StagInt n = 20;
  stag::Graph testGraph = stag::cycle_graph(n);
  Eigen::VectorXd eigenvalues = stag::compute_eigenvalues(
      &testGraph, stag::GraphMatrix::Adjacency, 5, stag::EigenSortRule::Largest);

  // For the cycle graph, eigenvalues other than 0 and 2 have multiplicity 2.
  StagReal largest_eigenvalue = 2 * cos((2 * M_PI * 0) / (StagReal) n);
  EXPECT_NEAR(eigenvalues[0], largest_eigenvalue, 0.000001);

  StagReal second_eigenvalue = 2 * cos((2 * M_PI * 1) / (StagReal) n);
  EXPECT_NEAR(eigenvalues[1], second_eigenvalue, 0.000001);
  EXPECT_NEAR(eigenvalues[2], second_eigenvalue, 0.000001);

  StagReal third_eigenvalue = 2 * cos((2 * M_PI * 2) / (StagReal) n);
  EXPECT_NEAR(eigenvalues[3], third_eigenvalue, 0.000001);
  EXPECT_NEAR(eigenvalues[4], third_eigenvalue, 0.000001);
}

TEST(SpectrumTest, CycleEigenvaluesAdjSmallest) {
  StagInt n = 20;
  stag::Graph testGraph = stag::cycle_graph(n);
  Eigen::VectorXd eigenvalues = stag::compute_eigenvalues(
      &testGraph, stag::GraphMatrix::Adjacency, 5, stag::EigenSortRule::Smallest);

  // For the cycle graph, eigenvalues other than 0 and 2 have multiplicity 2.
  StagReal largest_eigenvalue = 2 * (cos((2 * M_PI * 10) / (StagReal) n));
  EXPECT_NEAR(eigenvalues[0], largest_eigenvalue, 0.000001);

  StagReal second_eigenvalue = 2 * (cos((2 * M_PI * 9) / (StagReal) n));
  EXPECT_NEAR(eigenvalues[1], second_eigenvalue, 0.000001);
  EXPECT_NEAR(eigenvalues[2], second_eigenvalue, 0.000001);

  StagReal third_eigenvalue = 2 * (cos((2 * M_PI * 8) / (StagReal) n));
  EXPECT_NEAR(eigenvalues[3], third_eigenvalue, 0.000001);
  EXPECT_NEAR(eigenvalues[4], third_eigenvalue, 0.000001);
}

TEST(SpectrumTest, RandomGraphSpectrum) {
    // Create a graph from the SBM
    StagInt n = 100;
    stag::Graph testGraph = stag::sbm(n, 2, 0.5, 0.01);

    // Compute the first few eigenvalues and eigenvectors - there should be no exceptions
    // thrown.
    StagInt k = 3;
    stag::EigenSystem eigensystem = stag::compute_eigensystem(
        &testGraph, stag::GraphMatrix::NormalisedLaplacian, k, stag::EigenSortRule::Smallest);

    // The eigenvalues should be equal to 0, something 'small' and something 'large'.
    Eigen::VectorXd eigenvalues = get<0>(eigensystem);

    EXPECT_NEAR(eigenvalues[0], 0, 0.000001);
    EXPECT_LE(eigenvalues[1], 0.2);
    EXPECT_GE(eigenvalues[2], 0.5);
}

TEST(SpectrumTest, DisconnectedGraph) {
    // Create the data for the graph adjacency matrix of a disconnected graph
    //     0 2 0 0
    //     2 0 0 0
    //     0 0 0 1
    //     0 0 1 0
    std::vector<StagInt> rowStarts = {0, 1, 2, 3, 4};
    std::vector<StagInt> colIndices = {1, 0, 3, 2};
    std::vector<double> values = {2, 2, 1, 1};

    // Create the stag Graph object
    stag::Graph testGraph = stag::Graph(rowStarts, colIndices, values);

    // Compute the first 3 eigenvalues and eigenvectors.
    StagInt k = 3;
    stag::EigenSystem eigensystem = stag::compute_eigensystem(
        &testGraph, stag::GraphMatrix::Laplacian, k, stag::EigenSortRule::Smallest);

    // The eigenvalues should be equal to 0, 0, and something else.
    Eigen::VectorXd eigenvalues = get<0>(eigensystem);

    EXPECT_NEAR(eigenvalues[0], 0, 0.000001);
    EXPECT_NEAR(eigenvalues[1], 0, 0.000001);
    EXPECT_GE(eigenvalues[2], 0.1);
}

TEST(SpectrumTest, HugeGraph) {
  // Load the graph from file
  std::string filename = "test/data/test6.edgelist";
  stag::Graph graph = stag::load_edgelist(filename);

  // Compute the first 3 eigenvalues and eigenvectors.
  StagInt k = 3;
  stag::EigenSystem eigensystem = stag::compute_eigensystem(
      &graph, stag::GraphMatrix::Laplacian, k, stag::EigenSortRule::Smallest);
}

TEST(SpectrumTest, ArgumentChecking) {
  StagInt n = 10;
  stag::Graph testGraph = stag::complete_graph(n);

  StagInt k = -1;
  EXPECT_THROW(stag::compute_eigensystem(
      &testGraph, stag::GraphMatrix::Laplacian, k, stag::EigenSortRule::Smallest),
               std::invalid_argument);

  k = 0;
  EXPECT_THROW(stag::compute_eigensystem(
      &testGraph, stag::GraphMatrix::Laplacian, k, stag::EigenSortRule::Smallest),
               std::invalid_argument);

  k = n + 1;
  EXPECT_THROW(stag::compute_eigensystem(
      &testGraph, stag::GraphMatrix::Laplacian, k, stag::EigenSortRule::Smallest),
               std::invalid_argument);

  k = n;
  EXPECT_THROW(stag::compute_eigensystem(
      &testGraph, stag::GraphMatrix::Laplacian, k, stag::EigenSortRule::Smallest),
               std::invalid_argument);
}

TEST(SpectrumTest, RayleighQuotient) {
  stag::Graph testGraph = stag::complete_graph(3);
  const SprsMat* adj = testGraph.adjacency();
  Eigen::VectorXd vec(3);
  vec(0) = 1.0;
  vec(1) = 1.0;
  vec(2) = 1.0;

  double expected_rq = 2;
  double actual_rq = stag::rayleigh_quotient(adj, vec);
  EXPECT_NEAR(actual_rq, expected_rq, 0.00001);

  vec(1) = 0;
  vec(2) = -1;
  expected_rq = -1;
  actual_rq = stag::rayleigh_quotient(adj, vec);
  EXPECT_NEAR(actual_rq, expected_rq, 0.00001);

  vec(2) = 0;
  expected_rq = 0;
  actual_rq = stag::rayleigh_quotient(adj, vec);
  EXPECT_NEAR(actual_rq, expected_rq, 0.00001);
}

TEST(SpectrumTest, RayleighQuotientArguments) {
  stag::Graph testGraph = stag::complete_graph(4);
  const SprsMat* adj = testGraph.adjacency();
  Eigen::VectorXd vec(3);
  EXPECT_THROW(stag::rayleigh_quotient(adj, vec), std::invalid_argument);

  testGraph = stag::complete_graph(3);
  adj = testGraph.adjacency();
  vec(0) = 0;
  vec(1) = 0;
  vec(2) = 0;
  EXPECT_THROW(stag::rayleigh_quotient(adj, vec), std::invalid_argument);
}

TEST(SpectrumTest, PowerMethod) {
  stag::Graph testGraph = stag::complete_graph(3);
  const SprsMat* lap = testGraph.laplacian();
  Eigen::VectorXd vec(3);
  vec(0) = 0;
  vec(1) = 1.0;
  vec(2) = 0;

  Eigen::VectorXd expected_result(3);
  expected_result(0) = -1.0 / sqrt(6);
  expected_result(1) = 2.0 / sqrt(6);
  expected_result(2) = -1.0 / sqrt(6);

  Eigen::VectorXd actual_result = stag::power_method(lap, 2, vec);
  for (auto i = 0; i < 3; i++) {
    EXPECT_NEAR(actual_result(i), expected_result(i), 0.0001);
  }

  // Run the power method with no initial vector
  actual_result = stag::power_method(lap);

  // The rayleigh quotient should be close to 3
  EXPECT_NEAR(stag::rayleigh_quotient(lap, actual_result), 3, 0.5);

  // The power method with 0 iterations should return the same vector as the
  // input.
  actual_result = stag::power_method(lap, 0, vec);
  for (auto i = 0; i < 3; i++) {
    EXPECT_NEAR(actual_result(i), vec(i), 0.0001);
  }
}

TEST(SpectrumTest, PowerMethodArguments) {
  // The dimensions of the input vector needs to be close to correct.
  stag::Graph testGraph = stag::complete_graph(3);
  const SprsMat* lap = testGraph.laplacian();
  Eigen::VectorXd vec(4);
  vec(0) = 0;
  vec(1) = 1.0;
  vec(2) = 0;
  vec(3) = 0;
  EXPECT_THROW(stag::power_method(lap, vec), std::invalid_argument);

  // The number of iterations must be non-negative.
  testGraph = stag::complete_graph(4);
  lap = testGraph.laplacian();
  StagInt t = -1;
  EXPECT_THROW(stag::power_method(lap, t), std::invalid_argument);
}
