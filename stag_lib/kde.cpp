/*
   This file is provided as part of the STAG library and released under the GPL
   license.
*/

#include "kde.h"

#include <random.h>
#include <multithreading/ctpl_stl.h>
#include <data.h>

#define TWO_ROOT_TWO 2.828427124
#define TWO_ROOT_TWOPI 5.0132565
#define LOG_TWO 0.69314718056

// The CKNS algorithm has a couple of 'magic constants' which control the
// probability guarantee and variance bounds.
#define K2_CONSTANT 1       // K_2 = C log(n) p^{-k_j}
#define K1_CONSTANT 0.2     // K_1 = C log(n) / eps^2

// At a certain number of sampled points, we might as well brute-force the hash
// unit.
#define HASH_UNIT_CUTOFF 1000

/*
 * Used to disable compiler warning for unused variable.
 */
template<class T> void ignore_warning(const T&){}

/**
 * Compute the squared distance between the given two data points.
 *
 * @param u the first data point
 * @param v the second data point
 * @return the squared distance between \f$u\f$ and \f$v\f$.
 */
StagReal squared_distance(const stag::DataPoint& u, const stag::DataPoint& v) {
  assert(u.dimension == v.dimension);
  StagReal result = 0;
  for (StagUInt i = 0; i < u.dimension; i++) {
    result += SQR(u.coordinates[i] - v.coordinates[i]);
  }
  return result;
}

StagReal stag::gaussian_kernel(StagReal a, StagReal c) {
  return exp(-(a * c));
}

StagReal stag::gaussian_kernel(StagReal a, const stag::DataPoint& u,
                         const stag::DataPoint& v) {
  return stag::gaussian_kernel(a, squared_distance(u, v));
}

//------------------------------------------------------------------------------
// Beginning of CKNS Implementation
//------------------------------------------------------------------------------
/**
 * Compute the value of J for the CKNS algorithm, given
 *   - n, the number of data points
 *   - log2(n * mu), as an integer (at most log2(n)).
 *
 * @param n the number of data points
 * @param log_nmu the value of log2(n * mu)
 * @return
 */
StagInt ckns_J(StagInt n, StagInt log_nmu) {
  assert(log_nmu < log2(n));
  return ((StagInt) ceil(log2(n))) - log_nmu;
}

/**
 * Compute the sampling probability for the CKNS algorithm, given a value of
 * j and log2(n * mu).
 *
 * @param j
 * @param log_nmu the value of log2(n * mu)
 * @return
 */
StagReal ckns_p_sampling(StagInt j, StagInt log_nmu) {
  return pow(2, (StagReal) -j) * pow(2, (StagReal) -log_nmu);
}

StagReal ckns_gaussian_rj_squared(StagInt j, StagReal a) {
  return (StagReal) j * LOG_TWO / a;
}

/**
 * Create the LSH parameters for a particular application of E2LSH as part of
 * the CKNS algorithm with the Gaussian kernel.
 *
 * @param J the total number of sampling levels
 * @param j the current sampling level
 * @param n the total number of data points
 * @param d the dimension of the data
 * @param a the scaling parameter of the Gaussian kernel
 * @return the K and L parameters for this E2LSH table.
 */
std::vector<StagUInt> ckns_gaussian_create_lsh_params(
    StagInt J, StagInt j, StagInt n, StagReal a) {
  StagReal r_j = sqrt((StagReal) j * log(2) / a);
  StagReal p_j = stag::LSHFunction::collision_probability(r_j);
  StagReal phi_j = ceil((((StagReal) j)/((StagReal) J)) * (StagReal) (J - j + 1));
  StagUInt k_j = MAX(1, floor(- phi_j / log2(p_j)));
  StagUInt K_2 = ceil(K2_CONSTANT * log2((StagReal) n) * pow(2, phi_j));
  return {
      k_j, // parameter K
      K_2, // parameter L
  };
}

