
#include "Grid.h"
#include "Hedder.h"
#include "Bubble.h"
#include <limits>

BubbleGrid::BubbleGrid(const SDL_Rect& area, int size, int rows, int cols)
	: playArea(area)
	, bubbleSize(size)
	, maxRows(rows)
	, maxCols(cols)
{

}


bool BubbleGrid::WorldToGrid(float x, float y, int& outRow, int& outCol) const
{
	float radius = bubbleSize * 0.5f;

	float firstCenterX = playArea.x + radius;
	float firstCenterY = playArea.y + radius;
	float rowF = (y - firstCenterY) / bubbleSize;

	int rowWithOffset = (int)std::round(rowF);
	outRow = rowWithOffset - rowOffset;
		

	if (outRow < 0 || outRow >= maxRows)
		return false;

	float colF = (x - firstCenterX) / bubbleSize;

	if (outRow % 2 == 1)
	{
		colF -= 0.5f;
	}

	outCol = (int)std::round(colF);

	if (outCol < 0 || outCol >= maxCols)
		return false;

	return true;
}

void BubbleGrid::GridToWorld(int row, int col, float& outX, float& outY) const
{
	float radius = bubbleSize * 0.5f;

	float firstCenterX = playArea.x + radius;
	float firstCenterY = playArea.y + radius;

	int rowWithOffset = row + rowOffset;

	outY = firstCenterY + rowWithOffset * bubbleSize;

	if (row % 2 == 0)
	{
		outX = firstCenterX + col * bubbleSize;
	}
	else
	{
		outX = firstCenterX + bubbleSize * 0.5f +col * bubbleSize;
	}
}

bool BubbleGrid::IsValidCell(int row, int col) const
{
	return(row >= 0 && row < maxRows && col >= 0 && col < maxCols);
	
}

bool BubbleGrid::IsEmpty(int row, int col) const
{
	for (const auto& b : bubbles)
	{
		if (b.row == row && b.col == col)
			return false;

	}
	return true;
}

void BubbleGrid::AddBubble(int row, int col, int color)
{
	PlacedBubble b;
	b.row = row;
	b.col = col;
	b.color = color;

	GridToWorld(row, col, b.x, b.y);
	bubbles.push_back(b);

}

PlacedBubble* BubbleGrid::FindBubble(int row, int col)
{
	for (auto& b : bubbles)
	{
		if (b.row == row && b.col == col)
		{
			return &b;
		}
	}
	return nullptr;
}

