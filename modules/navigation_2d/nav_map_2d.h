/**************************************************************************/
/*  nav_map_2d.h                                                          */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/
/* Copyright (c) 2014-present Godot Engine contributors (see AUTHORS.md). */
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                  */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#pragma once

#include "2d/nav_map_iteration_2d.h"
#include "2d/nav_mesh_queries_2d.h"
#include "nav_rid_2d.h"
#include "nav_utils_2d.h"

#include "core/math/math_defs.h"
#include "core/object/worker_thread_pool.h"
#include "servers/navigation/navigation_globals.h"

#include <KdTree2d.h>
#include <RVOSimulator2d.h>

class NavLink2D;
class NavRegion2D;
class NavAgent2D;
class NavObstacle2D;

class NavMap2D : public NavRid2D {
	/// To find the polygons edges the vertices are displaced in a grid where
	/// each cell has the following cell_size.
	real_t cell_size = NavigationDefaults2D::NAV_MESH_CELL_SIZE;

	// For the inter-region merging to work, internal rasterization is performed.
	Vector2 merge_rasterizer_cell_size = Vector2(cell_size, cell_size);

	// This value is used to control sensitivity of internal rasterizer.
	float merge_rasterizer_cell_scale = 1.0;

	bool use_edge_connections = true;
	/// This value is used to detect the near edges to connect.
	real_t edge_connection_margin = NavigationDefaults2D::EDGE_CONNECTION_MARGIN;

	/// This value is used to limit how far links search to find polygons to connect to.
	real_t link_connection_radius = NavigationDefaults2D::LINK_CONNECTION_RADIUS;

	bool map_settings_dirty = true;

	/// Map regions.
	LocalVector<NavRegion2D *> regions;

	/// Map links.
	LocalVector<NavLink2D *> links;

	/// RVO avoidance world.
	RVO2D::RVOSimulator2D rvo_simulation;

	/// Avoidance controlled agents.
	LocalVector<NavAgent2D *> active_avoidance_agents;

	/// dirty flag when one of the agent's arrays are modified.
	bool agents_dirty = true;

	/// All the Agents (even the controlled one).
	LocalVector<NavAgent2D *> agents;

	/// All the avoidance obstacles (both static and dynamic).
	LocalVector<NavObstacle2D *> obstacles;

	/// Are rvo obstacles modified?
	bool obstacles_dirty = true;

	/// Change the id each time the map is updated.
	uint32_t iteration_id = 0;

	bool use_threads = true;
	bool avoidance_use_multiple_threads = true;
	bool avoidance_use_high_priority_threads = true;

	// Performance Monitor
	Nav2D::PerformanceData performance_data;

	struct {
		struct {
			RWLock rwlock;
			SelfList<NavRegion2D>::List list;
		} regions;
		struct {
			RWLock rwlock;
			SelfList<NavLink2D>::List list;
		} links;
		struct {
			RWLock rwlock;
			SelfList<NavAgent2D>::List list;
		} agents;
		struct {
			RWLock rwlock;
			SelfList<NavObstacle2D>::List list;
		} obstacles;
	} sync_dirty_requests;

	struct {
		struct {
			RWLock rwlock;
			SelfList<NavRegion2D>::List list;
		} regions;
	} async_dirty_requests;

	int path_query_slots_max = 4;

	bool use_async_iterations = true;

	uint32_t iteration_slot_index = 0;
	LocalVector<NavMapIteration2D> iteration_slots;
	mutable RWLock iteration_slot_rwlock;

	NavMapIterationBuild2D iteration_build;
	WorkerThreadPool::TaskID iteration_build_thread_task_id = WorkerThreadPool::INVALID_TASK_ID;
	static void _build_iteration_threaded(void *p_arg);

	bool iteration_dirty = true;
	bool iteration_building = false;
	bool iteration_ready = false;

	void _build_iteration();
	void _sync_iteration();

public:
	NavMap2D();
	~NavMap2D();

	uint32_t get_iteration_id() const { return iteration_id; }

	void set_cell_size(real_t p_cell_size);
	real_t get_cell_size() const {
		return cell_size;
	}

	void set_merge_rasterizer_cell_scale(float p_value);
	float get_merge_rasterizer_cell_scale() const {
		return merge_rasterizer_cell_scale;
	}

	void set_use_edge_connections(bool p_enabled);
	bool get_use_edge_connections() const {
		return use_edge_connections;
	}

	void set_edge_connection_margin(real_t p_edge_connection_margin);
	real_t get_edge_connection_margin() const {
		return edge_connection_margin;
	}

