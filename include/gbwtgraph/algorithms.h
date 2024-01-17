#ifndef GBWTGRAPH_ALGORITHMS_H
#define GBWTGRAPH_ALGORITHMS_H

#include <unordered_map>
#include <unordered_set>

#include <gbwtgraph/gbwtgraph.h>

#include <handlegraph/snarl_decomposition.hpp>

/*
  algorithms.h: Various graph algorithms.
*/

namespace gbwtgraph
{

//------------------------------------------------------------------------------

/*
  Return the weakly connected components in the graph. The components are sorted by
  the minimum node id, and the node ids in each component are also sorted.
*/
std::vector<std::vector<nid_t>> weakly_connected_components(const HandleGraph& graph);

/*
  Determine whether the given component is acyclic in a nice way. In such graphs,
  when we start from nodes with indegree 0 in forward orientation, we reach each node
  in a single orientation and find no cycles. Return the head nodes when the component
  passes the tests or an empty vector otherwise.

  Ignores node ids that are not present in the graph.
*/
std::vector<nid_t> is_nice_and_acyclic(const HandleGraph& graph, const std::vector<nid_t>& component);

/*
  Return a topological order of handles in the subgraph induced by the given node ids,
  or an empty vector if no such order exists.

  NOTE: Node ids that do not exist in the graph are ignored. In particular, some
  nodes of the original graph may be missing from the corresponding GBWTGraph if no
  path passes through them.

  If the subgraph is small, it may be a good idea to use CachedGBWTGraph instead of
  GBWTGraph.
*/
std::vector<handle_t> topological_order(const HandleGraph& graph, const std::unordered_set<nid_t>& subgraph);

//------------------------------------------------------------------------------

struct ConstructionJobs
{
  // Number of nodes in each job.
  std::vector<size_t> nodes_per_job;

  // Weakly conneted components as sorted lists of node ids.
  std::vector<std::vector<nid_t>> weakly_connected_components;

  // Mapping from node ids to component ids.
  std::unordered_map<nid_t, size_t> node_to_component;

  // Mapping from component ids to job ids.
  std::unordered_map<size_t, size_t> component_to_job;

  // Returns the number of construction jobs.
  size_t size() const { return this->nodes_per_job.size(); }

  // Returns the number of components.
  size_t components() const { return this->weakly_connected_components.size(); }

  // Returns the size of the given job in nodes.
  size_t job_size(size_t job_id) const { return this->nodes_per_job[job_id]; }

  // Maps a node identifier to a component identifier, or `components()` if
  // there is no such component.
  size_t component(nid_t node_id) const
  {
    auto iter = this->node_to_component.find(node_id);
    return (iter == this->node_to_component.end() ? this->components() : iter->second);
  }

  // Maps a node identifier to a job identifier, or `size()` if there is no such job.
  size_t job(nid_t node_id) const
  {
    auto iter = this->node_to_component.find(node_id);
    if(iter == this->node_to_component.end()) { return this->size(); }
    auto component_iter = this->component_to_job.find(iter->second);
    return (component_iter == this->component_to_job.end() ? this->size() : component_iter->second);
  }

  // Clears the jobs and tries to free the memory.
  void clear();
};

/*
  Partition the graph into weakly connected components and combine the components into
  GBWT construction job. Because the jobs do not overlap, partial GBWTs can be built
  in parallel and merged with the fast algorithm.

  At the moment, there is only one strategy for determining the jobs:

  * Sort the components by minimum node id and combine consecutive components as long
    as their total size in nodes does not exceed `size_bound`.

  TODO: Add different strategies for combining jobs.
*/
ConstructionJobs gbwt_construction_jobs(const HandleGraph& graph, size_t size_bound);

//------------------------------------------------------------------------------

struct TopLevelChain
{
  // Handle of the chain in the snarl decomposition.
  handlegraph::net_handle_t chain;

  // Handle of the first node visit in the chain.
  handle_t handle;

  // Offset of the chain in the children of the root snarl.
  size_t offset;
};

/*
  Partition the top-level chains in the snarl decomposition between GBWT construction
  jobs. This assumes that the top-level chains correspond to the weakly connected
  components in the graph.

  TODO: Is it possible to test this, as it needs a SnarlDecomposition?
*/
std::vector<std::vector<TopLevelChain>>
partition_chains(const handlegraph::SnarlDecomposition& snarls, const HandleGraph& graph, const ConstructionJobs& jobs);

//------------------------------------------------------------------------------

} // namespace gbwtgraph

#endif // GBWTGRAPH_ALGORITHMS_H