int BubbleGrid::RemoveCluster(int startRow, int startCol, int minMatch)
{
	PlacedBubble* start = FindBubble(startRow, startCol);
	if (!start)
		return 0;
	int startColor = start->color;
	bool startIsSpecial = (startColor == SPECIAL_COLOR);

	int startIndex = -1;
	for (int i = 0; i < (int)bubbles.size(); ++i)
	{
		if (bubbles[i].row == startRow && bubbles[i].col == startCol)
		{
			startIndex = i;
			break;
		}
	}

	if (startIndex == -1)
	{
		return 0;
	}
	
	static const int EVEN_NEIGHBOR[6][2] =
	{
		{0,-1},{0,1},{-1,0},{-1,-1},{1,0},{1,-1}

	};

	static const int ODD_NEIGHBOR[6][2] =
	{
		{0,-1},{0,1},{-1,0},{-1,1},{1,0},{1,1}

	};

	std::vector<int> stack;
	std::vector<int> cluster;
	std::vector<bool> visited(bubbles.size(), false);

	if (!startIsSpecial)
	{
		int targetColor = startColor;

		visited[startIndex] = true;
		stack.push_back(startIndex);

		while (!stack.empty())
		{
			int idx = stack.back();
			stack.pop_back();

			cluster.push_back(idx);

			const PlacedBubble& cur = bubbles[idx];
			const int (*NEIGHBOR)[2] = (cur.row % 2 == 0 ? EVEN_NEIGHBOR : ODD_NEIGHBOR);

			for (int i = 0; i < 6; ++i)
			{
				int nr = cur.row + NEIGHBOR[i][0];
				int nc = cur.col + NEIGHBOR[i][1];

				for (int j = 0; j < (int)bubbles.size(); ++j)
				{
					if (visited[j])
					{
						continue;
					}

					if (bubbles[j].row == nr && bubbles[j].col == nc)
					{
						int c = bubbles[j].color;
						
						if (c == targetColor || c == SPECIAL_COLOR)
						{
							visited[j] = true;
							stack.push_back(j);
						}
					}
				}
			}
		}
		if ((int)cluster.size() < minMatch)
			return 0;
	}

	else
	{
		std::vector<int> bestCluster;

		const PlacedBubble& s = bubbles[startIndex];
		const int (*NEIGHBOR0)[2] = (s.row % 2 == 0 ? EVEN_NEIGHBOR : ODD_NEIGHBOR);

		for (int n = 0; n < 6; ++n)
		{
			int nr = s.row + NEIGHBOR0[n][0];
			int nc = s.col + NEIGHBOR0[n][1];

			int neighborIndex = -1;
			for (int j = 0; j < (int)bubbles.size(); ++j)
			{
				if (bubbles[j].row == nr && bubbles[j].col == nc)
				{
					neighborIndex = j;
					break;
				}
			}


			if (neighborIndex == -1)
				continue;

			int candidateColor = bubbles[neighborIndex].color;
			if (candidateColor == SPECIAL_COLOR)
			{
				continue;
			}

			stack.clear();
			cluster.clear();
			visited.assign(bubbles.size(), false);

			int targetColor = candidateColor;

			visited[startIndex] = true;
			visited[neighborIndex] = true;
			stack.push_back(startIndex);
			stack.push_back(neighborIndex);

			while (!stack.empty())
			{
				int idx = stack.back();
				stack.pop_back();

				cluster.push_back(idx);

				const PlacedBubble& cur = bubbles[idx];
				const int (*NEIGHBOR)[2] = (cur.row % 2 == 0 ? EVEN_NEIGHBOR : ODD_NEIGHBOR);

				for (int i = 0; i < 6; ++i)
				{
					int rr = cur.row + NEIGHBOR[i][0];
					int cc = cur.col + NEIGHBOR[i][1];

					for (int j = 0; j < (int)bubbles.size(); ++j)
					{
						if (visited[j])
							continue;

						if (bubbles[j].row == rr && bubbles[j].col == cc)
						{
							int c = bubbles[j].color;
							if (c == targetColor || c == SPECIAL_COLOR)
							{
								visited[j] = true;
								stack.push_back(j);
							}
						}
					}
				}
			}

			if ((int)cluster.size() >= minMatch &&
				(int)cluster.size() > (int)bestCluster.size())
			{
				bestCluster = cluster;
			}

		}

		if ((int)bestCluster.size() < minMatch)
			return 0;

		cluster = bestCluster;
	}

	lastRemovedCluster.clear();
	for (int idx : cluster)
	{
		lastRemovedCluster.push_back(bubbles[idx]);
	}
	

	std::sort(cluster.begin(), cluster.end());
	for (int i = (int)cluster.size() - 1; i >= 0; --i)
	{
		bubbles.erase(bubbles.begin() + cluster[i]);
	}

	return (int)cluster.size();

	
}