//------------------------------------------------------------------------------
// Implementation of the CKNS Gaussian KDE Hash Unit
//
// The CKNS KDE data structure is made up of several E2LSH hashes of different
// subsets of the dataset. We define a data structure (class since we're in C++
// land) which represents one such E2LSH hash as part of the CKNS algorithm.
//
// Each unit corresponds to a given 'guess' of the value of a query KDE value,
// and a distance level from the query point. The KDE value guess is referred to
// as mu in the analysis of the CKNS algorithm, and the level is referred to by
// the index j.
//------------------------------------------------------------------------------
stag::CKNSGaussianKDEHashUnit::CKNSGaussianKDEHashUnit(
    StagReal kern_param, DenseMat* data, StagInt lognmu, StagInt j_small) {
  StagInt n = data->rows();
  StagInt d = data->cols();
  a = kern_param;
  log_nmu = lognmu;
  j = j_small;

  // Get the J parameter from the value of n and log(n * mu)
  StagInt J = ckns_J(n, log_nmu);
  assert(j <= J);

  // Initialise the random number generator for the sampling of the
  // dataset
  std::uniform_real_distribution<StagReal> uniform_distribution(0.0, 1.1);

  // Create an array of PPointT structures which will be used to point to the
  // Eigen data matrix.
  std::vector<stag::DataPoint> lsh_data;

  // We need to sample the data set with the correct sampling probability.
  StagReal p_sampling = ckns_p_sampling(j, log_nmu);

  StagUInt num_sampled_points = 0;
  for (StagInt i = 0; i < n; i++) {
    // Sample with probability p_sampling
    if (uniform_distribution(*get_global_rng()) <= p_sampling) {
      lsh_data.emplace_back(d, data->row(i).data());
      num_sampled_points++;
    }
  }

  // If the number of sampled points is below the cutoff, don't create an LSH
  // table, just store the points and we'll search through them at query time.
  if (num_sampled_points <= HASH_UNIT_CUTOFF) {
    below_cutoff = true;
    all_data = lsh_data;
  } else {
    below_cutoff = false;

    // Create the LSH parameters
    std::vector<StagUInt> lsh_parameters = ckns_gaussian_create_lsh_params(
        J, j, n, a);

    // Construct the LSH object
    LSH_buckets = stag::E2LSH(lsh_parameters[0],
                              lsh_parameters[1],
                              lsh_data);
  }
}

StagReal stag::CKNSGaussianKDEHashUnit::query(const stag::DataPoint& q) {
  std::vector<stag::DataPoint> near_neighbours;
  if (below_cutoff) {
    near_neighbours = all_data;
  } else {
    // Recover points within L_j - their distance is less than r_j.
    near_neighbours = LSH_buckets.get_near_neighbors(q);
  }

  StagReal p_sampling = ckns_p_sampling(j, log_nmu);
  StagReal rj_squared = ckns_gaussian_rj_squared(j, a);
  StagReal rj_minus_1_squared = 0;
  if (j > 1) {
    rj_minus_1_squared = ckns_gaussian_rj_squared(j-1, a);
  }

  StagReal total = 0;

  for (const auto& neighbor : near_neighbours) {
    StagReal d_sq = squared_distance(q, neighbor);

    // We return only points that are in L_j - that is, in the annulus between
    // r_{j-1} and r_j.
    if (d_sq <= rj_squared && d_sq > rj_minus_1_squared) {
      // Include this point in the estimate
      total += gaussian_kernel(a, d_sq) / p_sampling;
    }
  }
  return total;
}

