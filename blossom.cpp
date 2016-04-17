#include <cassert>
#include <numeric>
#include <queue>
#include <random>
#include <tuple>
#include <unordered_map>
#include <vector>

typedef int Vertex;

typedef std::vector<Vertex> Path;

struct Graph
{
	int numVertices() const
	{
		return m_adjacencyList.size();
	}

	std::vector<std::vector<Vertex>> m_adjacencyList;
};

class Matching
{
public:
	Matching(const Graph& graph) : m_graph(graph), m_matchedVertex(graph.numVertices(), -1), m_bridges(graph.numVertices()), m_clearToken(0), m_tree(graph.numVertices())
	{
		std::vector<Vertex> unmatchedVertices;

		// Start with a greedy maximal matching
		for (Vertex v = 0; v < m_graph.numVertices(); ++v)
		{
			if (m_matchedVertex[v] == -1)
			{
				bool found = false;
				for (auto w : m_graph.m_adjacencyList[v])
				{
					if (m_matchedVertex[w] == -1)
					{
						match(v, w);
						found = true;
						break;
					}
				}

				if (!found)
				{
					unmatchedVertices.push_back(v);
				}
			}
		}

		std::vector<Vertex> path;
		for(auto v : unmatchedVertices)
		{
			if (m_matchedVertex[v] == -1)
			{
				if(findAugmentingPath(v, path))
				{
					augment(path);
					path.clear();
				}
			}
		}
	}

	Vertex getMatchedVertex(Vertex v)
	{
		return m_matchedVertex[v];
	}

private:
	void match(Vertex v, Vertex w)
	{
		m_matchedVertex[v] = w;
		m_matchedVertex[w] = v;
	}

	void augment(std::vector<Vertex>& path)
	{
		for (int i = 0; i < path.size(); i += 2)
		{
			match(path[i], path[i + 1]);
		}
	}

	bool findAugmentingPath(Vertex root, std::vector<Vertex>& path)
	{
		// Clear out the forest
		int numVertices = m_graph.numVertices();

		m_clearToken++;

		// Start our tree root
		m_tree[root].m_depth = 0;
		m_tree[root].m_parent = -1;
		m_tree[root].m_clearToken = m_clearToken;
		m_tree[root].m_blossom = root;

		m_queue.push(root);

		while (!m_queue.empty())
		{
			Vertex v = m_queue.front();
			m_queue.pop();

			for (auto w : m_graph.m_adjacencyList[v])
			{
				if (examineEdge(root, v, w, path))
				{
					while (!m_queue.empty())
					{
						m_queue.pop();
					}

					return true;
				}
			}
		}

		return false;
	}

	bool examineEdge(Vertex root, Vertex v, Vertex w, std::vector<Vertex>& path)
	{
		Vertex vBar = find(v);
		Vertex wBar = find(w);

		if (vBar != wBar)
		{
			if (m_tree[wBar].m_clearToken != m_clearToken)
			{
				if (m_matchedVertex[w] == -1)
				{
					buildAugmentingPath(root, v, w, path);
					return true;
				}
				else
				{
					extendTree(v, w);
				}
			}
			else if (m_tree[wBar].m_depth % 2 == 0)
			{
				shrinkBlossom(v, w);
			}
		}

		return false;
	}

	void buildAugmentingPath(Vertex root, Vertex v, Vertex w, std::vector<Vertex>& path)
	{
		path.push_back(w);
		findPath(v, root, path);
	}

	void extendTree(Vertex v, Vertex w)
	{
		Vertex u = m_matchedVertex[w];

		Node& nodeV = m_tree[v];
		Node& nodeW = m_tree[w];
		Node& nodeU = m_tree[u];

		nodeW.m_depth = nodeV.m_depth + 1 + (nodeV.m_depth & 1);	// Must be odd, so we add either 1 or 2
		nodeW.m_parent = v;
		nodeW.m_clearToken = m_clearToken;
		nodeW.m_blossom = w;

		nodeU.m_depth = nodeW.m_depth + 1;
		nodeU.m_parent = w;
		nodeU.m_clearToken = m_clearToken;
		nodeU.m_blossom = u;

		m_queue.push(u);
	}

	void shrinkBlossom(Vertex v, Vertex w)
	{
		Vertex b = findCommonAncestor(v, w);

		shrinkPath(b, v, w);
		shrinkPath(b, w, v);
	}

