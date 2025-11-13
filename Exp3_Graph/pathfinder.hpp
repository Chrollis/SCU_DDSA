#ifndef PATHFINDER_HPP
#define PATHFINDER_HPP

#include <iostream>
#include <list>
#include <queue>
#include <cmath>
#include <unordered_map>
#include <stdexcept>
#include <unordered_set>
#include <string>

namespace chr {

	struct point {
		double x, y;
		double dot(const point& other) const { return x * other.x + y * other.y; }
		double distance_to(const point& other) const {
			double dx = x - other.x;
			double dy = y - other.y;
			return std::sqrt(dx * dx + dy * dy);
		}
	};
	bool operator==(const point& a, const point& b) { return a.x == b.x && a.y == b.y;}
	constexpr double UTM_K0 = 0.9996;
	constexpr double M_PI = 3.1415829535898;
	const double WGS84_A = 6378137.0;
	const double WGS84_B = 6356752.314245;
	const double WGS84_E2 = 0.00669437999013;
	int get_utm_zone(double lon) {
		return static_cast<int>((lon + 180.0) / 6.0) + 1;
	}
	point wgs84_to_utm(double lon, double lat) {
		double lat_rad = lat * M_PI / 180.0;
		double lon_rad = lon * M_PI / 180.0;
		int zone = get_utm_zone(lon);
		double lon_origin = (zone - 1) * 6.0 - 180.0 + 3.0;
		double lon_origin_rad = lon_origin * M_PI / 180.0;
		double e2 = WGS84_E2;
		double e4 = e2 * e2;
		double e6 = e4 * e2;
		double A0 = 1 - e2 / 4 - 3 * e4 / 64 - 5 * e6 / 256;
		double A2 = 3.0 / 8.0 * (e2 + e4 / 4 + 15 * e6 / 128);
		double A4 = 15.0 / 256.0 * (e4 + 3 * e6 / 4);
		double A6 = 35 * e6 / 3072;
		double M = WGS84_A * (A0 * lat_rad - A2 * std::sin(2 * lat_rad) +
			A4 * std::sin(4 * lat_rad) - A6 * std::sin(6 * lat_rad));
		double N = WGS84_A / std::sqrt(1 - e2 * std::sin(lat_rad) * std::sin(lat_rad));
		double T = std::tan(lat_rad) * std::tan(lat_rad);
		double C = e2 / (1 - e2) * std::cos(lat_rad) * std::cos(lat_rad);
		double A = (lon_rad - lon_origin_rad) * std::cos(lat_rad);
		double x = UTM_K0 * N * (A + (1 - T + C) * A * A * A / 6 +
			(5 - 18 * T + T * T + 72 * C - 58 * e2) * A * A * A * A * A / 120);
		double y = UTM_K0 * (M + N * std::tan(lat_rad) * (A * A / 2 +
			(5 - T + 9 * C + 4 * C * C) * A * A * A * A / 24 +
			(61 - 58 * T + T * T + 600 * C - 330 * e2) * A * A * A * A * A * A / 720));
		x += 500000.0;
		if (lat < 0) {
			y += 10000000.0;
		}
		return point(x, y);
	}
	class location {
	private:
		unsigned m_id;
		std::string m_name;
		point m_pos;
		std::unordered_map<unsigned, double> m_roads;
	public:
		location() :m_id(0), m_pos({ 0,0 }) {}
		location(unsigned id, const std::string& name, const point& pos) : m_name(name), m_pos(pos) {
			if (id == 0) {
				throw std::invalid_argument("地点ID不可为0");
			}
			m_id = id;
		}
		unsigned id() const { return m_id; }
		const std::string& name() const { return m_name; }
		point pos() const { return m_pos; }
		const std::unordered_map<unsigned, double>& roads() const { return m_roads; }
		void add_road(const location& to) {
			m_roads[to.m_id] = m_pos.distance_to(to.m_pos);
		}
		void remove_road(const location& to) {
			m_roads.erase(to.m_id);
		}
	};
	class city_map {
	private:
		std::unordered_map<unsigned, location> m_places;
		struct astar_node {
			double g;
			double f;
			unsigned parent;
			bool operator>(const astar_node& other) const {
				return f > other.f;
			}
		};
	public:
		const std::unordered_map<unsigned, location>& places() const { return m_places; }
		location& add_place(unsigned id, const std::string& name, const point& pos) {
			m_places[id] = std::move(location(id, name, pos));
			return m_places[id];
		}
		location& add_place(unsigned id, const std::string& name, double lon, double lat) {
			m_places[id] = std::move(location(id, name, wgs84_to_utm(lon, lat)));
			return m_places[id];
		}
		double add_road(unsigned from, unsigned to) {
			if (m_places.find(from) == m_places.end() ||
				m_places.find(to) == m_places.end()) {
				throw std::invalid_argument("地点ID不存在");
			}
			m_places[from].add_road(m_places[to]);
			return m_places[from].roads().at(to);
		}
		double add_biroad(unsigned from, unsigned to) {
			if (m_places.find(from) == m_places.end() ||
				m_places.find(to) == m_places.end()) {
				throw std::invalid_argument("地点ID不存在");
			}
			m_places[from].add_road(m_places[to]);
			m_places[to].add_road(m_places[from]);
			return m_places[from].roads().at(to);
		}
		bool has_road(unsigned from, unsigned to) {
			if (m_places.find(from) == m_places.end()) {
				return 0;
			}
			const auto& roads = m_places[from].roads();
			if (roads.find(to) == roads.end()) {
				return 0;
			}
			return 1;
		}
		double road_length(unsigned from, unsigned to) {
			if (m_places.find(from) == m_places.end()) {
				return 0;
			}
			const auto& roads = m_places[from].roads();
			if (roads.find(to) == roads.end()) {
				return 0;
			}
			return roads.at(to);
		}
		std::vector<unsigned> find_path(unsigned from, unsigned to) {
			if (m_places.find(from) == m_places.end() ||
				m_places.find(to) == m_places.end()) {
				throw std::invalid_argument("地点ID不存在");
			}
			if (from == to) {
				return { from };
			}
			std::unordered_map<unsigned, astar_node> nodes;
			std::priority_queue<
				std::pair<double, unsigned>,
				std::vector<std::pair<double, unsigned>>,
				std::greater<std::pair<double, unsigned>>
			> open_set;
			std::unordered_set<unsigned> closed_set;
			nodes[from] = { 0.0,m_places[from].pos().distance_to(m_places[to].pos()),0 };
			open_set.push({ nodes[from].f,from });
			while (!open_set.empty()) {
				auto id = open_set.top().second;
				open_set.pop();
				if (closed_set.find(id) != closed_set.end()) {
					continue;
				}
				if (id == to) {
					std::vector<unsigned> path;
					unsigned node_id = to;
					while (node_id != 0) {
						path.push_back(node_id);
						node_id = nodes[node_id].parent;
					}
					std::reverse(path.begin(), path.end());
					return path;
				}
				closed_set.insert(id);
				const auto& place = m_places[id];
				for (const auto& [neighbor, cost] : place.roads()) {
					if (closed_set.find(neighbor) != closed_set.end()) {
						continue;
					}
					double ng = nodes[id].g + cost;
					if (nodes.find(neighbor) == nodes.end() ||
						ng < nodes[neighbor].g) {
						double h = m_places[neighbor].pos().distance_to(m_places[to].pos());
						double nf = ng + h;
						nodes[neighbor] = { ng,nf,id };
						open_set.push({ nf,neighbor });
					}
				}
			}
			return {};
		}
		void print_path(const std::vector<unsigned>& path) {
			std::cout << m_places[path[0]].name();
			double sum = 0;
			for (size_t i = 1; i < path.size(); i++) {
				if (has_road(path[i - 1], path[i])) {
					double len = road_length(path[i - 1], path[i]);
					sum += len;
					std::string lens;
					if (len > 1000) {
						lens = std::to_string(len / 1000) + "km";
					}
					else {
						lens = std::to_string(len) + "m";
					}
					if (has_road(path[i], path[i - 1])) {
						std::cout << "<-" << lens << "->" << m_places[path[i]].name();
					}
					else {
						std::cout << "->" << lens << "->" << m_places[path[i]].name();
					}
				}
				else {
					throw std::invalid_argument("路径不存在");
				}
			}
			std::cout << "，总计" << sum / 1000 << "km，抵达\n";
		}
	};
}

#endif // !PATHFINDER_HPP