	void set_link_connection_radius(real_t p_link_connection_radius);
	real_t get_link_connection_radius() const {
		return link_connection_radius;
	}

	Nav2D::PointKey get_point_key(const Vector2 &p_pos) const;
	const Vector2 &get_merge_rasterizer_cell_size() const;

	void query_path(NavMeshQueries2D::NavMeshPathQueryTask2D &p_query_task);

	Vector2 get_closest_point(const Vector2 &p_point) const;
	Nav2D::ClosestPointQueryResult get_closest_point_info(const Vector2 &p_point) const;
	RID get_closest_point_owner(const Vector2 &p_point) const;

	void add_region(NavRegion2D *p_region);
	void remove_region(NavRegion2D *p_region);
	const LocalVector<NavRegion2D *> &get_regions() const {
		return regions;
	}

	void add_link(NavLink2D *p_link);
	void remove_link(NavLink2D *p_link);
	const LocalVector<NavLink2D *> &get_links() const {
		return links;
	}

	bool has_agent(NavAgent2D *p_agent) const;
	void add_agent(NavAgent2D *p_agent);
	void remove_agent(NavAgent2D *p_agent);
	const LocalVector<NavAgent2D *> &get_agents() const {
		return agents;
	}

	void set_agent_as_controlled(NavAgent2D *p_agent);
	void remove_agent_as_controlled(NavAgent2D *p_agent);

	bool has_obstacle(NavObstacle2D *p_obstacle) const;
	void add_obstacle(NavObstacle2D *p_obstacle);
	void remove_obstacle(NavObstacle2D *p_obstacle);
	const LocalVector<NavObstacle2D *> &get_obstacles() const {
		return obstacles;
	}

	Vector2 get_random_point(uint32_t p_navigation_layers, bool p_uniformly) const;

	void sync();
	void step(double p_delta_time);
	void dispatch_callbacks();

	// Performance Monitor
	int get_pm_region_count() const { return performance_data.pm_region_count; }
	int get_pm_agent_count() const { return performance_data.pm_agent_count; }
	int get_pm_link_count() const { return performance_data.pm_link_count; }
	int get_pm_polygon_count() const { return performance_data.pm_polygon_count; }
	int get_pm_edge_count() const { return performance_data.pm_edge_count; }
	int get_pm_edge_merge_count() const { return performance_data.pm_edge_merge_count; }
	int get_pm_edge_connection_count() const { return performance_data.pm_edge_connection_count; }
	int get_pm_edge_free_count() const { return performance_data.pm_edge_free_count; }
	int get_pm_obstacle_count() const { return performance_data.pm_obstacle_count; }

	int get_region_connections_count(NavRegion2D *p_region) const;
	Vector2 get_region_connection_pathway_start(NavRegion2D *p_region, int p_connection_id) const;
	Vector2 get_region_connection_pathway_end(NavRegion2D *p_region, int p_connection_id) const;

	void add_region_async_thread_join_request(SelfList<NavRegion2D> *p_async_request);
	void remove_region_async_thread_join_request(SelfList<NavRegion2D> *p_async_request);

	void add_region_sync_dirty_request(SelfList<NavRegion2D> *p_sync_request);
	void add_link_sync_dirty_request(SelfList<NavLink2D> *p_sync_request);
	void add_agent_sync_dirty_request(SelfList<NavAgent2D> *p_sync_request);
	void add_obstacle_sync_dirty_request(SelfList<NavObstacle2D> *p_sync_request);

	void remove_region_sync_dirty_request(SelfList<NavRegion2D> *p_sync_request);
	void remove_link_sync_dirty_request(SelfList<NavLink2D> *p_sync_request);
	void remove_agent_sync_dirty_request(SelfList<NavAgent2D> *p_sync_request);
	void remove_obstacle_sync_dirty_request(SelfList<NavObstacle2D> *p_sync_request);

	void set_use_async_iterations(bool p_enabled);
	bool get_use_async_iterations() const;

private:
	void _sync_dirty_map_update_requests();
	void _sync_dirty_avoidance_update_requests();
	void _sync_async_tasks();

	void compute_single_step(uint32_t p_index, NavAgent2D **p_agent);

	void compute_single_avoidance_step(uint32_t p_index, NavAgent2D **p_agent);

	void _sync_avoidance();
	void _update_rvo_simulation();
	void _update_rvo_obstacles_tree();
	void _update_rvo_agents_tree();

	void _update_merge_rasterizer_cell_dimensions();
};