//------------------------------------------------------------------------------
// CKNS Gaussian KDE
//
// We now come to the implementation of the full CKNS KDE data structure.
//------------------------------------------------------------------------------
stag::CKNSGaussianKDE::CKNSGaussianKDE(DenseMat* data,
                                       StagReal gaussian_param,
                                       StagReal eps) {
  assert(eps > 0 && eps <= 1);
  n = data->rows();
  a = gaussian_param;

  // We are going to create a grid of LSH data structures:
  //   log2(n * mu) ranges from 0 to floor(log2(n))
  //   i ranges from 1 to k1.
  //   j ranges from 1 to J.
  max_log_nmu = (StagInt) ceil(log2(n));
  num_log_nmu_iterations = ceil(max_log_nmu / 2);

  k1 = ceil(K1_CONSTANT * log(n) / SQR(eps));

  hash_units.resize(num_log_nmu_iterations);
  for (StagInt log_nmu_iter = 0;
       log_nmu_iter < num_log_nmu_iterations;
       log_nmu_iter++){
    hash_units[log_nmu_iter].resize(k1);
  }

  // For each value of n * mu, we'll create an array of LSH data structures.
  StagInt num_threads = std::thread::hardware_concurrency();
  ctpl::thread_pool pool(num_threads);
  StagInt max_J = ckns_J(n, 0);
  std::vector<std::future<StagInt>> futures;
  for (StagInt j_offset = max_J - 1; j_offset >= 0; j_offset--) {
    for (StagInt log_nmu_iter = 0;
         log_nmu_iter < num_log_nmu_iterations;
         log_nmu_iter++) {
      StagInt log_nmu = log_nmu_iter * 2;
      StagInt J = ckns_J(n, log_nmu);
      StagInt j = J - j_offset;
      if (j >= 1) {
        for (StagInt iter = 0; iter < k1; iter++) {
          futures.push_back(
              pool.push(
                  [&, log_nmu_iter, log_nmu, iter, j](int id) {
                    ignore_warning(id);
                    return add_hash_unit(log_nmu_iter, log_nmu, iter, j, data);
                  }
              )
          );
        }
      }
    }
  }

  // Join all the threads
  for (auto & future : futures) {
    future.get();
  }

  // Close the thread pool.
  pool.stop();
}

StagInt stag::CKNSGaussianKDE::add_hash_unit(StagInt log_nmu_iter,
                                             StagInt log_nmu,
                                             StagInt iter,
                                             StagInt j,
                                             DenseMat* data) {
  assert(log_nmu < max_log_nmu);
  CKNSGaussianKDEHashUnit new_hash_unit = CKNSGaussianKDEHashUnit(a, data, log_nmu, j);
  hash_units_mutex.lock();
  hash_units[log_nmu_iter][iter].push_back(new_hash_unit);
  hash_units_mutex.unlock();
  return 0;
}

/**
 * Compute the median value of a vector.
 */
StagReal median(std::vector<StagReal> &v)
{
  size_t n = v.size() / 2;
  std::nth_element(v.begin(), v.begin()+n, v.end());
  return v[n];
}