int BubbleGrid::RemoveConnectedRegionAnyColor(int startRow, int startCol)
{
	PlacedBubble* start = FindBubble(startRow, startCol);
	if (!start)
		return 0;

	int startIndex = -1;
	for (int i = 0; i < (int)bubbles.size(); ++i)
	{
		if (bubbles[i].row == startRow && bubbles[i].col == startCol)
		{
			startIndex = i;
			break;
		}
	}
	if (startIndex == -1)
		return 0;

	static const int EVEN_NEIGHBOR[6][2] =
	{
		{0,-1},{0,1},{-1,0},{-1,-1},{1,0},{1,-1}
	};
	static const int ODD_NEIGHBOR[6][2] =
	{
		{0,-1},{0,1},{-1,0},{-1,1},{1,0},{1,1}
	};

	std::vector<int> stack;
	std::vector<int> cluster;
	std::vector<bool> visited(bubbles.size(), false);

	visited[startIndex] = true;
	stack.push_back(startIndex);

	while (!stack.empty())
	{
		int idx = stack.back();
		stack.pop_back();

		cluster.push_back(idx);

		const PlacedBubble& cur = bubbles[idx];
		const int (*NEIGHBOR)[2] =
			(cur.row % 2 == 0 ? EVEN_NEIGHBOR : ODD_NEIGHBOR);

		for (int n = 0; n < 6; ++n)
		{
			int nr = cur.row + NEIGHBOR[n][0];
			int nc = cur.col + NEIGHBOR[n][1];

			for (int j = 0; j < (int)bubbles.size(); ++j)
			{
				if (visited[j]) continue;
				if (bubbles[j].row != nr || bubbles[j].col != nc) continue;

				visited[j] = true;
				stack.push_back(j);
			}
		}
	}

	if (cluster.empty())
		return 0;

	lastRemovedCluster.clear();
	for (int idx : cluster)
		lastRemovedCluster.push_back(bubbles[idx]);

	// 인덱스 큰 것부터 지우기
	std::sort(cluster.begin(), cluster.end());
	for (int i = (int)cluster.size() - 1; i >= 0; --i)
	{
		bubbles.erase(bubbles.begin() + cluster[i]);
	}

	return (int)cluster.size();
}


int BubbleGrid::RemoveRowAll(int targetRow)
{
	std::vector<int> toRemove;
	lastRemovedCluster.clear();

	for (int i = 0; i < (int)bubbles.size(); ++i)
	{
		if (bubbles[i].row == targetRow)
		{
			toRemove.push_back(i);
			lastRemovedCluster.push_back(bubbles[i]);
		}
	}

	if (toRemove.empty())
		return 0;

	std::sort(toRemove.begin(), toRemove.end());
	for (int i = (int)toRemove.size() - 1; i >= 0; --i)
	{
		bubbles.erase(bubbles.begin() + toRemove[i]);
	}

	return (int)toRemove.size();


}

int BubbleGrid::RemoveRadius(int centerRow, int centerCol, int radius)
{
	lastRemovedCluster.clear();

	if (radius < 0) radius = 0;

	auto& all = bubbles;

	auto tryRemove = [&](int r, int c)
		{
			for (size_t i = 0; i < all.size(); ++i)
			{
				if (all[i].row == r && all[i].col == c)
				{
					lastRemovedCluster.push_back(all[i]);
					all.erase(all.begin() + i);
					break;
				}
			}
		};

	tryRemove(centerRow, centerCol);

	if (radius == 0)
		return (int)lastRemovedCluster.size();

	const bool odd = (centerRow % 2) != 0;

	if (!odd)
	{
		tryRemove(centerRow - 1, centerCol - 1);
		tryRemove(centerRow - 1, centerCol);
		tryRemove(centerRow, centerCol - 1);
		tryRemove(centerRow, centerCol + 1);
		tryRemove(centerRow + 1, centerCol - 1);
		tryRemove(centerRow + 1, centerCol);
	}
	else
	{
		tryRemove(centerRow - 1, centerCol);
		tryRemove(centerRow - 1, centerCol + 1);
		tryRemove(centerRow, centerCol - 1);
		tryRemove(centerRow, centerCol + 1);
		tryRemove(centerRow + 1, centerCol);
		tryRemove(centerRow + 1, centerCol + 1);
	}

	return (int)lastRemovedCluster.size();
}