	void shrinkPath(Vertex b, Vertex v, Vertex w)
	{
		Vertex u = find(v);

		while (u != b)
		{
			makeUnion(b, u);
			assert(u != -1);
			assert(m_matchedVertex[u] != -1);
			u = m_matchedVertex[u];
			makeUnion(b, u);
			makeRepresentative(b);
			m_queue.push(u);
			m_bridges[u] = std::make_pair(v, w);
			u = find(m_tree[u].m_parent);
		}
	}

	Vertex findCommonAncestor(Vertex v, Vertex w)
	{
		while (w != v)
		{
			if(m_tree[v].m_depth > m_tree[w].m_depth)
			{
				v = m_tree[v].m_parent;
			}
			else
			{
				w = m_tree[w].m_parent;
			}
		}

		return find(v);
	}

	void findPath(Vertex s, Vertex t, Path& path)
	{
		if (s == t)
		{
			path.push_back(s);
		}
		else if (m_tree[s].m_depth % 2 == 0)
		{
			path.push_back(s);
			path.push_back(m_matchedVertex[s]);
			findPath(m_tree[m_matchedVertex[s]].m_parent, t, path);
		}
		else
		{
			Vertex v, w;
			std::tie(v, w) = m_bridges[s];

			path.push_back(s);

			int offset = path.size();
			findPath(v, m_matchedVertex[s], path);
			std::reverse(path.begin() + offset, path.end());

			findPath(w, t, path);
		}
	}

	void makeUnion(int x, int y)
	{
		int xRoot = find(x);
		m_tree[xRoot].m_blossom = find(y);
	}

	void makeRepresentative(int x)
	{
		int xRoot = find(x);
		m_tree[xRoot].m_blossom = x;
		m_tree[x].m_blossom = x;
	}

	int find(int x)
	{
		if (m_tree[x].m_clearToken != m_clearToken)
		{
			return x;
		}

		if (x != m_tree[x].m_blossom)
		{
			// Path compression
			m_tree[x].m_blossom = find(m_tree[x].m_blossom);
		}

		return m_tree[x].m_blossom;
	}


private:
	int m_clearToken;

	const Graph& m_graph;

	std::queue<Vertex> m_queue;
	std::vector<Vertex> m_matchedVertex;

	struct Node
	{
		Node()
		{
			m_clearToken = 0;
		}

		int m_depth;
		Vertex m_parent;
		Vertex m_blossom;

		int m_clearToken;
	};

	std::vector<Node> m_tree;

	std::vector<std::pair<Vertex, Vertex>> m_bridges;
};

int main()
{
	Graph graph;

	graph.m_adjacencyList.resize(28);
	graph.m_adjacencyList[0] = { 1 };
	graph.m_adjacencyList[1] = { 0, 2 };
	graph.m_adjacencyList[2] = { 1, 3, 8 };
	graph.m_adjacencyList[3] = { 2, 4 };
	graph.m_adjacencyList[4] = { 3, 5, 9 };
	graph.m_adjacencyList[5] = { 4, 6 };
	graph.m_adjacencyList[6] = { 5, 7 };
	graph.m_adjacencyList[7] = { 6, 8 };
	graph.m_adjacencyList[8] = { 2, 7, 27 };
	graph.m_adjacencyList[9] = { 4, 10, 11 };
	graph.m_adjacencyList[10] = { 9 };
	graph.m_adjacencyList[11] = { 9, 12, 13 };
	graph.m_adjacencyList[12] = { 11, 13 };
	graph.m_adjacencyList[13] = { 11, 12, 14 };
	graph.m_adjacencyList[14] = { 13, 15 };
	graph.m_adjacencyList[15] = { 14, 16, 18 };
	graph.m_adjacencyList[16] = { 15, 17 };
	graph.m_adjacencyList[17] = { 16, 21 };
	graph.m_adjacencyList[18] = { 15, 19 };
	graph.m_adjacencyList[19] = { 18, 20 };
	graph.m_adjacencyList[20] = { 19, 21 };
	graph.m_adjacencyList[21] = { 17, 20, 22 };
	graph.m_adjacencyList[22] = { 21, 23, 26 };
	graph.m_adjacencyList[23] = { 22, 24};
	graph.m_adjacencyList[24] = { 23, 25 };
	graph.m_adjacencyList[25] = { 24, 26 };
	graph.m_adjacencyList[26] = { 22, 25, 27 };
	graph.m_adjacencyList[27] = { 8, 26 };

	Matching matching(graph);

	return 0;
}