std::vector<StagReal> stag::CKNSGaussianKDE::query(DenseMat* q) {
  std::vector<StagReal> results(q->rows());

  std::vector<stag::DataPoint> query_points = matrix_to_datapoints(q);

  std::vector<std::vector<StagReal>> estimates(q->rows());
  for (auto i = 0; i < q->rows(); i++) {
    results[i] = 0;
    estimates[i].resize(k1);
  }

  StagUInt num_threads = std::thread::hardware_concurrency();
  ctpl::thread_pool pool((int) num_threads);

  // Iterate through possible values of mu , until we find a correct one for
  // each query.
  for (auto log_nmu_iter = num_log_nmu_iterations - 1;
       log_nmu_iter >= 0;
       log_nmu_iter--) {
    StagInt log_nmu = log_nmu_iter * 2;
    StagInt J = ckns_J(n, log_nmu);
    std::vector<StagReal> this_mu_estimates(q->rows());

    // Get an estimate from k1 copies of the CKNS data structure.
    // Take the median one to be the true estimate.
    //
    // For each iteration up to k1, we call a future which returns a vector
    // of estimates for each query point.
    std::vector<std::future<std::vector<StagReal>>> futures;
    for (auto iter = 0; iter < k1; iter++) {
      futures.push_back(
          pool.push(
              [&, estimates, iter, query_points, results, q, J, log_nmu_iter](int id) {
                ignore_warning(id);
                std::vector<StagReal> this_iter_estimates(q->rows(), 0);

                // Iterate through the shells for each value of j
                for (auto j = 1; j <= J; j++) {
                  for (auto i = 0; i < q->rows(); i++) {
                    if (results[i] > 0) continue;
                    this_iter_estimates[i] += hash_units[log_nmu_iter][iter][j - 1]
                        .query(query_points[i]);
                  }
                }

                // Return the estimates
                return this_iter_estimates;
              }
          )
      );
    }

    // Join the futures
    std::vector<StagReal> iter_estimates(q->rows());
    for (auto iter = 0; iter < k1; iter++) {
      iter_estimates = futures[iter].get();
      for (auto i = 0; i < q->rows(); i++) {
        estimates[i][iter] = iter_estimates[i];
      }
    }

    for (auto i = 0; i < q->rows(); i++) {
      if (results[i] > 0) continue;

      this_mu_estimates[i] = median(estimates[i]);

      // Check whether the estimate is at least mu, in which case we
      // return it.
      if (log(this_mu_estimates[i]) >= (StagReal) log_nmu) {
        results[i] = this_mu_estimates[i] / (StagReal) n;
      }
    }
  }

  for (auto i = 0; i < q->rows(); i++) {
    if (results[i] == 0) results[i] = 1/ (StagReal) n;
  }

  pool.stop();

  return results;
}

//------------------------------------------------------------------------------
// Exact Gaussian KDE Implementation
//------------------------------------------------------------------------------
stag::ExactGaussianKDE::ExactGaussianKDE(DenseMat *data, StagReal param) {
  all_data = stag::matrix_to_datapoints(data);
  a = param;
}

StagReal gaussian_kde_exact(StagReal a,
                            const std::vector<stag::DataPoint>& data,
                            const stag::DataPoint& query) {
  StagReal total = 0;
  for (const auto& i : data) {
    total += gaussian_kernel(a, query, i);
  }
  return total / (double) data.size();
}

std::vector<StagReal> stag::ExactGaussianKDE::query(DenseMat* query) {
  std::vector<StagReal> results(query->cols());

  std::vector<stag::DataPoint> query_points = stag::matrix_to_datapoints(query);

  StagInt num_threads = std::thread::hardware_concurrency();

  // Split the query into num_threads chunks.
  if (query->rows() < num_threads) {
    for (auto i = 0; i < query->rows(); i++) {
      results[i] = gaussian_kde_exact(a, all_data, query_points[i]);
    }
  } else {
    // Start the thread pool
    ctpl::thread_pool pool(num_threads);

    StagInt chunk_size = floor(query->rows() / num_threads);

    // The query size is large enough to be worth splitting.
    std::vector<std::future<std::vector<StagReal>>> futures;
    for (auto chunk_id = 0; chunk_id < num_threads; chunk_id++) {
      futures.push_back(
          pool.push(
              [&, query, chunk_size, chunk_id,
               num_threads, query_points] (int id) {
                ignore_warning(id);
                StagInt this_chunk_start = chunk_id * chunk_size;
                StagInt this_chunk_end = this_chunk_start + chunk_size;
                if (chunk_id == num_threads - 1) {
                  this_chunk_end = query->rows();
                }
                std::vector<StagReal> chunk_results(this_chunk_end - this_chunk_start);

                for (auto i = this_chunk_start; i < this_chunk_end; i++) {
                  chunk_results[i - this_chunk_start] = gaussian_kde_exact(
                      a, all_data, query_points[i]);
                }

                return chunk_results;
              }
          )
      );
    }

    StagInt next_index = 0;
    for (StagInt chunk_id = 0; chunk_id < num_threads; chunk_id++) {
      std::vector<StagReal> chunk_results = futures[chunk_id].get();

      for (auto res : chunk_results) {
        results[next_index] = res;
        next_index++;
      }
    }

    pool.stop();
  }

  return results;

}