bool BubbleGrid::FindNearestEmptyCell(float worldX, float worldY, int& outRow, int& outCol) const
{
	float bestDist2 = std::numeric_limits<float>::max();
	bool found = false;

	int baseRow, baseCol;
	bool hasBase = WorldToGrid(worldX, worldY, baseRow, baseCol);

	if (hasBase && IsValidCell(baseRow, baseCol))
	{
		static const int EVEN_NEIGHBOR[6][2] =
		{
			{0,-1},{0,1},{-1,0},{-1,-1},{1,0},{1,-1}
		};

		static const int ODD_NEIGHBOR[6][2] =
		{
			{0,-1},{0,1},{-1,0},{-1,1},{1,0},{1,1}
		};

		struct Candidate { int r, c; };
		std::vector<Candidate> candidates;
		candidates.push_back({ baseRow, baseCol });

		const int (*NEIGH)[2] = (baseRow % 2 == 0 ? EVEN_NEIGHBOR : ODD_NEIGHBOR);
		for (int i = 0; i < 6; ++i)
		{
			int nr = baseRow + NEIGH[i][0];
			int nc = baseCol + NEIGH[i][1];
			if (IsValidCell(nr, nc))
				candidates.push_back({ nr, nc });
		}
		
		for (const auto& cand : candidates)
		{
		
			if (!IsEmpty(cand.r, cand.c))
				continue;

			float cx, cy;
			GridToWorld(cand.r, cand.c, cx, cy);

			float dx = worldX - cx;
			float dy = worldY - cy;
			float dist2 = dx * dx + dy * dy;
			if (dist2 < bestDist2)
			{
				bestDist2 = dist2;
				outRow = cand.r;
				outCol = cand.c;
				found = true;
			}
		}
		if (found)
			return true;
	}

	bestDist2 = std::numeric_limits<float>::max();
	found = false;

	for (int r = 0; r < maxRows; ++r)
	{
	

		for (int c = 0; c < maxCols; ++c)
		{
			if (!IsValidCell(r, c))
				continue;

			if (!IsEmpty(r, c))
				continue;

			float cx, cy;
			GridToWorld(r, c, cx, cy);

			float dx = worldX - cx;
			float dy = worldY - cy;
			float dist2 = dx * dx + dy * dy;
			if (dist2 < bestDist2)
			{
				bestDist2 = dist2;
				outRow = r;
				outCol = c;
				found = true;
			}
		}
	}

	return found;
}

	
void BubbleGrid::SetRowOffset(int offset)
{
	rowOffset = offset;
	for (auto& b : bubbles)
	{
		GridToWorld(b.row, b.col, b.x, b.y);
	}
}

int BubbleGrid::RemoveFloatingBubbles()
{
	if (bubbles.empty())
		return 0;

	lastRemovedFloating.clear();

	std::vector<bool>visited(bubbles.size(), false);
	std::vector<int> stack;

	static const int EVEN_NEIGHBOR[6][2] =
	{
		{0,-1},{0,1},{-1,0},{-1,-1},{1,0},{1,-1}

	};

	static const int ODD_NEIGHBOR[6][2] =
	{
		{0,-1},{0,1},{-1,0},{-1,1},{1,0},{1,1}

	};

	for (int i = 0; i < (int)bubbles.size(); ++i)
	{
		if (bubbles[i].row != 0 || visited[i])
		{
			continue;
		}

			visited[i] = true;
			stack.push_back(i);

			while (!stack.empty())
			{
				int idx = stack.back();
				stack.pop_back();

				const auto& cur = bubbles[idx];
				const int(*NEIGHBOR)[2] = (cur.row % 2 == 0 ? EVEN_NEIGHBOR : ODD_NEIGHBOR);

				for (int n = 0; n < 6; ++n)
				{
					int nr = cur.row + NEIGHBOR[n][0];
					int nc = cur.col + NEIGHBOR[n][1];

					for (int j = 0; j < (int)bubbles.size();++j)
					{
						if (visited[j])
						{
							continue;
						}

						if (bubbles[j].row == nr && bubbles[j].col == nc)
						{
							visited[j] = true;
							stack.push_back(j);
						}
					}
				}


			}
	}

	std::vector<int> toRemove;
	for (int i = 0; i < (int)bubbles.size(); ++i)
	{
		if (!visited[i])
		{
			toRemove.push_back(i);
		}
	}
		if (toRemove.empty())
		{
			return 0;
		}

		for (int idx : toRemove)
		{
			lastRemovedFloating.push_back(bubbles[idx]);
		}

		std::sort(toRemove.begin(), toRemove.end());
		for (int k = (int)toRemove.size() - 1; k >= 0; --k)
		{
			bubbles.erase(bubbles.begin() + toRemove[k]);
		}

		return (int)toRemove.size();
	}

bool BubbleGrid::HasBubbleRow(int row) const
{
	for (const auto& b : bubbles)
	{
		int visualRow = b.row + rowOffset;  
		if (visualRow >= row)
			return true;
	}
	return false;
}


